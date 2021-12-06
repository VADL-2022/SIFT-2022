//
//  myMalloc_.h
//  SIFT
//
//  Created by VADL on 12/5/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#define MYMALLOC
#ifdef MYMALLOC

#ifndef myMalloc__h
#define myMalloc__h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

void * _malloc( size_t size );
void _free( void * addr );

#ifdef __cplusplus
}
#endif

#endif /* myMalloc__h */

#endif
