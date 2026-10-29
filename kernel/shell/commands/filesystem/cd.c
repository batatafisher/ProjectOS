#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdio.h>
#include <string.h>

void cd(char *args[], int argc) {
  if (argc == 1) {
    const char *cwd = shell_get_cwd();
    char full_path[CD_PATH_MAX] = "";

    if (args[0][0] == '/') {
      if (strlen(args[0]) >= CD_PATH_MAX) {
        printf("ERROR: path too long\n");
        return;
      }
      strcpy(full_path, args[0]);
    } else {
      if (strlen(args[0]) + strlen(cwd) >= CD_PATH_MAX) {
        printf("ERROR: path too long\n");
        return;
      }

      strcpy(full_path, cwd);

      strcat(full_path, args[0]);
    }

    int type = state(full_path);

    if (type < 0) {
      printf("ERROR: invalid path\n");
      return;
    }

    if (type == 1) {
      printf("ERROR: provided path is not a directory\n");
      return;
    }

    if (type != 2) {
      printf("ERROR: unknown file type\n");
      return;
    }
    int path_len = strlen(full_path);
    if (full_path[path_len - 1] == '/')
      shell_set_cwd(full_path);
    else
      shell_set_cwd(strcat(full_path, "/"));

    printf("ok\n");
    return;
  }
  printf("USAGE: cd [path]\n");
  return;
}
