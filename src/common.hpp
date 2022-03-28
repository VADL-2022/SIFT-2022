//
//  common.hpp
//  SIFT
//
//  Created by VADL on 10/31/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef common_hpp
#define common_hpp

#include "Includes.hpp"

#include "Config.hpp"

#include <python3.7m/Python.h>
#include <pybind11/pybind11.h>

// Each thread needs its own timer for meaningful results, so we make it thread_local.
// https://stackoverflow.com/questions/11983875/what-does-the-thread-local-mean-in-c11 : "When you declare a variable thread_local then each thread has its own copy. When you refer to it by name, then the copy associated with the current thread is used."
extern thread_local Timer t;

void drawCircle(cv::Mat& img, cv::Point cp, int radius);
void drawRect(cv::Mat& img, cv::Point center, cv::Size2f size, float orientation_degrees, int thickness = 2);
void drawSquare(cv::Mat& img, cv::Point center, int size, float orientation_degrees, int thickness = 2);

// Wrapper around imshow that reuses the same window each time
namespace commonUtils {
void imshow(std::string name, cv::Mat& mat);
}

extern thread_local cv::RNG rng; // Random number generator
extern thread_local cv::Scalar lastColor;
void resetRNG();
cv::Scalar nextRNGColor(int matDepth);

// This is a wrapper for calling Python using pybind11. Use it when you don't want unhandled Python exceptions to stop the entirety of SIFT (basically always use it).
// https://pybind11.readthedocs.io/en/stable/advanced/exceptions.html : "Any noexcept function should have a try-catch block that traps class:error_already_set (or any other exception that can occur). Note that pybind11 wrappers around Python exceptions such as pybind11::value_error are not Python exceptions; they are C++ exceptions that pybind11 catches and converts to Python exceptions. Noexcept functions cannot propagate these exceptions either. A useful approach is to convert them to Python exceptions and then discard_as_unraisable as shown below."
template <typename F>
void nonthrowing_python(F f) noexcept(true) {
    Timer t2;
    t2.reset();
    // https://docs.python.org/3/c-api/init.html#non-python-created-threads
    static thread_local PyGILState_STATE gstate;
    gstate = PyGILState_Ensure(); // Lock the global python interpreter (Python can only run on one thread, but it multiplexes Python threads among that thread!... I can't believe it isn't multicore!: {"
    // GIL does not allow multiple threads to execute Python code at the same time. If you don’t go into detail, GIL can be visualized as a sort of flag that carried over from thread to thread. Whoever has the flag can do the job. The flag is transmitted either every Python instruction or, for example, when some type of input-output operation is performed.
    // Therefore, different threads will not run in parallel and the program will simply switch between them executing them at different times. However, if in the program there is some “wait” (packages from the network, user request, time.sleep pause), then in such program the threads will be executed as if in parallel. This is because during such pauses the flag (GIL) can be passed to another thread.
    // "} -- https://pyneng.readthedocs.io/en/latest/book/19_concurrent_connections/cpython_gil.html )
    t2.logElapsed(/*id,*/ "nonthrowing_python PyGILState_Ensure");
    
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
    PyGILState_Release(gstate);
}

template <typename T>
struct PythonLocked {
    PythonLocked(T& t_) : t(t_)
    {}
    PythonLocked(PythonLocked& other) {
        nonthrowing_python([this, &other](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = other.t;
        });
    }
    PythonLocked& operator=(PythonLocked& other) {
        nonthrowing_python([this, &other](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t = other.t;
        });
        return *this;
    }
    ~PythonLocked() {
        nonthrowing_python([this](){ // <-- Lock the interpreter GIL lock to access the `t` variable
            this->t.~T();
        });
    }
    T t;
};

#endif /* common_hpp */
