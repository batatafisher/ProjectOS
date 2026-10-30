#include <kernel/mmu.h>
#include <kernel/panic.h>
#include <stdint.h>

void *get_physaddr(void *virtualaddr) {
  unsigned long pdindex = (unsigned long)virtualaddr >> 22;
  unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

  unsigned long *pd = (unsigned long *)0xFFFFF000;
  if (!(pd[pdindex] & 0x1)) {
    return NULL; // Page directory entry not present
  }

  unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
  if (!(pt[ptindex] & 0x1)) {
    return NULL; // page table entry not present
  }
  return (void *)((pt[ptindex] & ~0xFFF) +
                  ((unsigned long)virtualaddr & 0xFFF));
}

void map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
  // Make sure that both addresses are page-aligned.

  unsigned long pdindex = (unsigned long)virtualaddr >> 22;
  unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

  unsigned long *pd = (unsigned long *)0xFFFFF000;

  if (!(pd[pdindex] & 0x01)) {
    uint64_t *pmm_page = pmm_alloc_page();
    if (pmm_page == NULL)
      kerror("out of memory!!");
    pd[pdindex] = ((unsigned long)pmm_page) | (flags & 0xFFF) | 0x01; // Present
    unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
    for (int i = 0; i < 1024; i++) {
      pt[i] = 0; // Initialize the new page table entry to 0
    }
  }

  unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);

  if (pt[ptindex] & 0x01) {
    return; // return if it already mapped for now
  }

  pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

  asm volatile("invlpg (%0)" ::"r"(virtualaddr) : "memory");
}

void unmap_page(void *virtualaddr) {
  unsigned long pdindex = (unsigned long)virtualaddr >> 22;
  unsigned long ptindex = (unsigned long)virtualaddr >> 12 & 0x03FF;

  unsigned long *pd = (unsigned long *)0xFFFFF000;
  if (!(pd[pdindex] & 0x1)) {
    return; // Page directory entry not present
  }

  unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);
  if (!(pt[ptindex] & 0x1)) {
    return; // page table entry not present
  }

  pt[ptindex] = 0; // Clear the page table entry

  asm volatile("invlpg (%0)" ::"r"(virtualaddr) : "memory");
}
