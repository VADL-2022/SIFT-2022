//
//  malloc_override_osx.h
//  SIFT
//
//  Created by VADL on 12/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//
// https://gist.github.com/monitorjbl/3dc6d62cf5514892d5ab22a59ff34861

#ifndef malloc_override_osx_h
#define malloc_override_osx_h

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
#include <string>
#include <malloc/malloc.h>

typedef void* (*real_malloc)(size_t);
typedef void* (*real_calloc)(size_t, size_t);
typedef void (*real_free)(void*);
extern "C" void* malloc(size_t);
extern "C" void* calloc(size_t, size_t);
extern "C" void free(void*);

static real_malloc _r_malloc;
static real_calloc _r_calloc;
static real_free _r_free;

namespace memory{
    namespace _internal{
        std::atomic_ulong allocated(0);
    }

    unsigned long current(){
        return _internal::allocated.load();
    }
}

static void mtrace_init(void){
    _r_malloc = reinterpret_cast<real_malloc>(reinterpret_cast<long>(dlsym(RTLD_NEXT, "malloc")));
    _r_calloc = reinterpret_cast<real_calloc>(reinterpret_cast<long>(dlsym(RTLD_NEXT, "calloc")));
    _r_free = reinterpret_cast<real_free>(reinterpret_cast<long>(dlsym(RTLD_NEXT, "free")));
    if (NULL == _r_malloc || NULL == _r_calloc || NULL == _r_free) {
        fprintf(stderr, "Error in `dlsym`: %s\n", dlerror());
    }
}


void* malloc(size_t size){
    if(_r_malloc==NULL) {
        mtrace_init();
    }

    preMalloc();
    void *p = _r_malloc(size);
    postMalloc();
    size_t realSize = malloc_size(p);
    memory::_internal::allocated += realSize;
    MALLOC_LOG("malloc", realSize, p);
    return p;
}

void* calloc(size_t nitems, size_t size){
    if(_r_calloc==NULL) {
        mtrace_init();
    }
    
    preMalloc();
    void *p = _r_calloc(nitems, size);
    postMalloc();
    size_t realSize = malloc_size(p);
    memory::_internal::allocated += realSize;
    MALLOC_LOG("calloc", realSize, p);
    return p;
}

void free(void* p){
    if(_r_free==nullptr) {
        mtrace_init();
    }

    if(p != nullptr){
        size_t realSize = malloc_size(p);
        preFree();
        _r_free(p);
        postFree();
        memory::_internal::allocated -= realSize;
        MALLOC_LOG("free", realSize, p);
    } else {
        preFree();
        _r_free(p);
        postFree();
    }
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
