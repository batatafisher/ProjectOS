#include <fs/vfs.h>
#include <kernel/shell.h>
#include <string.h>

const char *touch(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];

    if (path[0] == '/') {
      int err = create(path);
      if (err != NO_ERROR)
        return "ERROR: couldnt create file";

      return "ok";
    }

    char full_path[128] = "";
    strcpy(full_path, shell_get_cwd());

    if (strcmp(shell_get_cwd(), "/") != 0)
      strcat(full_path, "/");

    strcat(full_path, path);

    int err = create(full_path);
    if (err != NO_ERROR)
      return "ERROR: couldnt create file";

    return "ok";
  }

  return "USAGE: touch [path]";
}
