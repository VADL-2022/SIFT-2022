#pragma once

#include <iostream>
#include "commonOutMutex.hpp"

#include <python3.7m/Python.h>
#include <pybind11/pybind11.h>
namespace py = pybind11;

// This is a wrapper for calling Python using pybind11. Use it when you don't want unhandled Python exceptions to stop the entirety of SIFT (basically always use it).
// https://pybind11.readthedocs.io/en/stable/advanced/exceptions.html : "Any noexcept function should have a try-catch block that traps class:error_already_set (or any other exception that can occur). Note that pybind11 wrappers around Python exceptions such as pybind11::value_error are not Python exceptions; they are C++ exceptions that pybind11 catches and converts to Python exceptions. Noexcept functions cannot propagate these exceptions either. A useful approach is to convert them to Python exceptions and then discard_as_unraisable as shown below."
template <typename F>
void nonthrowing_python_nolock(F f) noexcept(true) {
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
}
