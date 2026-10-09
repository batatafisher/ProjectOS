#include <drivers/ata.h>
#include <kernel/ext2.h>
#include <kernel/klib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NO_ERROR 0
#define ERROR_DIR_NOT_FOUND 1
#define ERROR_FILE_NOT_FOUND 2
#define ERROR_NO_PATH_PROVIDED 3
#define ERROR_FILESYSTEM 4
#define ERROR_NO_SPACE 5

#define BLOCK_SIZE 1024

uint16_t sector[256];

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
  read_groupblock(group, &bg);
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
  read_groupblock(group, &bg);

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
     * No existing block had enough space.
     *
     * At this point you would allocate a new
     * filesystem block and attach it to
     * parent->i_block[b].
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

        // Write back updated Block 2 safely
        write_block(2, block);

        // Return 1-based inode index
        return (g * sb->s_inodes_per_group) + i + 1;
      }
    }
  }

  return 0;
}

int create(const char *path) {
  char *argv[16];

  // Check if path ends with '/' before splitting (since split_path strips
  // slashes)
  size_t path_len = strlen(path);
  int is_dir = (path_len > 1 && path[path_len - 1] == '/');

  int argc = split_path((char *)path, argv, 16);

  if (argc == 0)
    return ERROR_NO_PATH_PROVIDED;
  if (path[0] != '/')
    return ERROR_DIR_NOT_FOUND;

  ext2_superblock sb;
  read_superblock(&sb);

  ext2_inodes parent_dir;
  uint32_t parent_inode_num = 2; // Root inode
  read_inode(parent_inode_num, &parent_dir, sb);

  // Traverse path to target parent directory
  for (int i = 0; i < argc - 1; i++) {
    // Save target inode number during traversal
    uint16_t block[512];
    for (int b = 0; b < 12; b++) {
      if (parent_dir.i_block[b] == 0)
        break;
      read_block(parent_dir.i_block[b], block);
      uint32_t offset = 0;
      while (offset < BLOCK_SIZE) {
        ext2_dir_entry *entry = (ext2_dir_entry *)((uint8_t *)block + offset);
        if (entry->inode != 0 && entry->name_len == strlen(argv[i]) &&
            memcmp(argv[i], entry->name, entry->name_len) == 0) {
          parent_inode_num = entry->inode;
          break;
        }
        if (entry->rec_len == 0)
          break;
        offset += entry->rec_len;
      }
    }
    if (lookup_dir(argv[i], &parent_dir, sb) != NO_ERROR)
      return ERROR_DIR_NOT_FOUND;
  }

  // 1. Allocate a free inode number
  uint32_t new_inode_num = allocate_inode(&sb);
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
    uint32_t dir_block_num = allocate_block(&sb);
    if (dir_block_num == 0)
      return ERROR_NO_SPACE;

    new_inode.i_block[0] = dir_block_num;

    // Populate default '.' and '..' entries inside the new block
    uint16_t dir_buf[512];
    memset(dir_buf, 0, sizeof(dir_buf));

    // Entry 1: '.' -> points to new_inode_num (rec_len = 12)
    ext2_dir_entry *dot = (ext2_dir_entry *)dir_buf;
    dot->inode = new_inode_num;
    dot->rec_len = 12;
    dot->name_len = 1;
    dot->file_type = 2; // EXT2_FT_DIR
    memcpy(dot->name, ".", 1);

    // Entry 2: '..' -> points to parent_inode_num (rec_len = 1012)
    ext2_dir_entry *dotdot = (ext2_dir_entry *)((uint8_t *)dir_buf + 12);
    dotdot->inode = parent_inode_num;
    dotdot->rec_len = BLOCK_SIZE - 12;
    dotdot->name_len = 2;
    dotdot->file_type = 2; // EXT2_FT_DIR
    memcpy(dotdot->name, "..", 2);

    // Write directory block to disk
    write_block(dir_block_num, dir_buf);

    // Increment parent directory link count
    parent_dir.i_links_count++;
    write_inode(parent_inode_num, &parent_dir, sb);

  } else {
    // --- REGULAR FILE CREATION ---
    new_inode.i_mode = 0x81A4; // Regular file (0x8000) + 0644 permissions
    new_inode.i_size = 0;
    new_inode.i_links_count = 1;
  }

  // 3. Write new inode metadata to disk
  write_inode(new_inode_num, &new_inode, sb);

  // 4. Insert entry in parent directory (type 2 = Dir, type 1 = File)
  uint8_t ftype = is_dir ? 2 : 1;
  int err = make_dir_entry(argv[argc - 1], new_inode_num, &parent_dir, ftype);
  if (err != NO_ERROR)
    return err;

  return NO_ERROR;
}
void test_disk() {
  printf("superblock: %d\n", sizeof(ext2_superblock));
  printf("group desc: %d\n", sizeof(ext2_blockgroup));
  printf("inode:      %d\n", sizeof(ext2_inodes));
  create("/testdir/file");
}
