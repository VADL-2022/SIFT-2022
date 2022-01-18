#include "../src/Queue.hpp"
#include <functional>

using QueuedFunction = std::pair<std::function<void(void)>, const char*>;
extern Queue<QueuedFunction, 8> mainDispatchQueue;
