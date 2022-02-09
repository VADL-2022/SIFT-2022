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
#include <sys/ioctl.h>

#include "pyMainThreadInterface.hpp"
#include "../src/fdstream.hpp"
#undef BUFFER_SIZE
#include "../common.hpp"

#include "signalHandlers.hpp"
#include <float.h>
#include "subscaleMain.hpp"

// G Forces
float TAKEOFF_G_FORCE = 0.5; // Takeoff is 5-7 g's or etc.
float MAIN_DEPLOYMENT_G_FORCE = 1; //Main parachute deployment is 10-15 g's
// Timings
const float ASCENT_IMAGE_CAPTURE = 1.6; // MECO is 1.6 seconds
const float IMU_ACCEL_DURATION = 1.0 / 10.0; // Seconds
const float IMU_MAIN_DEPLOYMENT_ACCEL_DURATION = 1.0 / 40.0; // Seconds
const char* /* must fit in long long */ timeAfterMainDeployment = nullptr; // Milliseconds
// Acceleration (Meters per second squared)
float IMU_ACCEL_MAGNITUDE_THRESHOLD_TAKEOFF_MPS = TAKEOFF_G_FORCE * 9.81 /*is set in main() also*/; // Meters per second squared
float IMU_ACCEL_MAGNITUDE_THRESHOLD_MAIN_PARACHUTE_MPS = MAIN_DEPLOYMENT_G_FORCE * 9.81 /*is set in main() also*/; // Meters per second squared
// Command Line Args
bool sendOnRadio_ = false, siftOnly = false, videoCapture = false, imuOnly = false;
// Sift params initialization
const char* siftParams = nullptr;
const char* extraSIFTArgs = nullptr;

auto startedDriverTime = std::chrono::steady_clock::now();
auto takeoffTime = std::chrono::steady_clock::now();
long long backupSIFTStartTime = -1; // Also used as the projected SIFT start time if IMU fails
std::string backupSIFTStartTime_str;
std::string TAKEOFF_G_FORCE_str;
#define USE_LIS331HH
#ifdef USE_LIS331HH // Using the alternative IMU
const char *LIS331HH_videoCapArgs[] = {NULL, NULL, NULL, NULL, NULL, NULL};
#endif
#ifdef USE_LIS331HH // Using the alternative IMU
const char* LIS331HH_calibrationFile = nullptr;
#endif
long long backupSIFTStopTime = -1;
std::string backupSIFTStopTime_str;
long long backupTakeoffTime = -1;
bool verbose = false;
// This holds the main deployment time if the IMU is working at the time of main deployment. Otherwise it holds the time SIFT was started.
auto mainDeploymentOrStartedSIFTTime = std::chrono::steady_clock::now(); // Not actually `now`
long long imuDataSourceOffset = 0; // For --imu-data-source-path only

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

double onGroundAltitude = DBL_MIN;
unsigned int numAltitudes = 0; // For running average
double computeAltitude(double kilopascals) {
  double altitudeFeet = 145366.45 * (1.0 - std::pow(10.0 * kilopascals / 1013.25, 0.190284)); // https://en.wikipedia.org/wiki/Pressure_altitude
  return altitudeFeet;
}

// Forward declare main deployment callback
template<typename LOG_T>
void checkMainDeploymentCallback(LOG_T *log, float fseconds);
// template void checkMainDeploymentCallback(LOG *log, float fseconds);
// template void checkMainDeploymentCallback(LOGFromFile_T *log, float fseconds);
// Other forward declarations
template<typename LOG_T>
void passIMUDataToSIFTCallback(LOG_T *log, float fseconds);
// https://stackoverflow.com/questions/2152002/how-do-i-force-a-particular-instance-of-a-c-template-to-instantiate
// template void passIMUDataToSIFTCallback(LOG *log, float fseconds);
// template void passIMUDataToSIFTCallback(LOGFromFile_T *log, float fseconds);

// Returns true on success
bool sendOnRadio() {
  std::cout << "sendOnRadio" << std::endl;
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
  reportStatus(Status::EnqueuingSIFT);
  mainDispatchQueue.enqueue([useIMU]() {
    //reportStatus(Status::StartingSIFT);
    bool ok = startDelayedSIFT_fork(sift_args, sizeof(sift_args) / sizeof(sift_args[0]), useIMU);
    if (!ok) {
      std::cout << "startDelayedSIFT_fork failed" << std::endl;
    }
    else {
      std::cout << "Finished SIFT" << std::endl;
      reportStatus(Status::FinishedSIFT);
    }
  }, "SIFT", QueuedFunctionType::Misc);
}
int fd[2]; // write() to fd[1] to send IMU data to sift after running startDelayedSIFT_fork()
//ofdstream toSIFT;
std::atomic<FILE*> toSIFT = nullptr;
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
      "XAUTHORITY=/home/pi/.Xauthority ./sift_exe_release_commandLine " +
    (extraSIFTArgs != nullptr ? std::string(extraSIFTArgs) : std::string("")) +
      " --main-mission " +
  (siftParams != nullptr
	       ? ("--sift-params " + std::string(siftParams))
	       : std::string("")) +
	  std::string(" --sleep-before-running ") +
    std::string(timeAfterMainDeployment) //+
    //std::string(" --no-preview-window") // --video-file-data-source
    + (useIMU ? (std::string(" --subscale-driver-fd ") + std::to_string(fd[0])) : "")
  ;
  
  printf("Forking with bash command: %s\n", siftCommandLine.c_str());
  pid_t pid = fork(); // create a new child process
  if (pid > 0) { // Parent process
    lastForkedPIDM.lock();
    lastForkedPID=pid;
    lastForkedPIDValid=true;
    lastForkedPIDM.unlock();
    
    close(fd[0]); // Close the read end of the pipe since we'll be writing data to the pipe instead of reading it
    
    //toSIFT.open(fd[1]);
    toSIFT = fdopen(fd[1], "w");
    
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
      lastForkedPIDM.lock();
      lastForkedPIDValid=false;
      lastForkedPIDM.unlock();
      return false;
    }
    lastForkedPIDM.lock();
    lastForkedPIDValid=false;
    lastForkedPIDM.unlock();
  } else if (pid == 0) { // Child process
    close(fd[1]); // Close write end of the pipe since we'll be receiving IMU data from the parent process on this pipe, not writing to it from the child process.
    
    sift_args[sift_args_size-2] = siftCommandLine.c_str();
  
    const char** args = sift_args;
    #define NO_EXEC_TEST
    #ifdef NO_EXEC_TEST
    // Just see if the pipe works
    int i = 0;
#define MSGSIZE 65536
    char buf[MSGSIZE+1/*for null terminator*/];
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100 + i+=3));
      
      // Read from fd
      ssize_t nread = read(fd[0], buf, MSGSIZE);
      
      std::cout << "nread from driverInput_fd: " << nread << std::endl;
      if (nread == -1) {
        perror("Error read()ing from driverInput_fd. Ignoring it for now. Error was"/* <The error is printed here by perror> */);
        break;
      }
      else if (nread == 0) {
        if (count == 0)
          std::cout << "No IMU data present yet" << std::endl;
        else
          std::cout << "No IMU data left" << std::endl;
        break;
      }
      buf[nread] = '\0'; // Null terminate
      
      printf("Read into buf: %s", buf);
    }
#else
    execvp((char*)args[0], (char**)args); // one variant of exec
    perror("Failed to run execvp to run SIFT"); // Will only print if error with execvp.
    #endif
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
template<typename LOG_T>
void checkTakeoffCallback(LOG_T *log, float fseconds) {
  double kilopascals = log->mImu->pres;
  double altitudeFeet = computeAltitude(kilopascals);
  if (onGroundAltitude == DBL_MIN) {
    onGroundAltitude = altitudeFeet;
    numAltitudes = 1;
  }
  else {
    // Actually ignore the first altitude value because it is strangely high.
    double currentOnGroundAltitude = onGroundAltitude / numAltitudes;
    double percentDifference = abs(altitudeFeet - currentOnGroundAltitude) / currentOnGroundAltitude;
    if (percentDifference > 0.5) {
      // Replace
      onGroundAltitude = altitudeFeet;
      numAltitudes = 1;
    }
    else {
      onGroundAltitude += altitudeFeet; // Running average
      numAltitudes += 1;
    }
  }
  if (verbose) {
    std::cout << "Altitude: " << altitudeFeet << " ft, average: " << onGroundAltitude / numAltitudes << " ft" << std::endl;
  }

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
  if (verbose) {
    printf("checkTakeoffCallback: times with offset are: fseconds %f, imu timestamp seconds %f, accel mag: %f\n", fseconds, timeSeconds, magnitude);
  }
  // Check for IMU disconnect/failure to deliver packets
  const float EPSILON = 1.0/15; // Max time between packets before IMU is considered failed. i.e. <15 Hz out of 40 Hz.
  bool force = false;
  if (fseconds - timeSeconds > EPSILON && timeSeconds != 0) {
    reportStatus(Status::IMUNotRespondingInCheckTakeoffCallback);
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
      ((LOG_T*)v->mLog)->userCallback = (reinterpret_cast<void(*)()>(&checkMainDeploymentCallback<LOG_T>));
      #endif
      
      puts("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\nTarget time reached, the rocket has launched\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
      v->startTime = -1; // Reset timer so we don't detect a main parachute deployment afterwards (although IMU would have to report higher g's to do that which is unlikely)
      
      // Record takeoff time
      takeoffTime = std::chrono::steady_clock::now();
      reportStatus(Status::TakeoffTimeRecordedInCheckTakeoffCallback);

      // Record takeoff altitude
      std::cout << "Takeoff altitude: " << altitudeFeet << " ft, average: " << onGroundAltitude / numAltitudes << " ft" << std::endl;
      
      // Start SIFT which will wait for the configured amount of time until main parachute deployment and stabilization:
      //   bool ok = startDelayedSIFT();
      //   g_state = State_WaitingForMainStabilizationTime;
      g_state = STATE_WaitingForMainParachuteDeployment; // Now wait till main has deployed

      // [nvm the below, already done on the pad in VADL2022::VADL2022():
      // Take the ascent picture 
      // if (videoCapture) {
      //   pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);
      // }
    }
  }
  else {
    // Reset timer
    if (v->startTime != -1) {
      puts("Not enough acceleration; resetting timer");
      v->startTime = -1;
      reportStatus(Status::NotEnoughAccelInCheckTakeoffCallback);
    }
  }
}

// Will: this is called on a non-main thread (on a thread for the IMU)
// Callback for waiting on main parachute deployment
template<typename LOG_T>
void checkMainDeploymentCallback(LOG_T *log, float fseconds) {
  VADL2022* v = (VADL2022*)log->callbackUserData;
  float magnitude = log->mImu->linearAccelNed.mag();
  float timeSeconds = log->mImu->timestamp / 1.0e9;
  fseconds -= fsecondsOffset;
  timeSeconds -= timeSecondsOffset;
  if (verbose) {
    printf("checkMainDeploymentCallback: times with offset are: fseconds %f, imu timestamp seconds %f, accel mag: %f\n", fseconds, timeSeconds, magnitude);
  }
  bool force = false;
  const float EPSILON = 1.0/15; // Max time between packets before IMU is considered failed. i.e. <15 Hz out of 40 Hz.
  
  // Check if backup time elapsed in case IMU doesn't ever report main deployment:
  auto millisSinceTakeoff = since(takeoffTime).count();
  const float MIN_TIME_FOR_CATCH_UP = 1000;
  const long long TIME_FOR_IMU_TO_CATCH_UP = MIN_TIME_FOR_CATCH_UP + IMU_MAIN_DEPLOYMENT_ACCEL_DURATION * 1000; // milliseconds to allow IMU to catch up to the fact that we experienced main deployment etc.
  if (backupSIFTStartTime + TIME_FOR_IMU_TO_CATCH_UP < millisSinceTakeoff) { // Past our backup time, force trigger
    auto millisTillSIFT = backupSIFTStartTime - millisSinceTakeoff;
    std::cout << "Too much time elapsed without main deployment. Forcing trigger." << std::endl;
    magnitude = FLT_MAX; force = true;
    reportStatus(Status::TooMuchTimeElapsedWithoutMainDeployment_ThereforeForcingTrigger);
  }
  // Check for IMU disconnect/failure to deliver packets
  else if (fseconds - timeSeconds > EPSILON && timeSeconds != 0) {
    // Wait for SIFT backup time
    auto millisSinceTakeoff = since(takeoffTime).count();
    if (backupSIFTStartTime > millisSinceTakeoff) { // Then we have to wait
      auto millisTillSIFT = backupSIFTStartTime - millisSinceTakeoff;
      std::cout << "IMU considered not responding. Waiting until projected SIFT start time, which is " << (millisTillSIFT/1000.0) << " seconds away from now..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(millisTillSIFT));
    } else {
      reportStatus(Status::IMUNotRespondingInMainDeploymentCallback);
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
    if (duration >= IMU_MAIN_DEPLOYMENT_ACCEL_DURATION || force) {
      // Stop these checkMainDeploymentCallback callbacks
      #if !defined(__x86_64__) && !defined(__i386__) && !defined(__arm64__) && !defined(__aarch64__)
      #error On these processor architectures above, pointer store or load should be an atomic operation. But without these, check the specifics of the processor.
      #else
      if (!videoCapture) {
        mainDeploymentOrStartedSIFTTime = std::chrono::steady_clock::now();
	((LOG_T*)v->mLog)->userCallback = (reinterpret_cast<void(*)()>(&passIMUDataToSIFTCallback<LOG_T>));
      }
      else {
	((LOG_T*)v->mLog)->userCallback = nullptr;			    
      }
      #endif
      
      puts("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\nTarget time reached, main parachute has deployed\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
      v->startTime = -1; // Reset timer
      
      // Let video capture python script know that the main parachute has deployed in order to swap cameras
      if (videoCapture) {
	// Stop the python videocapture script
        if (isRunningPython)
          raise(SIGINT);
	
	gpioSetMode(26, PI_OUTPUT); // Set GPIO26 as output.
	gpioWrite(26, 1); // Set GPIO26 high.

        reportStatus(Status::SwitchingToSecondCamera);
	// Run the python videocapture script again on the second camera
	pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);
      }
      else {
        // Stop the same videocapture script from on the pad in VADL2022::VADL2022():
        if (isRunningPython)
          raise(SIGINT);
        
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
      reportStatus(Status::NotEnoughAccelInMainDeploymentCallback);
    }
  }
}

template<typename LOG_T>
void passIMUDataToSIFTCallback(LOG_T *log, float fseconds) {
  float timeSeconds = log->mImu->timestamp / 1.0e9;
  fseconds -= fsecondsOffset;
  timeSeconds -= timeSecondsOffset;
  
  // Convert log->mImu->pres (pressure) to altitude using barometric formula (Cam) and use that to check for nearing the ground + stopping SIFT
  // TODO: IMU has altitude? See VADL2021-Source-Continued/VectorNav/include/vn/packet.h in https://github.com/VADL-2022/VADL2021-Source-Continued
  double kilopascals = log->mImu->pres;
  double altitudeFeet = computeAltitude(kilopascals);
  double relativeAltitude = altitudeFeet - (onGroundAltitude / numAltitudes);
  static bool onceFlag = true;
  if (onceFlag || verbose) {
    onceFlag = false;
    std::cout << "Altitude: " << altitudeFeet << " ft, relative altitude: " << relativeAltitude << " ft" << std::endl;
  }
  if (relativeAltitude < 50) {
    // Future: stop SIFT here but not sure if this would stop too early, so we just output what would happen:
    static bool onceFlag2 = true;
    if (onceFlag2 ||verbose)
      onceFlag2 = false;
      std::cout << "Relative altitude " << relativeAltitude << " is less than 50 feet, would normally try stopping SIFT" << std::endl;
    //raise(SIGINT); // <-- to stop SIFT
    reportStatus(Status::AltitudeLessThanDesiredAmount);
  }

  // Stop SIFT on timer time elapsed
  if (verbose)
    std::cout << "Time till SIFT stops: " << backupSIFTStopTime - since(mainDeploymentOrStartedSIFTTime).count() << " milliseconds" << std::endl;
  if (since(mainDeploymentOrStartedSIFTTime).count() > backupSIFTStopTime && !mainDispatchQueueDrainThenStop) {
    std::cout << "Stopping SIFT on backup time elapsed" << std::endl;
    reportStatus(Status::StoppingSIFTOnBackupTimeElapsed);
    raise(SIGINT);
    // Also close main dispatch queue so the subscale driver terminates
    mainDispatchQueueDrainThenStop = true;
  }



  
  //return;



  
  float magnitude = log->mImu->linearAccelNed.mag();
  // Give this data to SIFT
  //if (toSIFT.isOpen()) {
  if (toSIFT != nullptr) {
    // Check for IMU failure first
    if (verbose) {
      printf("passIMUDataToSIFTCallback: times with offset are: fseconds %f, imu timestamp seconds %f, accel mag: %f\n", fseconds, timeSeconds, magnitude);
    }
    // Check for IMU disconnect/failure to deliver packets
    const float EPSILON = 1.0/15; // Max time between packets before IMU is considered failed. i.e. <15 Hz out of 40 Hz.
    if (fseconds - timeSeconds > EPSILON && timeSeconds != 0) {
      std::cout << "IMU considered not responding. Telling SIFT we're not using it for now" << std::endl;
      // Notify SIFT that IMU failed
      // toSIFT << "\n" << -1.0f;
      // toSIFT.flush();
      fprintf(toSIFT, "\n%a", -1.0f);
      fflush(toSIFT);
      
      VADL2022* v = (VADL2022*)log->callbackUserData;
      ((LOG_T*)v->mLog)->userCallback = nullptr;
      reportStatus(Status::IMUNotRespondingInPassIMUDataToSIFTCallback);
      return;
    }

    auto/*IMU or IMUData*/& imu = *log->mImu;
  
    // toSIFT << "\n"
    //        << fseconds << ","
    //        << log->mImu->yprNed[0] << "," << log->mImu->yprNed[1] << "," << log->mImu->yprNed[2] << ","
    //        << log->mImu->qtn[0] << "," << log->mImu->qtn[1] << "," << log->mImu->qtn[2] << "," << log->mImu->qtn[3] << ","
    //        << log->mImu->rate[0] << "," << log->mImu->rate[1] << "," << log->mImu->rate[2] << ","
    //        << log->mImu->accel[0] << "," << log->mImu->accel[1] << "," << log->mImu->accel[2] << ","
    //        << log->mImu->mag[0] << "," << log->mImu->mag[1] << "," << log->mImu->mag[2] << ","
    //        << log->mImu->temp << "," << log->mImu->pres << "," << log->mImu->dTime << ","
    //        << log->mImu->dTheta[0] << "," << log->mImu->dTheta[1] << "," << log->mImu->dTheta[2] << ","
    //        << log->mImu->dVel[0] << "," << log->mImu->dVel[1] << "," << log->mImu->dVel[2] << ","
    //        << log->mImu->magNed[0] << "," << log->mImu->magNed[1] << "," << log->mImu->magNed[2] << ","
    //        << log->mImu->accelNed[0] << "," << log->mImu->accelNed[1] << "," << log->mImu->accelNed[2] << ","
    //        << log->mImu->linearAccelBody[0] << "," << log->mImu->linearAccelBody[1] << "," << log->mImu->linearAccelBody[2] << ","
    //        << log->mImu->linearAccelNed[0] << "," << log->mImu->linearAccelNed[1] << "," << log->mImu->linearAccelNed[2] << ",";

    fprintf(toSIFT, "\n%a" // fseconds -- timestamp in seconds since gpio library initialization (that is, essentially since the driver program started)
                    "," "%" PRIu64 "" // timestamp  // `PRIu64` is a nasty thing needed for uint64_t format strings.. ( https://stackoverflow.com/questions/9225567/how-to-print-a-int64-t-type-in-c )
                    ",%a,%a,%a" // yprNed
                    ",%a,%a,%a,%a" // qtn
                    ",%a,%a,%a" // rate
                    ",%a,%a,%a" // accel
                    ",%a,%a,%a" // mag
                    ",%a,%a,%a" // temp,pres,dTime
                    ",%a,%a,%a" // dTheta
                    ",%a,%a,%a" // dVel
                    ",%a,%a,%a" // magNed
                    ",%a,%a,%a" // accelNed
                    ",%a,%a,%a" // linearAccelBody
                    ",%a,%a,%a" // linearAccelNed
                    "," // Nothing after this comma on purpose.
                    ,
                    fseconds,
                    imu.timestamp,
                    imu.yprNed.x, imu.yprNed.y, imu.yprNed.z,
                    imu.qtn.x, imu.qtn.y, imu.qtn.z, imu.qtn.w,
                    imu.rate.x, imu.rate.y, imu.rate.z,
                    imu.accel.x, imu.accel.y, imu.accel.z,
                    imu.mag.x, imu.mag.y, imu.mag.z,
                    imu.temp, imu.pres, imu.dTime,
                    imu.dTheta.x, imu.dTheta.y, imu.dTheta.z,
                    imu.dVel.x, imu.dVel.y, imu.dVel.z,
                    imu.magNed.x, imu.magNed.y, imu.magNed.z,
                    imu.accelNed.x, imu.accelNed.y, imu.accelNed.z,
                    imu.linearAccelBody.x, imu.linearAccelBody.y, imu.linearAccelBody.z,
                    imu.linearAccelNed.x, imu.linearAccelNed.y, imu.linearAccelNed.z);
    fflush(toSIFT);
    
    int cnt;
    if (ioctl(fd[1] /* <-- used by toSIFT */, FIONREAD, &cnt) < 0) { // "If you're on Linux, this call should tell you how many unread bytes are in the pipe" --Professor Balasubramanian
      perror("driver: ioctl to check bytes in fd[1] failed (ignoring)");
    }
    else {
      printf("driver: ioctl says %d bytes are left in the fd[1] pipe (pipe fd is %d)\n", cnt, fd[1]);
    }
  
    // toSIFT.flush();
  }
  else {
    if (verbose)
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
  void* /*LOG::UserCallback or LOGFromFile::UserCallback*/ callback = reinterpret_cast<void*>(reinterpret_cast<void(*)()>(&checkTakeoffCallback<LOG>));
  // bool sendOnRadio_ = false, siftOnly = false, videoCapture = false;
  bool forceNoIMU = false;
  long long flushIMUDataEveryNMilliseconds = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--imu-record-only") == 0) { // Don't run anything but IMU data recording
      callback = nullptr;
      imuOnly = true;
    }
    else if (strcmp(argv[i], "--imu-data-source-path") == 0) { // Grab IMU data from a file instead of the VectorNav
      if (i+1 < argc) {
        imuDataSourcePath = argv[i+1];
        callback = reinterpret_cast<void*>(reinterpret_cast<void(*)()>(&checkTakeoffCallback<LOGFromFile>));
      }
      else {
	puts("Expected path");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--imu-data-source-time-offset") == 0) { // Offset by n milliseconds by seeking past timestamps less than this amount in the file given by `--imu-data-source-path`
      if (i+1 < argc) {
        imuDataSourceOffset = std::stoll(argv[i+1]);
      }
      else {
	puts("Expected seconds");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--sift-and-imu-only") == 0) { // Don't run anything but SIFT with provided IMU data + the IMU data recording, starting SIFT without waiting for takeoff/parachute events.
      forceSkipNonSIFTCallbacks = true;
    }
    else if (strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
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
    else if (strcmp(argv[i], "--main-deployment-g-force") == 0) { // Override thing
      if (i+1 < argc) {
	MAIN_DEPLOYMENT_G_FORCE = stof(argv[i+1]);
	IMU_ACCEL_MAGNITUDE_THRESHOLD_MAIN_PARACHUTE_MPS = MAIN_DEPLOYMENT_G_FORCE * 9.81 ; // Meters per second squared
	std::cout << "Set main deployment g force to " << MAIN_DEPLOYMENT_G_FORCE << std::endl;
      }
      else {
	puts("Expected g force");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--takeoff-g-force") == 0) { // Override thing
      if (i+1 < argc) {
	TAKEOFF_G_FORCE = stof(argv[i+1]);
	IMU_ACCEL_MAGNITUDE_THRESHOLD_TAKEOFF_MPS = TAKEOFF_G_FORCE * 9.81; // Meters per second squared
	std::cout << "Set takeoff g force to " << TAKEOFF_G_FORCE << std::endl;
      }
      else {
	puts("Expected g force");
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
    else if (strcmp(argv[i], "--backup-sift-start-time") == 0) { // Time in milliseconds since launch at which to countdown up to --sift-start-time and then start SIFT, *only* used if the IMU fails
      if (i+1 < argc) {
	backupSIFTStartTime = std::stoll(argv[i+1]); // Must be long long
      }
      else {
	puts("Expected start time");
	exit(1);
      }
      i++;
    }
    else if (strcmp(argv[i], "--backup-sift-stop-time") == 0) { // Time in milliseconds since main deployment at which to stop SIFT but as an upper bound (don't make it possibly too low, since time for descent varies a lot)
      if (i+1 < argc) {
	backupSIFTStopTime = std::stoll(argv[i+1]); // Must be long long
      }
      else {
	puts("Expected stop time");
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
    #ifdef USE_LIS331HH // Using the alternative IMU
    else if (strcmp(argv[i], "--LIS331HH-imu-calibration-file") == 0) {
      if (i+1 < argc) {
        LIS331HH_calibrationFile = argv[i+1];
      }
      else {
	puts("Expected calibration file");
	exit(1);
      }
      i++;
    }
    #endif
    else if (i+1 < argc && strcmp(argv[i], "--sift-params") == 0) {
      siftParams = argv[i+1];
      i++;
    }
    else if (i+1 < argc && strcmp(argv[i], "--extra-sift-exe-args") == 0) {
      extraSIFTArgs = argv[i+1];
      i++;
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
  if (backupSIFTStopTime == -1 && !videoCapture && !imuOnly) {
    puts("Need to provide --backup-sift-stop-time");
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

#ifdef USE_LIS331HH // Using the alternative IMU
  if (videoCapture) {
    backupSIFTStartTime_str = std::to_string(backupSIFTStartTime);
    TAKEOFF_G_FORCE_str = std::to_string(TAKEOFF_G_FORCE);
    backupSIFTStopTime_str = std::to_string(backupSIFTStopTime);
    LIS331HH_videoCapArgs[0] = "0";
    LIS331HH_videoCapArgs[1] = TAKEOFF_G_FORCE_str.c_str();
    LIS331HH_videoCapArgs[2] = backupSIFTStartTime_str.c_str();
    LIS331HH_videoCapArgs[3] = LIS331HH_calibrationFile;
    LIS331HH_videoCapArgs[4] = backupSIFTStopTime_str.c_str();
    pyRunFile("subscale_driver/LIS331_loop.py", 5, (char **)LIS331HH_videoCapArgs);

    // Then send on radio afterwards (into dispatch queue)
    auto ret = sendOnRadio();
  }
#endif

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
    if (vn && imuDataSourcePath == nullptr)
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
    if (imuDataSourcePath) {
      mLog = (void *)new LOGFromFile((LOGFromFile::UserCallback)callback, this, imuDataSourcePath);
      // Seek past times less than imuDataSourceOffset if any
      if (imuDataSourceOffset > 0) {
        IMUData imu;
        while (imu.timestamp / 1.0e9 < imuDataSourceOffset / 1000.0) {
          ((LOGFromFile *)mLog)->scanRow(imu); // Seek past this row
        }
      }
      ((LOGFromFile*)mLog)->receive();
    }
    else {
      mLog = (void*)new LOG((LOG::UserCallback)callback, this, mImu, flushIMUDataEveryNMilliseconds); //, nullptr /*mLidar*/, nullptr /*mLds*/);
      mImu->receive();
      ((LOG*)mLog)->receive();
    }

    // Always start the video to ensure we get some data in case takeoff detection fails or SIFT doesn't start etc.
    // Take an ascent video (on SIFT and video capture pi's)
    std::cout << "Starting failsafe video" << std::endl;
    pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);
  }
  else {
    if (forceNoIMU) std::cout << "Forcing no IMU" << std::endl;
    
    mLog = nullptr;
    mImu = nullptr;

    if (videoCapture) {
#ifndef USE_LIS331HH
      videoCaptureOnlyThread = std::make_unique<std::thread>([](){
        // Take the ascent video
        pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);

        std::cout << "Using backupSIFTStartTime for a delay of " << backupSIFTStartTime << " milliseconds, then switching cameras..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(backupSIFTStartTime));

        if (isRunningPython)      
          raise(SIGINT); // Stop python
        while (isRunningPython) { // Wait
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        gpioSetMode(26, PI_OUTPUT); // Set GPIO26 as output.
        gpioWrite(26, 1); // Set GPIO26 high.

        reportStatus(Status::SwitchingToSecondCamera);
        // Run the python videocapture script again on the second camera
        pyRunFile("subscale_driver/videoCapture.py", 0, nullptr);

        std::cout << "Using backupSIFTStopTime for a delay of " << backupSIFTStopTime << " milliseconds, then stopping 2nd camera..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(backupSIFTStopTime));

        if (isRunningPython)
          raise(SIGINT); // Stop python
        while (isRunningPython) { // Wait
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      });
#endif
    }
    else {
      //#ifndef USE_LIS331HH
      std::cout << "Using backupSIFTStartTime for a delay of " << backupSIFTStartTime << " milliseconds, then launching..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(backupSIFTStartTime));
      //#else
      // Alternate IMU handles this for us, all good
      //#endif

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

        if (imuDataSourcePath == nullptr) {
          delete ((LOG*)mLog);
        }
        else {
          delete ((LOGFromFile *)mLog);
        }
        // delete (mMotor);
	// delete (mLds);
	// delete (mLidar);
	delete (mImu);

	disconnect_GPIO();
	disconnect_Python();

	cout << "Main: Destoryed" << endl;
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
