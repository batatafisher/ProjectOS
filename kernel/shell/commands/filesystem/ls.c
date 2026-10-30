#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/asmutils.h>
#include <kernel/mmu.h>
#include <kernel/shell.h>
#include <stddef.h>
#include <stdio.h>

void ls(char *args[], int argc) {

  static dir_info_t entries[64];

  const char *path = (argc > 0) ? get_full_path(args[0]) : shell_get_cwd();
  int count = list(path, entries, 63);

  if (count <= 0) {
    printf("ERROR: failed to list directory");
    return;
  }
  for (int i = 0; i < count; i++) {
    printf("%s", entries[i].name);
    if (entries[i].file_type == 2)
      printf("/");

    printf("\n");
  }
  return;
}
