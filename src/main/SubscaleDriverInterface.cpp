//
//  SubscaleDriverInterface.cpp
//  SIFT
//
//  Created by VADL on 2/11/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "SubscaleDriverInterface.hpp"

#include "../common.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../siftMain.hpp"

int subscaleDriverInterface_count;
IMUData subscaleDriverInterface_imu;
std::mutex subscaleDriverInterfaceMutex_imuData;

void runOneIteration(int driverInput_fd) {
    // IMU
    IMUData& imu = subscaleDriverInterface_imu;
    if (CMD_CONFIG(verbose)) {
        t.reset();
        std::cout << "Subscale driver interface thread: grabbing from subscale driver fd..." << std::endl;
    }
    const double EPSILON = 0.001;
    int count=0;
    // Read all data out first
#define MSGSIZE 65536
    char buf[MSGSIZE+1/*for null terminator*/];
    while (true) {
        int cnt;
        if (ioctl(driverInput_fd, FIONREAD, &cnt) < 0) { // "If you're on Linux, this call should tell you how many unread bytes are in the pipe" --Professor Balasubramanian
            perror("ioctl to check bytes in driverInput_fd failed (ignoring)");
        }
        else {
            if (CMD_CONFIG(verbose)) {
                printf("ioctl says %d bytes are left in the driverInput_fd pipe\n", cnt);
            }
        }
        
        // Read from fd
        // NOTE: this is set up to block until the fd gets data.
        ssize_t nread = read(driverInput_fd, buf, MSGSIZE); // "The read will return "resource temporarily unavailable" if the pipe is empty, O_NONBLOCK is set, and you try to read." --Professor Balasubramanian

        if (CMD_CONFIG(verbose)) {
            std::cout << "nread from driverInput_fd: " << nread << std::endl;
        }
        if (nread == -1) {
            perror("Error read()ing from driverInput_fd. Ignoring it for now. Error was"/* <The error is printed here by perror> */);
            break;
        }
        else if (nread == 0) {
            if (CMD_CONFIG(verbose)) {
                if (count == 0)
                    std::cout << "No IMU data present yet" << std::endl;
                else
                    std::cout << "No IMU data left" << std::endl;
            }
            break;
        }
        buf[nread] = '\0'; // Null terminate
        
        // Grab from the buf
        int charsReadTotal = 0;
        subscaleDriverInterfaceMutex_imuData.lock();
        while (true) {
            // https://stackoverflow.com/questions/3320533/how-can-i-get-how-many-bytes-sscanf-s-read-in-its-last-operation : "With scanf and family, use %n in the format string. It won't read anything in, but it will cause the number of characters read so far (by this call) to be stored in the corresponding parameter (expects an int*)."
            int charsRead;
            int ret = sscanf(buf + charsReadTotal, "\n%a" // fseconds -- timestamp in seconds since gpio library initialization (that is, essentially since the driver program started)
                             "%n"
                   , &imu.fseconds, &charsRead); // Returns number of items read on success
            if (ret == EOF || ret != 1 /*want 1 item read*/) {
                // EOF is ok/expected once we read all data
                if (CMD_CONFIG(verbose) || ret != EOF) {
                    std::cout << "driverInput_fd gave incomplete data for fseconds (" << (ret == EOF ? std::string("EOF") : std::to_string(ret)) << "), ignoring for now" << std::endl;
                }
                break;
            }
            charsReadTotal += charsRead;
            if (CMD_CONFIG(verbose)) {
                std::cout << "fseconds: " << imu.fseconds << ", charsRead: " << charsRead << std::endl;
            }
            if (fabs(imu.fseconds- -1) < EPSILON) {
                // IMU failed, ignore its data
                std::cout << "SIFT considering IMU as failed for this grab attempt." << std::endl;
//                fclose(driverInput_file);
//                driverInput_file = nullptr;
                continue; // Make sure to seek past all the other junk that may be after this -1 value.
            }
            ret = sscanf(buf + charsReadTotal,
               "," "%" PRIu64 "" // timestamp  // `PRIu64` is a nasty thing needed for uint64_t format strings.. ( https://stackoverflow.com/questions/9225567/how-to-print-a-int64-t-type-in-c )
               ",%a,%a,%a" // yprNed
               ",%a,%a,%a,%a" // qtn
               ",%a,%a,%a" // rate
               ",%a,%a,%a" // accel
               ",%a,%a,%a" // mag
               ",%a,%a,%a" // temp,pres,dTime
               ",%a,%a,%a" // dTheta
               ",%a,%a,%a" // dVel
               ",%a,%a,%a" // magNed
               ",%a,%a,%a" // accelNed
               ",%a,%a,%a" // linearAccelBody
               ",%a,%a,%a" // linearAccelNed
               "," // Nothing after this comma on purpose.
               "%n"
               ,
               &imu.timestamp,
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
               &imu.linearAccelNed.x, &imu.linearAccelNed.y, &imu.linearAccelNed.z,
               &charsRead
               );
            if (ret == EOF || ret != 38 /*number of `&imu`[...] items passed to the sscanf above*/) {
                // (EOF here is an error)
                std::cout << "driverInput_fd gave incomplete data (" << (ret == EOF ? std::string("EOF") : std::to_string(ret)) << "), ignoring for now" << std::endl;
                break;
            }
            if (CMD_CONFIG(verbose) {
                std::cout << "charsRead: " << charsRead << std::endl;
            }
            charsReadTotal += charsRead;
            count++;
        }
        subscaleDriverInterface_count = count;
        subscaleDriverInterfaceMutex_imuData.unlock();
        if (count > 0) {
            break;
        }
    }
    if (CMD_CONFIG(verbose)) {
        std::cout << "subscale driver interface thread: read " << count << " line(s) of IMU data" << std::endl; // Should be 1 line read (count == 1) ideally
        t.logElapsed("subscale driver interface thread: grabbing from subscale driver fd");
    }
}

void* subscaleDriverInterfaceThreadFunc(void* arg) {
    static_assert(sizeof(void*) >= sizeof(int));
    int driverInput_fd = (int)(intmax_t)arg;
    while (!stoppedMain()) {
        runOneIteration(driverInput_fd);
    }
    return nullptr;
}
