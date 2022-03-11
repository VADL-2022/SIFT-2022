//
//  commonOutMutex.h
//  SIFT
//
//  Created by VADL on 3/10/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef commonOutMutex_h
#define commonOutMutex_h

// C wrappers for the outMutex

#ifdef __cplusplus
extern "C" {
#endif

extern void out_guard_lock();
extern void out_guard_unlock();

#ifdef __cplusplus
}
#endif

#endif /* commonOutMutex_h */
