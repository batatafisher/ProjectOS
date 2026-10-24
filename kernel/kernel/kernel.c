#include <arch/i386/multiboot1.h>
#include <drivers/init_drivers.h>
#include <fs/ext2.h>
#include <input/keyboard.h>
#include <kernel/asmutils.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mmu.h>
#include <kernel/pic.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stdint.h>
#include <stdio.h>

char kernel_buffer[256] = {"thank u so much"};
char happy[10] = {"happy :D"};

void kernel_main(uint32_t magic, uint32_t mbi_phys) {
  cli();
  init_gdt();
  init_PIC(0x20, 0x28);
  init_idt();

  init_pmm();
  init_tty();
  init_shell();
  init_drivers(1000);
  init_fs();

  sti();
  multiboot_info_t *mbi = map_multiboot1(magic, mbi_phys);

  printf("%s  %s \n", kernel_buffer, happy);

  while (1) {

    run_shell();
    __asm__ volatile("hlt");
  }
}
