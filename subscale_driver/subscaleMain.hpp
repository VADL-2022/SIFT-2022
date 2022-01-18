#include "../src/Queue.hpp"
#include <functional>

using QueuedFunction = void*;
extern Queue<QueuedFunction, 8> mainDispatchQueue;
