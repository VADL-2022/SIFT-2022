//
//  jemalloc_wrapper.h
//  SIFT
//
//  Created by VADL on 12/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef jemalloc_wrapper_h
#define jemalloc_wrapper_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// http://jemalloc.net/jemalloc.3.html

// Standard API:

//void *IMPLmalloc(    size_t size);
// 
//void *IMPLcalloc(    size_t number,
//     size_t size);
// 
//int IMPLposix_memalign(    void **ptr,
//     size_t alignment,
//     size_t size);
// 
//void *IMPLaligned_alloc(    size_t alignment,
//     size_t size);
// 
//void *IMPLrealloc(    void *ptr,
//     size_t size);
// 
//void IMPLfree(    void *ptr);

// Non-standard API:

size_t IMPLmalloc_size(    const void *ptr);

// TIP: TRY this from jemalloc ( http://jemalloc.net/jemalloc.3.html ) :
/*

 void malloc_stats_print(    void (*write_cb) (void *, const char *) ,
     void *cbopaque,
     const char *opts);
 */

#ifdef __cplusplus
}
#endif

#endif /* jemalloc_wrapper_h */
