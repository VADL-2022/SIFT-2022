//
//  pthread_mutex_lock_.cpp
//  SIFT
//
//  Created by VADL on 3/6/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "pthread_mutex_lock_.h"

#ifndef CMD_CONFIG
// Subscale driver, etc.
#define CMD_CONFIG(x) (false) // debugMutexDeadlocks is not supported from subscale driver for now
#include <iostream>
#include "tools/backtrace/backtrace.hpp"
#include "../commonOutMutex.hpp"
#else
// SIFT
#include "main/siftMainCmdConfig.hpp"
#endif
#include <thread>
#include "tools/backtrace/backtrace.hpp"

int pthread_mutex_lock_(pthread_mutex_t *mutex) {
    if (CMD_CONFIG(debugMutexDeadlocks)) {
        // Try to lock for a bit with a timer
        std::chrono::time_point<std::chrono::steady_clock> startTries; // https://en.cppreference.com/w/cpp/chrono/time_point
        bool setStartTries = false, showedMessage = false;
        while (true) {
            int ret = pthread_mutex_trylock(mutex);
            if (ret == 0) {
                // Locked successfully
                if (showedMessage) {
                    // Inform the user about the successful lock since we showed a message saying it wasn't locking previously
                    { out_guard(); std::cout << "******* A mutex was locked after not locking soon enough previously. Stack trace: *******\n";
                        backtrace(std::cout);
                    }
                }
                return ret;
            }
            else if (ret == EBUSY) {
                // Already locked, try again next time
                if (!setStartTries) {
                    startTries = std::chrono::steady_clock::now();
                    setStartTries = true;
                }
                else {
                    // Check too long elapsed
                    auto now = std::chrono::steady_clock::now();
                    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTries);
                    if (millis.count() > CMD_CONFIG(debugReportMutexLockAttemptsLongerThanNMilliseconds)) {
                        if (!showedMessage) { out_guard(); std::cout << "****************************************************************** A mutex is taking too long to lock! Stack trace of failing lock attempt: ******************************************************************\n";
                            backtrace(std::cout);
                            showedMessage = true;
                            // TODO: as part of continuous integration, use this fantastic library for debugging race conditions and locks etc.: Helgrind (part of Valgrind, use `--tool=helgrind` for Valgrind command line): https://pmem.io/valgrind/generated/hg-manual.html
                        }
                    }
                }
                // Sleep a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            else {
                // Report error
                printf("pthread_mutex_trylock() returned an error: %d", ret); // For more info: https://pubs.opengroup.org/onlinepubs/9699919799/functions/pthread_mutex_lock.html
                return ret;
            }
        }
    }
    else {
        return pthread_mutex_lock(mutex);
    }
}
