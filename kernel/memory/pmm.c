#include <arch/i386/arch.h>
#include <kernel/klib.h>
#include <kernel/mmu.h>
#include <kernel/panic.h>
#include <stdint.h>

static uint32_t bitmap_bytes;
static uint32_t total_pages;
static uint8_t *pmm_bitmap;
static uint32_t bitmap_end_phys;

void init_pmm(multiboot_info_t *multiboot) {
  if (!test_bit(6, (uint8_t *)&multiboot->flags))
    kerror("bad multiboot mmap entry");
  uint32_t mmap_length = multiboot->mmap_length;
  uint32_t mmap_addr = multiboot->mmap_addr;
  uint32_t start_addr = mmap_addr;
  uint64_t highest_addr = 0;

  while ((mmap_addr - start_addr) < mmap_length) {
    multiboot_mmap_entry_t *mmap_entry =
        ((void *)((uint32_t)(mmap_addr) + KERNEL_VMA));
    if (mmap_entry->type == 1 &&
        mmap_entry->addr + mmap_entry->len > highest_addr)
      highest_addr = mmap_entry->addr + mmap_entry->len;
    mmap_addr += mmap_entry->size + sizeof(uint32_t);
  }
  if (highest_addr > 0xffffffff)
    highest_addr = 0xffffffff;
  total_pages = highest_addr / PAGE_SIZE;
  bitmap_bytes = (total_pages + 7) / 8;
  uintptr_t aligned_addr = ((uintptr_t)endkernel + PAGE_SIZE - 1) &
                           ~(unsigned)(PAGE_SIZE - 1); // virtual
  pmm_bitmap = (uint8_t *)aligned_addr;
  uint32_t bitmap_start_phys = aligned_addr - KERNEL_VMA;
  bitmap_end_phys = bitmap_start_phys + bitmap_bytes;

  for (uint32_t i = 0; i < bitmap_bytes; i++) {
    pmm_bitmap[i] = 0xff;
  }

  mmap_addr = start_addr;
  while ((mmap_addr - start_addr) < mmap_length) {
    multiboot_mmap_entry_t *mmap_entry =
        ((void *)((uint32_t)(mmap_addr) + KERNEL_VMA));
    if (mmap_entry->type == 1) {
      uint64_t start = (mmap_entry->addr + PAGE_SIZE - 1) &
                       ~(uint64_t)(PAGE_SIZE - 1); // round start UP
      uint64_t end = (mmap_entry->addr + mmap_entry->len) &
                     ~(uint64_t)(PAGE_SIZE - 1); // round end DOWN
      uint64_t mb_start =
          (uint64_t)get_physaddr(multiboot) & ~(uint64_t)(PAGE_SIZE - 1);
      uint64_t mb_end =
          (uint64_t)get_physaddr(multiboot) + sizeof(multiboot_info_t);
      for (uint64_t i = start / PAGE_SIZE;
           i < end / PAGE_SIZE && i < total_pages; i++) {
        if (i * PAGE_SIZE < bitmap_end_phys)
          continue;

        else if (i * PAGE_SIZE >= mb_start && i * PAGE_SIZE < mb_end)
          continue;
        // TODO: later add whatever we want to reserve here, like modules ,
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
      if (!(pmm_bitmap[i] & (1 << j))) {            // page is free
        pmm_bitmap[i] |= (1 << j);                  // mark page as used
        return (void *)(((i * 8) + j) * PAGE_SIZE); // return physical address
      }
    }
  }
  return NULL; // no free pages
}

void pmm_free_page(void *physaddr) {
  if ((uint32_t)physaddr < bitmap_end_phys)
    kerror("Attempt to free a page that is allocated by the kernel");
  if ((uint32_t)physaddr % PAGE_SIZE != 0)
    kerror("Attempt to free a page that is not page-aligned");
  uint32_t page_index = (uint32_t)physaddr / PAGE_SIZE;
  if (page_index >= total_pages)
    kerror("Attempt to a free an out of bound page");
  if (!test_bit(page_index, pmm_bitmap))
    kerror("Attempt to free an already free page");
  clear_bit(page_index, pmm_bitmap);
}
