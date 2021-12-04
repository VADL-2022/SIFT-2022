//
//  jemalloc_wrapper.c
//  SIFT
//
//  Created by VADL on 12/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#include "jemalloc_wrapper.h"

#ifdef USE_JEMALLOC
#include <jemalloc/jemalloc.h>

//void *IMPLmalloc(    size_t size) {
//    return malloc(size);
//}
//
//void *IMPLcalloc(    size_t number,
//                 size_t size) {
//    return calloc(number, size);
//}
//
//int IMPLposix_memalign(    void **ptr,
//     size_t alignment,
//                       size_t size) {
//    return posix_memalign(ptr, alignment, size);
//}
//
//void *IMPLaligned_alloc(    size_t alignment,
//                        size_t size) {
//    return aligned_alloc(alignment, size);
//}
//
//void *IMPLrealloc(    void *ptr,
//                  size_t size) {
//    return realloc(ptr, size);
//}
//
//void IMPLfree(    void *ptr) {
//    free(ptr);
//}

size_t IMPLmalloc_size(    const void *ptr) {
    return malloc_usable_size(ptr); // "The return value may be larger than the size that was requested during allocation." ( http://jemalloc.net/jemalloc.3.html )
}

#endif
