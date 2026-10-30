#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void cat(char *args[], int argc) {
  if (argc == 1) {
    char *path = args[0];
    int bytes_count;
    static uint8_t buffer[1024];
    char *full_path = get_full_path(path);
    if (strcmp(full_path, "err") == 0) {
      printf("ERROR: path name too long");
      return;
    }
    int fd = open(full_path, 0);
    if (fd >= 0) {
      bytes_count = read(fd, buffer, 1023);
      close(fd);
      if (bytes_count < 0) {
        printf("ERROR: couldnt read file");
        return;
      }
      buffer[bytes_count] = '\0';
      printf("%s", (char *)buffer);
      return;
    } else {
      printf("ERROR: invalid path");
      return;
    }
  }
  printf("USAGE: cat [path]");
  return;
}
