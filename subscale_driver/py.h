#ifndef PY_H
#define PY_H

#include <python3.7m/Python.h> /* must be first */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void S_ShowLastError(void);
  
// Returns true on success
bool S_RunString(const char *str);

// Returns true on success
bool S_RunFile(const char *path, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
