#include "subscaleMain.hpp"

#include <python3.7m/Python.h>
#include "VADL2022.hpp"
#include <mutex>

Queue<QueuedFunction, 8> mainDispatchQueue;
std::atomic<bool> isRunningPython = false;

pid_t lastPyPID;
bool lastPyPIDValid=false;
std::mutex lastPyPIDM;

int main(int argc, char **argv) {
  VADL2022 vadl(argc, argv);

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
