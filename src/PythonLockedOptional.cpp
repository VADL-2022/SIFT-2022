//
//  PythonLockedOptional.cpp
//  SIFT
//
//  Created by VADL on 3/28/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "PythonLockedOptional.hpp"

#include "Timer.hpp"

#include "main/siftMainCmdConfig.hpp"

#include "pthread_mutex_lock_.h"

static thread_local bool lockedAlready;

// For debugging
pthread_mutex_t debugMutex;
std::atomic<bool> inittedDebugMutex = false;

PyGILState_STATE* lockPython() {
    if (lockedAlready) {
        return nullptr;
    }
    lockedAlready = true;
    
    Timer t2;
    t2.reset();
    // https://docs.python.org/3/c-api/init.html#non-python-created-threads
    static thread_local PyGILState_STATE gstate;
    if (CMD_CONFIG(debugMutexDeadlocks)) {
        if (!inittedDebugMutex.compare_exchange_strong() {
            pthread_mutex_init(&debugMutex, nullptr);
            inittedDebugMutex = true;
        }
        
        // Lock this so we get reports of deadlocks since the PyGILState_Ensure() below doesn't report them as expected:
        pthread_mutex_lock_(&debugMutex);
    }
    gstate = PyGILState_Ensure(); // Lock the global python interpreter (Python can only run on one thread, but it multiplexes Python threads among that thread!... I can't believe it isn't multicore!: {"
    // GIL does not allow multiple threads to execute Python code at the same time. If you don’t go into detail, GIL can be visualized as a sort of flag that carried over from thread to thread. Whoever has the flag can do the job. The flag is transmitted either every Python instruction or, for example, when some type of input-output operation is performed.
    // Therefore, different threads will not run in parallel and the program will simply switch between them executing them at different times. However, if in the program there is some “wait” (packages from the network, user request, time.sleep pause), then in such program the threads will be executed as if in parallel. This is because during such pauses the flag (GIL) can be passed to another thread.
    // "} -- https://pyneng.readthedocs.io/en/latest/book/19_concurrent_connections/cpython_gil.html )
    if (CMD_CONFIG(verbose)) {
        t2.logElapsed(/*id,*/ "nonthrowing_python PyGILState_Ensure");
    }
    return &gstate;
}
void unlockPython(PyGILState_STATE* gstate) {
    if (gstate == nullptr) return;
    
    if (CMD_CONFIG(debugMutexDeadlocks)) {
        if (!inittedDebugMutex) {
            printf("***** Mutex warning: unlocked python despite it not being locked already *****\n");
        }
        else {
            // Unlock this so we get reports of deadlocks etc.
            pthread_mutex_unlock(&debugMutex);
        }
    }
    
    if (lockedAlready) {
        /* Release the thread. No Python API allowed beyond this point. */
        PyGILState_Release(*gstate);
        
        lockedAlready = false;
    }
    else {
        printf("PyGILState_Release unlocked already, not unlocking\n");
    }
}
