#include <iostream>
#include <pigpio.h>
#include <fstream>
#include "config.hpp"
#include "LOG.hpp"
#include "IMU.hpp"
// #include "LIDAR.hpp"
// #include "LDS.hpp"
#include "../common.hpp"

using namespace std;

void LOG::callback(void *userData)
{
    LOG *data = (LOG *)userData;
    int seconds;
    int microseconds;
    float fseconds;

    gpioTime(PI_TIME_RELATIVE, &seconds, &microseconds);
    fseconds = seconds + microseconds / 1000000.0;
    if (data->userCallback != nullptr) {
      reinterpret_cast<UserCallback>(data->userCallback)(data, fseconds);
    }

    if (LOG_ACTIVE)
    {
      bool flush;
      float flushToLogEveryNSeconds = data->flushToLogEveryNMilliseconds / 1000.0;
      if (data->flushToLogEveryNMilliseconds == 0) {
	flush = true;
      }
      else if (data->fseconds_lastFlush == -1 || fseconds - data->fseconds_lastFlush > flushToLogEveryNSeconds) {
	flush = true;
	data->fseconds_lastFlush = fseconds;
      }
      else {
        flush = false;
      }

      if (flush) {
        if (data->flushToLogEveryNMilliseconds != 0) {
	    { out_guard();
	      std::cout << "Flushing IMU data to log" << std::endl; }
        }
        data->mLog << endl;
      }
      else {
        data->mLog << '\n';
      }
      data->mLog << fseconds << /*"," << data->mImu->timestamp <<*/ ","
                   << data->mImu->yprNed[0] << "," << data->mImu->yprNed[1] << "," << data->mImu->yprNed[2] << ","
                   << data->mImu->qtn[0] << "," << data->mImu->qtn[1] << "," << data->mImu->qtn[2] << "," << data->mImu->qtn[3] << ","
                   << data->mImu->rate[0] << "," << data->mImu->rate[1] << "," << data->mImu->rate[2] << ","
                   << data->mImu->accel[0] << "," << data->mImu->accel[1] << "," << data->mImu->accel[2] << ","
                   << data->mImu->mag[0] << "," << data->mImu->mag[1] << "," << data->mImu->mag[2] << ","
                   << data->mImu->temp << "," << data->mImu->pres << "," << data->mImu->dTime << ","
                   << data->mImu->dTheta[0] << "," << data->mImu->dTheta[1] << "," << data->mImu->dTheta[2] << ","
                   << data->mImu->dVel[0] << "," << data->mImu->dVel[1] << "," << data->mImu->dVel[2] << ","
                   << data->mImu->magNed[0] << "," << data->mImu->magNed[1] << "," << data->mImu->magNed[2] << ","
                   << data->mImu->accelNed[0] << "," << data->mImu->accelNed[1] << "," << data->mImu->accelNed[2] << ","
                   << data->mImu->linearAccelBody[0] << "," << data->mImu->linearAccelBody[1] << "," << data->mImu->linearAccelBody[2] << ","
                   << data->mImu->linearAccelNed[0] << "," << data->mImu->linearAccelNed[1] << "," << data->mImu->linearAccelNed[2] << ","
	  //<< data->mLidar->amplitude << "," << data->mLidar->distance << ","
	  //    << data->mLds->light[0] << "," << data->mLds->light[1] << "," << data->mLds->light[2] << "," << data->mLds->light[3] << ",";
	  ;
    }
    if (VERBOSE)
    {
        { out_guard();
	  cout << "Yaw: " << data->mImu->yprNed[0] << " Pitch: " << data->mImu->yprNed[1] << " Roll: " << data->mImu->yprNed[2]
	       << " Accel: " << data->mImu->linearAccelBody.mag()
	       << " Distance: " << 0.0f /*data->mLidar->distance*/ << endl; }
    }
}

LOG::LOG(UserCallback userCallback_, void* callbackUserData_, IMU *imu, long long flushToLogEveryNMilliseconds_) : callbackUserData(callbackUserData_), mImu(imu),
														  flushToLogEveryNMilliseconds(flushToLogEveryNMilliseconds_)
{
    userCallback = reinterpret_cast<void(*)()>(userCallback_);
  
    if (LOG_ACTIVE || VERBOSE)
    {
        { out_guard();
          cout << "Log: Connecting" << endl; }

        time_t now = time(nullptr);
        char timestamp[17];

        sprintf(timestamp, "_%04d%02d%02d_%02d%02d%02d",
                localtime(&now)->tm_year + 1900,
                localtime(&now)->tm_mon + 1,
                localtime(&now)->tm_mday,
                localtime(&now)->tm_hour,
                localtime(&now)->tm_min,
                localtime(&now)->tm_sec);
	mLogFileName = LOG_FILE + timestamp + ".csv";
        mLog.open(mLogFileName);

        if (!mLog.is_open())
        {
	    { out_guard();
              cout << "Log: Failed to Open Log File" << endl; }
            exit(1);
        }

        mLog << "Timestamp,"
             << "Yaw,Pitch,Roll,"
             << "Qtn[0], Qtn[1], Qtn[2], Qtn[3],"
             << "Rate X,Rate Y,Rate Z,"
             << "Accel X,Accel Y,Accel Z,"
             << "Mag X,Mag Y,Mag Z,"
             << "Temp,Pres,dTime,"
             << "dTheta X,dTheta Y,dTheta Z,"
             << "dVel X,dVel Y,dVel Z,"
             << "MagNed X,MagNed Y,MagNed Z,"
             << "AccelNed X,AccelNed Y,AccelNed Z,"
             << "LinearAccel X,LinearAccel Y,LinearAccel Z,"
             << "LinearAccelNed X,LinearAccelNed Y,LinearAccelNed Z,"
             << "Amplitude,Distance,"
             << "Light 1,Light 2,Light 3,Light 4,"
             << "Comments";

	{ out_guard();
	  cout << "LOG: File Created at " << LOG_FILE << timestamp << ".csv" << endl;
	  cout << "Log: Connected" << endl; }
    }
}

LOG::~LOG()
{
    if (LOG_ACTIVE || VERBOSE)
    {
        { out_guard();
	  cout << "Log: Disconnecting" << endl; }

        halt();
        mLog.close();

	{ out_guard();
	  cout << "Log: Disconnected" << endl; }
    }
}

void LOG::write(string data)
{
    if (LOG_ACTIVE || VERBOSE)
    {
        mLog << data;

        return;
    }
}

void LOG::receive()
{
    if (LOG_ACTIVE || VERBOSE)
    {
        { out_guard();
	  cout << "LOG: Initiating Receiver" << endl; }

        gpioSetTimerFuncEx(LOG_TIMER, 1000 / FREQUENCY, callback, this);

	{ out_guard();
	  cout << "LIDAR: Initiated Receiver" << endl; }
    }
}

void LOG::halt()
{
    if (LOG_ACTIVE || VERBOSE)
    {
	{ out_guard();
	  cout << "LOG: Destroying Receiver" << endl; }

        gpioSetTimerFuncEx(LOG_TIMER, 1000 / FREQUENCY, nullptr, this);

	{ out_guard();
	  cout << "LOG: Destroyed Receiver" << endl; }
    }
}
