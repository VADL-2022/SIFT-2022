//
//  malloc_with_free_all.cpp
//  SIFT
//
//  Created by VADL on 12/4/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "malloc_with_free_all.h"

#include "../timers.hpp"

extern "C" {

thread_local char* mallocWithFreeAll_origPtr;
thread_local char* mallocWithFreeAll_current;
thread_local char* mallocWithFreeAll_max;
thread_local enum MallocImpl currentMallocImpl = MallocImpl_Default;

void beginMallocWithFreeAll(size_t reservedBytes) {
    preMalloc();
    mallocWithFreeAll_origPtr = mallocWithFreeAll_current = (char*)malloc(reservedBytes); // Call the real malloc to get memory
    postMalloc();
    mallocWithFreeAll_max = mallocWithFreeAll_current + reservedBytes;
    currentMallocImpl = MallocImpl_PointerIncWithFreeAll;
}

void endMallocWithFreeAll() {
    free(mallocWithFreeAll_origPtr);
    currentMallocImpl = MallocImpl_Default;
}

}
