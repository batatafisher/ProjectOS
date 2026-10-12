#include <drivers/ata.h>
#include <kernel/ext2.h>
#include <kernel/klib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
#define ERROR_TEST 11

#define BLOCK_SIZE 1024

static ext2_filesystem fs;

uint16_t sector[256];

#define MAX_FILES 128

static struct file open_files[MAX_FILES];

const driverdata_t qemu_disk = {
    .tf = 0x1f0, .dcr = 0x3f6, .sbits = 0xe0, .stLBA = 0, .prtlen = 131072};

const driverdata_t *disk = &qemu_disk;

int split_path(char *str, char *args[], int max_args) {
  int argc = 0;
  char *p = str;
  if (*p == '/')
    p++;
  while (*p != '\0' && argc < max_args) {

    if (*p == '\0')
      break;

    args[argc++] = p;

    while (*p != '\0' && *p != '/')
      p++;

    if (*p == '/') {
      *p = '\0';
      p++;
    }
  }
  return argc;
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
  read_block(2, block);
  memcpy(bg, (uint8_t *)block + sizeof(ext2_blockgroup) * group_num,
         sizeof(ext2_blockgroup));
}

void read_inode(int inode_num, ext2_inodes *inode, ext2_superblock sb) {
  uint16_t block[512];

  uint32_t group = (inode_num - 1) / sb.s_inodes_per_group;
  ext2_blockgroup bg;

  // Use cached group block for primary group 0, read disk otherwise
  if (group == 0) {
    bg = fs.bg;
  } else {
    read_groupblock(group, &bg);
  }

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
  write_ata_st(&sectors, &ptr, disk, &lba_num);
}

void write_inode(uint32_t inode_num, ext2_inodes *inode, ext2_superblock sb) {
  uint16_t block[512];

  uint32_t group = (inode_num - 1) / sb.s_inodes_per_group;
  ext2_blockgroup bg;

  if (group == 0) {
    bg = fs.bg;
  } else {
    read_groupblock(group, &bg);
  }

  uint32_t index = (inode_num - 1) % sb.s_inodes_per_group;
  uint32_t inode_table_block =
      bg.bg_inode_table + (index * sb.s_inode_size) / BLOCK_SIZE;
  uint32_t inode_offset = (index * sb.s_inode_size) % BLOCK_SIZE;

  // Read the block containing this inode table section
  read_block(inode_table_block, block);

  // Overwrite the specific inode entry in the block buffer
  memcpy((uint8_t *)block + inode_offset, inode, sizeof(ext2_inodes));

  // Flush modified block back to disk
  write_block(inode_table_block, block);
}

void free_block(uint32_t block_num) {
  if (block_num == 0)
    return;

  uint32_t index = (block_num - 1) % fs.sb.s_blocks_per_group;

  // 1. Read block bitmap directly using cached fs.bg
  uint16_t bitmap_buf[512];
  read_block(fs.bg.bg_block_bitmap, bitmap_buf);

  // 2. Clear bit and write bitmap back to disk
  clear_bit(index, (uint8_t *)bitmap_buf);
  write_block(fs.bg.bg_block_bitmap, bitmap_buf);

  // 3. Update cached group descriptor and write back Block 2
  fs.bg.bg_free_blocks_count++;
  write_block(2, (uint16_t *)&fs.bg);
}

void free_inode(uint32_t inode_num) {
  if (inode_num == 0)
    return;

  uint32_t index = (inode_num - 1) % fs.sb.s_inodes_per_group;

  // 1. Read inode bitmap directly using cached fs.bg
  uint16_t bitmap_buf[512];
  read_block(fs.bg.bg_inode_bitmap, bitmap_buf);

  // 2. Clear bit and write bitmap back to disk
  clear_bit(index, (uint8_t *)bitmap_buf);
  write_block(fs.bg.bg_inode_bitmap, bitmap_buf);

  // 3. Update cached group descriptor and write back Block 2
  fs.bg.bg_free_inodes_count++;
  write_block(2, (uint16_t *)&fs.bg);
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

uint32_t allocate_block(ext2_superblock *sb) {
  uint16_t block[512];
  uint32_t num_groups = (sb->s_blocks_count + sb->s_blocks_per_group - 1) /
                        sb->s_blocks_per_group;

  for (uint32_t g = 0; g < num_groups; g++) {
    // 1. Read Block 2 (Group Descriptor Table)
    read_block(2, block);
    ext2_blockgroup *bg =
        (ext2_blockgroup *)((uint8_t *)block + (sizeof(ext2_blockgroup) * g));

    if (bg->bg_free_blocks_count == 0)
      continue;

    // 2. Read Block Bitmap Block
    uint16_t bitmap_block[512];
    read_block(bg->bg_block_bitmap, bitmap_block);
    uint8_t *bitmap = (uint8_t *)bitmap_block;

    for (uint32_t i = 0; i < sb->s_blocks_per_group; i++) {
      if (!test_bit(i, bitmap)) {
        // Mark bit as used and write bitmap back to disk
        set_bit(i, bitmap);
        write_block(bg->bg_block_bitmap, bitmap_block);

        // Update free block count directly inside the read Block 2 buffer
        bg->bg_free_blocks_count--;

        // Keep cached Group Descriptor in memory in sync if group 0
        if (g == 0) {
          fs.bg.bg_free_blocks_count--;
        }

        // Write back updated Block 2 safely
        write_block(2, block);

        // Return 1-based block index
        return (g * sb->s_blocks_per_group) + i + 1;
      }
    }
  }

  return 0;
}

uint32_t allocate_inode(ext2_superblock *sb) {
  uint16_t block[512];
  uint32_t num_groups = (sb->s_inodes_count + sb->s_inodes_per_group - 1) /
                        sb->s_inodes_per_group;

  for (uint32_t g = 0; g < num_groups; g++) {
    // 1. Read Block 2 (Group Descriptor Table)
    read_block(2, block);
    ext2_blockgroup *bg =
        (ext2_blockgroup *)((uint8_t *)block + (sizeof(ext2_blockgroup) * g));

    if (bg->bg_free_inodes_count == 0)
      continue;

    // 2. Read Inode Bitmap Block
    uint16_t bitmap_block[512];
    read_block(bg->bg_inode_bitmap, bitmap_block);
    uint8_t *bitmap = (uint8_t *)bitmap_block;

    for (uint32_t i = 0; i < sb->s_inodes_per_group; i++) {
      if (!test_bit(i, bitmap)) {
        // Mark bit as used and write bitmap back to disk
        set_bit(i, bitmap);
        write_block(bg->bg_inode_bitmap, bitmap_block);

        // Update free inode count directly inside the read Block 2 buffer
        bg->bg_free_inodes_count--;

        // Keep cached Group Descriptor in memory in sync if group 0
        if (g == 0) {
          fs.bg.bg_free_inodes_count--;
        }

        // Write back updated Block 2 safely
        write_block(2, block);

        // Return 1-based inode index
        return (g * sb->s_inodes_per_group) + i + 1;
      }
    }
  }

  return 0;
}

// Resolves a single entry name inside a parent directory to its inode number
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

  size_t path_len = strlen(path);
  if (path_len > 1 && path[path_len - 1] == '/')
    return ERROR_INVALID_PATH;

  char *argv[16];
  int argc = split_path((char *)path, argv, 16);

  // Always start traversal at Cached Root (Inode 2)
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
  read_groupblock(0, &fs.bg);
  read_inode(2, &fs.root_inode, fs.sb);

  if (fs.sb.s_magic != 0xEF53) {
    printf("Error: Not a valid ext2 filesystem (magic: 0x%x)\n", fs.sb.s_magic);
    return;
  }

  printf("Mounted ext2 filesystem: succefully\n");
}

void init_ext2() {
  mount_ext2();
  // TODO: create the standard files and directories architecture here
}

int create(const char *path) {
  if (!path || path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  char *argv[16];

  // Check if path ends with '/' before splitting (since split_path strips
  // slashes)
  size_t path_len = strlen(path);
  int is_dir = (path_len > 1 && path[path_len - 1] == '/');

  int argc = split_path((char *)path, argv, 16);

  if (argc == 0)
    return ERROR_NO_PATH_PROVIDED;

  ext2_inodes parent_dir = fs.root_inode;
  uint32_t parent_inode_num = 2; // Default to root

  // If creating inside a subdirectory (e.g. "/a/b/file.txt"), resolve parent
  // directory path
  if (argc > 1) {
    // Construct parent directory path string
    ;

    char parent_path[256] = "";
    for (int i = 0; i < argc - 1; i++) {

      strcat(parent_path, "/");
      strcat(parent_path, argv[i]);
    }

    int err = resolve_path(parent_path, &parent_dir, &parent_inode_num);
    if (err != NO_ERROR)
      return ERROR_DIR_NOT_FOUND;
  }
  // 1. Allocate a free inode number
  uint32_t new_inode_num = allocate_inode(&fs.sb);

  if (new_inode_num == 0)
    return ERROR_NO_SPACE;

  // 2. Initialize new inode
  ext2_inodes new_inode;
  memset(&new_inode, 0, sizeof(ext2_inodes));

  if (is_dir) {
    // --- DIRECTORY CREATION ---
    new_inode.i_mode = 0x41ED; // Directory (0x4000) + 0755 permissions
    new_inode.i_size = BLOCK_SIZE;
    new_inode.i_links_count = 2; // '.' and parent reference

    // Allocate 1 data block for directory contents
    uint32_t dir_block_num = allocate_block(&fs.sb);
    if (dir_block_num == 0)
      return ERROR_NO_SPACE;

    new_inode.i_block[0] = dir_block_num;

    // Populate default '.' and '..' entries inside the new block
    uint16_t dir_buf[512];
    memset(dir_buf, 0, sizeof(dir_buf));

    // '.' -> points to new_inode_num (rec_len = 12)
    ext2_dir_entry *dot = (ext2_dir_entry *)dir_buf;
    dot->inode = new_inode_num;
    dot->rec_len = 12;
    dot->name_len = 1;
    dot->file_type = 2;
    memcpy(dot->name, ".", 1);

    // '..' -> points to parent_inode_num (rec_len = 1012)
    ext2_dir_entry *dotdot = (ext2_dir_entry *)((uint8_t *)dir_buf + 12);
    dotdot->inode = parent_inode_num;
    dotdot->rec_len = BLOCK_SIZE - 12;
    dotdot->name_len = 2;
    dotdot->file_type = 2; // EXT2_FT_DIR
    memcpy(dotdot->name, "..", 2);

    write_block(dir_block_num, dir_buf);

    parent_dir.i_links_count++;
    write_inode(parent_inode_num, &parent_dir, fs.sb);

    if (parent_inode_num == 2) {
      fs.root_inode.i_links_count++;
    }

  } else {
    // --- REGULAR FILE CREATION ---
    new_inode.i_mode = 0x81A4; // Regular file (0x8000) + 0644 permissions
    new_inode.i_size = 0;
    new_inode.i_links_count = 1;
  }

  // 3. Write new inode metadata to disk
  write_inode(new_inode_num, &new_inode, fs.sb);

  // 4. Insert entry in parent directory (type 2 = Dir, type 1 = File)
  uint8_t ftype = is_dir ? 2 : 1;
  int err = make_dir_entry(argv[argc - 1], new_inode_num, &parent_dir, ftype);
  if (err != NO_ERROR)
    return err;

  return NO_ERROR;
}

int write(int fd, const uint8_t *data, size_t size) {

  if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used)
    return ERROR_FILESYSTEM;

  int needed_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

  if (needed_blocks >
      12) // TODO: for now only direct blocks are written consider expanding
    return ERROR_FILE_TOO_BIG;

  ext2_inodes inode = open_files[fd].inode;
  uint32_t inode_num = open_files[fd].inode_num;

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
  write_inode(inode_num, &inode, fs.sb);

  if (inode_num == 2) {
    fs.root_inode = inode;
  }

  return NO_ERROR;
}

int read(int fd, uint8_t *buffer, size_t size) {
  if (fd < 0 || fd >= MAX_FILES || !open_files[fd].used)
    return ERROR_FILESYSTEM;

  ext2_inodes inode = open_files[fd].inode;

  // Verify target is a regular file, not a directory
  if ((inode.i_mode & 0xF000) == 0x4000)
    return ERROR_INVALID_PATH;

  // Clamp requested size to actual file size
  size_t bytes_to_read = size;
  if (bytes_to_read > inode.i_size) {
    bytes_to_read = inode.i_size;
  }

  // Calculate needed 1024-byte blocks
  int needed_blocks = (bytes_to_read + BLOCK_SIZE - 1) / BLOCK_SIZE;

  // Direct blocks limit check (i_block[0..11])
  if (needed_blocks > 12)
    return ERROR_FILE_TOO_BIG;

  size_t bytes_remaining = bytes_to_read;
  size_t offset = 0;

  for (int i = 0; i < needed_blocks; i++) {
    // Check for unallocated block sparse hole
    if (inode.i_block[i] == 0) {
      memset(buffer + offset, 0,
             (bytes_remaining < BLOCK_SIZE) ? bytes_remaining : BLOCK_SIZE);
    } else {
      uint16_t block[512]; // 1024-byte buffer
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

int open(const char *path, uint32_t flags) {
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

int unlink(const char *path) {

  uint32_t path_len = strlen(path);
  int i = path_len - 1;

  while (i >= 0 && path[i] != '/') {
    i--;
  }

  if (i < 0 || i == path_len - 1)
    return ERROR_INVALID_PATH;

  char parent_path[64];
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

        // 1. Unlink directory entry from parent block
        if (prev_entry) {
          prev_entry->rec_len += entry->rec_len;
        } else {
          entry->inode = 0;
        }
        write_block(parent.i_block[b], block);

        // 2. Decrement link count
        target_inode.i_links_count--;

        if (target_inode.i_links_count == 0) {
          // Free direct blocks in block bitmap
          for (int k = 0; k < 12; k++) {
            if (target_inode.i_block[k] != 0) {
              free_block(target_inode.i_block[k]);
              target_inode.i_block[k] = 0;
            }
          }
          // Free inode in inode bitmap
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

int list_dir(const char *path, dir_info_t *out_entries, int max_entries) {
  ext2_inodes parent;
  uint32_t inode_num;

  int err = resolve_path(path, &parent, &inode_num);
  if (err != NO_ERROR)
    return -err;

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

  return count; // Returns number of files found
}

void test_disk() {
  printf("superblock: %d\n", sizeof(ext2_superblock));
  printf("group desc: %d\n", sizeof(ext2_blockgroup));
  printf("inode:      %d\n", sizeof(ext2_inodes));
  dir_info_t entries[64];
  int max_entries = 64;
  int num = list_dir("/", entries, max_entries);
  for (int i = 0; i < num; i++) {
    printf("name = %s, file_type = %d\n", entries[i].name,
           entries[i].file_type);
  }
  unlink("/hello.txt");
  num = list_dir("/", entries, max_entries);
  for (int i = 0; i < num; i++) {
    printf("name = %s, file_type = %d\n", entries[i].name,
           entries[i].file_type);
  }
}
