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

using namespace std;

const float GS = 6;
const float IMU_ACCEL_MAGNITUDE_THRESHOLD = GS * 9.81; // g's converted to meters per second squared.
const float IMU_ACCEL_DURATION = 1.0 / 10.0; // Seconds
const char* /* must fit in long long */ timeFromTakeoffToMainDeploymentAndStabilization = "2000"; // Milliseconds

void startDelayedSIFT(VADL2022 *v) {
  puts("Forking");
  pid_t pid = fork(); // create a new child process
  if (pid > 0) {
    int status = 0;
    if (wait(&status) != -1) {
      if (WIFEXITED(status)) {
	// now check to see what its exit status was
	printf("The exit status was: %d\n", WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
	// it was killed by a signal
	printf("The signal that killed me was %d\n", WTERMSIG(status));
      }
    } else {
      printf("Error waiting!\n");
    }
  } else if (pid == 0) {
    const char *args[] = { "./sift_exe_release_commandLine","--main-mission", "--sift-params","-C_edge 2", "--sleep-before-running",(timeFromTakeoffToMainDeploymentAndStabilization), (const char *)0 };
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
    float duration = v->currentTime - v->startTime;
    printf("Exceeded acceleration magnitude threshold for %f seconds\n", duration);
    v->currentTime = fseconds;
    if (duration >= IMU_ACCEL_DURATION) {
      // Stop these checkTakeoffCallback callbacks
      #if !defined(__x86_64__) && !defined(__i386__) && !defined(__arm64__) && !defined(__aarch64__)
      #error On these processor architectures above, pointer store or load should be an atomic operation. But without these, check the specifics of the processor.
      #else
      v->mLog->userCallback = nullptr;
      #endif
      
      puts("Target time reached, we are considered having just lifted off");
      
      // Start SIFT which will wait for the configured amount of time until main parachute deployment and stabilization:
      startDelayedSIFT(v);
    }
  }
  else {
    // Reset timer
    if (v->currentTime != 0) {
      puts("Not enough acceleration; resetting timer");
    }
    v->startTime = fseconds;
    v->currentTime = 0;
  }
}

VADL2022::VADL2022(int argc, char** argv)
{
	cout << "Main: Initiating" << endl;

	// Parse command-line args
	LOG::UserCallback callback = checkTakeoffCallback;
	for (int i = 1; i < argc; i++) {
          if (strcmp(argv[i], "--imu-record-only") == 0) {
	    callback = nullptr;
          }
        }
	
	connect_GPIO();
	connect_Python();
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

	cout << "Python: Connected" << endl;
}

void VADL2022::disconnect_Python()
{
	cout << "Python: Disconnecting" << endl;

	Py_Finalize();

	cout << "Python: Disconnected" << endl;
}
