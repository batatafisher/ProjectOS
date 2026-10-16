#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <string.h>

const char *rm(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    int err;
    if (path[0] == '/') {
      int stat = state(path);
      if (stat == 1)
        err = unlink(path);
      else if (stat == 2)
        err = rmdir(path);
      else
        err = stat;

      if (err != NO_ERROR)
        return "ERROR: cannot remove file";
      return "ok";
    } else {
      char full_path[128] = "";
      strcpy(full_path, shell_get_cwd());

      if (strcmp(shell_get_cwd(), "/") != 0)
        strcat(full_path, "/");

      strcat(full_path, path);
      int stat = state(full_path);
      if (stat == 1)
        err = unlink(full_path);
      else if (stat == 2)
        err = rmdir(full_path);
      else
        err = stat;

      if (err != NO_ERROR)
        return "ERROR: connot remove file";
      return "ok";
    }
  }
  return "USAGE: rm [path]";
}
