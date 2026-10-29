#include <kernel/klib.h>
#include <kernel/mmu.h>
#include <kernel/panic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define KHEAP_START 0xD0000000
#define PAGE_SIZE 4096
#define PAGE_FLAGS 0x2 // the vmm automatically adds the 0x1 (present) flag

static int current_heap_pages;
static size_t used_size;

void *allocate_page() {
  void *phys_addr = pmm_alloc_page();

  if (phys_addr == NULL)
    return NULL;

  void *virt_addr = (void *)(KHEAP_START + current_heap_pages * PAGE_SIZE);

  map_page(phys_addr, virt_addr, PAGE_FLAGS);

  current_heap_pages++;
  return virt_addr;
}

void init_heap(void) {
  current_heap_pages = 0;
  used_size = 0;
  void *addr = allocate_page();

  if (addr == NULL)
    kerror("page allocation returned NULL in function : init_heap"); // out of
                                                                     // memory
                                                                     // (maybe)

  heap_block_t *heap = (heap_block_t *)addr;

  heap->size = PAGE_SIZE - sizeof(heap_block_t);
  heap->type = 0; // free
  heap->next = NULL;
}

void merge_free_blocks(heap_block_t *block1, heap_block_t *block2) {
  block1->size += block2->size + sizeof(heap_block_t);
  block1->next = block2->next;
}

void *kmalloc(size_t heap_size) {
  if (heap_size < 1)
    return NULL;
  size_t size = round2(ALIGNMENT, heap_size);
  uint64_t *addr = (uint64_t *)KHEAP_START;
  heap_block_t *heap_block = (heap_block_t *)addr;
  heap_block_t *previous_block = NULL;

  while (1) {

    if (heap_block->type == 0) {      // free
      if (heap_block->size >= size) { // found a block!
        if (heap_block->size >= size + sizeof(heap_block_t) +
                                    ALIGNMENT) { // has enough space to slice it
          uint64_t address =
              (uint64_t)((uint8_t *)heap_block + sizeof(heap_block_t) +
                         size); // NOTE: heap_block_t is aligned to 8 bytes so
                                // this whole address is also aligned to 8 bytes
          heap_block_t *new_heap_block = (heap_block_t *)address;
          new_heap_block->size = heap_block->size - size - sizeof(heap_block_t);
          new_heap_block->type = 0;
          new_heap_block->next = heap_block->next;
          heap_block->next = new_heap_block;
          heap_block->size = size;
          heap_block->type = 1;
          used_size += size;
          return (void *)(heap_block + 1);
        }
        heap_block->type = 1;
        used_size += size;
        return (void *)(heap_block + 1);
      } else {
        if (previous_block !=
            NULL) { // here both previous and current block were free but dont
                    // have enough space individually to hold the data needed so
                    // we merge them
          merge_free_blocks(previous_block, heap_block);
          heap_block = previous_block;
          previous_block = NULL;
          continue;
        }
        if (heap_block->next == NULL) {
          void *new_page = allocate_page();
          if (new_page == NULL)
            return NULL;
          heap_block->size += PAGE_SIZE;
          continue;
        }

        previous_block = heap_block;
        heap_block = heap_block->next;
      }
    } else if (heap_block->type == 1) {
      previous_block = NULL;
      if (heap_block->next == NULL) {
        void *new_page = allocate_page();
        if (new_page == NULL)
          return NULL;
        heap_block_t *new_heap_block = (heap_block_t *)new_page;
        heap_block->next = new_heap_block;
        new_heap_block->size = PAGE_SIZE - sizeof(heap_block_t);
        new_heap_block->type = 0;
        new_heap_block->next = NULL;
        heap_block = new_heap_block;
        continue;
      }
      heap_block = heap_block->next;
      continue;
    } else
      kerror("unexpected type of heap block of %d",
             heap_block->type); // how tf are we even here
  }
}

void *kcalloc(size_t count, size_t size) {
  void *addr = kmalloc(count * size);
  if (addr == NULL)
    return addr;
  memset(addr, 0, count * size);
  return addr;
}

void kfree(void *ptr) {
  if (ptr == NULL)
    return; // i understand the appeal now

  heap_block_t *heap_block = (heap_block_t *)ptr;
  heap_block = heap_block - 1;

  if (heap_block->type == 0)
    kerror("tried to free an already not used heap block");

  heap_block->type = 0;
  used_size -= heap_block->size;

  heap_block_t *prev_heap_block = heap_block;
  heap_block = heap_block->next;

  // NOTE: since heap_block_t dont have access to a prevous block its only
  // possible to merge blocks after it
  // TODO: maybe add a new variable in the heap_block_t struct to point to the
  // previous block header to fix this issue

  while (heap_block != NULL && heap_block->type == 0) {
    merge_free_blocks(prev_heap_block, heap_block);
    heap_block = heap_block->next;
  }

  return;
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr)
    return kmalloc(size);

  heap_block_t *heap_block = (heap_block_t *)ptr - 1;
  if (size <= heap_block->size) {
    return ptr;
  }

  void *new_ptr = kmalloc(size);

  if (new_ptr == NULL) {
    return NULL;
  }

  if (size > heap_block->size)
    memcpy(new_ptr, ptr, heap_block->size);

  kfree(ptr);

  return new_ptr;
}

size_t get_heap_size_u() { return used_size; }
int get_pages_u() { return current_heap_pages; }
