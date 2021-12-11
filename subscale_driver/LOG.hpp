#ifndef LOG_HPP
#define LOG_HPP

#include <fstream>
#include "IMU.hpp"
//#include "LIDAR.hpp"
//#include "LDS.hpp"

using namespace std;
class LOG
{
public:
    ofstream mLog; // Log File

    typedef void (*UserCallback)(LOG* log, float fseconds);
    // Anything except IMU* may be nullptr:
    LOG(UserCallback userCallback_, void* callbackUserData_, IMU *);
    ~LOG();
    void receive();
    void halt();
    void write(string);

private:
    UserCallback userCallback;
    void* callbackUserData;
    IMU *mImu;     // IMU
    // LIDAR *mLidar; // LIDAR
    // LDS *mLds;     // LDS

    static void callback(void *);
};

#endif
