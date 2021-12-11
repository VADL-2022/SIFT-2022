#ifndef LOG_HPP
#define LOG_HPP

#include <fstream>
#include "../include/IMU.hpp"
#include "../include/LIDAR.hpp"
#include "../include/LDS.hpp"

using namespace std;
class LOG
{
public:
    ofstream mLog; // Log File

    typedef void (*UserCallback)(LOG* log, float fseconds);
    // Anything except IMU* may be nullptr:
    LOG(UserCallback userCallback_, IMU *, LIDAR *, LDS *);
    ~LOG();
    void receive();
    void halt();
    void write(string);

private:
    UserCallback userCallback;
    IMU *mImu;     // IMU
    LIDAR *mLidar; // LIDAR
    LDS *mLds;     // LDS

    static void callback(void *);
};

#endif
