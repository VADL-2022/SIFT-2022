//
//  demangle.hpp
//  SIFT
//
//  Created by VADL on 1/1/22.
//  Copyright © 2022 VADL. All rights reserved.
//
// https://stackoverflow.com/questions/3355683/c-stack-trace-from-unhandled-exception

#ifndef demangle_hpp
#define demangle_hpp

char const *
get_demangled_name(char const * const symbol) noexcept;

#endif /* demangle_hpp */
