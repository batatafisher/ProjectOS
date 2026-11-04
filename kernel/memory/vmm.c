#include "arch/i386/arch.h"
#include <kernel/asmutils.h>
#include <kernel/mmu.h>
#include <kernel/panic.h>
#include <stdint.h>

extern char text_start[], text_end[], rodata_start[], rodata_end[];

static void ro_protect_range(unsigned long a, unsigned long b) {
  for (a &= ~(unsigned long)(PAGE_SIZE - 1); a < b; a += PAGE_SIZE) {
    unsigned long *pt = PT_VADDR(a >> 22);
    pt[(a >> 12) & RECURSIVE_SLOT] &= ~0x2; // Clear the writable bit
    flush_tlb_single(a);
  }
}

void init_vmm() {
  ro_protect_range((unsigned long)text_start, (unsigned long)text_end);
  ro_protect_range((unsigned long)rodata_start, (unsigned long)rodata_end);
  unsigned long cr0;
  __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
  __asm__ volatile("mov %0, %%cr0" : : "r"(cr0 | 0x10000)); // Enable WP bit
}

void *get_physaddr(void *virtualaddr) {
  unsigned long pdindex = (unsigned long)virtualaddr >> 22;
  unsigned long ptindex = (unsigned long)virtualaddr >> 12 & RECURSIVE_SLOT;

  unsigned long *pd = (unsigned long *)PD_VMA;
  if (!(pd[pdindex] & 0x1)) {
    return NULL; // Page directory entry not present
  }

  unsigned long *pt = PT_VADDR(pdindex);
  if (!(pt[ptindex] & 0x1)) {
    return NULL; // page table entry not present
  }
  return (void *)((pt[ptindex] & ~(PAGE_SIZE - 1)) +
                  ((unsigned long)virtualaddr & (PAGE_SIZE - 1)));
}

void map_page(void *physaddr, void *virtualaddr, unsigned int flags) {
  // Make sure that both addresses are page-aligned.

  unsigned long pdindex = (unsigned long)virtualaddr >> 22;
  unsigned long ptindex = (unsigned long)virtualaddr >> 12 & RECURSIVE_SLOT;

  unsigned long *pd = (unsigned long *)PD_VMA;

  if (!(pd[pdindex] & 0x01)) {
    uint64_t *pmm_page = pmm_alloc_page();
    if (pmm_page == NULL)
      kerror("out of memory!!");
    pd[pdindex] =
        ((unsigned long)pmm_page) | (flags & (PAGE_SIZE - 1)) | 0x01; // Present
    unsigned long *pt = PT_VADDR(pdindex);
    for (int i = 0; i < 1024; i++) {
      pt[i] = 0; // Initialize the new page table entry to 0
    }
  }

  unsigned long *pt = PT_VADDR(pdindex);

  if (pt[ptindex] & 0x01)
    return; // return if it already mapped for now

  if (physaddr == NULL)
    kerror("tried to map a NULL physical address");

  pt[ptindex] =
      ((unsigned long)physaddr) | (flags & (PAGE_SIZE - 1)) | 0x01; // Present

  asm volatile("invlpg (%0)" ::"r"(virtualaddr) : "memory");
}

void unmap_page(void *virtualaddr) {
  unsigned long pdindex = (unsigned long)virtualaddr >> 22;
  unsigned long ptindex = (unsigned long)virtualaddr >> 12 & RECURSIVE_SLOT;

  unsigned long *pd = (unsigned long *)PD_VMA;
  if (!(pd[pdindex] & 0x1)) {
    return; // Page directory entry not present
  }

  unsigned long *pt = PT_VADDR(pdindex);
  if (!(pt[ptindex] & 0x1)) {
    return; // page table entry not present
  }

  pt[ptindex] = 0; // Clear the page table entry

  asm volatile("invlpg (%0)" ::"r"(virtualaddr) : "memory");
}
