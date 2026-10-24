#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <string.h>

const char *cd(char *args[], int argc) {
  if (argc == 1) {
    const char *cwd = shell_get_cwd();
    char full_path[CD_PATH_MAX] = "";

    if (args[0][0] == '/') {
      if (strlen(args[0]) >= CD_PATH_MAX)
        return "ERROR: path too long";
      strcpy(full_path, args[0]);
    } else {
      if (strlen(args[0]) + strlen(cwd) >= CD_PATH_MAX)
        return "ERROR: path too long";

      strcpy(full_path, cwd);

      strcat(full_path, args[0]);
    }

    int type = state(full_path);

    if (type < 0)
      return "ERROR: invalid path";

    if (type == 1)
      return "ERROR: provided path is not a directory";

    if (type != 2)
      return "ERROR: unknown file type";
    int path_len = strlen(full_path);
    if (full_path[path_len - 1] == '/')
      shell_set_cwd(full_path);
    else
      shell_set_cwd(strcat(full_path, "/"));

    return "ok";
  }
  return "USAGE: cd [path]";
}
