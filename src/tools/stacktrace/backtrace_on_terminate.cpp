//
//  backtrace_on_terminate.cpp
//  SIFT
//
//  Created by VADL on 1/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//
// Based on https://stackoverflow.com/questions/3355683/c-stack-trace-from-unhandled-exception

#include "backtrace_on_terminate.hpp"

#include "demangle.hpp"
#include "backtrace.hpp"

#include <iostream>
#include <type_traits>
#include <exception>
#include <memory>
#include <typeinfo>

#include <cstdlib>

#include <cxxabi.h>

namespace
{

[[noreturn]]
void
backtrace_on_terminate() noexcept;

//static_assert(std::is_same< std::terminate_handler, decltype(&backtrace_on_terminate) >{});

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
//std::unique_ptr< std::remove_pointer_t< std::terminate_handler >, decltype(std::set_terminate) & > terminate_handler{std::set_terminate(backtrace_on_terminate), std::set_terminate};
#pragma clang diagnostic pop

[[noreturn]]
void
backtrace_on_terminate() noexcept
{
    //std::set_terminate(terminate_handler.release()); // to avoid infinite looping if any
    backtrace(std::clog);
    if (std::exception_ptr ep = std::current_exception()) {
        try {
            std::rethrow_exception(ep);
        } catch (std::exception const & e) {
            std::clog << "backtrace: unhandled exception std::exception:what(): " << e.what() << std::endl;
        } catch (...) {
            if (std::type_info * et = abi::__cxa_current_exception_type()) {
                std::clog << "backtrace: unhandled exception type: " << get_demangled_name(et->name()) << std::endl;
            } else {
                std::clog << "backtrace: unhandled unknown exception" << std::endl;
            }
        }
    }
    std::_Exit(EXIT_FAILURE);
}

}
