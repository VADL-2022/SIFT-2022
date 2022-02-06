#include <iostream>
#include <pigpio.h>
#include <fstream>
#include "config.hpp"
#include "LOGFromFile.hpp"
#include "IMU.hpp"
// #include "LIDAR.hpp"
// #include "LDS.hpp"
#include <stdio.h>
#include <string.h>

using namespace std;

void LOGFromFile::scanRow(IMUData& out) {
    LOGFromFile* data = this;
    IMUData& imu = out;
    
    if (!data->first) {
      fscanf(data->mLog, "\n");
    }
    else {
      data->first = false;
    }
    // fscanf(data->mLog, "%f," // fseconds -- timestamp in seconds since gpio library initialization (that is, essentially since the driver program started)
    // 	   , &imu.fseconds);
    float timestampSeconds;
    fscanf(data->mLog,
	   "%f"
	   ",%f,%f,%f" // yprNed
	   ",%f,%f,%f,%f" // qtn
	   ",%f,%f,%f" // rate
	   ",%f,%f,%f" // accel
	   ",%f,%f,%f" // mag
	   ",%f,%f,%f" // temp,pres,dTime
	   ",%f,%f,%f" // dTheta
	   ",%f,%f,%f" // dVel
	   ",%f,%f,%f" // magNed
	   ",%f,%f,%f" // accelNed
	   ",%f,%f,%f" // linearAccelBody
	   ",%f,%f,%f" // linearAccelNed
	   "," // Nothing after this comma on purpose.
	   ,
	   &timestampSeconds,
	   &imu.yprNed.x, &imu.yprNed.y, &imu.yprNed.z,
	   &imu.qtn.x, &imu.qtn.y, &imu.qtn.z, &imu.qtn.w,
	   &imu.rate.x, &imu.rate.y, &imu.rate.z,
	   &imu.accel.x, &imu.accel.y, &imu.accel.z,
	   &imu.mag.x, &imu.mag.y, &imu.mag.z,
	   &imu.temp, &imu.pres, &imu.dTime,
	   &imu.dTheta.x, &imu.dTheta.y, &imu.dTheta.z,
	   &imu.dVel.x, &imu.dVel.y, &imu.dVel.z,
	   &imu.magNed.x, &imu.magNed.y, &imu.magNed.z,
	   &imu.accelNed.x, &imu.accelNed.y, &imu.accelNed.z,
	   &imu.linearAccelBody.x, &imu.linearAccelBody.y, &imu.linearAccelBody.z,
	   &imu.linearAccelNed.x, &imu.linearAccelNed.y, &imu.linearAccelNed.z
	   );
    imu.timestamp = timestampSeconds * 1e9; // Convert to nanoseconds
}

void LOGFromFile::callback(void *userData)
{
    LOGFromFile *data = (LOGFromFile *)userData;

    IMUData& imu = *data->mImu;
    data->scanRow(imu);
    
    int seconds;
    int microseconds;
    float fseconds;

    gpioTime(PI_TIME_RELATIVE, &seconds, &microseconds);
    fseconds = seconds + microseconds / 1000000.0;
    if (data->userCallback != nullptr) {
      reinterpret_cast<UserCallback>(data->userCallback)(data, fseconds);
    }

    if (VERBOSE)
    {
        cout << "Yaw: " << data->mImu->yprNed[0] << " Pitch: " << data->mImu->yprNed[1] << " Roll: " << data->mImu->yprNed[2]
             << " Accel: " << data->mImu->linearAccelBody.mag()
             << " Distance: " << 0.0f /*data->mLidar->distance*/ << endl;
    }
}

LOGFromFile::LOGFromFile(UserCallback userCallback_, void* callbackUserData_, const char* filename) : callbackUserData(callbackUserData_)
{
    mImu = &mImu_;
    userCallback = reinterpret_cast<void(*)()>(userCallback_);
    
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "LogFromFile: Connecting" << endl;

        mLog = fopen(filename, "r");

        if (!mLog)
        {
	  cout << "LogFromFile: Failed to Open Log File " << filename << " with error " << strerror(errno) << endl;
	  exit(1);
        }

	// Skip first line
	char c;
	do {
	  c = fgetc(mLog);
	} while (c != '\n');

        cout << "LogFromFile: File Opened: " << filename << endl;
        cout << "LogFromFile: Connected" << endl;
    }
}

LOGFromFile::~LOGFromFile()
{
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "Log: Disconnecting" << endl;

        halt();
	fclose(mLog);

        cout << "Log: Disconnected" << endl;
    }
}

void LOGFromFile::receive()
{
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "LOG: Initiating Receiver" << endl;

        gpioSetTimerFuncEx(LOG_TIMER, 1000 / FREQUENCY, callback, this);

        cout << "LIDAR: Initiated Receiver" << endl;
    }
}

void LOGFromFile::halt()
{
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "LOG: Destroying Receiver" << endl;

        gpioSetTimerFuncEx(LOG_TIMER, 1000 / FREQUENCY, nullptr, this);

        cout << "LOG: Destroyed Receiver" << endl;
    }
}
