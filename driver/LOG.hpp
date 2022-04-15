#ifndef LOG_HPP
#define LOG_HPP

#include <fstream>
#include "IMU.hpp"
//#include "LIDAR.hpp"
//#include "LDS.hpp"
#include "LOGBase.hpp"

class LOG : public LOGBase
{
public:
    std::ofstream mLog; // Log File
    std::string mLogFileName; // File name of the opened log file in the ofstream above, if any

    typedef void (*UserCallback)(LOG* log, float fseconds);
    // Anything except IMU* may be nullptr:
    LOG(UserCallback userCallback_, void* callbackUserData_, IMU *, long long flushToLogEveryNMilliseconds_=0 /*0 to flush immediately*/);
    ~LOG();
    void receive();
    void halt();
    void write(std::string);

    void* callbackUserData;
    IMU *mImu;     // IMU
    long long flushToLogEveryNMilliseconds;
    float fseconds_lastFlush = -1;
    
private:
    // LIDAR *mLidar; // LIDAR
    // LDS *mLds;     // LDS

    static void callback(void *);
};

#endif
