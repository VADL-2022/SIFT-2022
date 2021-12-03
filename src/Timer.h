//
//  Timer.h
//  SIFT
//
//  Created by VADL on 12/3/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef Timer_h
#define Timer_h

#ifdef __cplusplus
extern "C" {
#endif

struct Timer_C {
    void* reservedSpaceHackThisIsBad[8];
};

void Timer_Timer(struct Timer_C* out_t);
void Timer_reset(struct Timer_C* t);
double Timer_elapsed(struct Timer_C* t);
void Timer_logElapsed(struct Timer_C* t, const char* name);
void Timer_logElapsed_thread(struct Timer_C* t, int threadID, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* Timer_h */
