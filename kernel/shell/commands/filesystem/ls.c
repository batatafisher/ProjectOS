#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <string.h>

const char *ls(char *args[], int argc) {
  static char buffer[512];
  static dir_info_t entries[64];

  buffer[0] = '\0';

  const char *path = (argc > 0) ? args[0] : shell_get_cwd();
  int count = list(path, entries, 64);

  if (count < 0)
    return "ls: failed to list directory\n";

  for (int i = 0; i < count; i++) {
    strcat(buffer, entries[i].name);
    if (entries[i].file_type == 2)
      strcat(buffer, "/");

    strcat(buffer, "\n");
  }

  return buffer;
}
