#ifndef _FS_VFS_H
#define _FS_VFS_H

#include <stddef.h>
#include <stdint.h>

#define NO_ERROR 0
#define ERROR_DIR_NOT_FOUND 1
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_NO_PATH_PROVIDED 3
#define ERROR_FILESYSTEM 4
#define ERROR_NO_SPACE 5
#define ERROR_INVALID_PATH 6
#define ERROR_FILE_TOO_BIG 7
#define ERROR_EXPECTED_FILE_FOUND_DIR 8
#define WARNING_NO_SPCAE_IN_FILE 9
#define UNEXPECTED_ERROR 10
#define ERROR_DIR_ALREADY_EXIST 11
#define ERROR_FILE_ALREADY_EXIST 12
#define ERROR_FORBIDDEN 13
#define ERROR_DIR_NOT_EMPTY 14
#define ERROR_NOT_A_DIR 15
#define ERROR_TEST 16

typedef struct {
  uint32_t inode;
  uint8_t file_type;
  char name[256];
} dir_info_t;

int init_fs();
int create(const char *path);
int mkdir(const char *path);
int open(const char *path, uint32_t flags);
int read(int fd, uint8_t *buffer, size_t size);
int write(int fd, const uint8_t *data, size_t size);
int list(const char *path, dir_info_t *out_entries, int max_entries);
int unlink(const char *path);
int rmdir(const char *path);
int close(int fd);
int state(const char *path);

#endif
