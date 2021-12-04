//
//  Timer.cpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "Timer.hpp"

#include <iostream>

// Based on https://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c
Timer::Timer() : beg_(clock_::now()) {}
void Timer::reset() { beg_ = clock_::now(); }
std::chrono::milliseconds::rep Timer::elapsed() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>
            (clock_::now() - beg_).count(); }
std::chrono::nanoseconds::rep Timer::elapsedNanos() const {
return std::chrono::duration_cast<std::chrono::nanoseconds>
        (clock_::now() - beg_).count(); }
void Timer::logElapsed(const char* name) const {
    std::cout << name << " took " << elapsed() << " milliseconds" << std::endl;
}
void Timer::logElapsed(int threadID, const char* name) const {
    std::cout << "Thread " << threadID << ": " << name << " took " << elapsed() << " milliseconds" << std::endl;
}
