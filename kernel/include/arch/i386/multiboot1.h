#ifndef _ARCH_I386_MULTIBOOT1_H
#define _ARCH_I386_MULTIBOOT1_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
  uint32_t flags;                /* which fields below are valid */
  uint32_t mem_lower, mem_upper; /* KB below 1MB / above 1MB, flag bit 0 */
  uint32_t boot_device;
  uint32_t cmdline; /* physical pointer, flag bit 2 */
  uint32_t mods_count, mods_addr;
  uint32_t syms[4];
  uint32_t mmap_length; /* flag bit 6: the memory map */
  uint32_t mmap_addr;   /* physical pointer */
  /* more fields follow (drives, bootloader name, VBE, framebuffer) */
} multiboot_info_t;

typedef struct __attribute__((packed)) {
  uint32_t size; /* size of the REST of this entry; not counting this field */
  uint64_t addr;
  uint64_t len;
  uint32_t type; /* 1 = usable RAM, 2 = reserved, 3 = ACPI reclaimable,
                    4 = ACPI NVS, 5 = bad */
} multiboot_mmap_entry_t;

multiboot_info_t *map_multiboot1(uint32_t magic, uint32_t mbi_phys);

#endif
