#include <drivers/ata.h>
#include <fs/ext2.h>
#include <kernel/klib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SIZE 1024

static ext2_filesystem fs;

uint16_t sector[256];

#define MAX_FILES 128

static struct file open_files[MAX_FILES];

const driverdata_t qemu_disk = {
    .tf = 0x1f0, .dcr = 0x3f6, .sbits = 0xe0, .stLBA = 0, .prtlen = 131072};

const driverdata_t *disk = &qemu_disk;

int split_path(const char *str, char *args[], int max_args) {
  const char *p = str;
  if (*p == '/')
    p++;
  return split_with(p, "/", args, max_args, 64);
}

void read_block(const unsigned int block_num, uint16_t *block) {
  uint16_t *ptr = block;
  int32_t sectors = 2;
  uint32_t lba_num = block_num * 2;
  read_ata_st(&sectors, &ptr, disk, &lba_num);
}

void read_superblock(ext2_superblock *sb) {
  uint16_t block[512];
  read_block(1, block);
  memcpy(sb, block, sizeof(ext2_superblock));
}

void read_groupblock(int group_num, ext2_blockgroup *bg) {
  uint16_t block[512];
  uint32_t table_start = (BLOCK_SIZE == 1024) ? 2 : 1;
  uint32_t block_num =
      table_start + (group_num * sizeof(ext2_blockgroup)) / BLOCK_SIZE;
  uint32_t offset = (group_num * sizeof(ext2_blockgroup)) % BLOCK_SIZE;

  read_block(block_num, block);
  memcpy(bg, (uint8_t *)block + offset, sizeof(ext2_blockgroup));
}

void read_inode(int inode_num, ext2_inodes *inode, ext2_superblock sb) {
  uint16_t block[512];

  uint32_t group = (inode_num - 1) / sb.s_inodes_per_group;
  ext2_blockgroup bg = fs.gdt[group];

  uint32_t index = (inode_num - 1) % sb.s_inodes_per_group;
  uint32_t inode_table_block =
      bg.bg_inode_table + (index * sb.s_inode_size) / BLOCK_SIZE;

  uint32_t inode_offset = (index * sb.s_inode_size) % BLOCK_SIZE;

  read_block(inode_table_block, block);
  memcpy(inode, (uint8_t *)block + inode_offset, sizeof(ext2_inodes));
}

void write_block(const unsigned int block_num, uint16_t *block) {
  const uint16_t *ptr = block;
  int32_t sectors = 2;
  uint32_t lba_num = block_num * 2;
  if (!write_ata_st(&sectors, &ptr, disk, &lba_num))
    printf("NOOOOOOOOOOOOOOOOOOOOOOOO!!");
}

void write_inode(uint32_t inode_num, ext2_inodes *inode, ext2_superblock sb) {
  uint16_t block[512];

  uint32_t group = (inode_num - 1) / sb.s_inodes_per_group;
  ext2_blockgroup bg = fs.gdt[group];

  uint32_t index = (inode_num - 1) % sb.s_inodes_per_group;
  uint32_t inode_table_block =
      bg.bg_inode_table + (index * sb.s_inode_size) / BLOCK_SIZE;
  uint32_t inode_offset = (index * sb.s_inode_size) % BLOCK_SIZE;

  uint8_t inode_record[256];

  read_block(inode_table_block, block);

  uint8_t *disk_inode = (uint8_t *)block + inode_offset;

  /* Preserve the existing on-disk inode, including fields you don't model. */
  memcpy(inode_record, disk_inode, fs.sb.s_inode_size);

  /* Replace only the bytes represented by your in-memory structure. */
  memcpy(inode_record, inode, sizeof(ext2_inodes));

  memcpy(disk_inode, inode_record, fs.sb.s_inode_size);

  write_block(inode_table_block, block);
}

void flush_gdt(uint32_t group_num) {
  uint16_t block[512];
  uint32_t table_start = (BLOCK_SIZE == 1024) ? 2 : 1;
  uint32_t block_num =
      table_start + (group_num * sizeof(ext2_blockgroup)) / BLOCK_SIZE;
  uint32_t offset = (group_num * sizeof(ext2_blockgroup)) % BLOCK_SIZE;

  read_block(block_num, block);
  memcpy((uint8_t *)block + offset, &fs.gdt[group_num],
         sizeof(ext2_blockgroup));
  write_block(block_num, block);
}

void flush_superblock() {
  uint16_t block[512];
  read_block(1, block);
  memcpy(block, &fs.sb, sizeof(ext2_superblock));
  write_block(1, block);
}

void free_block(uint32_t block_num) {
  if (block_num == 0)
    return;

  uint32_t relative_block = block_num - fs.sb.s_first_data_block;

  uint32_t group_num = relative_block / fs.sb.s_blocks_per_group;

  uint32_t index = relative_block % fs.sb.s_blocks_per_group;

  uint16_t bitmap_buf[512];

  read_block(fs.gdt[group_num].bg_block_bitmap, bitmap_buf);

  clear_bit(index, (uint8_t *)bitmap_buf);

  write_block(fs.gdt[group_num].bg_block_bitmap, bitmap_buf);

  fs.gdt[group_num].bg_free_blocks_count++;
  fs.sb.s_free_blocks_count++;

  flush_gdt(group_num);
  flush_superblock();
}

void free_inode(uint32_t inode_num) {
  if (inode_num == 0)
    return;

  uint32_t relative_inode = inode_num - 1;

  uint32_t group_num = relative_inode / fs.sb.s_inodes_per_group;

  uint32_t index = relative_inode % fs.sb.s_inodes_per_group;
  uint16_t bitmap_buf[512];
  read_block(fs.gdt[group_num].bg_inode_bitmap, bitmap_buf);

  clear_bit(index, (uint8_t *)bitmap_buf);
  write_block(fs.gdt[group_num].bg_inode_bitmap, bitmap_buf);

  fs.gdt[group_num].bg_free_inodes_count++;
  fs.sb.s_free_inodes_count++;

  flush_gdt(group_num);
  flush_superblock();
}

int lookup_dir(const char *name, ext2_inodes *inode, ext2_superblock sb) {
  uint16_t block[512];
  int found = 0;
  for (int b = 0; b < 12; b++) {
    if (inode->i_block[b] == 0)
      break;
    read_block(inode->i_block[b], block);

    uint32_t offset = 0;

    while (offset < BLOCK_SIZE) {
      ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

      if (entry->inode != 0 && entry->name_len == strlen(name) &&
          memcmp(name, entry->name, entry->name_len) == 0) {

        read_inode(entry->inode, inode, sb);

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

uint32_t allocate_block(ext2_superblock *sb) {
  uint32_t num_groups = (sb->s_blocks_count + sb->s_blocks_per_group - 1) /
                        sb->s_blocks_per_group;

  for (uint32_t g = 0; g < num_groups; g++) {
    ext2_blockgroup *bg = &fs.gdt[g];

    if (bg->bg_free_blocks_count == 0)
      continue;

    // 2. Read Block Bitmap Block
    uint16_t bitmap_block[512];
    read_block(bg->bg_block_bitmap, bitmap_block);
    uint8_t *bitmap = (uint8_t *)bitmap_block;

    for (uint32_t i = 0; i < sb->s_blocks_per_group; i++) {
      if (!test_bit(i, bitmap)) {
        set_bit(i, bitmap);
        write_block(bg->bg_block_bitmap, bitmap_block);

        bg->bg_free_blocks_count--;
        fs.sb.s_free_blocks_count--;

        flush_gdt(g);
        flush_superblock();

        return (g * sb->s_blocks_per_group) + i + 1;
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

    uint16_t bitmap_block[512];
    read_block(bg->bg_inode_bitmap, bitmap_block);
    uint8_t *bitmap = (uint8_t *)bitmap_block;

    for (uint32_t i = 0; i < fs.sb.s_inodes_per_group; i++) {
      if (!test_bit(i, bitmap)) {
        set_bit(i, bitmap);
        write_block(bg->bg_inode_bitmap, bitmap_block);

        bg->bg_free_inodes_count--;
        fs.sb.s_free_inodes_count--;

        flush_gdt(g);
        flush_superblock();
        return (g * fs.sb.s_inodes_per_group) + i + 1;
      }
    }
  }

  return 0;
}

int make_dir_entry(const char *name, uint32_t new_inode, ext2_inodes *parent,
                   uint8_t file_type) {
  uint16_t block[512];

  uint16_t name_len = strlen(name);
  uint16_t needed = 8 + ((name_len + 3) & ~3);

  for (int b = 0; b < 12; b++) {

    if (parent->i_block[b] != 0) {

      read_block(parent->i_block[b], block);

      uint32_t offset = 0;

      while (offset < BLOCK_SIZE) {

        ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);

        if (entry->rec_len == 0)
          return ERROR_FILESYSTEM;

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

          write_block(parent->i_block[b], block);

          return 0;
        }

        offset += entry->rec_len;
      }
    }

    /*
    No existing block had enough space.
    At this point you would allocate a new
    filesystem block and attach it to
    parent->i_block[b].
    */
  }

  return ERROR_NO_SPACE;
}

uint32_t lookup_name(const char *name, const ext2_inodes *parent) {
  uint16_t block[512];

  for (int b = 0; b < 12; b++) {
    if (parent->i_block[b] == 0)
      break;

    read_block(parent->i_block[b], block);
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
  char new_path[512] = "";

  size_t path_len = strlen(path);
  if (path_len > 1 && path[path_len - 1] == '/' && path_len < 512)
    memcpy(new_path, path, path_len - 1);
  else
    memcpy(new_path, path, path_len);
  char storage[16][64];
  char *argv[16];

  for (int i = 0; i < 16; i++)
    argv[i] = storage[i];

  int argc = split_path(new_path, argv, 16);
  if (argc < 0)
    return ERROR_INVALID_PATH; // Always start traversal at Cached Root (Inode
                               // 2)
  uint32_t current_ino = 2;
  ext2_inodes current_inode = fs.root_inode;

  for (int i = 0; i < argc; i++) {
    // Check if current inode is actually a directory (0x4000 bit mask)
    if ((current_inode.i_mode & 0xF000) != 0x4000) {
      return ERROR_DIR_NOT_FOUND; // Tried to traverse through a regular file
    }

    uint32_t next_ino = lookup_name(argv[i], &current_inode);
    if (next_ino == 0) {
      return ERROR_FILE_NOT_FOUND; // Segment not found
    }

    current_ino = next_ino;
    read_inode(current_ino, &current_inode, fs.sb);
  }

  // Export results
  if (out_inode)
    *out_inode = current_inode;
  if (out_inode_num)
    *out_inode_num = current_ino;

  return NO_ERROR;
}

void mount_ext2() {
  read_superblock(&fs.sb);
  if (fs.sb.s_magic != 0xEF53) {
    printf("Error: Not a valid ext2 filesystem (magic: 0x%x)\n", fs.sb.s_magic);
    return;
  }
  for (uint32_t i = 0; i < fs.sb.s_blocks_count / fs.sb.s_blocks_per_group; i++)
    read_groupblock(i, &fs.gdt[i]);

  read_inode(2, &fs.root_inode, fs.sb);

  printf("Mounted ext2 filesystem: succefully\n");
}

void init_ext2() {
  mount_ext2();
  // TODO: create the standard files and directories architecture here
}

int mkdir_ext2(const char *path) {
  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  char storage[16][64];
  char *argv[16];
  // initializing argv to not blow that shi up

  for (int i = 0; i < 16; i++)
    argv[i] = storage[i];

  int argc = split_path(path, argv, 16);

  if (argc < 0)
    return ERROR_DIR_NOT_FOUND;

  if (argc == 0)
    return ERROR_NO_PATH_PROVIDED;

  ext2_inodes parent_dir = fs.root_inode;
  uint32_t parent_inode_num = 2; // Default to root

  if (argc > 1) {

    char parent_path[256] = "";
    for (int i = 0; i < argc - 1; i++) {

      strcat(parent_path, "/");
      strcat(parent_path, argv[i]);
    }

    int err = resolve_path(parent_path, &parent_dir, &parent_inode_num);
    if (err != NO_ERROR)
      return ERROR_DIR_NOT_FOUND;
  }

  char child_name[256] = "";
  strcpy(child_name, argv[argc - 1]);
  if (lookup_name(child_name, &parent_dir) != 0)
    return ERROR_DIR_ALREADY_EXIST;

  uint32_t new_inode_num = allocate_inode();

  if (new_inode_num == 0) {
    return ERROR_NO_SPACE;
  }

  ext2_inodes new_inode;
  memset(&new_inode, 0, sizeof(ext2_inodes));

  new_inode.i_mode = 0x41ED; // Directory (0x4000) + 0755 permissions
  new_inode.i_size = BLOCK_SIZE;
  new_inode.i_links_count = 2;
  new_inode.i_blocks = BLOCK_SIZE / 512;

  uint32_t dir_block_num = allocate_block(&fs.sb);
  if (dir_block_num == 0) {
    free_inode(new_inode_num);
    return ERROR_NO_SPACE;
  }
  new_inode.i_block[0] = dir_block_num;

  uint16_t dir_buf[512];
  memset(dir_buf, 0, sizeof(dir_buf));

  ext2_dir_entry *dot = (ext2_dir_entry *)dir_buf;
  dot->inode = new_inode_num;
  dot->rec_len = 12;
  dot->name_len = 1;
  dot->file_type = 2;
  memcpy(dot->name, ".", 1);

  ext2_dir_entry *dotdot = (ext2_dir_entry *)((uint8_t *)dir_buf + 12);
  dotdot->inode = parent_inode_num;
  dotdot->rec_len = BLOCK_SIZE - 12;
  dotdot->name_len = 2;
  dotdot->file_type = 2;
  memcpy(dotdot->name, "..", 2);

  uint8_t ftype = 2;
  int err = make_dir_entry(argv[argc - 1], new_inode_num, &parent_dir, ftype);
  if (err != NO_ERROR) {
    free_block(dir_block_num);
    free_inode(new_inode_num);
    return err;
  }

  write_block(dir_block_num, dir_buf);

  parent_dir.i_links_count++;
  uint32_t group = (new_inode_num - 1) / fs.sb.s_inodes_per_group;

  fs.gdt[group].bg_used_dirs_count++;
  flush_gdt(group);
  write_inode(parent_inode_num, &parent_dir, fs.sb);

  if (parent_inode_num == 2) {
    fs.root_inode.i_links_count++;
  }

  write_inode(new_inode_num, &new_inode,
              fs.sb); // it is duplicated but its the only way

  return NO_ERROR;
}

int create_ext2(const char *path) {
  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  char storage[16][64];
  char *argv[16];
  // initializing argv to not blow that shi up

  for (int i = 0; i < 16; i++)
    argv[i] = storage[i];

  int argc = split_path(path, argv, 16);

  if (argc < 0)
    return ERROR_DIR_NOT_FOUND;

  if (argc == 0)
    return ERROR_NO_PATH_PROVIDED;

  ext2_inodes parent_dir = fs.root_inode;
  uint32_t parent_inode_num = 2; // Default to root

  if (argc > 1) {

    char parent_path[256] = "";
    for (int i = 0; i < argc - 1; i++) {

      strcat(parent_path, "/");
      strcat(parent_path, argv[i]);
    }

    int err = resolve_path(parent_path, &parent_dir, &parent_inode_num);
    if (err != NO_ERROR)
      return ERROR_DIR_NOT_FOUND;
  }

  char child_name[256] = "";
  strcpy(child_name, argv[argc - 1]);
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

  uint8_t ftype = 1;
  int err = make_dir_entry(argv[argc - 1], new_inode_num, &parent_dir, ftype);
  if (err != NO_ERROR) {
    free_inode(new_inode_num);
    return err;
  }

  write_inode(new_inode_num, &new_inode, fs.sb);

  return NO_ERROR;
}

int write_ext2(int fd, const uint8_t *data, size_t size) {

  if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used)
    return ERROR_FILESYSTEM;

  int needed_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  if (needed_blocks >
      12) // TODO: for now only direct blocks are written consider expanding
    return ERROR_FILE_TOO_BIG;

  ext2_inodes inode = open_files[fd].inode;
  uint32_t inode_num = open_files[fd].inode_num;

  if ((inode.i_mode & 0xF000) == 0x4000)
    return ERROR_INVALID_PATH;

  for (int i = 0; i < needed_blocks; i++) {
    uint16_t block[512];

    uint32_t datablock_num = inode.i_block[i];
    if (datablock_num == 0) {
      datablock_num = allocate_block(&fs.sb);
      if (datablock_num == 0)
        return ERROR_NO_SPACE;
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

    write_block(datablock_num, block);
  }

  // Update inode metadata
  inode.i_size = size;
  inode.i_blocks = needed_blocks * (BLOCK_SIZE / 512);
  open_files[fd].inode = inode; // to keep the cached inode updated

  write_inode(inode_num, &inode, fs.sb);

  if (inode_num == 2) {
    fs.root_inode = inode;
  }

  return NO_ERROR;
}

int read_ext2(int fd, uint8_t *buffer, size_t size) {
  if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used)
    return ERROR_FILESYSTEM;

  ext2_inodes inode = open_files[fd].inode;

  // Verify target is a regular file, not a directory
  if ((inode.i_mode & 0xF000) == 0x4000)
    return ERROR_INVALID_PATH;

  size_t bytes_to_read = size;
  if (bytes_to_read > inode.i_size) {
    bytes_to_read = inode.i_size;
  }

  int needed_blocks = (bytes_to_read + BLOCK_SIZE - 1) / BLOCK_SIZE;

  if (needed_blocks > 12)
    return ERROR_FILE_TOO_BIG;

  size_t bytes_remaining = bytes_to_read;
  size_t offset = 0;

  for (int i = 0; i < needed_blocks; i++) {
    if (inode.i_block[i] == 0) {
      memset(buffer + offset, 0,
             (bytes_remaining < BLOCK_SIZE) ? bytes_remaining : BLOCK_SIZE);
    } else {
      uint16_t block[BLOCK_SIZE / 2]; // 1024-byte buffer
      read_block(inode.i_block[i], block);

      size_t chunk =
          (bytes_remaining < BLOCK_SIZE) ? bytes_remaining : BLOCK_SIZE;
      memcpy(buffer + offset, block, chunk);
    }

    size_t chunk_read =
        (bytes_remaining < BLOCK_SIZE) ? bytes_remaining : BLOCK_SIZE;
    bytes_remaining -= chunk_read;
    offset += chunk_read;
  }
  buffer[bytes_to_read] = '\0';

  return NO_ERROR;
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
  uint32_t path_len = strlen(path);

  int i = path_len - 1;

  while (i >= 0 && path[i] != '/') {
    i--;
  }

  if (i < 0 || i == path_len - 1)
    return ERROR_INVALID_PATH;

  char parent_path[64];
  char dir_name[32];

  if (i >= (int)sizeof(parent_path) || (path_len - i - 1) >= sizeof(dir_name))
    return ERROR_INVALID_PATH;

  if (i == 0) {
    strcpy(parent_path, "/");
  } else {
    memcpy(parent_path, path, i);
    parent_path[i] = '\0';
  }

  memcpy(dir_name, path + i + 1, path_len - i - 1);
  dir_name[path_len - i - 1] = '\0';

  if ((strlen(dir_name) == 1 && dir_name[0] == '.') ||
      (strlen(dir_name) == 2 && dir_name[0] == '.' && dir_name[1] == '.'))
    return ERROR_INVALID_PATH;

  ext2_inodes parent;
  uint32_t parent_num;
  int err = resolve_path(parent_path, &parent, &parent_num);
  if (err != NO_ERROR)
    return err;

  uint32_t target_inode_num = lookup_name(dir_name, &parent);
  if (target_inode_num == 0)
    return ERROR_DIR_NOT_FOUND;
  else if (target_inode_num == 2)
    return ERROR_FORBIDDEN;

  ext2_inodes directory_inode;
  read_inode(target_inode_num, &directory_inode, fs.sb);

  // Verify target is a directory
  if ((directory_inode.i_mode & 0xf000) != 0x4000)
    return ERROR_NOT_A_DIR;

  uint16_t block[512];

  for (int b = 0; b < 12; b++) {
    if (directory_inode.i_block[b] == 0)
      break;

    read_block(directory_inode.i_block[b], block);
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

    read_block(parent.i_block[b], block);
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

        write_block(parent.i_block[b], block);
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
    free_block(directory_inode.i_block[b]);
    directory_inode.i_block[b] = 0;
  }

  parent.i_links_count--;
  write_inode(parent_num, &parent, fs.sb);
  uint32_t group = (target_inode_num - 1) / fs.sb.s_inodes_per_group;

  fs.gdt[group].bg_used_dirs_count--;
  flush_gdt(group);

  if (parent_num == 2) {
    fs.root_inode.i_links_count--;
  }

  directory_inode.i_blocks = 0;
  directory_inode.i_links_count = 0;
  directory_inode.i_dtime = 0x5F000000;
  write_inode(target_inode_num, &directory_inode, fs.sb);
  free_inode(target_inode_num);

  return NO_ERROR;
}

int unlink_ext2(const char *path) {

  uint32_t path_len = strlen(path);
  int i = path_len - 1;

  while (i >= 0 && path[i] != '/') {
    i--;
  }

  if (i < 0 || i == path_len - 1)
    return ERROR_INVALID_PATH;

  char parent_path[128];
  char file_name[32];

  if (i == 0) {
    strcpy(parent_path, "/");
  } else {
    memcpy(parent_path, path, i);
    parent_path[i] = '\0';
  }

  memcpy(file_name, path + i + 1, path_len - i - 1);
  file_name[path_len - i - 1] = '\0';

  ext2_inodes parent;
  uint32_t parent_num;
  int err = resolve_path(parent_path, &parent, &parent_num);
  if (err != NO_ERROR)
    return err;

  uint16_t block[512];

  for (int b = 0; b < 12; b++) {
    if (parent.i_block[b] == 0)
      break;

    read_block(parent.i_block[b], block);
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
        read_inode(target_inode_num, &target_inode, fs.sb);

        if (prev_entry != NULL) {
          prev_entry->rec_len += entry->rec_len;
        } else {
          entry->inode = 0;
        }
        write_block(parent.i_block[b], block);

        target_inode.i_links_count--;

        if (target_inode.i_links_count == 0) {
          for (int k = 0; k < 12; k++) {
            if (target_inode.i_block[k] != 0) {
              free_block(target_inode.i_block[k]);
              target_inode.i_block[k] = 0;
            }
          }
          target_inode.i_dtime =
              0x5F000000; /* anything > s_inodes_count; real time later */
          write_inode(target_inode_num, &target_inode, fs.sb);
          free_inode(target_inode_num);
        } else {
          write_inode(target_inode_num, &target_inode, fs.sb);
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

  uint16_t block[512];
  int count = 0;

  for (int b = 0; b < 12; b++) {
    if (parent.i_block[b] == 0)
      break;

    read_block(parent.i_block[b], block);
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
