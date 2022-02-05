#ifndef LOGFROMFILE_HPP
#define LOGFROMFILE_HPP

#include <fstream>
#include "IMU.hpp"
//#include "LIDAR.hpp"
//#include "LDS.hpp"
#include "../src/IMUData.hpp"

class LOGFromFile
{
public:
    FILE* mLog; // Log File

    typedef void (*UserCallback)(LOGFromFile* log, float fseconds);
    // Anything except IMU* may be nullptr:
    LOGFromFile(UserCallback userCallback_, void* callbackUserData_, const char* filename);
    ~LOGFromFile();
    void receive();
    void halt();

    UserCallback userCallback;
    void* callbackUserData;
    IMUData *mImu;     // IMU
    
private:
    // LIDAR *mLidar; // LIDAR
    // LDS *mLds;     // LDS

    static void callback(void *);
};

#endif
