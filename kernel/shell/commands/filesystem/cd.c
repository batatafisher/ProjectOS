#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdio.h>
#include <string.h>

void cd(char *args[], int argc) {
  if (argc == 1) {
    char *full_path;

    full_path = get_full_path(args[0]);
    if (strcmp(full_path, "err") == 0) {
      printf("ERROR: invalid path");
      return;
    }

    int type = state(full_path);

    if (type < 0) {
      printf("ERROR: invalid path");
      return;
    }

    if (type == 1) {
      printf("ERROR: provided path is not a directory");
      return;
    }

    if (type != 2) {
      printf("ERROR: unknown file type");
      return;
    }
    int path_len = strlen(full_path);
    if (full_path[path_len - 1] == '/')
      shell_set_cwd(full_path);
    else
      shell_set_cwd(strcat(full_path, "/"));

    printf("ok");
    return;
  }
  printf("USAGE: cd [path]");
  return;
}
