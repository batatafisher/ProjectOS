#include <fs/ext2.h>
#include <fs/vfs.h>
#include <stddef.h>
#include <stdint.h>

// === a struct of functions for forward compatibility ===
typedef struct {
  int (*create)(const char *path);
  int (*mkdir)(const char *path);
  int (*open)(const char *path, uint32_t flags);
  int (*read)(int fd, uint8_t *buffer, size_t size);
  int (*write)(int fd, const uint8_t *data, size_t size);
  int (*list)(const char *path, dir_info_t *out_entries, int max_entries);
  int (*unlink)(const char *path);
  int (*rmdir)(const char *path);
  int (*close)(int fd);
  int (*stat)(const char *path);

  int (*init_fs)();
} vfs_driver_t;

vfs_driver_t ext2 = {.create = create_ext2,
                     .mkdir = mkdir_ext2,
                     .open = open_ext2,
                     .read = read_ext2,
                     .write = write_ext2,
                     .list = list_dir_ext2,
                     .unlink = unlink_ext2,
                     .close = close_ext2,
                     .stat = stat_ext2,
                     .init_fs = init_ext2,
                     .rmdir = remove_dir_ext2};

vfs_driver_t *active_fs = &ext2;

int state(const char *path) {
  if (!active_fs || !active_fs->stat)
    return -ERROR_FILESYSTEM;

  return active_fs->stat(path);
}

int init_fs() {
  if (!active_fs || !active_fs->init_fs)
    return ERROR_FILESYSTEM;

  return active_fs->init_fs();
}

int create(const char *path) {
  if (!active_fs || !active_fs->create)
    return ERROR_FILESYSTEM;

  return active_fs->create(path);
}

int mkdir(const char *path) {
  if (!active_fs || !active_fs->mkdir)
    return ERROR_FILESYSTEM;

  return active_fs->mkdir(path);
}

int open(const char *path, uint32_t flags) {
  if (!active_fs || !active_fs->open)
    return ERROR_FILESYSTEM;

  return active_fs->open(path, flags);
}

int read(int fd, uint8_t *buffer, size_t size) {
  if (!active_fs || !active_fs->read)
    return ERROR_FILESYSTEM;

  return active_fs->read(fd, buffer, size);
}

int write(int fd, const uint8_t *data, size_t size) {
  if (!active_fs || !active_fs->write)
    return ERROR_FILESYSTEM;

  return active_fs->write(fd, data, size);
}

int list(const char *path, dir_info_t *out_entries, int max_entries) {
  if (!active_fs || !active_fs->list)
    return ERROR_FILESYSTEM;

  return active_fs->list(path, out_entries, max_entries);
}

int unlink(const char *path) {
  if (!active_fs || !active_fs->unlink)
    return ERROR_FILESYSTEM;

  return active_fs->unlink(path);
}

int rmdir(const char *path) {
  if (!active_fs || !active_fs->rmdir)
    return ERROR_FILESYSTEM;

  return active_fs->rmdir(path);
}

int close(int fd) {
  if (!active_fs || !active_fs->close)
    return ERROR_FILESYSTEM;

  return active_fs->close(fd);
}
