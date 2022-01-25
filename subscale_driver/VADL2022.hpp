#ifndef VADL2022_HPP
#define VADL2022_HPP

#include "LOG.hpp"
#include "IMU.hpp"

using namespace std;

class VADL2022
{
public:
    LOG *mLog = nullptr;     // LOG
    IMU *mImu = nullptr;     // IMU
    // LIDAR *mLidar; // LIDAR
    // LDS *mLds;     // LDS
    // MOTOR *mMotor; // ODRIVE

    VADL2022(int argc, char** argv);
    ~VADL2022();

    // void GDS();
    // void PDS();
    // void IS();
    // void LS();
    // void COMMS();

    float startTime = -1; // Seconds

private:
    //bool GDSTimeout = 0;

    void connect_GPIO();
    void disconnect_GPIO();
    void connect_Python();
    void disconnect_Python();

    //static void *GDSReleaseTimeoutThread(void *);
};

#endif
