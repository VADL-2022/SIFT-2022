//
//  PythonLockedOptional.hpp
//  SIFT
//
//  Created by VADL on 3/28/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef PythonLockedOptional_hpp
#define PythonLockedOptional_hpp

#include "../pythonCommon.hpp"

#include "tools/backtrace/backtrace.hpp"

PyGILState_STATE* lockPython();
void unlockPython(PyGILState_STATE* gstate);

// This is a wrapper for calling Python using pybind11. Use it when you don't want unhandled Python exceptions to stop the entirety of SIFT (basically always use it).
template <typename F>
void nonthrowing_python(F f) noexcept(true) {
    PyGILState_STATE* gstate = lockPython();
    
    nonthrowing_python_nolock(f);
    
    /* Release the thread. No Python API allowed beyond this point. */
    unlockPython(gstate);
}

template <typename T>
struct PythonLockedOptional {
    PythonLockedOptional()
    {
        //puts("PythonLockedOptional: PythonLockedOptional()"); fflush(stdout);
    }
    PythonLockedOptional(const T& t_)
    {
        //puts("PythonLockedOptional: PythonLockedOptional(T&)"); fflush(stdout);
        nonthrowing_python([this, &t_](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = t_;
        });
    }
    PythonLockedOptional(const PythonLockedOptional& other) {
        //puts("PythonLockedOptional: PythonLockedOptional(PythonLockedOptional&)"); fflush(stdout);
        nonthrowing_python([this, &other](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = other.t;
        });
    }
    PythonLockedOptional& operator=(const PythonLockedOptional& other) {
        //puts("PythonLockedOptional: operator=PythonLockedOptional"); fflush(stdout);
        nonthrowing_python([this, &other](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = other.t;
        });
        return *this;
    }
    PythonLockedOptional& operator=(const T& t) {
        //puts("PythonLockedOptional: operator=T"); fflush(stdout);
        nonthrowing_python([this, &t](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = t;
        });
        return *this;
    }
    ~PythonLockedOptional() {
        //puts("PythonLockedOptional: ~PythonLockedOptional()"); fflush(stdout);
        nonthrowing_python([this](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t.reset();
        });
    }
    
    T* operator-> ()
    {
        //puts("PythonLockedOptional: operator->"); fflush(stdout);
        backtrace(std::cout);
        return &*t;
    }
    T& operator* ()
    {
        //puts("PythonLockedOptional: operator*"); fflush(stdout);
        backtrace(std::cout);
      return *t;
    }
    
    std::optional<T> t;
};

#endif /* PythonLockedOptional_hpp */
