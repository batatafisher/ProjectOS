#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdio.h>
#include <string.h>

void rm(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    int err;
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0) {
      printf("ERROR: path name too long\n");
      return;
    }
    int stat = state(full_path);
    if (stat == 1)
      err = unlink(full_path);
    else if (stat == 2)
      err = rmdir(full_path);
    else
      err = stat;

    if (err != NO_ERROR) {
      printf("ERROR: connot remove file\n");
      return;
    }
    printf("ok\n");
    return;
  }
  printf("USAGE: rm [path]\n");
}
