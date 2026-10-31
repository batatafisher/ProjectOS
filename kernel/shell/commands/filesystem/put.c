#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void put(char *args[], int argc) {
  if (argc > 1) {
    char *path = args[0];
    int err;
    size_t size = 0;
    for (int i = 0; i < argc; i++)
      size += strlen(args[i]) + 1;

    char *buffer = kcalloc(1, sizeof(char) * size);
    if (buffer == NULL) {
      printf("ERROR: out of memory");
      return;
    }
    for (int i = 1; i < argc; i++) {
      strcat(buffer, args[i]);
      strcat(buffer, " ");
    }
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0) {
      kfree(buffer);
      printf("ERROR: path name too long");
      return;
    }
    int fd = open(full_path, 0);
    if (fd >= 0) {
      err = write(fd, (uint8_t *)buffer, strlen(buffer));
      close(fd);
      kfree(buffer);
      if (err != NO_ERROR) {
        printf("ERROR: couldnt write to file");
        return;
      }
      printf("ok");
      return;
    } else {
      kfree(buffer);
      printf("ERROR: invalid path");
      return;
    }
  }
  printf("USAGE: put [path] [text]");
  return;
}
