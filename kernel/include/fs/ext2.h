#ifndef _KERNEL_EXT2_H
#define _KERNEL_EXT2_H

#include <fs/vfs.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint32_t s_inodes_count;
  uint32_t s_blocks_count;
  uint32_t s_r_blocks_count;
  uint32_t s_free_blocks_count;
  uint32_t s_free_inodes_count;

  uint32_t s_first_data_block;

  uint32_t s_log_block_size;
  int32_t s_log_frag_size;

  uint32_t s_blocks_per_group;
  uint32_t s_frags_per_group;
  uint32_t s_inodes_per_group;

  uint32_t s_mtime;
  uint32_t s_wtime;

  uint16_t s_mnt_count;
  int16_t s_max_mnt_count;

  uint16_t s_magic;

  uint16_t s_state;
  uint16_t s_errors;
  uint16_t s_minor_rev_level;

  uint32_t s_lastcheck;
  uint32_t s_checkinterval;

  uint32_t s_creator_os;
  uint32_t s_rev_level;

  uint16_t s_def_resuid;
  uint16_t s_def_resgid;

  /* EXT2_DYNAMIC_REV superblock fields */

  uint32_t s_first_ino;
  uint16_t s_inode_size;
  uint16_t s_block_group_nr;

  uint32_t s_feature_compat;
  uint32_t s_feature_incompat;
  uint32_t s_feature_ro_compat;

  uint8_t s_uuid[16];

  char s_volume_name[16];

  char s_last_mounted[64];

  uint32_t s_algorithm_usage_bitmap;

  /* Performance hints */

  uint8_t s_prealloc_blocks;
  uint8_t s_prealloc_dir_blocks;

  uint16_t s_padding1;

  /* Journaling support */

  uint8_t s_journal_uuid[16];

  uint32_t s_journal_inum;
  uint32_t s_journal_dev;
  uint32_t s_last_orphan;

  /* Directory indexing */

  uint32_t s_hash_seed[4];

  uint8_t s_def_hash_version;

  uint8_t s_reserved_char_pad;
  uint16_t s_reserved_word_pad;

  /* Other options */

  uint32_t s_default_mount_opts;
  uint32_t s_first_meta_bg;

  uint32_t s_reserved[190];

} ext2_superblock;

typedef struct __attribute__((packed)) {
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_bitmap;
  uint32_t bg_inode_table;

  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs_count;

  uint16_t bg_pad;

  uint32_t bg_reserved[3];
} ext2_blockgroup;

typedef struct __attribute__((packed)) {
  uint16_t i_mode;
  uint16_t i_uid;

  uint32_t i_size;

  uint32_t i_atime;
  uint32_t i_ctime;
  uint32_t i_mtime;
  uint32_t i_dtime;

  uint16_t i_gid;
  uint16_t i_links_count;

  uint32_t i_blocks;
  uint32_t i_flags;

  uint32_t i_osd1;

  uint32_t i_block[15];

  uint32_t i_generation;
  uint32_t i_file_acl;
  uint32_t i_dir_acl;
  uint32_t i_faddr;

  uint8_t i_osd2[12];
} ext2_inodes;

typedef struct {
  ext2_superblock sb;
  ext2_inodes root_inode;
  ext2_blockgroup *gdt;
} ext2_filesystem;

typedef struct __attribute__((packed)) {
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
  char name[];
} ext2_dir_entry;

struct file {
  ext2_inodes inode;
  uint32_t inode_num;
  uint32_t offset;
  uint32_t flags;
  bool used;
};

_Static_assert(sizeof(ext2_superblock) == 1024, "Bad superblock size");

_Static_assert(sizeof(ext2_blockgroup) == 32, "Bad group descriptor size");

_Static_assert(sizeof(ext2_inodes) == 128, "Bad inode size");

int init_ext2();

int create_ext2(const char *path);
int mkdir_ext2(const char *path);
int write_ext2(int fd, const uint8_t *data, size_t size);
int read_ext2(int fd, uint8_t *buffer, size_t size);
int list_dir_ext2(const char *path, dir_info_t *out_entries, int max_entries);
int unlink_ext2(const char *path);
int remove_dir_ext2(const char *path);
int stat_ext2(const char *path);

int open_ext2(const char *path, uint32_t flags);
int close_ext2(int fd);

#endif
