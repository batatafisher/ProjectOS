#include <drivers/ata.h>
#include <fs/ext2.h>
#include <kernel/klib.h>
#include <kernel/mmu.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 1024

static ext2_filesystem fs;

#define MAX_FILES 128
#define MAX_FILENAME 255
#define MAX_PATH_DEPTH 16

static struct file open_files[MAX_FILES];

const driverdata_t qemu_disk = {
    .tf = 0x1f0, .dcr = 0x3f6, .sbits = 0xe0, .stLBA = 0, .prtlen = 131072};

const driverdata_t *disk = &qemu_disk;

int split_path(const char *str, char *args[], int max_args) {
  const char *p = str;
  if (*p == '/')
    p++;
  return split_with(p, "/", args, max_args, 255);
}

bool read_block(const unsigned int block_num, uint16_t *block) {
  uint16_t *ptr = block;
  int32_t sectors = 2;
  uint32_t lba_num = block_num * 2;
  return read_ata_st(&sectors, &ptr, disk, &lba_num);
}

bool read_superblock(ext2_superblock *sb) {
  uint16_t block[BLOCK_SIZE / 2];
  if (!read_block(1, block))
    return false;
  memcpy(sb, block, sizeof(ext2_superblock));
  return true;
}

bool read_groupblock(int group_num, ext2_blockgroup *bg) {
  uint16_t block[BLOCK_SIZE / 2];
  uint32_t table_start = (BLOCK_SIZE == 1024) ? 2 : 1;
  uint32_t block_num =
      table_start + (group_num * sizeof(ext2_blockgroup)) / BLOCK_SIZE;
  uint32_t offset = (group_num * sizeof(ext2_blockgroup)) % BLOCK_SIZE;

  if (!read_block(block_num, block))
    return false;
  memcpy(bg, (uint8_t *)block + offset, sizeof(ext2_blockgroup));
  return true;
}

bool read_inode(int inode_num, ext2_inodes *inode) {
  uint16_t block[BLOCK_SIZE / 2];

  uint32_t group = (inode_num - 1) / fs.sb.s_inodes_per_group;
  ext2_blockgroup bg = fs.gdt[group];

  uint32_t index = (inode_num - 1) % fs.sb.s_inodes_per_group;
  uint32_t inode_table_block =
      bg.bg_inode_table + (index * fs.sb.s_inode_size) / BLOCK_SIZE;

  uint32_t inode_offset = (index * fs.sb.s_inode_size) % BLOCK_SIZE;

  if (!read_block(inode_table_block, block))
    return false;
  memcpy(inode, (uint8_t *)block + inode_offset, sizeof(ext2_inodes));
  return true;
}

bool write_block(const unsigned int block_num, uint16_t *block) {
  const uint16_t *ptr = block;
  int32_t sectors = 2;
  uint32_t lba_num = block_num * 2;
  return write_ata_st(&sectors, &ptr, disk, &lba_num);
}

bool write_inode(uint32_t inode_num, ext2_inodes *inode) {
  if (inode_num == 2)
    fs.root_inode = *inode;

  uint16_t block[BLOCK_SIZE / 2];

  uint32_t group = (inode_num - 1) / fs.sb.s_inodes_per_group;
  ext2_blockgroup bg = fs.gdt[group];

  uint32_t index = (inode_num - 1) % fs.sb.s_inodes_per_group;
  uint32_t inode_table_block =
      bg.bg_inode_table + (index * fs.sb.s_inode_size) / BLOCK_SIZE;
  uint32_t inode_offset = (index * fs.sb.s_inode_size) % BLOCK_SIZE;

  uint8_t inode_record[256];

  if (!read_block(inode_table_block, block))
    return false;

  uint8_t *disk_inode = (uint8_t *)block + inode_offset;

  /* Preserve the existing on-disk inode, including fields you don't model. */
  memcpy(inode_record, disk_inode, fs.sb.s_inode_size);

  /* Replace only the bytes represented by your in-memory structure. */
  memcpy(inode_record, inode, sizeof(ext2_inodes));

  memcpy(disk_inode, inode_record, fs.sb.s_inode_size);

  return write_block(inode_table_block, block);
}

bool flush_gdt(uint32_t group_num) {
  uint16_t block[BLOCK_SIZE / 2];
  uint32_t table_start = (BLOCK_SIZE == 1024) ? 2 : 1;
  uint32_t block_num =
      table_start + (group_num * sizeof(ext2_blockgroup)) / BLOCK_SIZE;
  uint32_t offset = (group_num * sizeof(ext2_blockgroup)) % BLOCK_SIZE;

  if (!read_block(block_num, block))
    return false;
  memcpy((uint8_t *)block + offset, &fs.gdt[group_num],
         sizeof(ext2_blockgroup));
  return write_block(block_num, block);
}

bool flush_superblock() {
  uint16_t block[BLOCK_SIZE / 2];
  if (!read_block(1, block))
    return false;
  memcpy(block, &fs.sb, sizeof(ext2_superblock));
  return write_block(1, block);
}

bool free_block(uint32_t block_num) {
  if (block_num == 0)
    return false;

  uint32_t relative_block = block_num - fs.sb.s_first_data_block;

  uint32_t group_num = relative_block / fs.sb.s_blocks_per_group;

  uint32_t index = relative_block % fs.sb.s_blocks_per_group;

  uint16_t bitmap_buf[BLOCK_SIZE / 2];

  if (!read_block(fs.gdt[group_num].bg_block_bitmap, bitmap_buf))
    return false;

  clear_bit(index, (uint8_t *)bitmap_buf);

  if (!write_block(fs.gdt[group_num].bg_block_bitmap, bitmap_buf))
    return false;

  fs.gdt[group_num].bg_free_blocks_count++;
  fs.sb.s_free_blocks_count++;

  if (!flush_gdt(group_num))
    return false;
  return flush_superblock();
}

bool free_inode(uint32_t inode_num) {
  if (inode_num == 0)
    return false;

  uint32_t relative_inode = inode_num - 1;

  uint32_t group_num = relative_inode / fs.sb.s_inodes_per_group;

  uint32_t index = relative_inode % fs.sb.s_inodes_per_group;
  uint16_t bitmap_buf[BLOCK_SIZE / 2];
  if (!read_block(fs.gdt[group_num].bg_inode_bitmap, bitmap_buf))
    return false;

  clear_bit(index, (uint8_t *)bitmap_buf);
  if (!write_block(fs.gdt[group_num].bg_inode_bitmap, bitmap_buf))
    return false;

  fs.gdt[group_num].bg_free_inodes_count++;
  fs.sb.s_free_inodes_count++;

  if (!flush_gdt(group_num))
    return false;
  return flush_superblock();
}

int lookup_dir(const char *name, ext2_inodes *inode) {
  uint16_t block[BLOCK_SIZE / 2];
  int found = 0;
  for (int b = 0; b < 12; b++) {
    if (inode->i_block[b] == 0)
      break;
    if (!read_block(inode->i_block[b], block))
      return ERROR_STORAGE_DRIVER;

    uint32_t offset = 0;

    while (offset < BLOCK_SIZE) {
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

      if (entry->inode != 0 && entry->name_len == strlen(name) &&
          memcmp(name, entry->name, entry->name_len) == 0) {

        if (!read_inode(entry->inode, inode))
          return ERROR_STORAGE_DRIVER;

        found = 1;
        break;
      }
      if (entry->rec_len == 0)
        break;
      offset += entry->rec_len;
    }

    if (found)
      break;
  }
  if (!found)
    return ERROR_DIR_NOT_FOUND;
  return NO_ERROR;
}

uint32_t allocate_block() {
  uint32_t num_groups = (fs.sb.s_blocks_count + fs.sb.s_blocks_per_group - 1) /
                        fs.sb.s_blocks_per_group;

  for (uint32_t g = 0; g < num_groups; g++) {
    ext2_blockgroup *bg = &fs.gdt[g];

    if (bg->bg_free_blocks_count == 0)
      continue;

    uint16_t bitmap_block[BLOCK_SIZE / 2];
    if (!read_block(bg->bg_block_bitmap, bitmap_block))
      return 0;
    uint8_t *bitmap = (uint8_t *)bitmap_block;

    for (uint32_t i = 0; i < fs.sb.s_blocks_per_group; i++) {
      if (!test_bit(i, bitmap)) {
        set_bit(i, bitmap);
        if (!write_block(bg->bg_block_bitmap, bitmap_block))
          return 0;

        bg->bg_free_blocks_count--;
        fs.sb.s_free_blocks_count--;

        if (!flush_gdt(g) || !flush_superblock())
          return 0;

        return (g * fs.sb.s_blocks_per_group) + i + fs.sb.s_first_data_block;
      }
    }
  }

  return 0;
}

uint32_t allocate_inode() {
  uint32_t num_groups = (fs.sb.s_inodes_count + fs.sb.s_inodes_per_group - 1) /
                        fs.sb.s_inodes_per_group;

  for (uint32_t g = 0; g < num_groups; g++) {
    ext2_blockgroup *bg = &fs.gdt[g];

    if (bg->bg_free_inodes_count == 0)
      continue;

    uint16_t bitmap_block[BLOCK_SIZE / 2];
    if (!read_block(bg->bg_inode_bitmap, bitmap_block))
      return 0;
    uint8_t *bitmap = (uint8_t *)bitmap_block;

    for (uint32_t i = 0; i < fs.sb.s_inodes_per_group; i++) {
      if (!test_bit(i, bitmap)) {
        set_bit(i, bitmap);
        if (!write_block(bg->bg_inode_bitmap, bitmap_block))
          return 0;

        bg->bg_free_inodes_count--;
        fs.sb.s_free_inodes_count--;

        if (!flush_gdt(g) || !flush_superblock())
          return 0;

        return (g * fs.sb.s_inodes_per_group) + i + 1;
      }
    }
  }

  return 0;
}

int make_dir_entry(const char *name, uint32_t new_inode, ext2_inodes *parent,
                   uint8_t file_type) {
  uint16_t block[BLOCK_SIZE / 2];

  uint16_t name_len = strlen(name);
  uint16_t needed = 8 + ((name_len + 3) & ~3);

  for (int b = 0; b < 12; b++) {

    if (parent->i_block[b] == 0) {
      uint32_t new_block_num = allocate_block();
      if (new_block_num == 0)
        break;
      parent->i_block[b] = new_block_num;
      parent->i_size += BLOCK_SIZE;
      parent->i_blocks += BLOCK_SIZE / 512;
      memset(block, 0, BLOCK_SIZE);
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block);
      entry->rec_len = BLOCK_SIZE;
      entry->inode = new_inode;
      entry->name_len = name_len;
      entry->file_type = file_type;
      memcpy(entry->name, name, name_len);
      if (!write_block(new_block_num, block))
        return ERROR_STORAGE_DRIVER;
      return NO_ERROR;
    }

    if (parent->i_block[b] != 0) {

      if (!read_block(parent->i_block[b], block))
        return ERROR_STORAGE_DRIVER;

      uint32_t offset = 0;

      while (offset < BLOCK_SIZE) {

        ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

        if (entry->rec_len == 0)
          return ERROR_FILESYSTEM;
        if (entry->inode == 0) {
          if (entry->rec_len >= needed) {
            entry->inode = new_inode;
            entry->name_len = name_len;
            entry->file_type = file_type;

            memcpy(entry->name, name, name_len);

            if (!write_block(parent->i_block[b], block))
              return ERROR_STORAGE_DRIVER;

            return NO_ERROR;
          }
        } else {
          uint16_t actual = 8 + ((entry->name_len + 3) & ~3);

          uint16_t available = entry->rec_len - actual;

          if (available >= needed) {

            uint16_t old_rec_len = entry->rec_len;

            entry->rec_len = actual;

            offset += actual;

            ext2_dir_entry *new_entry =
                (ext2_dir_entry *)((uint8_t *)block + offset);

            new_entry->inode = new_inode;
            new_entry->rec_len = old_rec_len - actual;
            new_entry->name_len = name_len;
            new_entry->file_type = file_type;

            memcpy(new_entry->name, name, name_len);

            if (!write_block(parent->i_block[b], block))
              return ERROR_STORAGE_DRIVER;

            return NO_ERROR;
          }
        }

        offset += entry->rec_len;
      }
    }
  }

  return ERROR_NO_SPACE;
}

uint32_t lookup_name(const char *name, const ext2_inodes *parent) {
  uint16_t block[BLOCK_SIZE / 2];

  for (int b = 0; b < 12; b++) {
    if (parent->i_block[b] == 0)
      break;

    if (!read_block(parent->i_block[b], block))
      return 0;
    uint32_t offset = 0;

    while (offset < BLOCK_SIZE) {
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

      if (entry->inode != 0 && entry->name_len == strlen(name) &&
          memcmp(name, entry->name, entry->name_len) == 0) {
        return entry->inode; // Found matching file or directory inode!
      }

      if (entry->rec_len == 0)
        break;
      offset += entry->rec_len;
    }
  }

  return 0; // Not found
}

int resolve_path(const char *path, ext2_inodes *out_inode,
                 uint32_t *out_inode_num) {

  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  uint32_t current_ino = 2;
  ext2_inodes current_inode = fs.root_inode;
  int big_good = 0;
  int p = 0;

  for (int i = 0; i < MAX_PATH_DEPTH; i++) {
    for (; path[p] != '\0'; p++) {
      if (path[p] != '/')
        break;
    }

    // Check if current inode is actually a directory (0x4000 bit mask)
    if ((current_inode.i_mode & 0xF000) != 0x4000) {
      return ERROR_DIR_NOT_FOUND; // Tried to traverse through a regular file
    }

    if (path[p] == '\0') {
      if (i == 0) {
        if (out_inode)
          *out_inode = fs.root_inode;
        if (out_inode_num)
          *out_inode_num = 2;
        return NO_ERROR;
      } else if (i > 0) {
        big_good = 1;
        break;
      } else {
        return UNEXPECTED_ERROR;
      }
    }

    char name[MAX_FILENAME + 1] = "";
    int good = 0;
    for (int j = 0; j <= MAX_FILENAME; j++) {
      if (path[p + j] == '/') {
        p += j + 1;
        name[j] = '\0';
        good = 1;
        break;
      }
      if (path[p + j] == '\0') {
        p += j;
        name[j] = '\0';
        big_good = 1;
        good = 1;
        break;
      }
      name[j] = path[p + j];
    }
    if (good == 0)
      return ERROR_FILENAME_TOO_BIG;

    uint32_t next_ino = lookup_name(name, &current_inode);
    if (next_ino == 0) {
      return ERROR_FILE_NOT_FOUND; // Segment not found
    }

    current_ino = next_ino;
    if (!read_inode(current_ino, &current_inode))
      return ERROR_STORAGE_DRIVER;

    if (big_good)
      break;
  }
  if (!big_good)
    return ERROR_PATH_EXEEDED_MAX_DEPTH;

  // Export results
  if (out_inode)
    *out_inode = current_inode;
  if (out_inode_num)
    *out_inode_num = current_ino;

  return NO_ERROR;
}

void mount_ext2() {
  if (!read_superblock(&fs.sb)) {
    printf("Error: Failed to read superblock\n");
    return;
  }
  if (fs.sb.s_magic != 0xEF53) {
    printf("Error: Not a valid ext2 filesystem (magic: 0x%x)\n", fs.sb.s_magic);
    return;
  }
  uint32_t count = (fs.sb.s_blocks_count + fs.sb.s_blocks_per_group - 1) /
                   fs.sb.s_blocks_per_group;
  fs.gdt = kcalloc(count, sizeof(ext2_blockgroup));

  for (uint32_t i = 0; i < count; i++) {
    if (!read_groupblock(i, &fs.gdt[i])) {
      printf("Error: Failed to read group block %u\n", i);
      return;
    }
  }

  if (!read_inode(2, &fs.root_inode)) {
    printf("Error: Failed to read root inode\n");
    return;
  }

  printf("Mounted ext2 filesystem: succefully\n");
}

void init_ext2() {
  mount_ext2();
  // TODO: create the standard files and directories architecture here
}

int mkdir_ext2(const char *path) {
  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  int len = strlen(path);
  while (len > 1 && path[len - 1] == '/') {
    len--;
  }

  // Cannot recreate root directory
  if (len == 1 && path[0] == '/') {
    return ERROR_DIR_ALREADY_EXIST;
  }

  int last_slash = -1;
  for (int i = len - 1; i >= 0; i--) {
    if (path[i] == '/') {
      last_slash = i;
      break;
    }
  }

  if (last_slash == -1)
    return ERROR_DIR_NOT_FOUND;

  char parent_path[MAX_FILENAME * MAX_PATH_DEPTH + 1];
  char child_name[MAX_FILENAME + 1];

  int child_len = len - (last_slash + 1);
  if (child_len <= 0)
    return ERROR_DIR_NOT_FOUND;
  if (child_len > MAX_FILENAME)
    return ERROR_FILENAME_TOO_BIG;

  if (last_slash == 0) {
    parent_path[0] = '/';
    parent_path[1] = '\0';
  } else {
    if (last_slash >= (int)sizeof(parent_path))
      return ERROR_FILENAME_TOO_BIG;
    memcpy(parent_path, path, last_slash);
    parent_path[last_slash] = '\0';
  }

  memcpy(child_name, path + last_slash + 1, child_len);
  child_name[child_len] = '\0';

  ext2_inodes parent_dir;
  uint32_t parent_inode_num;
  int err = resolve_path(parent_path, &parent_dir, &parent_inode_num);
  if (err != NO_ERROR)
    return err;

  if ((parent_dir.i_mode & 0xF000) != 0x4000)
    return ERROR_INVALID_PATH;

  if (lookup_name(child_name, &parent_dir) != 0)
    return ERROR_DIR_ALREADY_EXIST;

  uint32_t new_inode_num = allocate_inode();
  if (new_inode_num == 0)
    return ERROR_NO_SPACE;

  uint32_t dir_block_num = allocate_block();
  if (dir_block_num == 0) {
    free_inode(new_inode_num);
    return ERROR_NO_SPACE;
  }

  ext2_inodes new_inode;
  memset(&new_inode, 0, sizeof(ext2_inodes));
  new_inode.i_mode = 0x41ED; // Directory (0x4000) + 0755
  new_inode.i_size = BLOCK_SIZE;
  new_inode.i_links_count = 2; // '.' and parent entry
  new_inode.i_blocks = BLOCK_SIZE / 512;
  new_inode.i_block[0] = dir_block_num;

  uint16_t dir_buf[BLOCK_SIZE / 2];
  memset(dir_buf, 0, BLOCK_SIZE);

  ext2_dir_entry *dot = (ext2_dir_entry *)dir_buf;
  dot->inode = new_inode_num;
  dot->rec_len = 12;
  dot->name_len = 1;
  dot->file_type = 2; // Directory
  memcpy(dot->name, ".", 1);

  ext2_dir_entry *dotdot = (ext2_dir_entry *)((uint8_t *)dir_buf + 12);
  dotdot->inode = parent_inode_num;
  dotdot->rec_len = BLOCK_SIZE - 12;
  dotdot->name_len = 2;
  dotdot->file_type = 2; // Directory
  memcpy(dotdot->name, "..", 2);

  if (!write_block(dir_block_num, dir_buf)) {
    free_block(dir_block_num);
    free_inode(new_inode_num);
    return ERROR_STORAGE_DRIVER;
  }

  err = make_dir_entry(child_name, new_inode_num, &parent_dir, 2);
  if (err != NO_ERROR) {
    free_block(dir_block_num);
    free_inode(new_inode_num);
    return err;
  }

  parent_dir.i_links_count++;
  uint32_t group = (new_inode_num - 1) / fs.sb.s_inodes_per_group;
  fs.gdt[group].bg_used_dirs_count++;

  if (!flush_gdt(group) || !write_inode(parent_inode_num, &parent_dir) ||
      !write_inode(new_inode_num, &new_inode)) {
    return ERROR_STORAGE_DRIVER;
  }

  return NO_ERROR;
}

int create_ext2(const char *path) {
  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  int len = strlen(path);
  while (len > 1 && path[len - 1] == '/') {
    len--;
  }

  // Cannot recreate root directory
  if (len == 1 && path[0] == '/') {
    return ERROR_FILE_ALREADY_EXIST;
  }

  int last_slash = -1;
  for (int i = len - 1; i >= 0; i--) {
    if (path[i] == '/') {
      last_slash = i;
      break;
    }
  }

  if (last_slash == -1)
    return ERROR_DIR_NOT_FOUND;

  char parent_path[MAX_FILENAME * MAX_PATH_DEPTH + 1];
  char child_name[MAX_FILENAME + 1];

  int child_len = len - (last_slash + 1);
  if (child_len <= 0)
    return ERROR_DIR_NOT_FOUND;
  if (child_len > MAX_FILENAME)
    return ERROR_FILENAME_TOO_BIG;

  if (last_slash == 0) {
    parent_path[0] = '/';
    parent_path[1] = '\0';
  } else {
    if (last_slash >= (int)sizeof(parent_path))
      return ERROR_FILENAME_TOO_BIG;
    memcpy(parent_path, path, last_slash);
    parent_path[last_slash] = '\0';
  }

  memcpy(child_name, path + last_slash + 1, child_len);
  child_name[child_len] = '\0';

  ext2_inodes parent_dir;
  uint32_t parent_inode_num;
  int err = resolve_path(parent_path, &parent_dir, &parent_inode_num);
  if (err != NO_ERROR)
    return err;

  if ((parent_dir.i_mode & 0xF000) != 0x4000)
    return ERROR_INVALID_PATH;

  if (lookup_name(child_name, &parent_dir) != 0)
    return ERROR_FILE_ALREADY_EXIST;

  uint32_t new_inode_num = allocate_inode();
  if (new_inode_num == 0)
    return ERROR_NO_SPACE;

  ext2_inodes new_inode;
  memset(&new_inode, 0, sizeof(ext2_inodes));
  new_inode.i_mode = 0x81A4; // Regular file (0x8000) + 0644 permissions
  new_inode.i_size = 0;
  new_inode.i_links_count = 1;

  // add directory entry to parent (file_type = 1 for regular file)
  uint8_t ftype = 1;
  err = make_dir_entry(child_name, new_inode_num, &parent_dir, ftype);
  if (err != NO_ERROR) {
    free_inode(new_inode_num);
    return err;
  }

  if (!write_inode(parent_inode_num, &parent_dir) ||
      !write_inode(new_inode_num, &new_inode)) {
    return ERROR_STORAGE_DRIVER;
  }

  return NO_ERROR;
}

int write_ext2(int fd, const uint8_t *data, size_t size) {

  if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used)
    return ERROR_FILESYSTEM;

  int needed_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  if (needed_blocks > 12)
    return ERROR_FILE_TOO_BIG;

  ext2_inodes inode = open_files[fd].inode;
  uint32_t inode_num = open_files[fd].inode_num;

  if ((inode.i_mode & 0xF000) == 0x4000)
    return ERROR_INVALID_PATH;

  for (int i = 0; i < needed_blocks; i++) {
    uint16_t block[BLOCK_SIZE / 2];

    uint32_t datablock_num = inode.i_block[i];
    if (datablock_num == 0) {
      datablock_num = allocate_block();
      if (datablock_num == 0) {
        for (int j = 0; j < i; j++)
          if (!free_block(inode.i_block[j]))
            return ERROR_STORAGE_DRIVER;
        return ERROR_NO_SPACE;
      }
      inode.i_block[i] = datablock_num;
    }

    if (i == needed_blocks - 1) {
      size_t bytes_written = i * BLOCK_SIZE;
      size_t remaining_bytes = size - bytes_written;

      memcpy(block, data + bytes_written, remaining_bytes);
      memset((uint8_t *)block + remaining_bytes, 0,
             BLOCK_SIZE - remaining_bytes);
    } else {
      memcpy(block, data + (i * BLOCK_SIZE), BLOCK_SIZE);
    }

    if (!write_block(datablock_num, block))
      return ERROR_STORAGE_DRIVER;
  }

  // Update inode metadata
  int old_blocks = (inode.i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  if (old_blocks > needed_blocks) {
    for (int i = needed_blocks; i < old_blocks; i++) {
      if (!free_block(inode.i_block[i]))
        return ERROR_STORAGE_DRIVER;
      inode.i_block[i] = 0;
    }
  }
  inode.i_size = size;
  inode.i_blocks = needed_blocks * (BLOCK_SIZE / 512);

  open_files[fd].inode = inode; // to keep the cached inode updated

  if (!write_inode(inode_num, &inode))
    return ERROR_STORAGE_DRIVER;

  return NO_ERROR;
}

int read_ext2(int fd, uint8_t *buffer, size_t size) {
  if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used)
    return -ERROR_FILESYSTEM;

  ext2_inodes inode = open_files[fd].inode;

  // Verify target is a regular file, not a directory
  if ((inode.i_mode & 0xF000) == 0x4000)
    return -ERROR_EXPECTED_FILE_FOUND_DIR;

  size_t bytes_to_read = size;
  if (bytes_to_read > inode.i_size) {
    bytes_to_read = inode.i_size;
  }

  int needed_blocks = (bytes_to_read + BLOCK_SIZE - 1) / BLOCK_SIZE;

  if (needed_blocks > 12)
    return -ERROR_FILE_TOO_BIG;

  size_t bytes_remaining = bytes_to_read;
  size_t offset = 0;

  for (int i = 0; i < needed_blocks; i++) {
    if (inode.i_block[i] == 0) {
      memset(buffer + offset, 0,
             (bytes_remaining < BLOCK_SIZE) ? bytes_remaining : BLOCK_SIZE);
    } else {
      uint16_t block[BLOCK_SIZE / 2]; // 1024-byte buffer
      if (!read_block(inode.i_block[i], block))
        return -ERROR_STORAGE_DRIVER;

      size_t chunk =
          (bytes_remaining < BLOCK_SIZE) ? bytes_remaining : BLOCK_SIZE;
      memcpy(buffer + offset, block, chunk);
    }

    size_t chunk_read =
        (bytes_remaining < BLOCK_SIZE) ? bytes_remaining : BLOCK_SIZE;
    bytes_remaining -= chunk_read;
    offset += chunk_read;
  }

  return bytes_to_read;
}

int open_ext2(const char *path, uint32_t flags) {
  ext2_inodes inode;
  uint32_t inode_num;
  int err = resolve_path(path, &inode, &inode_num);
  if (err != NO_ERROR)
    return -1;

  for (int i = 0; i < MAX_FILES; i++) {
    if (!open_files[i].used) {
      open_files[i].inode = inode;
      open_files[i].inode_num = inode_num;
      open_files[i].offset = 0;
      open_files[i].flags = flags;
      open_files[i].used = true;

      return i;
    }
  }

  return -1;
}

int remove_dir_ext2(const char *path) {
  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  int len = strlen(path);
  while (len > 1 && path[len - 1] == '/') {
    len--;
  }

  // Cannot remove root directory
  if (len == 1 && path[0] == '/') {
    return ERROR_FORBIDDEN;
  }

  int last_slash = -1;
  for (int i = len - 1; i >= 0; i--) {
    if (path[i] == '/') {
      last_slash = i;
      break;
    }
  }

  if (last_slash == -1)
    return ERROR_DIR_NOT_FOUND;

  char parent_path[MAX_FILENAME * MAX_PATH_DEPTH + 1];
  char dir_name[MAX_FILENAME + 1];

  int child_len = len - (last_slash + 1);
  if (child_len <= 0)
    return ERROR_DIR_NOT_FOUND;
  if (child_len > MAX_FILENAME)
    return ERROR_FILENAME_TOO_BIG;

  if (last_slash == 0) {
    parent_path[0] = '/';
    parent_path[1] = '\0';
  } else {
    if (last_slash >= (int)sizeof(parent_path))
      return ERROR_FILENAME_TOO_BIG;
    memcpy(parent_path, path, last_slash);
    parent_path[last_slash] = '\0';
  }

  memcpy(dir_name, path + last_slash + 1, child_len);
  dir_name[child_len] = '\0';

  if ((strlen(dir_name) == 1 && dir_name[0] == '.') ||
      (strlen(dir_name) == 2 && dir_name[0] == '.' && dir_name[1] == '.'))
    return ERROR_INVALID_PATH;

  ext2_inodes parent;
  uint32_t parent_num;
  int err = resolve_path(parent_path, &parent, &parent_num);
  if (err != NO_ERROR)
    return err;

  if ((parent.i_mode & 0xF000) != 0x4000)
    return ERROR_INVALID_PATH;

  uint32_t target_inode_num = lookup_name(dir_name, &parent);
  if (target_inode_num == 0)
    return ERROR_DIR_NOT_FOUND;
  else if (target_inode_num == 2)
    return ERROR_FORBIDDEN;

  ext2_inodes directory_inode;
  if (!read_inode(target_inode_num, &directory_inode))
    return ERROR_STORAGE_DRIVER;

  // Verify target is a directory
  if ((directory_inode.i_mode & 0xf000) != 0x4000)
    return ERROR_NOT_A_DIR;

  uint16_t block[BLOCK_SIZE / 2];

  for (int b = 0; b < 12; b++) {
    if (directory_inode.i_block[b] == 0)
      break;

    if (!read_block(directory_inode.i_block[b], block))
      return ERROR_STORAGE_DRIVER;

    int offset = 0;
    while (offset < BLOCK_SIZE) {
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

      if (entry->rec_len == 0)
        break;

      if (entry->inode != 0) {
        if (!(entry->name_len == 1 && entry->name[0] == '.') &&
            !(entry->name_len == 2 && entry->name[0] == '.' &&
              entry->name[1] == '.')) {
          return ERROR_DIR_NOT_EMPTY;
        }
      }

      offset += entry->rec_len;
    }
  }

  int entry_removed = 0;
  for (int b = 0; b < 12 && !entry_removed; b++) {
    if (parent.i_block[b] == 0)
      break;

    if (!read_block(parent.i_block[b], block))
      return ERROR_STORAGE_DRIVER;

    int offset = 0;
    ext2_dir_entry *prev_entry = NULL;

    while (offset < BLOCK_SIZE) {
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

      if (entry->rec_len == 0)
        break;

      if (entry->inode == target_inode_num &&
          entry->name_len == strlen(dir_name) &&
          strncmp(entry->name, dir_name, entry->name_len) == 0) {

        if (prev_entry != NULL) {
          prev_entry->rec_len += entry->rec_len;
        } else {
          entry->inode = 0;
        }

        if (!write_block(parent.i_block[b], block))
          return ERROR_STORAGE_DRIVER;

        entry_removed = 1;
        break;
      }

      prev_entry = entry;
      offset += entry->rec_len;
    }
  }

  if (!entry_removed)
    return ERROR_DIR_NOT_FOUND;

  for (int b = 0; b < 12; b++) {
    if (directory_inode.i_block[b] == 0)
      break;
    if (!free_block(directory_inode.i_block[b]))
      return ERROR_STORAGE_DRIVER;
    directory_inode.i_block[b] = 0;
  }

  parent.i_links_count--;
  if (!write_inode(parent_num, &parent))
    return ERROR_STORAGE_DRIVER;

  uint32_t group = (target_inode_num - 1) / fs.sb.s_inodes_per_group;

  fs.gdt[group].bg_used_dirs_count--;
  if (!flush_gdt(group))
    return ERROR_STORAGE_DRIVER;

  directory_inode.i_blocks = 0;
  directory_inode.i_links_count = 0;
  directory_inode.i_dtime = 0x5F000000;
  if (!write_inode(target_inode_num, &directory_inode) ||
      !free_inode(target_inode_num)) {
    return ERROR_STORAGE_DRIVER;
  }

  return NO_ERROR;
}

int unlink_ext2(const char *path) {
  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  int len = strlen(path);
  while (len > 1 && path[len - 1] == '/') {
    len--;
  }

  // Cannot unlink root directory
  if (len == 1 && path[0] == '/') {
    return ERROR_FORBIDDEN;
  }

  int last_slash = -1;
  for (int i = len - 1; i >= 0; i--) {
    if (path[i] == '/') {
      last_slash = i;
      break;
    }
  }

  if (last_slash == -1)
    return ERROR_DIR_NOT_FOUND;

  char parent_path[MAX_FILENAME * MAX_PATH_DEPTH + 1];
  char file_name[MAX_FILENAME + 1];

  int child_len = len - (last_slash + 1);
  if (child_len <= 0)
    return ERROR_DIR_NOT_FOUND;
  if (child_len > MAX_FILENAME)
    return ERROR_FILENAME_TOO_BIG;

  if (last_slash == 0) {
    parent_path[0] = '/';
    parent_path[1] = '\0';
  } else {
    if (last_slash >= (int)sizeof(parent_path))
      return ERROR_FILENAME_TOO_BIG;
    memcpy(parent_path, path, last_slash);
    parent_path[last_slash] = '\0';
  }

  memcpy(file_name, path + last_slash + 1, child_len);
  file_name[child_len] = '\0';

  ext2_inodes parent;
  uint32_t parent_num;
  int err = resolve_path(parent_path, &parent, &parent_num);
  if (err != NO_ERROR)
    return err;

  if ((parent.i_mode & 0xF000) != 0x4000)
    return ERROR_INVALID_PATH;

  uint16_t block[BLOCK_SIZE / 2];

  for (int b = 0; b < 12; b++) {
    if (parent.i_block[b] == 0)
      break;

    if (!read_block(parent.i_block[b], block))
      return ERROR_STORAGE_DRIVER;

    uint32_t offset = 0;
    ext2_dir_entry *prev_entry = NULL;

    while (offset < BLOCK_SIZE) {
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

      if (entry->rec_len == 0)
        break;

      if (entry->inode != 0 && entry->name_len == strlen(file_name) &&
          memcmp(file_name, entry->name, entry->name_len) == 0) {

        uint32_t target_inode_num = entry->inode;
        ext2_inodes target_inode;
        if (!read_inode(target_inode_num, &target_inode))
          return ERROR_STORAGE_DRIVER;

        if ((target_inode.i_mode & 0xf000) == 0x4000)
          return ERROR_EXPECTED_FILE_FOUND_DIR;

        if (prev_entry != NULL) {
          prev_entry->rec_len += entry->rec_len;
        } else {
          entry->inode = 0;
        }

        if (!write_block(parent.i_block[b], block))
          return ERROR_STORAGE_DRIVER;

        target_inode.i_links_count--;

        if (target_inode.i_links_count == 0) {
          for (int k = 0; k < 12; k++) {
            if (target_inode.i_block[k] != 0) {
              if (!free_block(target_inode.i_block[k]))
                return ERROR_STORAGE_DRIVER;
              target_inode.i_block[k] = 0;
            }
          }
          target_inode.i_dtime = 0x5F000000;
          if (!write_inode(target_inode_num, &target_inode) ||
              !free_inode(target_inode_num)) {
            return ERROR_STORAGE_DRIVER;
          }
        } else {
          if (!write_inode(target_inode_num, &target_inode))
            return ERROR_STORAGE_DRIVER;
        }

        return NO_ERROR;
      }

      prev_entry = entry;
      offset += entry->rec_len;
    }
  }

  return ERROR_FILE_NOT_FOUND;
}

int list_dir_ext2(const char *path, dir_info_t *out_entries, int max_entries) {
  ext2_inodes parent;
  uint32_t inode_num;

  int err = resolve_path(path, &parent, &inode_num);
  if (err != NO_ERROR) {
    return -err;
  }

  if ((parent.i_mode & 0xF000) != 0x4000)
    return -ERROR_INVALID_PATH;

  uint16_t block[BLOCK_SIZE / 2];
  int count = 0;

  for (int b = 0; b < 12; b++) {
    if (parent.i_block[b] == 0)
      break;

    if (!read_block(parent.i_block[b], block))
      return -ERROR_STORAGE_DRIVER;

    uint32_t offset = 0;

    while (offset < BLOCK_SIZE) {
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

      if (entry->rec_len == 0)
        break;

      if (entry->inode != 0) {
        if (count >= max_entries)
          return count; // Buffer full, return what we have

        out_entries[count].inode = entry->inode;
        out_entries[count].file_type = entry->file_type;

        // Safely copy string name with null terminator
        memcpy(out_entries[count].name, entry->name, entry->name_len);
        out_entries[count].name[entry->name_len] = '\0';

        count++;
      }

      offset += entry->rec_len;
    }
  }

  return count;
}

int close_ext2(int fd) {
  if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used)
    return ERROR_FILESYSTEM;

  open_files[fd].used = false;
  return NO_ERROR;
}

int stat_ext2(const char *path) {
  if (!path || path[0] != '/')
    return -ERROR_INVALID_PATH;

  ext2_inodes inode;

  int err = resolve_path(path, &inode, NULL);
  if (err != NO_ERROR)
    return -err;

  uint16_t type = inode.i_mode & 0xF000;

  if (type == 0x8000)
    return 1; // Regular file

  if (type == 0x4000)
    return 2; // Directory

  return -ERROR_FILESYSTEM; // Unsupported inode type
}
