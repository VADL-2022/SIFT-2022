//
//  backtrace_on_terminate.hpp
//  SIFT
//
//  Created by VADL on 1/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//
// Based on https://stackoverflow.com/questions/3355683/c-stack-trace-from-unhandled-exception

#ifndef backtrace_on_terminate_hpp
#define backtrace_on_terminate_hpp

[[noreturn]]
void
backtrace_on_terminate() noexcept;

#endif /* backtrace_on_terminate_hpp */
