#ifndef VADL2022_HPP
#define VADL2022_HPP

#include "LOG.hpp"
#include "LOGFromFile.hpp"
#include "IMU.hpp"

using namespace std;

class VADL2022
{
public:
    void* /*LOG or LOGFromFile*/ mLog = nullptr;
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

    //bool GDSTimeout = 0;

    const char* imuDataSourcePath = nullptr;

    const char* launchBox = nullptr;
    const char* launchAngle = nullptr;
    
private:
    void connect_GPIO(bool initCppGpio);
    void disconnect_GPIO();
    void connect_Python();
public:
    static void disconnect_Python();
private:

    //static void *GDSReleaseTimeoutThread(void *);
};

#endif
