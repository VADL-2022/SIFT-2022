//
//  timers.hpp
//  SIFT
//
//  Created by VADL on 12/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef timers_hpp
#define timers_hpp

#include "Timer.hpp"

extern thread_local Timer mallocFreeTimer;

// Per-thread malloc timer. To access and reset it, grab it from each thread that runs SIFT or does any form of memory allocation.
extern thread_local std::chrono::nanoseconds::rep mallocTimerAccumulator; // http://www.cplusplus.com/reference/chrono/nanoseconds/
// Per-thread free timer.
extern thread_local std::chrono::nanoseconds::rep freeTimerAccumulator;

#define preMalloc() { mallocFreeTimer.reset(); }
#define postMalloc() { mallocTimerAccumulator += mallocFreeTimer.elapsedNanos(); }
#define preFree() { mallocFreeTimer.reset(); }
#define postFree() { freeTimerAccumulator += mallocFreeTimer.elapsedNanos(); }


// Stats
extern thread_local size_t numMallocsThisFrame;
extern thread_local size_t numPointerIncMallocsThisFrame;
extern thread_local size_t mallocWithFreeAll_hitLimitCount;


#endif /* timers_hpp */
