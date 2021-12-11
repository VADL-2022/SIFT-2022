#include <iostream>
#include <pigpio.h>
#include <fstream>
#include "../include/config.hpp"
#include "../include/LOG.hpp"
#include "../include/IMU.hpp"
#include "../include/LIDAR.hpp"
#include "../include/LDS.hpp"

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
      data->userCallback(data, fseconds);
    }

    if (LOG_ACTIVE)
    {
        data->mLog << endl
                   << fseconds << ","
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
        cout << "Yaw: " << data->mImu->yprNed[0] << " Pitch: " << data->mImu->yprNed[1] << " Roll: " << data->mImu->yprNed[2]
             << " Accel: " << data->mImu->linearAccelBody.mag()
             << " Distance: " << data->mLidar->distance << endl;
    }
}

LOG::LOG(UserCallback userCallback_, IMU *imu, LIDAR *lidar, LDS *lds) : userCallback(userCallback_), mImu(imu), mLidar(lidar), mLds(lds)
{
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "Log: Connecting" << endl;

        time_t now = time(nullptr);
        char timestamp[17];

        sprintf(timestamp, "_%04d%02d%02d_%02d%02d%02d",
                localtime(&now)->tm_year + 1900,
                localtime(&now)->tm_mon + 1,
                localtime(&now)->tm_mday,
                localtime(&now)->tm_hour,
                localtime(&now)->tm_min,
                localtime(&now)->tm_sec);
        mLog.open(LOG_FILE + timestamp + ".csv");

        if (!mLog.is_open())
        {
            cout << "Log: Failed to Open Log File" << endl;
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

        cout << "LOG: File Created at " << LOG_FILE << timestamp << ".csv" << endl;
        cout << "Log: Connected" << endl;
    }
}

LOG::~LOG()
{
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "Log: Disconnecting" << endl;

        halt();
        mLog.close();

        cout << "Log: Disconnected" << endl;
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
        cout << "LOG: Initiating Receiver" << endl;

        gpioSetTimerFuncEx(LOG_TIMER, 1000 / FREQUENCY, callback, this);

        cout << "LIDAR: Initiated Receiver" << endl;
    }
}

void LOG::halt()
{
    if (LOG_ACTIVE || VERBOSE)
    {
        cout << "LOG: Destroying Receiver" << endl;

        gpioSetTimerFuncEx(LOG_TIMER, 1000 / FREQUENCY, nullptr, this);

        cout << "LOG: Destroyed Receiver" << endl;
    }
}
