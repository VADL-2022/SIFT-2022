//
//  ThreadSafePrinter.hpp
//  SIFT
//
//  Created by VADL on 11/14/21.
//  Copyright © 2021 VADL. All rights reserved.
//

#ifndef ThreadSafePrinter_hpp
#define ThreadSafePrinter_hpp

#include <iostream>

// https://stackoverflow.com/questions/14718124/how-to-easily-make-stdcout-thread-safe
class ThreadSafePrinter
{
    static mutex m;
    static thread_local stringstream ss;
public:
    ThreadSafePrinter() = default;
    ~ThreadSafePrinter()
    {
        lock_guard  lg(m);
        std::cout << ss.str();
        ss.clear();
    }

    template<typename T>
    ThreadSafePrinter& operator << (const T& c)
    {
        ss << c;
        return *this;
    }


    // this is the type of std::cout
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;

    // this is the function signature of std::endl
    typedef CoutType& (*StandardEndLine)(CoutType&);

    // define an operator<< to take in std::endl
    ThreadSafePrinter& operator<<(StandardEndLine manip)
    {
        manip(ss);
        return *this;
    }
};
/* Usage:
 mutex ThreadSafePrinter::m;
 thread_local stringstream ThreadSafePrinter::ss;
 #define tsprint ThreadSafePrinter()

 void main()
 {
     tsprint << "asd ";
     tsprint << "dfg";
 }
 */

#endif /* ThreadSafePrinter_hpp */
