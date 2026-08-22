#include <kernel/mmu.h>

#define npages 1024 * 1024 // 4GB
static uint8_t pmm_bitmap[npages / 8];

void init_pmm() {
  // mark all pages as free
  for (int i = 0; i < npages / 8; i++) {
    pmm_bitmap[i] = 0;
  }

  for (int i = 0; i < ((uint32_t)&endkernel - 0xC0000000 + 0xfff) / 4096; i++) {
    pmm_bitmap[i / 8] |= (1 << (i % 8)); // mark pages used by kernel as used
  }
}

void *pmm_alloc_page() {
  for (int i = 0; i < npages / 8; i++) {
    if (pmm_bitmap[i] == 0xFF)
      continue;
    for (int j = 0; j < 8; j++) {
      if (!(pmm_bitmap[i] & (1 << j))) {       // page is free
        pmm_bitmap[i] |= (1 << j);             // mark page as used
        return (void *)(((i * 8) + j) * 4096); // return physical address
      }
    }
  }
  return NULL; // no free pages
}

void pmm_free_page(void *physaddr) {
  if ((uint32_t)physaddr <= (uint32_t)&endkernel - 0xC0000000)
    return; // kerror("Attempt to free a page that is allocated by the kernel");
  if ((uint32_t)physaddr % 4096 != 0)
    return; // kerror("Attempt to free a page that is not page-aligned");
  uint32_t page_index = (uint32_t)physaddr / 4096;
  pmm_bitmap[page_index / 8] &= ~(1 << (page_index % 8)); // mark page as free
}
