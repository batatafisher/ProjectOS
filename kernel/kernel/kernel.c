#include <stdio.h>

#include <kernel/mmu.h>
#include <kernel/tty.h>
#include <stdint.h>

char kernel_buffer[256] = {"thank u so much"};
char happy[10] = {"happy :D"};

void kernel_main(void) {
  terminal_initialize();
  printf("%s  %s \n", kernel_buffer, happy);
  uint32_t phys_addr = (uint32_t)get_physaddr(kernel_buffer);
  printf("Physical address of kernel_buffer: 0x%x\n", phys_addr);
  uint32_t addr = (uint32_t)get_physaddr((void *)0x39292929);
  if (addr == NULL) {
    printf("the address 0x39292929 isnt present\n\n");
  }
  map_page((void *)0x00200000, (void *)0x00200000, 0x03);
  uint32_t addr2 = (uint32_t)get_physaddr((void *)0x00200000);
  if (addr2 == NULL) {
    printf("map failed!!");
  } else {
    printf("map succeeded, yay!!");
  }
}
