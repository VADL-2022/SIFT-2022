#include <python3.7m/Python.h>
#include <iostream>
#include <pigpio.h>
#include "IMU.hpp"
#include "config.hpp"
#include "VADL2022.hpp"
#include "LOG.hpp"
// #include "LIDAR.hpp"
// #include "LDS.hpp"
// #include "MOTOR.hpp"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "pyMainThreadInterface.hpp"

using namespace std;

// G Forces
const float TAKEOFF_G_FORCE = 1; // Takeoff is 5-7 g's or etc.
const float MAIN_DEPLOYMENT_G_FORCE = 1; //Main parachute deployment is 10-15 g's
// Timings
const float ASCENT_IMAGE_CAPTURE = 1.6; // MECO is 1.6 seconds
const float IMU_ACCEL_DURATION = 1.0 / 10.0; // Seconds
const char* /* must fit in long long */ timeAfterMainDeployment = nullptr; // Milliseconds
// Acceleration (Meters per second squared)
const float IMU_ACCEL_MAGNITUDE_THRESHOLD_TAKEOFF_MPS = TAKEOFF_G_FORCE * 9.81; // Meters per second squared
const float IMU_ACCEL_MAGNITUDE_THRESHOLD_MAIN_PARACHUTE_MPS = MAIN_DEPLOYMENT_G_FORCE * 9.81 ; // Meters per second squared
// Command Line Args
bool sendOnRadio_ = false, siftOnly = false, videoCapture = false;
// Sift params initialization
const char* siftParams = nullptr;

// Forward declare main deployment callback
void checkMainDeploymentCallback(LOG *log, float fseconds);

// Returns true on success
bool sendOnRadio() {
  //return S_RunFile("radio.py", 0, nullptr);

  S_RunFile("videoCapture.py", 0, nullptr);
  return true;
}

enum State {
  State_WaitingForTakeoff, //Waiting for rocket launch
  STATE_WaitingForMainParachuteDeployment, //Waiting for the deployment of main parachute
  State_WaitingForMainStabilizationTime, // SIFT has been spawned at this point
};
State g_state = State_WaitingForTakeoff;
//const char *sift_args[] = { "/nix/store/c8jrsv8sqzx3a23mfjhg23lccwsnaipa-lldb-12.0.1/bin/lldb","--","./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",(timeAfterMainDeployment), (const char *)0 };
//const char *sift_args[] = { "./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",(timeAfterMainDeployment), (const char *)0 };
bool startDelayedSIFT() {
  //bool ret = pyRunFile("sift.py",0,nullptr);

  // https://unix.stackexchange.com/questions/118811/why-cant-i-run-gui-apps-from-root-no-protocol-specified
  // https://askubuntu.com/questions/294736/run-a-shell-script-as-another-user-that-has-no-password
  std::string s = "sudo -H -u pi bash -c \"XAUTHORITY=/home/pi/.Xauthority ./sift_exe_release_commandLine --main-mission " + (siftParams != nullptr ? ("--sift-params " + std::string(siftParams)) : std::string("")) + std::string(" --sleep-before-running ") + std::string(timeAfterMainDeployment) + std::string(" --no-preview-window \""); // --video-file-data-source
  int ret = system(s.c_str());
  printf("system returned %d\n", ret);
  
  return ret == 0;// "If command is NULL, then a nonzero value if a shell is
  // available, or 0 if no shell is available." ( https://man7.org/linux/man-pages/man3/system.3.html )
}
// void startDelayedSIFT_fork_notWorking() { //Actually works, need xauthority for root above
//   puts("Forking");
//   pid_t pid = fork(); // create a new child process
//   if (pid > 0) {
//     int status = 0;
//     if (wait(&status) != -1) {
//       if (WIFEXITED(status)) {
// 	// now check to see what its exit status was
// 	printf("The exit status was: %d\n", WEXITSTATUS(status));

// 	// Check for saved image
// 	// TODO: ^

// 	// Send the image over the radio
// 	sendOnRadio();
//       } else if (WIFSIGNALED(status)) {
// 	// it was killed by a signal
// 	printf("The signal that killed me was %d\n", WTERMSIG(status));
//       }
//     } else {
//       printf("Error waiting!\n");
//     }
//   } else if (pid == 0) {
//     const char* args = sift_args;
//     execvp((char*)args[0], (char**)args); // one variant of exec
//     perror("Failed to run execvp to run SIFT"); // Will only print if error with execvp.
//     exit(1); // TODO: saves IMU data? If not, set atexit or std terminate handler
//   } else {
//     perror("Error with fork");
//   }
// }

// Will: this is called on a non-main thread (on a thread for the IMU)
// Callback for waiting on takeoff
void checkTakeoffCallback(LOG *log, float fseconds) {
  VADL2022* v = (VADL2022*)log->callbackUserData;
  float magnitude = log->mImu->linearAccelNed.mag();
  printf("Accel mag: %f\n", magnitude);
  if (g_state == State_WaitingForTakeoff && magnitude > IMU_ACCEL_MAGNITUDE_THRESHOLD_TAKEOFF_MPS) {
    // Record this, it must last for IMU_ACCEL_DURATION
    if (v->startTime == -1) {
      v->startTime = fseconds;
    }
    float duration = fseconds - v->startTime;
    printf("Exceeded acceleration magnitude threshold for %f seconds\n", duration);
    if (duration >= IMU_ACCEL_DURATION) {
      // Stop these checkTakeoffCallback callbacks
      #if !defined(__x86_64__) && !defined(__i386__) && !defined(__arm64__) && !defined(__aarch64__)
      #error On these processor architectures above, pointer store or load should be an atomic operation. But without these, check the specifics of the processor.
      #else
      v->mLog->userCallback = checkMainDeploymentCallback;
      #endif
      
      puts("Target time reached, the rocket has launched");
      
    // Start SIFT which will wait for the configured amount of time until main parachute deployment and stabilization:
    //   bool ok = startDelayedSIFT();
    //   g_state = State_WaitingForMainStabilizationTime;

		// Take the ascent picture 
		if (videoCapture) {
			pyRunFile("./subscale_driver/videoCapture.py", 0, nullptr);
		}
    }
  }
  else {
    // Reset timer
    if (v->startTime != -1) {
      puts("Not enough acceleration; resetting timer");
      v->startTime = -1;
    }
  }
}

// Will: this is called on a non-main thread (on a thread for the IMU)
// Callback for waiting on main parachute deployment
void checkMainDeploymentCallback(LOG *log, float fseconds) {
	VADL2022* v = (VADL2022*)log->callbackUserData;
	float magnitude = log->mImu->linearAccelNed.mag();
	printf("Accel mag: %f\n", magnitude);
  	if (g_state == STATE_WaitingForMainParachuteDeployment && magnitude > IMU_ACCEL_MAGNITUDE_THRESHOLD_MAIN_PARACHUTE_MPS) {
		// Record this, it must last for IMU_ACCEL_DURATION
		if (v->startTime == -1) {
			v->startTime = fseconds;
		}
		float duration = fseconds - v->startTime;
		printf("Exceeded acceleration magnitude threshold for %f seconds\n", duration);
		if (duration >= IMU_ACCEL_DURATION) {
			// Stop these checkMainDeploymentCallback callbacks
			#if !defined(__x86_64__) && !defined(__i386__) && !defined(__arm64__) && !defined(__aarch64__)
				#error On these processor architectures above, pointer store or load should be an atomic operation. But without these, check the specifics of the processor.
			#else
				v->mLog->userCallback = nullptr;
			#endif

			puts("Target time reached, main parachute has deployed");

			// Start SIFT which will wait for the configured amount of time until main parachute deployment and stabilization:
			if (!videoCapture) {
				bool ok = startDelayedSIFT();
			}
			g_state = State_WaitingForMainStabilizationTime;
		}
	}
	else {
		// Reset timer
		if (v->startTime != -1) {
			puts("Not enough acceleration; resetting timer");
			v->startTime = -1;
		}
	}
}

// bool RunFile(const std::string& path)
// 	{
// 		FILE* fp = fopen( path.c_str(), "r" );
// 		if ( fp == NULL )
// 		{
// 		  //Engine::out(Engine::ERROR) << "[PyScript] Error opening file: " << path << std::endl;
// 		  std::cout << "[PyScript] Error opening file: " << path << std::endl;
// 			return false;
// 		}
// 		int re = PyRun_SimpleFile( fp, path.c_str() );
// 		fclose( fp );

// 		return (re == 0);
// 	}

VADL2022::VADL2022(int argc, char** argv)
{
	cout << "Main: Initiating" << endl;

	// Parse command-line args
	LOG::UserCallback callback = checkTakeoffCallback;
	// bool sendOnRadio_ = false, siftOnly = false, videoCapture = false;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--imu-record-only") == 0) { // Don't run anything but IMU data recording
			callback = nullptr;
		}
		else if (strcmp(argv[i], "--sift-start-time") == 0) { // Time in milliseconds
			if (i+1 < argc) {
				timeAfterMainDeployment = argv[i+1]; // Must be long long
			}
			else {
				puts("Expected start time");
				exit(1);
			}
		}
		else if (strcmp(argv[i], "--gpio-test-only") == 0) { // Don't run anything but GPIO radio upload
			sendOnRadio_ = true;
		}
		else if (strcmp(argv[i], "--sift-only") == 0) { // Don't run anything but SIFT
			siftOnly = true;
		}
		else if (strcmp(argv[i], "--video-capture") == 0) { // Run video saving from camera only
			videoCapture = true;
		}
		else if (i+1 < argc && strcmp(argv[i], "--sift-params") == 0) {
			siftParams = argv[i+1];
		}
	}

	if (timeAfterMainDeployment == nullptr && !videoCapture) {
		puts("Need to provide --sift-start-time");
		exit(1);
	}

    connect_GPIO();
	connect_Python();
	if (sendOnRadio_) {
		auto ret = sendOnRadio();
		std::cout << "sendOnRadio returned: " << ret << std::endl;
		return;
	}
	else if (siftOnly) {
		startDelayedSIFT();
		return;
	}
	bool vn = true;
	try {
		mImu = new IMU();
	}
	catch (const vn::not_found &e) {
		std::cout << "VectorNav not found: vn::not_found: " << e.what() << " ; continuing without it." << std::endl;
		vn=false;
	}
	catch (const char* e) {
		std::cout << "VectorNav not connected; continuing without it." << std::endl;
		vn = false;
	}
	// mLidar = new LIDAR();
	// mLds = new LDS();
	// mMotor = new MOTOR();
	if (vn) {
		mLog = new LOG(callback, this, mImu); //, nullptr /*mLidar*/, nullptr /*mLds*/);
		mImu->receive();
		mLog->receive();
	}
	else {
		mLog = nullptr;
		mImu = nullptr;
	}

	cout << "Main: Initiated" << endl;

	// Start video capture if doing so
        // if (videoCapture) {
	//   std::cout << "python3" << std::endl;
	//   system("sudo -H -u pi `which nix-shell` --run \"`which python3` ./subscale_driver/videoCapture.py\""); // Doesn't handle sigint
	//   //RunFile("./subscale_driver/videoCapture.py");
	//   std::cout << "end python3" << std::endl;
        // }
}

VADL2022::~VADL2022()
{
	cout << "Main: Destorying" << endl;

	delete (mLog);
	// delete (mMotor);
	// delete (mLds);
	// delete (mLidar);
	delete (mImu);

	cout << "Main: Destoryed" << endl;
}

void VADL2022::connect_GPIO()
{
	cout << "GPIO: Connecting" << endl;

	if (gpioInitialise() < 0)
	{
		cout << "GPIO: Failed to Connect" << endl;
		exit(1);
	}

	cout << "GPIO: Connected" << endl;
}

void VADL2022::disconnect_GPIO()
{
	cout << "GPIO: Disconnecting" << endl;

	gpioTerminate();

	cout << "GPIO: Disconnected" << endl;
}

void VADL2022::connect_Python()
{
	cout << "Python: Connecting" << endl;

	Py_Initialize();
	int ret = PyRun_SimpleString("import sys; #print(sys.path); \n\
for p in ['', '/nix/store/ga036m4z5f5g459f334ma90sp83rk7wv-python3-3.9.6-env/lib/python3.9/site-packages', '/nix/store/9gk5f9hwib0xrqyh17sgwfw3z1vk9ach-opencv-4.5.2/lib/python3.9/site-packages', '/nix/store/kn746xv48sp9ix26ja06wx2xv0m1g1jj-python3.9-numpy-1.20.3/lib/python3.9/site-packages', '/nix/store/mj50n3hsqrgfxjmywsz4ymhayjfpqlhf-python3-3.9.6/lib/python3.9/site-packages', '/nix/store/c8jrsv8sqzx3a23mfjhg23lccwsnaipa-lldb-12.0.1/lib/python3.9/site-packages', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python37.zip', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7/lib-dynload', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7/site-packages', '/nix/store/k9y7xyi8h0fpvsglq04hkggn5pzanb72-python3-3.7.11-env/lib/python3.7/site-packages']: \n\
    sys.path.append(p); \n\
#print(sys.path)");
	if (ret == -1) {
		cout << "Failed to setup Python path" << endl;
		exit(1);
	}

	cout << "Python: Connected" << endl;
}

void VADL2022::disconnect_Python()
{
	cout << "Python: Disconnecting" << endl;

	Py_Finalize();

	cout << "Python: Disconnected" << endl;
}
