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
double Timer::elapsed() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>
            (clock_::now() - beg_).count(); }
void Timer::logElapsed(const char* name) const {
    std::cout << name << " took " << elapsed() << " milliseconds" << std::endl;
}
void Timer::logElapsed(int threadID, const char* name) const {
    std::cout << "Thread " << threadID << ": " << name << " took " << elapsed() << " milliseconds" << std::endl;
}

#include "Timer.h"
// C API:

extern "C" {

void Timer_Timer(struct Timer_C* out_t) {
    new (reinterpret_cast<Timer*>(out_t)) Timer();
}
void Timer_reset(struct Timer_C* t) {
    reinterpret_cast<Timer*>(t)->reset();
}
double Timer_elapsed(struct Timer_C* t) {
    return reinterpret_cast<Timer*>(t)->elapsed();
}
void Timer_logElapsed(struct Timer_C* t, const char* name) {
    reinterpret_cast<Timer*>(t)->logElapsed(name);
}
void Timer_logElapsed_thread(struct Timer_C* t, int threadID, const char* name) {
    reinterpret_cast<Timer*>(t)->logElapsed(threadID, name);
}

}
