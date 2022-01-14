//
//  backtrace_on_terminate.hpp
//  SIFT
//
//  Created by VADL on 1/11/22.
//  Copyright © 2022 VADL. All rights reserved.
//
// Based on https://stackoverflow.com/questions/3355683/c-stack-trace-from-unhandled-exception
// Pass #define BACKTRACE_SET_TERMINATE to this header file before including it to enable changing std::set_terminate

#ifndef backtrace_on_terminate_h
#define backtrace_on_terminate_h

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

#ifdef BACKTRACE_SET_TERMINATE
static_assert(std::is_same< std::terminate_handler, decltype(&backtrace_on_terminate) >{});

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
std::unique_ptr< std::remove_pointer_t< std::terminate_handler >, decltype(std::set_terminate) & > terminate_handler{std::set_terminate(backtrace_on_terminate), std::set_terminate};
#pragma clang diagnostic pop
#endif

}

#endif /* backtrace_on_terminate_h */
