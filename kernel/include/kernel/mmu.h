#ifndef _kernel_mmu_h
#define _kernel_mmu_h

#include <arch/i386/multiboot1.h>
#include <stddef.h>
#include <stdint.h>

#define ALIGNMENT 8

extern char endkernel[];

typedef struct __attribute__((aligned(ALIGNMENT)))
heap_block {   // describes the availebility of each block in a
               // page and its size
  int type;    // 1 = used ; 0 = free
  size_t size; // size of the block
  struct heap_block *next;
} heap_block_t;

void *get_physaddr(void *virtualaddr);
void init_pmm(multiboot_info_t *multiboot);
void pmm_free_page(void *physaddr);
void *pmm_alloc_page();
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);
void unmap_page(void *virtualaddr);

void init_heap(void);
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t size);

size_t get_heap_size_u();
int get_pages_u();

#endif
