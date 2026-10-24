#include <kernel/klib.h>
#include <kernel/mmu.h>
#include <stdint.h>

static uint32_t bitmap_bytes;
static uint32_t total_pages;
static uint8_t *pmm_bitmap;

void init_pmm(multiboot_info_t *multiboot) {
  if (!test_bit(6, (uint8_t *)&multiboot->flags))
    return; // kerror("bad multiboot mmap entry");
  uint32_t mmap_length = multiboot->mmap_length;
  uint32_t mmap_addr = multiboot->mmap_addr;
  uint32_t start_addr = mmap_addr;
  uint64_t highest_addr = 0;

  while ((mmap_addr - start_addr) < mmap_length) {
    multiboot_mmap_entry_t *mmap_entry =
        ((void *)((uint32_t)(mmap_addr) + 0xC0000000));
    if (mmap_entry->type == 1 &&
        mmap_entry->addr + mmap_entry->len > highest_addr)
      highest_addr = mmap_entry->addr + mmap_entry->len;
    mmap_addr += mmap_entry->size + sizeof(uint32_t);
  }
  if (highest_addr > 0xffffffff)
    highest_addr = 0xffffffff;
  total_pages = highest_addr / 4096;
  bitmap_bytes = (total_pages + 7) / 8;
  uintptr_t aligned_addr = ((uintptr_t)endkernel + 4095) & ~4095U; // virtual
  pmm_bitmap = (uint8_t *)aligned_addr;
  uint32_t bitmap_start_phys = aligned_addr - 0xC0000000;
  uint32_t bitmap_end_phys = bitmap_start_phys + bitmap_bytes;

  for (uint32_t i = 0; i < bitmap_bytes; i++) {
    pmm_bitmap[i] = 0xff;
  }

  mmap_addr = start_addr;
  while ((mmap_addr - start_addr) < mmap_length) {
    multiboot_mmap_entry_t *mmap_entry =
        ((void *)((uint32_t)(mmap_addr) + 0xC0000000));
    if (mmap_entry->type == 1) {
      uint64_t start = (mmap_entry->addr + 4095) & ~4095ULL; // round start UP
      uint64_t end =
          (mmap_entry->addr + mmap_entry->len) & ~4095ULL; // round end DOWN
      uint64_t mb_start = (uint64_t)get_physaddr(multiboot) & ~4095ULL;
      uint64_t mb_end =
          (uint64_t)get_physaddr(multiboot) + sizeof(multiboot_info_t);
      for (uint64_t i = start / 4096; i < end / 4096 && i < total_pages; i++) {
        if (i * 4096 < bitmap_end_phys)
          continue;

        else if (i * 4096 >= mb_start && i * 4096 < mb_end)
          continue;
        // TODO: later add whatever we want to reserve her like modules ,
        // cmdline etc
        clear_bit(i, pmm_bitmap);
      }
    }

    mmap_addr += mmap_entry->size + sizeof(uint32_t);
  }
}

void *pmm_alloc_page() {
  for (uint32_t i = 0; i < bitmap_bytes; i++) {
    if (pmm_bitmap[i] == 0xFF)
      continue;
    for (uint32_t j = 0; j < 8; j++) {
      if (!(pmm_bitmap[i] & (1 << j))) {       // page is free
        pmm_bitmap[i] |= (1 << j);             // mark page as used
        return (void *)(((i * 8) + j) * 4096); // return physical address
      }
    }
  }
  return NULL; // no free pages
}

void pmm_free_page(void *physaddr) {
  if ((uint32_t)physaddr < (uint32_t)&endkernel - 0xC0000000)
    return; // kerror("Attempt to free a page that is allocated by the kernel");
  if ((uint32_t)physaddr % 4096 != 0)
    return; // kerror("Attempt to free a page that is not page-aligned");
  uint32_t page_index = (uint32_t)physaddr / 4096;
  if (page_index >= total_pages)
    return; // kerror("Attempt to a free an out of bound page");
  clear_bit(page_index, pmm_bitmap);
}
