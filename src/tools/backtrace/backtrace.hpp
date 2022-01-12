//
//  backtrace.hpp
//  SIFT
//
//  Created by VADL on 1/11/22.
//  Copyright © 2022 VADL. All rights reserved.
//
// https://stackoverflow.com/questions/3355683/c-stack-trace-from-unhandled-exception

#ifndef backtrace_hpp
#define backtrace_hpp

#include <ostream>

void
backtrace(std::ostream & _out) noexcept;

#endif /* backtrace_hpp */
