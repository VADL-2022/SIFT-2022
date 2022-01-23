#include "py.h"
#include "subscaleMain.hpp"

inline bool pyRunFile(const char *path, int argc, char **argv) {
  mainDispatchQueue.enqueue([=](){
        S_RunFile(path, argc, argv);
  },path,QueuedFunctionType::Python);

  return true;
}
