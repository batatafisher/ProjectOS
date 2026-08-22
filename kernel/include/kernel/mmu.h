#ifndef _kernel_mmu_h
#define _kernel_mmu_h

#include <stddef.h>
#include <stdint.h>

extern uint32_t endkernel;

void *get_physaddr(void *virtualaddr);
void init_pmm();
void pmm_free_page(void *physaddr);
void *pmm_alloc_page();
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void unmap_page(void *virtualaddr);

#endif
