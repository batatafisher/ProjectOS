#include <arch/i386/multiboot1.h>
#include <kernel/klib.h>
#include <stdio.h>

#define P2V(p) ((void *)((uint32_t)(p) + 0xC0000000))

multiboot_info_t *map_multiboot1(uint32_t magic, uint32_t mbi_phys) {
  multiboot_info_t *mbi = P2V(mbi_phys);
  if (magic != 0x2BADB002 ||
      !(mbi->flags & (1 << 6))) { /* no map: panic or use mem_upper */
  }

  uint8_t *p = P2V(mbi->mmap_addr), *end = p + mbi->mmap_length;
  while (p < end) {
    multiboot_mmap_entry_t *e = (void *)p;
    printf("%x-%x type %d\n", (uint32_t)e->addr, (uint32_t)(e->addr + e->len),
           (int)e->type);
    p += e->size + 4;
  }
  return mbi;
}
