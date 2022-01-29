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
#include <thread>
#include <chrono>
#include <float.h>

#include "pyMainThreadInterface.hpp"
#include "../src/fdstream.hpp"
#include "../common.hpp"

#include "signalHandlers.hpp"

// G Forces
const float TAKEOFF_G_FORCE = 0.5; // Takeoff is 5-7 g's or etc.
const float MAIN_DEPLOYMENT_G_FORCE = 0.5; //Main parachute deployment is 10-15 g's
// Timings
const float ASCENT_IMAGE_CAPTURE = 1.6; // MECO is 1.6 seconds
const float IMU_ACCEL_DURATION = 1.0 / 10.0; // Seconds
const char* /* must fit in long long */ timeAfterMainDeployment = nullptr; // Milliseconds
// Acceleration (Meters per second squared)
const float IMU_ACCEL_MAGNITUDE_THRESHOLD_TAKEOFF_MPS = TAKEOFF_G_FORCE * 9.81; // Meters per second squared
const float IMU_ACCEL_MAGNITUDE_THRESHOLD_MAIN_PARACHUTE_MPS = MAIN_DEPLOYMENT_G_FORCE * 9.81 ; // Meters per second squared
// Command Line Args
bool sendOnRadio_ = false, siftOnly = false, videoCapture = false, imuOnly = false;
// Sift params initialization
const char* siftParams = nullptr;

auto startedDriverTime = std::chrono::steady_clock::now();
auto takeoffTime = std::chrono::steady_clock::now();
long long backupSIFTStartTime = -1; // Also used as the projected SIFT start time if IMU fails
long long backupTakeoffTime = -1;

std::string gpioUserPermissionFixingCommands;
std::string gpioUserPermissionFixingCommands_arg;
std::string siftCommandLine;
const char *sift_args[] =
  {
    // "sudo",
    // "-H",
    // "-u",
    // "pi",
    "bash",
    "-c",
    NULL, // Placeholder for actual SIFT command line (`siftCommandLine` variable)
    NULL // Need NULL at the end of exec args ( https://stackoverflow.com/questions/20449182/execvp-bad-address-error/20451532 )
  };

// Python log file
ofstream cToPythonLogFile;

// Forward declare main deployment callback
void checkMainDeploymentCallback(LOG *log, float fseconds);
// Other forward declarations
void passIMUDataToSIFTCallback(LOG *log, float fseconds);

// Returns true on success
bool sendOnRadio() {
  return pyRunFile("subscale_driver/radio.py", 0, nullptr);
}

enum State {
  State_WaitingForTakeoff, //Waiting for rocket launch
  STATE_WaitingForMainParachuteDeployment, //Waiting for the deployment of main parachute
  State_WaitingForMainStabilizationTime, // SIFT has been spawned at this point
};
State g_state = State_WaitingForTakeoff;
//const char *sift_args[] = { "/nix/store/c8jrsv8sqzx3a23mfjhg23lccwsnaipa-lldb-12.0.1/bin/lldb","--","./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",(timeAfterMainDeployment), (const char *)0 };
//const char *sift_args[] = { "./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge","2", "--sleep-before-running",(timeAfterMainDeployment), (const char *)0 };
// bool startDelayedSIFT() {
//   //bool ret = pyRunFile("sift.py",0,nullptr);

//   // https://unix.stackexchange.com/questions/118811/why-cant-i-run-gui-apps-from-root-no-protocol-specified
//   // https://askubuntu.com/questions/294736/run-a-shell-script-as-another-user-that-has-no-password
//   std::string s = "sudo -H -u pi bash -c \"XAUTHORITY=/home/pi/.Xauthority ./sift_exe_release_commandLine --main-mission " + (siftParams != nullptr ? ("--sift-params " + std::string(siftParams)) : std::string("")) + std::string(" --sleep-before-running ") + std::string(timeAfterMainDeployment) + std::string(" --no-preview-window \""); // --video-file-data-source
//   int ret = system(s.c_str());
//   printf("system returned %d\n", ret);
  
//   return ret == 0;// "If command is NULL, then a nonzero value if a shell is
//   // available, or 0 if no shell is available." ( https://man7.org/linux/man-pages/man3/system.3.html )
// }
bool startDelayedSIFT_fork(const char *sift_args[], size_t sift_args_size, bool useIMU);
void startDelayedSIFT(bool useIMU) {
  mainDispatchQueue.enqueue([useIMU]() {
    bool ok = startDelayedSIFT_fork(sift_args, sizeof(sift_args) / sizeof(sift_args[0]), useIMU);
    if (!ok) {
      std::cout << "startDelayedSIFT_fork failed" << std::endl;
    }
    else {
      std::cout << "Started SIFT " << std::endl;
    }
  }, "SIFT", QueuedFunctionType::Misc);
}
int fd[2]; // write() to fd[1] to send IMU data to sift after running startDelayedSIFT_fork()
ofdstream toSIFT;
bool startDelayedSIFT_fork(const char *sift_args[], size_t sift_args_size, bool useIMU) { //Actually works, need xauthority for root above
  if (pipe(fd)) {
    printf("Error with pipe!\n");
    return false;
  }
  
  // Fill in the sift_args (do this here instead of in the child process because {"
  // After a fork() in a multithreaded program, the child can
  // safely call only async-signal-safe functions (see
  // signal-safety(7)) until such time as it calls execve(2).
  // "} ( https://man7.org/linux/man-pages/man2/fork.2.html )
  // and I don't think std::string is async-signal-safe because it could call malloc
  siftCommandLine =
      "XAUTHORITY=/home/pi/.Xauthority ./sift_exe_release_commandLine "
      "--main-mission " +
  (siftParams != nullptr
	       ? ("--sift-params " + std::string(siftParams))
	       : std::string("")) +
	  std::string(" --sleep-before-running ") +
	  std::string(timeAfterMainDeployment) +
	  std::string(" --no-preview-window") // --video-file-data-source
    + (useIMU ? (std::string(" --subscale-driver-fd ") + std::to_string(fd[0])) : "")
  ;
  
  puts("Forking");
  pid_t pid = fork(); // create a new child process
  if (pid > 0) { // Parent process
    close(fd[0]); // Close the read end of the pipe since we'll be writing data to the pipe instead of reading it
    
    toSIFT.open(fd[1]);
    
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
      return false;
    }
  } else if (pid == 0) { // Child process
    close(fd[1]); // Close write end of the pipe since we'll be receiving IMU data from the parent process on this pipe, not writing to it from the child process.
    
    sift_args[sift_args_size-2] = siftCommandLine.c_str();
  
    const char** args = sift_args;
    execvp((char*)args[0], (char**)args); // one variant of exec
    perror("Failed to run execvp to run SIFT"); // Will only print if error with execvp.
    return false; //exit(1); // [DONETODO: answer is yes, because of flushing with endl] saves IMU data? If not, set atexit or std terminate handler
  } else {
    perror("Error with fork");
    return false;
  }
  return true;
}

// https://stackoverflow.com/questions/2808398/easily-measure-elapsed-time
template <
  class result_t   = std::chrono::milliseconds,
  class clock_t    = std::chrono::steady_clock,
  class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
  return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

float fsecondsOffset = FLT_MIN;
float timeSecondsOffset = 0;
bool forceSkipNonSIFTCallbacks = false;

// Will: this is called on a non-main thread (on a thread for the IMU)
// Callback for waiting on takeoff
void checkTakeoffCallback(LOG *log, float fseconds) {
  VADL2022* v = (VADL2022*)log->callbackUserData;
  float magnitude = log->mImu->linearAccelNed.mag();
  float timeSeconds = log->mImu->timestamp / 1.0e9;
  // First run: set fsecondsOffset
  if (fsecondsOffset == FLT_MIN || fsecondsOffset == 0 || timeSecondsOffset == 0) { // Feel free to change this, may be a hack
    fsecondsOffset = fseconds;
    timeSecondsOffset = timeSeconds;
  }
  fseconds -= fsecondsOffset;
  timeSeconds -= timeSecondsOffset;
  printf("checkTakeoffCallback: times with offset are: fseconds %f, imu timestamp seconds %f, accel mag: %f\n", fseconds, timeSeconds, magnitude);
  // Check for IMU disconnect/failure to deliver packets
  const float EPSILON = 1.0/15; // Max time between packets before IMU is considered failed. i.e. <15 Hz out of 40 Hz.
  bool force = false;
  if (fseconds - timeSeconds > EPSILON && timeSeconds != 0) {
    long long milliSeconds = backupTakeoffTime;
    std::cout << "IMU considered not responding. Waiting until projected launch time guesttimate which is " << milliSeconds/1000.0 << " seconds away from now..." << std::endl;
    // Wait for SIFT backup time
    std::this_thread::sleep_for(std::chrono::milliseconds(milliSeconds));
    // Start SIFT or video capture
    magnitude = FLT_MAX; force=true; // Hack to force next if statement to succeed
  }
  if ((g_state == State_WaitingForTakeoff && magnitude > IMU_ACCEL_MAGNITUDE_THRESHOLD_TAKEOFF_MPS) || forceSkipNonSIFTCallbacks) {
    // Record this, it must last for IMU_ACCEL_DURATION
    if (v->startTime == -1) {
      v->startTime = fseconds;
    }
    float duration = fseconds - v->startTime;
    printf("Exceeded acceleration magnitude threshold for %f seconds\n", duration);
    if (duration >= IMU_ACCEL_DURATION || force) {
      // Stop these checkTakeoffCallback callbacks
      #if !defined(__x86_64__) && !defined(__i386__) && !defined(__arm64__) && !defined(__aarch64__)
      #error On these processor architectures above, pointer store or load should be an atomic operation. But without these, check the specifics of the processor.
      #else
      v->mLog->userCallback = checkMainDeploymentCallback;
      #endif
      
      puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\nTarget time reached, the rocket has launched\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
      v->startTime = -1; // Reset timer so we don't detect a main parachute deployment afterwards (although IMU would have to report higher g's to do that which is unlikely)

      // Record takeoff time
      takeoffTime = std::chrono::steady_clock::now();
      
      // Start SIFT which will wait for the configured amount of time until main parachute deployment and stabilization:
      //   bool ok = startDelayedSIFT();
      //   g_state = State_WaitingForMainStabilizationTime;
      g_state = STATE_WaitingForMainParachuteDeployment; // Now wait till main has deployed

      // Take the ascent picture 
      if (videoCapture) {
	pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);
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
  float timeSeconds = log->mImu->timestamp / 1.0e9;
  fseconds -= fsecondsOffset;
  timeSeconds -= timeSecondsOffset;
  printf("checkMainDeploymentCallback: times with offset are: fseconds %f, imu timestamp seconds %f, accel mag: %f\n", fseconds, timeSeconds, magnitude);
  // Check for IMU disconnect/failure to deliver packets
  const float EPSILON = 1.0/15; // Max time between packets before IMU is considered failed. i.e. <15 Hz out of 40 Hz.
  bool force = false;
  if (fseconds - timeSeconds > EPSILON && timeSeconds != 0) {
    // Wait for SIFT backup time
    auto millisSinceTakeoff = since(takeoffTime).count();
    if (backupSIFTStartTime > millisSinceTakeoff) {
      auto millisTillSIFT = backupSIFTStartTime - millisSinceTakeoff;
      std::cout << "IMU considered not responding. Waiting until projected SIFT start time, which is " << (millisTillSIFT/1000.0) << " seconds away from now..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(millisTillSIFT));
    } else {
      std::cout << "IMU considered not responding. No need to wait until projected SIFT start time, with " << (millisSinceTakeoff - backupSIFTStartTime )/1000.0 << " seconds to spare" << std::endl;
    }
    // Start SIFT or video capture
    magnitude = FLT_MAX; force=true; // Hack to force next if statement to succeed
  }
  if ((g_state == STATE_WaitingForMainParachuteDeployment && magnitude > IMU_ACCEL_MAGNITUDE_THRESHOLD_MAIN_PARACHUTE_MPS) || forceSkipNonSIFTCallbacks) {
    // Record this, it must last for IMU_ACCEL_DURATION
    if (v->startTime == -1) {
      v->startTime = fseconds;
    }
    float duration = fseconds - v->startTime;
    printf("Exceeded acceleration magnitude threshold for %f seconds\n", duration);
    if (duration >= IMU_ACCEL_DURATION || force) {
      // Stop these checkMainDeploymentCallback callbacks
      #if !defined(__x86_64__) && !defined(__i386__) && !defined(__arm64__) && !defined(__aarch64__)
      #error On these processor architectures above, pointer store or load should be an atomic operation. But without these, check the specifics of the processor.
      #else
      if (!videoCapture) {
	v->mLog->userCallback = passIMUDataToSIFTCallback;
      }
      else {
	v->mLog->userCallback = nullptr;			    
      }
      #endif
      
      puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\nTarget time reached, main parachute has deployed\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
      v->startTime = -1; // Reset timer
      
      // Let video capture python script know that the main parachute has deployed in order to swap cameras
      if (videoCapture) {
	// Stop the python videocapture script
	raise(SIGINT);
	
	gpioSetMode(26, PI_OUTPUT); // Set GPIO26 as output.
	gpioWrite(26, 1); // Set GPIO26 high.

	// Run the python videocapture script again on the second camera
	pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);
      }
      else {
	// Start SIFT which will wait for the configured amount of time until main parachute deployment and stabilization:
	startDelayedSIFT(true /* <--boolean: when true, use the IMU in SIFT*/);
	// ^if an error happens, continue with this error, we might as well try recording IMU data at least.
	g_state = State_WaitingForMainStabilizationTime; // Now have sift use sift_time to wait for stabilization
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

void passIMUDataToSIFTCallback(LOG *log, float fseconds) {
  float magnitude = log->mImu->linearAccelNed.mag();
  // Give this data to SIFT
  if (toSIFT.isOpen()) {
  // Check for IMU failure first
  float timeSeconds = log->mImu->timestamp / 1.0e9;
  fseconds -= fsecondsOffset;
  timeSeconds -= timeSecondsOffset;
  printf("passIMUDataToSIFTCallback: times with offset are: fseconds %f, imu timestamp seconds %f, accel mag: %f\n", fseconds, timeSeconds, magnitude);
  // Check for IMU disconnect/failure to deliver packets
  const float EPSILON = 1.0/15; // Max time between packets before IMU is considered failed. i.e. <15 Hz out of 40 Hz.
  if (fseconds - timeSeconds > EPSILON && timeSeconds != 0) {
    std::cout << "IMU considered not responding. Telling SIFT we're not using it" << std::endl;
    // Notify SIFT that IMU failed
    toSIFT << "\n" << -1.0f;
    toSIFT.flush();
    
    VADL2022* v = (VADL2022*)log->callbackUserData;
    v->mLog->userCallback = nullptr;
    return;
  }
  
            toSIFT << "\n"
                   << fseconds << ","
                   << log->mImu->yprNed[0] << "," << log->mImu->yprNed[1] << "," << log->mImu->yprNed[2] << ","
                   << log->mImu->qtn[0] << "," << log->mImu->qtn[1] << "," << log->mImu->qtn[2] << "," << log->mImu->qtn[3] << ","
                   << log->mImu->rate[0] << "," << log->mImu->rate[1] << "," << log->mImu->rate[2] << ","
                   << log->mImu->accel[0] << "," << log->mImu->accel[1] << "," << log->mImu->accel[2] << ","
                   << log->mImu->mag[0] << "," << log->mImu->mag[1] << "," << log->mImu->mag[2] << ","
                   << log->mImu->temp << "," << log->mImu->pres << "," << log->mImu->dTime << ","
                   << log->mImu->dTheta[0] << "," << log->mImu->dTheta[1] << "," << log->mImu->dTheta[2] << ","
                   << log->mImu->dVel[0] << "," << log->mImu->dVel[1] << "," << log->mImu->dVel[2] << ","
                   << log->mImu->magNed[0] << "," << log->mImu->magNed[1] << "," << log->mImu->magNed[2] << ","
                   << log->mImu->accelNed[0] << "," << log->mImu->accelNed[1] << "," << log->mImu->accelNed[2] << ","
                   << log->mImu->linearAccelBody[0] << "," << log->mImu->linearAccelBody[1] << "," << log->mImu->linearAccelBody[2] << ","
                   << log->mImu->linearAccelNed[0] << "," << log->mImu->linearAccelNed[1] << "," << log->mImu->linearAccelNed[2] << ",";
  
    toSIFT.flush();
  }
  else {
    std::cout << "passIMUDataToSIFTCallback: toSIFT is not open, not doing anything" << std::endl;
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

  gpioUserPermissionFixingCommands = std::string("sudo usermod -a -G gpio pi && sudo usermod -a -G i2c pi && sudo chown root:gpio /dev/mem && sudo chmod g+w /dev/mem && sudo chown root:gpio /var/run && sudo chmod g+w /var/run && sudo chown root:gpio /dev && sudo chmod g+w /dev"
						 // For good measure, even though the Makefile does it already (this won't take effect until another run of the executable, so that's why we do it in the Makefile) :
						 "&& sudo setcap cap_sys_rawio+ep \"$1\""); // The chown and chmod for /var/run fixes `Can't lock /var/run/pigpio.pid` and it's ok to do this since /var/run is a tmpfs created at boot
  gpioUserPermissionFixingCommands_arg = argv[0];
	  
  // Parse command-line args
  LOG::UserCallback callback = checkTakeoffCallback;
  // bool sendOnRadio_ = false, siftOnly = false, videoCapture = false;
  long long backupSIFTStartTime = -1;
  bool forceNoIMU = false;
  long long flushIMUDataEveryNMilliseconds = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--imu-record-only") == 0) { // Don't run anything but IMU data recording
      callback = nullptr;
      imuOnly = true;
    }
    if (strcmp(argv[i], "--sift-and-imu-only") == 0) { // Don't run anything but SIFT with provided IMU data + the IMU data recording, starting SIFT without waiting for takeoff/parachute events.
      forceSkipNonSIFTCallbacks = true;
    }
    else if (strcmp(argv[i], "--sift-start-time") == 0) { // Time in milliseconds since main deployment
      if (i+1 < argc) {
	timeAfterMainDeployment = argv[i+1];
      }
      else {
	puts("Expected start time");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--backup-takeoff-time") == 0) { // Time in milliseconds until takeoff approximately in case IMU fails
      if (i+1 < argc) {
	backupTakeoffTime = std::stoll(argv[i+1]); // Must be long long
      }
      else {
	puts("Expected backup takeoff time");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--backup-sift-start-time") == 0) { // Time in milliseconds since launch at which to countdown from --sift-start-time and then start SIFT, *only* used if the IMU fails
      if (i+1 < argc) {
	backupSIFTStartTime = std::stoll(argv[i+1]); // Must be long long
      }
      else {
	puts("Expected start time");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--flush-imu-data-every-n-milliseconds") == 0) { // Time in milliseconds to delay flushing of data points from the IMU into the log file.
      if (i+1 < argc) {
        puts("--flush-imu-data-every-n-milliseconds currently not supported since it doesn't flush when signals happen or something like that, todo is implement it maybe with atexit or std::set_terminate");
        exit(1);
	//flushIMUDataEveryNMilliseconds = std::stoll(argv[i+1]); // Must be long long
      }
      else {
	puts("Expected flush milliseconds");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--force-no-imu") == 0) { // Mostly for debugging purposes. This forces the driver to consider the IMU as non-existent even though it might be connected.
      forceNoIMU = true;
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
    else {
      printf("Unrecognized command-line argument given: %s", argv[i]);
      printf(" (command line was:\n");
      for (int i = 0; i < argc; i++) {
	printf("\t%s\n", argv[i]);
      }
      printf(")\n");
      puts("Exiting.");
      exit(1);
    }
  }

  if (timeAfterMainDeployment == nullptr && !videoCapture && !imuOnly) {
    puts("Need to provide --sift-start-time");
    exit(1);
  }
  if (backupSIFTStartTime == -1 && !videoCapture && !imuOnly) {
    puts("Need to provide --backup-sift-start-time");
    exit(1);
  }
  if (backupTakeoffTime == -1 && !imuOnly) {
    puts("Need to provide --backup-takeoff-time, using 60000 milliseconds (60 seconds) is recommended");
    exit(1);
  }

  connect_GPIO();
  connect_Python();
	
  // Ensure Python gets sigints and other signals
  // We do this after connect_GPIO() because "For those of us who ended up here wanting to implement their own signal handler, make sure you do your signal() call AFTER you call gpioInitialise(). This will override the pigpio handler." ( https://github.com/fivdi/pigpio/issues/127 )
  installSignalHandlers();
	
  if (sendOnRadio_) {
    auto ret = sendOnRadio();
    std::cout << "sendOnRadio returned: " << ret << std::endl;
    return;
  }
  else if (siftOnly) {
    startDelayedSIFT(false /* <--boolean: when true, use the IMU in SIFT*/);
    return;
  }
  bool vn = forceNoIMU ? false : true;
  try {
    if (vn)
      mImu = new IMU();
  }
  catch (const vn::not_found &e) {
    std::cout << "VectorNav not found: vn::not_found: " << e.what();
    if (imuOnly) {
      std::cout << std::endl;
      exit(1);
    }
    else {
      std::cout << " ; continuing without it." << std::endl;
    }
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
    mLog = new LOG(callback, this, mImu, flushIMUDataEveryNMilliseconds); //, nullptr /*mLidar*/, nullptr /*mLds*/);
    mImu->receive();
    mLog->receive();
  }
  else {
    if (forceNoIMU) std::cout << "Forcing no IMU" << std::endl;

    std::cout << "Using backupSIFTStartTime for a delay of " << backupSIFTStartTime << " milliseconds, then launching..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(backupSIFTStartTime));
    
    mLog = nullptr;
    mImu = nullptr;
    
    if (videoCapture) {
      // Take the ascent video
      pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);
    }
    else {
      startDelayedSIFT(false /* <--boolean: when true, use the IMU in SIFT*/);
    }
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

	disconnect_GPIO();
	disconnect_Python();

	cout << "Main: Destoryed" << endl;
}

bool runCommandWithFork(const char* commandWithArgs[] /* array with NULL as the last element */) {
  const char* command = commandWithArgs[0];
  printf("Forking to run command \"%s\"\n", command);
  pid_t pid = fork(); // create a new child process
  if (pid > 0) { // Parent process
    int status = 0;
    if (wait(&status) != -1) {
      if (WIFEXITED(status)) {
	// now check to see what its exit status was
	printf("The exit status for command \"%s\" was: %d\n", command, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
	// it was killed by a signal
	printf("The signal that killed command \"%s\" was %d\n", command, WTERMSIG(status));
      }
    } else {
      printf("Error waiting for command \"%s\"!\n", command);
      return false;
    }
  } else if (pid == 0) { // Child process
    execvp((char*)command, (char**)commandWithArgs); // one variant of exec
    printf("Failed to run execvp to run command \"%s\": %s\n", command, strerror(errno)); // Will only print if error with execvp.
    return false;
  } else {
    printf("Error with fork for command \"%s\": %s\n", command, strerror(errno));
    return false;
  }
  return true;
}
void VADL2022::connect_GPIO()
{
  cout << "GPIO: Connecting" << endl;

  // Prepare for gpioInitialise() by setting perms, etc.
  const char* args[] = {"bash", "-c", gpioUserPermissionFixingCommands.c_str(), "bash", // {"
    // If the -c option is present, then commands are read from string.
    // If there are arguments after the string, they are assigned to the positional
    // parameters, starting with $0.
    // "} -- https://linux.die.net/man/1/bash
    gpioUserPermissionFixingCommands_arg.c_str(), NULL};
  if (!runCommandWithFork(args)) {
    std::cout << "Failed to prepare for gpioInitialise by running gpioUserPermissionFixingCommands. Exiting." << std::endl;
    exit(1);
  } 
  
  if (gpioInitialise() < 0) {
    cout << "GPIO: Failed to Connect" << endl;
    //exit(1);
  }
  
  gpioSetPullUpDown(26, PI_PUD_DOWN); // Sets a pull-down on pin 26

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
  common::connect_Python();
}

void VADL2022::disconnect_Python()
{
  cout << "Python: Disconnecting" << endl;

  Py_Finalize();

  cout << "Python: Disconnected" << endl;
}
