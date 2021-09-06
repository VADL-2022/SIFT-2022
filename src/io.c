#include "io.h"

#define _GNU_SOURCE
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

int forEachInDir(const char* dir, void(*process)(const char* filename)) {
  // https://stackoverflow.com/questions/9743485/natural-sort-of-directory-filenames-in-c
  struct dirent **namelist;
  int n,i;

  n = scandir(dir, &namelist, 0, versionsort);
  if (n < 0) {
    perror("scandir");
    return 1;
  }
  else
  {
    for(i =0 ; i < n; ++i)
    {
      printf("%s\n", namelist[i]->d_name);
      process(namelist[i]->d_name);
      free(namelist[i]);
    }
    free(namelist);
  }
  return 0;
}
