#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <string.h>

const char *touch(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0)
      return "ERROR: path name too long";
    int err = create(full_path);
    if (err != NO_ERROR)
      return "ERROR: couldnt create file";

    return "ok";
  }

  return "USAGE: touch [path]";
}
