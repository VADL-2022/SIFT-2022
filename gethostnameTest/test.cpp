#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <limits.h>

// https://github.com/eddic/fastcgipp/issues/13
#if __APPLE__
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255 // HACK
#endif
#endif

int main() {
  char hostname[HOST_NAME_MAX + 1];
  int retval = gethostname(hostname, HOST_NAME_MAX + 1);
  if (retval == 0) { // success
    printf("%s\n", hostname);
  }
  else {
    perror("Error");
    printf("%d\n", retval);
  }
}
