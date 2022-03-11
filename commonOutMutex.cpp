//
//  commonOutMutex.cpp
//  SIFT
//
//  Created by VADL on 3/10/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "commonOutMutex.hpp"
#include "commonOutMutex.h"

namespace common {
    std::mutex outMutex;
}

extern "C" void out_guard_lock() {
    common::outMutex.lock();
}
extern "C" void out_guard_unlock() {
    common::outMutex.unlock();
}
