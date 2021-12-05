//
//  malloc_override_osx.hpp
//  SIFT
//
//  Created by VADL on 12/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//
// https://gist.github.com/monitorjbl/3dc6d62cf5514892d5ab22a59ff34861

#ifndef malloc_override_osx_h
#define malloc_override_osx_h

#include "../timers.hpp"
#include "malloc_with_free_all.h"

/* Overrides memory allocation functions so that allocation amounts can be
 * tracked. Note that the malloc'd size is not necessarily equal to the
 * size requested, but is indicative of how much memory was actually
 * allocated.
 *
 * If MALLOC_DEBUG_OUTPUT is defined, extra logging will be done to stderr
 * whenever a memory allocation is made/freed. Turning this on will add
 * significant overhead to memory allocation, which will cause program
 * slowdowns.s
 */

#ifdef MALLOC_DEBUG_OUTPUT
#define MALLOC_LOG(type, size, address)                                                     \
{                                                                                           \
  fprintf(stderr, "%p -> %s(%ld) -> %ld\n", (address), (type), (size), memory::current());  \
}
#else
#define MALLOC_LOG(X,Y,Z)
#endif

#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
#include <atomic>


#ifdef USE_JEMALLOC
#include "jemalloc_wrapper.h"
#else
#define IMPLmalloc_size(x) malloc_size(x)
#endif

//#define CALL_r_malloc(x) IMPLmalloc(x)
//#define CALL_r_calloc(x, y) IMPLcalloc(x, y)
//#define CALL_r_free(x) IMPLfree(x)
#ifdef __APPLE__
#define CALLmalloc_size(x) IMPLmalloc_size(x)
#else
#define CALLmalloc_size(x) 0
#endif

#define CALL_r_malloc(x) _r_malloc(x)
#define CALL_r_calloc(x, y) _r_calloc(x, y)
#define CALL_r_free(x) _r_free(x)
#define CALL_r_realloc(x, y) _r_realloc(x, y)


typedef void* (*real_malloc)(size_t);
typedef void* (*real_calloc)(size_t, size_t);
typedef void (*real_free)(void*);
typedef void* (*real_realloc)(void*, size_t);
extern "C" void* malloc(size_t);
extern "C" void* calloc(size_t, size_t);
extern "C" void free(void*);
extern "C" void* realloc(void*, size_t);

static real_malloc _r_malloc;
static real_calloc _r_calloc;
static real_free _r_free;
static real_realloc _r_realloc;

namespace memory{
    namespace _internal{

    #ifdef __APPLE__
        std::atomic_ulong allocated(0);
    #else
       std::atomic<unsigned long> allocated(0);
    #endif
    }

    unsigned long current(){
        return _internal::allocated.load();
    }
}

// NOTE: When linking jemalloc, these (at least in one case_ grab libjemalloc.2.dylib's malloc functions instead of the malloc, etc. from C standard library. It can be seen in the debugger of Xcode, and jemalloc_wrapper.h's functions cause infinite recursion if a call to them is implemented by the CALL_r_malloc(), etc. macros
static void mtrace_init(void){
    _r_malloc = reinterpret_cast<real_malloc>(reinterpret_cast<long>(dlsym(RTLD_NEXT, "malloc")));
    _r_calloc = reinterpret_cast<real_calloc>(reinterpret_cast<long>(dlsym(RTLD_NEXT, "calloc")));
    _r_free = reinterpret_cast<real_free>(reinterpret_cast<long>(dlsym(RTLD_NEXT, "free")));
    _r_realloc = reinterpret_cast<real_realloc>(reinterpret_cast<long>(dlsym(RTLD_NEXT, "realloc")));
    if (NULL == _r_malloc || NULL == _r_calloc || NULL == _r_free || NULL == _r_realloc) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
}

void* mallocModeDidHandle(size_t size) {
    // Check our malloc mode
    if (currentMallocImpl == MallocImpl_PointerIncWithFreeAll) {
        if (
            // Pointer increment if we can.
            mallocWithFreeAll_current + size < mallocWithFreeAll_max) {
            // Pointer increment since we can.
            void* ret = mallocWithFreeAll_current;
            mallocWithFreeAll_current += size;
            postMalloc();
            numPointerIncMallocsThisFrame++;
            return ret;
        }
        else {
            mallocWithFreeAll_hitLimitCount++;
        }
    }
    else {
        return nullptr;
    }
}

void* malloc(size_t size){
    if(_r_malloc==NULL) {
        mtrace_init();
    }

    // Thread-safe because thread_local (TLS) variables are used here:
    preMalloc();
    // Check our malloc mode
    void *p;
    if ((p = mallocModeDidHandle(size)) == nullptr) {
        p = CALL_r_malloc(size);
        postMalloc();
    }
    numMallocsThisFrame++;
    #ifdef __APPLE__
    size_t realSize = malloc_size(p);
    memory::_internal::allocated += realSize;
    #endif
    MALLOC_LOG("malloc", realSize, p);
    return p;
}

void* calloc(size_t nitems, size_t size){
    if(_r_calloc==NULL) {
        mtrace_init();
    }
    
    preMalloc();
    // Check our malloc mode
    void *p;
    if ((p = mallocModeDidHandle(size)) == nullptr) {
        p = CALL_r_calloc(nitems, size);
        postMalloc();
    }
    else {
        memset(p, 0, size); // calloc implementation
    }
    size_t realSize = CALLmalloc_size(p);
    memory::_internal::allocated += realSize;
    MALLOC_LOG("calloc", realSize, p);
    return p;
}

void free(void* p){
    if(_r_free==nullptr) {
        mtrace_init();
    }

    if(p != nullptr){
        size_t realSize = CALLmalloc_size(p);
        preFree();
        // Check our malloc mode
        if (currentMallocImpl == MallocImpl_PointerIncWithFreeAll) {
            // No-op, nothing to free
        }
        else {
            CALL_r_free(p);
        }
        postFree();
        memory::_internal::allocated -= realSize;
        MALLOC_LOG("free", realSize, p);
    } else {
        preFree();
        if (currentMallocImpl == MallocImpl_Default) {
            CALL_r_free(p);
        }
        postFree();
    }
}

void* realloc(void* p, size_t new_size){
    if(_r_realloc==nullptr) {
        mtrace_init();
    }

    preMalloc();
    // No malloc-mode supported
    assert(currentMallocImpl == MallocImpl_Default);
    p = CALL_r_realloc(p, new_size);
    postMalloc();
    return p;
}

void *operator new(size_t size) noexcept(false){
    return malloc(size);
}

void *operator new [](size_t size) noexcept(false){
    return malloc(size);
}

void operator delete(void * p) throw(){
    free(p);
}

#endif /* malloc_override_osx_h */
