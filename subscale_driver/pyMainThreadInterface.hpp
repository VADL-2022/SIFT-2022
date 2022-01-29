#pragma once

#include <mutex>

extern pid_t lastForkedPID;
extern bool lastForkedPIDValid;
extern std::mutex lastForkedPIDM;

extern bool runCommandWithFork(const char* commandWithArgs[] /* array with NULL as the last element */);

extern bool pyRunFile(const char *path, int argc, char **argv);
