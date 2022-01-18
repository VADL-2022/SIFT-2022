#include "../src/Queue.hpp"
#include <functional>

using QueuedFunction = std::function<void(void)>;
extern Queue<QueuedFunction, 8> mainDispatchQueue;
