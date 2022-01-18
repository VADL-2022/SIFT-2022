#include "py.h"
#include "siftMain.hpp"

inline bool pyRunFile(const char *path, int argc, char **argv) {
    mainDispatchQueue.enqueue([=](){
        S_RunFile(path, argc, argv);
    })    
}