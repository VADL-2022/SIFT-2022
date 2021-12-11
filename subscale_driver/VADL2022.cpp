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
const float IMU_ACCEL_MAGNITUDE_THRESHOLD = GS * 9.81; // g's converted to meters per second squared.
const float IMU_ACCEL_DURATION = 1.0 / 10.0; // Seconds
const char* /* must fit in long long */ timeFromTakeoffToMainDeploymentAndStabilization = nullptr; // Milliseconds

// Returns true on success
bool sendOnRadio() {
    const char* str = R"(

import random
if __name__ == '__main__':
    ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1) # gpio14
    ser.reset_input_buffer()
    
    while True:
        Data = random.randint(1,4)
        ser.write(str(data).encode('utf-8')))";
    std::cout << str << std::endl;
    
    PyObject *main_module = PyImport_AddModule("__main__"); /* borrowed */
    if(!main_module)
      return false;
    
    PyObject *global_dict = PyModule_GetDict(main_module); /* borrowed */
    
    PyObject *result = PyRun_StringFlags(str, Py_single_input /* Py_single_input for a single statement, or Py_file_input for more than a statement */, global_dict, global_dict, NULL);
    Py_XDECREF(result);

    if(PyErr_Occurred()) {
        S_ShowLastError();
	return false;
    }

    return true;
}

void startDelayedSIFT(VADL2022 *v) {
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
	bool sendOnRadio_ = false;
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
