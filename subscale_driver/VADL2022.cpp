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

#include "py.h"

using namespace std;

const float GS = 3; // Takeoff is 5-7 g's or etc.; around middle is 3 g's
const float ASCENT_IMAGE_CAPTURE = 1.6; // MECO is 1.6 seconds
//const float IMU_ACCEL_MAGNITUDE_THRESHOLD = GS * 9.81; // g's converted to meters per second squared.
// For testing //
const float IMU_ACCEL_MAGNITUDE_THRESHOLD = 1;
// //
const float IMU_ACCEL_DURATION = 1.0 / 10.0; // Seconds
const char* /* must fit in long long */ timeFromTakeoffToMainDeploymentAndStabilization = nullptr; // Milliseconds

// Returns true on success
bool sendOnRadio() {
    const char* str = R"(import serial
import random
if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1) # gpio14
    ser.reset_input_buffer()
    
    while True:
        Data = random.randint(1,4)
        ser.write(str(data).encode('utf-8')))";
    //std::cout << str << std::endl;

    return S_RunString(str);
}

//const char *sift_args[] = { "/nix/store/c8jrsv8sqzx3a23mfjhg23lccwsnaipa-lldb-12.0.1/bin/lldb","--","./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",(timeFromTakeoffToMainDeploymentAndStabilization), (const char *)0 };
//const char *sift_args[] = { "./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",(timeFromTakeoffToMainDeploymentAndStabilization), (const char *)0 };
void startDelayedSIFT() {
    const char* args = sift_args;
    std::string s = R"(import subprocess
p = subprocess.Popen(["./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",)" + std::to_string(timeFromTakeoffToMainDeploymentAndStabilization) + R"("])
)";
    return S_RunString(s.c_str());
}
void startDelayedSIFT_fork_notWorking() {
  puts("Forking");
  pid_t pid = fork(); // create a new child process
  if (pid > 0) {
    int status = 0;
    if (wait(&status) != -1) {
      if (WIFEXITED(status)) {
	// now check to see what its exit status was
	printf("The exit status was: %d\n", WEXITSTATUS(status));

	// Check for saved image
	// TODO: ^

	// Send the image over the radio
	sendOnRadio();
      } else if (WIFSIGNALED(status)) {
	// it was killed by a signal
	printf("The signal that killed me was %d\n", WTERMSIG(status));
      }
    } else {
      printf("Error waiting!\n");
    }
  } else if (pid == 0) {
    const char* args = sift_args;
    execvp((char*)args[0], (char**)args); // one variant of exec
    perror("Failed to run execvp to run SIFT"); // Will only print if error with execvp.
    exit(1); // TODO: saves IMU data? If not, set atexit or std terminate handler
  } else {
    perror("Error with fork");
  }
}

void checkTakeoffCallback(LOG *log, float fseconds) {
  VADL2022* v = (VADL2022*)log->callbackUserData;
  static float timer = 0;
  float magnitude = log->mImu->linearAccelNed.mag();
  printf("Accel mag: %f\n", magnitude);
  if (magnitude > IMU_ACCEL_MAGNITUDE_THRESHOLD) {
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
      v->mLog->userCallback = nullptr;
      #endif
      
      puts("Target time reached, we are considered having just lifted off");
      
      // Start SIFT which will wait for the configured amount of time until main parachute deployment and stabilization:
      startDelayedSIFT();
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

VADL2022::VADL2022(int argc, char** argv)
{
	cout << "Main: Initiating" << endl;

	// Parse command-line args
	LOG::UserCallback callback = checkTakeoffCallback;
	bool sendOnRadio_ = false, siftOnly = false;
	for (int i = 1; i < argc; i++) {
          if (strcmp(argv[i], "--imu-record-only") == 0) { // Don't run anything but IMU data recording
	    callback = nullptr;
          }
          else if (strcmp(argv[i], "--sift-start-time") == 0) { // Don't run anything but IMU data recording
            if (i+1 < argc) {
	      timeFromTakeoffToMainDeploymentAndStabilization = argv[i+1]; // Must be long long
            }
	    else {
	      puts("Expected start time");
	      exit(1);
            }
          }
          else if (strcmp(argv[i], "--gpio-test-only") == 0) { // Don't run anything but GPIO radio upload
	    sendOnRadio_ = true;
          }
          else if (strcmp(argv[i], "--sift-only") == 0) { // Don't run anything but GPIO radio upload
	    siftOnly = true;
          }
        }

        if (timeFromTakeoffToMainDeploymentAndStabilization == nullptr) {
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
        mImu = new IMU();
	// mLidar = new LIDAR();
	// mLds = new LDS();
	// mMotor = new MOTOR();
	mLog = new LOG(callback, this, mImu);//, nullptr /*mLidar*/, nullptr /*mLds*/);

	mImu->receive();
	mLog->receive();

	cout << "Main: Initiated" << endl;
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
	PyRun_SimpleString("import sys; #print(sys.path); \n\
for p in ['', '/nix/store/ga036m4z5f5g459f334ma90sp83rk7wv-python3-3.9.6-env/lib/python3.9/site-packages', '/nix/store/9gk5f9hwib0xrqyh17sgwfw3z1vk9ach-opencv-4.5.2/lib/python3.9/site-packages', '/nix/store/kn746xv48sp9ix26ja06wx2xv0m1g1jj-python3.9-numpy-1.20.3/lib/python3.9/site-packages', '/nix/store/mj50n3hsqrgfxjmywsz4ymhayjfpqlhf-python3-3.9.6/lib/python3.9/site-packages', '/nix/store/c8jrsv8sqzx3a23mfjhg23lccwsnaipa-lldb-12.0.1/lib/python3.9/site-packages', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python37.zip', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7/lib-dynload', '/nix/store/xsvipsgllvyg9ys19pm2pz9qpgfhzmp9-python3-3.7.11/lib/python3.7/site-packages', '/nix/store/k9y7xyi8h0fpvsglq04hkggn5pzanb72-python3-3.7.11-env/lib/python3.7/site-packages']: \n\
    sys.path.append(p); \n\
#print(sys.path)");

	cout << "Python: Connected" << endl;
}

void VADL2022::disconnect_Python()
{
	cout << "Python: Disconnecting" << endl;

	Py_Finalize();

	cout << "Python: Disconnected" << endl;
}
