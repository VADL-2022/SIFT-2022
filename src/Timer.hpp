//
//  Timer.hpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef Timer_hpp
#define Timer_hpp

#include <chrono>

#include <iostream>
#include "../common.hpp"

class Timer {
public:
    Timer();
    void reset();
    std::chrono::milliseconds::rep elapsed() const;
    std::chrono::nanoseconds::rep elapsedNanos() const;
    void logElapsed(const char* name) const;
    void logElapsed(int threadID, const char* name) const;
    template <typename ThreadInfoT /* can be threadID or thread name */>
    static void logNanos(ThreadInfoT threadInfo, const char* name, std::chrono::nanoseconds::rep nanos) {
        { out_guard();
            std::cout << "Thread " << threadInfo << ": " << name << " took " << nanos / 1000.0 << " milliseconds" << std::endl; }
    }
private:
    typedef std::chrono::high_resolution_clock clock_;
    clock_::time_point beg_;
};

#endif /* Timer_hpp */
