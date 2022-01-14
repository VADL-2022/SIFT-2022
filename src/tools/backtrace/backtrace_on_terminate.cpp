//
//  backtrace_on_terminate.cpp
//  SIFT
//
//  Created by VADL on 1/14/22.
//  Copyright © 2022 VADL. All rights reserved.
//

#include "backtrace_on_terminate.hpp"

[[noreturn]]
void
backtrace_on_terminate() noexcept
{
#ifdef BACKTRACE_SET_TERMINATE
    std::set_terminate(terminate_handler.release()); // to avoid infinite looping if any
#else
    std::set_terminate(nullptr); // to avoid infinite looping if any
#endif
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
