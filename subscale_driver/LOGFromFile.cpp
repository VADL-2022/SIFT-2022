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

void LOGFromFile::callback(void *userData)
{
    LOGFromFile *data = (LOGFromFile *)userData;

    IMUData& imu = *data->mImu;
    fscanf(data->mLog, "\n%f" // fseconds -- timestamp in seconds since gpio library initialization (that is, essentially since the driver program started)
	   , &imu.fseconds);
    fscanf(data->mLog,
	   ",$x,$x,$x" // yprNed
	   ",$x,$x,$x,$x" // qtn
	   ",$x,$x,$x" // rate
	   ",$x,$x,$x" // accel
	   ",$x,$x,$x" // mag
	   ",$x,$x,$x" // temp,pres,dTime
	   ",$x,$x,$x" // dTheta
	   ",$x,$x,$x" // dVel
	   ",$x,$x,$x" // magNed
	   ",$x,$x,$x" // accelNed
	   ",$x,$x,$x" // linearAccelBody
	   ",$x,$x,$x" // linearAccelNed
	   "," // Nothing after this comma on purpose.
	   ,
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

LOGFromFile::LOGFromFile(UserCallback userCallback_, void* callbackUserData_, const char* filename) : userCallback(reinterpret_cast<void(*)()>(userCallback_)), callbackUserData(callbackUserData_)
{
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "LogFromFile: Connecting" << endl;

        mLog = fopen(filename, "r");

        if (!mLog)
        {
	  cout << "LogFromFile: Failed to Open Log File " << filename << " with error " << strerror(errno) << endl;
	  exit(1);
        }

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
