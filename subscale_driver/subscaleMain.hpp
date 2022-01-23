#include "../src/Queue.hpp"
#include <functional>
#include <atomic>

enum QueuedFunctionType {
  Misc,
  Python
};

struct QueuedFunction {
  std::function<void(void)> f;
  const char *description;
  QueuedFunctionType type;
};

extern Queue<QueuedFunction, 8> mainDispatchQueue;
extern std::atomic<bool> isRunningPython;
