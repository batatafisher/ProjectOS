#include <kernel/mmu.h>

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
  // Here you need to check whether the PD entry is present.
  // When it is not present, you need to create a new empty PT and
  // adjust the PDE accordingly.
  if (!(pd[pdindex] & 0x01)) {
    return; // for now just return to avoid crash
  }

  unsigned long *pt = ((unsigned long *)0xFFC00000) + (0x400 * pdindex);

  if (pt[ptindex] & 0x01) {
    return; // return if it already mapped for now
  }

  pt[ptindex] = ((unsigned long)physaddr) | (flags & 0xFFF) | 0x01; // Present

  // Now you need to flush the entry in the TLB
  // or you might not notice the change.
}
