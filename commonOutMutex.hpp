//
//  commonOutMutex.hpp
//  SIFT
//
//  Created by VADL on 3/10/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef commonOutMutex_hpp
#define commonOutMutex_hpp

#include <mutex>

namespace common {
  // https://stackoverflow.com/questions/35087781/using-line-in-a-macro-definition : "The problem is that in the preprocessor, the ## takes place before __LINE__ is expanded. If you add another layer of indirection, you can get the desired result." + "For technical reasons you actually need two macros (sometimes if you use this in an existing macro you don't need the second one, it seems...)" :
#define TOKEN_PASTE(x, y) x##y
#define CAT(x,y) TOKEN_PASTE(x,y)
// Guards output to console by making it synchronized among threads *if* you call this out_guard() before all printf's or cout's (a single call to printf likely locks its own mutex so that is ok). This is only needed if you cout/printf multiple things (multiple lines or `<<` calls) in one logical "output" bit of information (*including* an std::endl since it requires more than one `<<` -- so use the out_guard() for that! (Or use std::cout << "yourTextHere\n" but this has no flush so only use it for non-urgent things)). This is because printf and cout synchronize each call to them (likely using a mutex) internally.
// WARNING: do not call this twice on the same thread without an enclosing scope to release the lock, or else the locks will deadlock forever!
// WARNING 2: do not use this with pthread_ functions called while the lock is active, or else a pthread_cancel() may leave it locked, causing future locks to fail. (Sometimes destructors aren't called from pthread_cancel() I think ( https://stackoverflow.com/questions/2163090/when-i-kill-a-pthread-in-c-do-destructors-of-objects-on-stacks-get-called ), therefore std::lock_guard's destructor won't be called.)
#define out_guard() std::lock_guard<std::mutex> CAT(coutLock, __LINE__) (common::outMutex);

  // https://www.modernescpp.com/index.php/synchronized-outputstreams
  extern std::mutex outMutex; // Usage: call the `out_guard()` macro or `std::lock_guard<std::mutex> outLock(outMutex);` and then cout's or printf's within that scope will flush (if necessary) only when the end of the enclosing scope is reached *only if* you use this lock guard on all cout's and printf's.
  // Also note that printf and cout are already thread-safe ( https://stackoverflow.com/questions/23586682/how-to-use-printf-in-multiple-threads ) and likely lock a mutex within each printf() or `<<` call. Furthermore, printf and cout are synchronized by default: https://stackoverflow.com/questions/1595355/syncing-iostream-with-stdio ; to disable it (which we don't want to do), you would call `std::ios::sync_with_stdio(false);` and then cout will not synchronize with printf anymore.
  // Also note that it is sufficient if a mutex is locked in a cout since it does effectively become a "critical section" except not in terms of preemption preventing, since: "A context switch is always possible in Linux, you cannot monopolize the CPU, but threads that fail acquiring the lock will block and thus won't be selected by the scheduler until the lock is available again." ( https://stackoverflow.com/questions/3103578/can-context-switch-take-place-while-in-a-critical-section ) -- key point is that it is "until the lock is available again" -- then the thread can resume its operation and no other threads can use cout in the same section until then.
}

#endif /* commonOutMutex_hpp */
