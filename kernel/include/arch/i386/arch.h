#ifndef _ARCH_I386_ARCH_H
#define _ARCH_I386_ARCH_H

#define KERNEL_VMA 0xC0000000
#define KHEAP_START 0xD0000000
#define PAGE_SIZE 4096
#define RECURSIVE_SLOT 1023
#define PT_BASE_VMA ((unsigned long)RECURSIVE_SLOT << 22)
#define PD_VMA (PT_BASE_VMA + ((unsigned long)RECURSIVE_SLOT << 12))
#define PT_VADDR(pdindex) ((unsigned long *)(PT_BASE_VMA + ((pdindex) << 12)))

#endif
