#ifndef _kernel_MMU_H
#define _kernel_MMU_H

#include <stddef.h>

void *get_physaddr(void *virtualaddr);
void map_page(void *physaddr, void *virtualaddr, unsigned int flags);

#endif
