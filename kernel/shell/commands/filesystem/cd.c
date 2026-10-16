#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <string.h>

const char *cd(char *args[], int argc) {
  if (argc != 1)
    return "USAGE: cd [PATH]";

  const char *cwd = shell_get_cwd();
  char full_path[128] = "";

  if (args[0][0] == '/') {
    strcpy(full_path, args[0]);
  } else {
    strcpy(full_path, cwd);

    if (strcmp(cwd, "/") != 0)
      strcat(full_path, "/");

    strcat(full_path, args[0]);
  }

  int type = state(full_path);

  if (type < 0)
    return "ERROR: invalid path";

  if (type == 1)
    return "ERROR: provided path is not a directory";

  if (type != 2)
    return "ERROR: unknown file type";

  shell_set_cwd(full_path);

  return "ok";
}
