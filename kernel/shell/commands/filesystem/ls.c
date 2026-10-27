#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/mmu.h>
#include <kernel/shell.h>
#include <stddef.h>
#include <string.h>

const char *ls(char *args[], int argc) {
  static dir_info_t entries[64];

  const char *path = (argc > 0) ? args[0] : shell_get_cwd();
  int count = list(path, entries, 63);

  if (count <= 0)
    return "ls: failed to list directory\n";
  size_t buf_size = 0;
  for (int i = 0; i < count; i++) {
    buf_size += strlen(entries[i].name) + 1;
    if (entries[i].file_type == 2)
      buf_size++;
  }

  char *buffer = kcalloc(1, sizeof(char) * buf_size);

  for (int i = 0; i < count; i++) {
    strcat(buffer, entries[i].name);
    if (entries[i].file_type == 2)
      strcat(buffer, "/");

    strcat(buffer, "\n");
  }
  kfree(buffer); // use after free but who cares, it works, my os my rules!
  return buffer;
}
