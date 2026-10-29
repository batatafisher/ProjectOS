#include <arch/i386/multiboot1.h>
#include <drivers/init_drivers.h>
#include <fs/ext2.h>
#include <input/keyboard.h>
#include <kernel/asmutils.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/mmu.h>
#include <kernel/panic.h>
#include <kernel/pic.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stdint.h>

void kernel_main(uint32_t magic, uint32_t mbi_phys) {
  cli();

  init_gdt();
  init_PIC(0x20, 0x28);
  init_idt();
  init_tty();
  multiboot_info_t *mbi = init_multiboot1(magic, mbi_phys);
  if (mbi == NULL) {
    kerror("bad multiboot info");
  }

  init_pmm(mbi);
  init_heap();

  init_shell();

  init_drivers(1000);
  init_fs();

  sti();

  while (1) {

    run_shell();
    hlt();
  }
}
