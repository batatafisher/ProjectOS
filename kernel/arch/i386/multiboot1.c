#include <arch/i386/multiboot1.h>
#include <kernel/klib.h>
#include <stddef.h>
#include <stdint.h>

#define kernel_P2V(p) ((void *)((uint32_t)(p) + 0xC0000000))

multiboot_info_t *init_multiboot1(uint32_t magic, uint32_t mbi_phys) {
  multiboot_info_t *mbi = kernel_P2V(mbi_phys);
  if (magic != 0x2BADB002)
    return NULL;
  return mbi;
}
