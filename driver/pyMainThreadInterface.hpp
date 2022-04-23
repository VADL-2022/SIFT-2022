#pragma once

#include <mutex>
#include <functional>

extern pid_t lastForkedPID;
extern bool lastForkedPIDValid;
extern std::mutex lastForkedPIDM;

extern bool runCommandWithFork(const char* commandWithArgs[] /* array with NULL as the last element */);

extern bool pyRunFile(const char *path, int argc, char **argv);





// https://stackoverflow.com/questions/2067988/recursive-lambda-functions-in-c11
template <class F>
struct recursive {
  F f;
  template <class... Ts>
  decltype(auto) operator()(Ts&&... ts)  const { return f(std::ref(*this), std::forward<Ts>(ts)...); }

  template <class... Ts>
  decltype(auto) operator()(Ts&&... ts)  { return f(std::ref(*this), std::forward<Ts>(ts)...); }
};

template <class F> recursive(F) -> recursive<F>;
auto const rec = [](auto f){ return recursive{std::move(f)}; };

// Demo:
// auto fib = rec([&](auto&& fib, int i) {
// // implementation detail omitted.
// });
// "It is similar to the let rec keyword in OCaml, although not the same."
