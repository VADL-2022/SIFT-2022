//
//  PythonLockedOptional.hpp
//  SIFT
//
//  Created by VADL on 3/28/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#ifndef PythonLockedOptional_hpp
#define PythonLockedOptional_hpp

#include <iostream>
#include "../commonOutMutex.hpp"

#include <python3.7m/Python.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

#include "tools/backtrace/backtrace.hpp"

PyGILState_STATE* lockPython();
void unlockPython(PyGILState_STATE* gstate);

// This is a wrapper for calling Python using pybind11. Use it when you don't want unhandled Python exceptions to stop the entirety of SIFT (basically always use it).
// https://pybind11.readthedocs.io/en/stable/advanced/exceptions.html : "Any noexcept function should have a try-catch block that traps class:error_already_set (or any other exception that can occur). Note that pybind11 wrappers around Python exceptions such as pybind11::value_error are not Python exceptions; they are C++ exceptions that pybind11 catches and converts to Python exceptions. Noexcept functions cannot propagate these exceptions either. A useful approach is to convert them to Python exceptions and then discard_as_unraisable as shown below."
template <typename F>
void nonthrowing_python(F f) noexcept(true) {
    PyGILState_STATE* gstate = lockPython();
    
    try {
        f();
    } catch (py::error_already_set &eas) {
        // Discard the Python error using Python APIs, using the C++ magic
        // variable __func__. Python already knows the type and value and of the
        // exception object.
        eas.discard_as_unraisable(__func__);
    } catch (const std::exception &e) {
        // Log and discard C++ exceptions.
        { out_guard();
            std::cout << "C++ exception from python: " << e.what() << std::endl; }
    }
    
    /* Release the thread. No Python API allowed beyond this point. */
    unlockPython(gstate);
}

template <typename T>
struct PythonLockedOptional {
    PythonLockedOptional()
    {
        puts("PythonLockedOptional: PythonLockedOptional()"); fflush(stdout);
    }
    PythonLockedOptional(const T& t_)
    {
        puts("PythonLockedOptional: PythonLockedOptional(T&)"); fflush(stdout);
        nonthrowing_python([this, &t_](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = t_;
        });
    }
    PythonLockedOptional(const PythonLockedOptional& other) {
        puts("PythonLockedOptional: PythonLockedOptional(PythonLockedOptional&)"); fflush(stdout);
        nonthrowing_python([this, &other](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = other.t;
        });
    }
    PythonLockedOptional& operator=(const PythonLockedOptional& other) {
        puts("PythonLockedOptional: operator=PythonLockedOptional"); fflush(stdout);
        nonthrowing_python([this, &other](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = other.t;
        });
        return *this;
    }
    PythonLockedOptional& operator=(const T& t) {
        puts("PythonLockedOptional: operator=T"); fflush(stdout);
        nonthrowing_python([this, &t](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = t;
        });
        return *this;
    }
    ~PythonLockedOptional() {
        puts("PythonLockedOptional: ~PythonLockedOptional()"); fflush(stdout);
        nonthrowing_python([this](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t.reset();
        });
    }
    
    T* operator-> ()
    {
        puts("PythonLockedOptional: operator->"); fflush(stdout);
        backtrace(std::cout);
        return &*t;
    }
    T& operator* ()
    {
        puts("PythonLockedOptional: operator*"); fflush(stdout);
        backtrace(std::cout);
      return *t;
    }
    
    std::optional<T> t;
};

#endif /* PythonLockedOptional_hpp */
