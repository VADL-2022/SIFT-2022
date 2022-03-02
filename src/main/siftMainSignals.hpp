//
//  siftMainSignals.hpp
//  SIFT
//
//  Created by VADL on 3/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef siftMainSignals_hpp
#define siftMainSignals_hpp

#include <atomic>

extern std::atomic<bool> g_stop;

struct DataOutputBase;
extern DataOutputBase* g_o2;

// Installs signal handlers on the given thread. Run this for each thread... even though it isn't apparently required, it is for safety in case we forget.. ( https://unix.stackexchange.com/questions/225687/what-happens-to-a-multithreaded-linux-process-if-it-gets-a-signal )
extern void installSignalHandlers();

#endif /* siftMainSignals_hpp */
