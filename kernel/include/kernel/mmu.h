#ifndef _kernel_mmu_h
#define _kernel_mmu_h

#include <arch/i386/multiboot1.h>
#include <stddef.h>
#include <stdint.h>

extern char endkernel[];

void *get_physaddr(void *virtualaddr);
void init_pmm(multiboot_info_t *multiboot);
void pmm_free_page(void *physaddr);
void *pmm_alloc_page();
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void unmap_page(void *virtualaddr);

#endif
