//
//  pthread_mutex_lock_.h
//  SIFT
//
//  Created by VADL on 3/6/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef pthread_mutex_lock__h
#define pthread_mutex_lock__h

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int pthread_mutex_lock_(pthread_mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* pthread_mutex_lock__h */
