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
