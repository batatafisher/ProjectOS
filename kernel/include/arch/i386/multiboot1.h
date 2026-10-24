#ifndef _ARCH_I386_MULTIBOOT1_H
#define _ARCH_I386_MULTIBOOT1_H

#include <stdint.h>

#include <stdint.h>

/* Memory Map Entry Structure */
typedef struct __attribute__((packed)) {
  uint32_t size; /* Size of the entry excluding this 'size' field */
  uint64_t addr; /* Base physical address */
  uint64_t len;  /* Region length in bytes */
  uint32_t type; /* 1 = Usable, 2 = Reserved, 3 = ACPI Reclaimable, 4 = ACPI
                    NVS, 5 = Bad */
} multiboot_mmap_entry_t;

/* Loaded Module Descriptor Structure */
typedef struct __attribute__((packed)) {
  uint32_t mod_start; /* Physical start address of the loaded module */
  uint32_t mod_end;   /* Physical end address of the loaded module */
  uint32_t string;    /* Physical pointer to module command-line string */
  uint32_t reserved;  /* Must be set to 0 */
} multiboot_module_t;

/* Complete Multiboot 1 Information Structure */
typedef struct __attribute__((packed)) {
  /* Bitmask specifying which fields below are valid */
  uint32_t flags;

  /* Basic Memory Info (Bit 0) */
  uint32_t mem_lower; /* KiB below 1 MiB */
  uint32_t mem_upper; /* KiB above 1 MiB */

  /* Boot Device Info (Bit 1) */
  uint32_t boot_device;

  /* Physical pointer to kernel command-line string (Bit 2) */
  uint32_t cmdline;

  /* Boot Modules (Bit 3) */
  uint32_t mods_count;
  uint32_t mods_addr; /* Physical pointer to array of multiboot_module_t */

  /* Executable Symbol Table Information (Bits 4 or 5) */
  union {
    struct {
      uint32_t tabsize;
      uint32_t strsize;
      uint32_t addr;
      uint32_t reserved;
    } aout_sym;
    struct {
      uint32_t num;
      uint32_t size;
      uint32_t addr;
      uint32_t shndx;
    } elf_sec;
  } syms;

  /* BIOS Memory Map (Bit 6) */
  uint32_t mmap_length;
  uint32_t mmap_addr; /* Physical pointer to array of multiboot_mmap_entry_t */

  /* Drive Information (Bit 7) */
  uint32_t drives_length;
  uint32_t drives_addr;

  /* ROM Configuration Table (Bit 8) */
  uint32_t config_table;

  /* Bootloader Name (Bit 9) */
  uint32_t boot_loader_name; /* Physical pointer to null-terminated string */

  /* APM Table Pointer (Bit 10) */
  uint32_t apm_table;

  /* VBE Video Info (Bit 11) */
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint16_t vbe_mode;
  uint16_t vbe_interface_seg;
  uint16_t vbe_interface_off;
  uint16_t vbe_interface_len;

  /* Framebuffer Layout (Bit 12) */
  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
  uint8_t framebuffer_type; /* 0 = Indexed, 1 = Direct RGB, 2 = EGA Text */

  union {
    struct {
      uint32_t framebuffer_palette_addr;
      uint16_t framebuffer_palette_num_colors;
    } palette;
    struct {
      uint8_t framebuffer_red_field_position;
      uint8_t framebuffer_red_mask_size;
      uint8_t framebuffer_green_field_position;
      uint8_t framebuffer_green_mask_size;
      uint8_t framebuffer_blue_field_position;
      uint8_t framebuffer_blue_mask_size;
    } color;
  } color_info;
} multiboot_info_t;

multiboot_info_t *map_multiboot1(uint32_t magic, uint32_t mbi_phys);

#endif
