//
//  malloc_override_osx.hpp
//  SIFT
//
//  Created by VADL on 12/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//
// https://gist.github.com/monitorjbl/3dc6d62cf5514892d5ab22a59ff34861

#define MYMALLOC

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
#include <assert.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
#include <atomic>

#include <backward.hpp>
#include <sstream>


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

extern "C" real_malloc _r_malloc   ;
extern "C" real_calloc _r_calloc   ;
extern "C" real_free _r_free       ;
extern "C" real_realloc _r_realloc ;

extern void mtrace_init(void);

extern void* mallocModeDidHandle(size_t size);

extern void* malloc(size_t size);

extern void* calloc(size_t nitems, size_t size);

extern void free(void* p);

extern void* realloc(void* p, size_t new_size);

extern void *operator new(size_t size) noexcept(false);

extern void *operator new [](size_t size) noexcept(false);

extern void operator delete(void * p) throw();

#endif /* malloc_override_osx_h */
