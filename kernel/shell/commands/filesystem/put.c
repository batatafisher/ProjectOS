#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void put(char *args[], int argc) {
  if (argc > 1) {
    char *path = args[0];
    int err;
    char buffer[1024] = "";
    for (int i = 1; i < argc; i++) {
      strcat(buffer, args[i]);
      strcat(buffer, " ");
    }
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0) {
      printf("ERROR: path name too long\n");
      return;
    }
    int fd = open(full_path, 0);
    if (fd >= 0) {
      err = write(fd, (uint8_t *)buffer, strlen(buffer));

      close(fd);
      if (err != NO_ERROR) {
        printf("ERROR: couldnt write to file\n");
        return;
      }
      printf("ok\n");
      return;
    } else {
      printf("ERROR: invalid path\n");
      return;
    }
  }
  printf("USAGE: put [path] [text]\n");
  return;
}
