//
//  malloc_with_free_all.h
//  SIFT
//
//  Created by VADL on 12/4/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef malloc_with_free_all_hpp
#define malloc_with_free_all_hpp

#ifdef __cplusplus
extern "C" {
#else
//#define thread_local _Thread_local // Supported by C11 C standard revision according to https://stackoverflow.com/questions/32245103/how-does-the-gcc-thread-work
#endif

#include <stddef.h>

enum MallocImpl {
    MallocImpl_Default,
    MallocImpl_PointerIncWithFreeAll
};
extern _Thread_local char* mallocWithFreeAll_origPtr;
extern _Thread_local char* mallocWithFreeAll_current;
extern _Thread_local char* mallocWithFreeAll_max;
extern _Thread_local enum MallocImpl currentMallocImpl;

// For the current thread, this function makes all malloc calls only a pointer increment when malloc is called with the given size limits (`reservedBytes`) remaining in available memory. If malloc is called (before a call to `endMallocWithFreeAll()`) and that would require more than the size remaining available, then regular malloc will be used for that malloc call.
void beginMallocWithFreeAll(size_t reservedBytes);

// For the current thread, frees all resources allocated with the above and goes back to regular malloc.
void endMallocWithFreeAll();

#ifdef __cplusplus
}
#endif

#endif /* malloc_with_free_all_hpp */
