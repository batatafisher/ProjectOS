#include "../commands.h"
#include <fs/vfs.h>
#include <kernel/shell.h>
#include <stdint.h>
#include <string.h>

const char *put(char *args[], int argc) {
  if (argc > 1) {
    char *path = args[0];
    int err;
    char buffer[1024] = "";
    for (int i = 1; i < argc; i++) {
      strcat(buffer, args[i]);
      strcat(buffer, " ");
    }
    if (path[0] == '/') {
      int fd = open(path, 0);
      if (fd >= 0) {
        err = write(fd, (uint8_t *)buffer, strlen(buffer));
        close(fd);
        if (err != NO_ERROR) {
          return "ERROR: couldnt write to file";
        }
        return "ok";
      } else {
        return "ERROR: invalid path";
      }
    } else {
      char full_path[128] = "";
      strcpy(full_path, shell_get_cwd());

      if (strcmp(shell_get_cwd(), "/") != 0)
        strcat(full_path, "/");

      strcat(full_path, path);
      int fd = open(full_path, 0);
      if (fd >= 0) {
        err = write(fd, (uint8_t *)buffer, strlen(buffer));

        close(fd);
        if (err != NO_ERROR) {
          return "ERROR: couldnt write to file";
        }
        return "ok";
      } else {
        return "ERROR: invalid path";
      }
    }
  }
  return "USAGE: put [path] [text]";
}
