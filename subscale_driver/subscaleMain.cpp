#include "subscaleMain.hpp"

#include <python3.8/Python.h>
#include "VADL2022.hpp"
#include <thread>

Queue<QueuedFunction, 8> mainDispatchQueue;
std::atomic<bool> isRunningPython = false;

int main(int argc, char **argv) {
  VADL2022 vadl(argc, argv);

  std::thread::id this_id = std::this_thread::get_id();
  std::cout << "thread " << this_id << " running dispatch queue" << std::endl;
    
  QueuedFunction f;
  while (true) {
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
  }

  PyRun_SimpleString("input('Done')");
  return 0;
}
