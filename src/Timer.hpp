#ifndef Timer_hpp
#define Timer_hpp

#include <chrono>

class Timer {
public:
    Timer();
    void reset();
    double elapsed() const;
    void logElapsed(const char* name) const;
private:
    typedef std::chrono::high_resolution_clock clock_;
    std::chrono::steady_clock::time_point beg_;
};

#endif /* Timer_hpp */
