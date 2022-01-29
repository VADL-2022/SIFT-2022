#include "subscaleMain.hpp"

#include <python3.7m/Python.h>
#include "VADL2022.hpp"

Queue<QueuedFunction, 8> mainDispatchQueue;
std::atomic<bool> isRunningPython = false;
std::atomic<bool> mainDispatchQueueRunning = false;

int main(int argc, char **argv) {
  VADL2022 vadl(argc, argv);

  QueuedFunction f;
  mainDispatchQueueRunning = true;
  do {
    std::cout << "Main dispatch queue waiting for next function..." << std::endl;
    mainDispatchQueue.dequeue(&f);
    std::cout << "Main dispatch queue executing function with description: " << f.description << std::endl;
    if (f.type == QueuedFunctionType::Python) {
      isRunningPython = true;
    }
    f.f();
    if (f.type == QueuedFunctionType::Python) {
      isRunningPython = false;
    }
  } while (mainDispatchQueueRunning);

  std::cout << "Main dispatch queue no longer running" << std::endl;
  //PyRun_SimpleString("input('Done')");
  return 0;
}
