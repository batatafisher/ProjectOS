#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <string.h>

const char *mk_dir(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0)
      return "ERROR: path name too long";
    int err = mkdir(full_path);
    if (err != NO_ERROR)
      return "ERROR: couldnt create directory";

    return "ok";
  }

  return "USAGE: mkdir [path]";
}
