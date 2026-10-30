#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdio.h>
#include <string.h>

void touch(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0) {
      printf("ERROR: path name too long");
      return;
    }
    int err = create(full_path);
    if (err != NO_ERROR) {
      printf("ERROR: couldnt create file");
      return;
    }

    printf("ok");
    return;
  }

  printf("USAGE: touch [path]");
}
