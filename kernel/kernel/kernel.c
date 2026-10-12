#include <drivers/init_drivers.h>
#include <input/keyboard.h>
#include <kernel/ext2.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/klib.h>
#include <kernel/mmu.h>
#include <kernel/pic.h>
#include <kernel/shell.h>
#include <kernel/tty.h>
#include <stdint.h>
#include <stdio.h>

char kernel_buffer[256] = {"thank u so much"};
char happy[10] = {"happy :D"};

void mmu_tests(void) {
  printf("\n--- STARTING MMU TESTS ---\n");

  // 1. Kernel Buffer Translation Test
  uint32_t phys_addr = (uint32_t)get_physaddr(kernel_buffer);
  printf("[1] Kernel Buffer Phys: 0x%x\n", phys_addr);

  // 2. Unmapped Memory Detection Test
  uint32_t unmapped_check = (uint32_t)get_physaddr((void *)0x39292929);
  if (unmapped_check == 0) {
    printf("[2] Unmapped detection passed! (0x39292929 is safely missing)\n");
  } else {
    printf("[2] Unmapped detection FAILED!\n");
  }

  // 3. Basic Mapping & Read/Write Test
  map_page((void *)0x00200000, (void *)0x00200000, 0x03);
  uint32_t map_check = (uint32_t)get_physaddr((void *)0x00200000);

  if (map_check != 0) {
    uint32_t *test_ptr = (uint32_t *)0x00200000;
    *test_ptr = 0xDEADBEEF;        // Write Test
    if (*test_ptr == 0xDEADBEEF) { // Read Test
      printf("[3] Basic map and Read/Write passed!\n");
    }
  } else {
    printf("[3] Basic map FAILED!\n");
  }

  // 4. Boundary & Dynamic Allocation Test
  void *fresh_frame = pmm_alloc_page();
  void *crazy_virtual = (void *)0x50000000;
  map_page(fresh_frame, crazy_virtual, 0x03);

  if ((uint32_t)get_physaddr(crazy_virtual) == (uint32_t)fresh_frame) {
    uint32_t *crazy_ptr = (uint32_t *)crazy_virtual;
    *crazy_ptr = 0xCAFEBABE; // Write Test on new table
    if (*crazy_ptr == 0xCAFEBABE) {
      printf("[4] Boundary mapping & deep write passed! (Virt 0x50000000 -> "
             "Phys 0x%x)\n",
             (uint32_t)fresh_frame);
    }
  } else {
    printf("[4] Boundary map FAILED!\n");
  }

  // 5. Collision & Safety Test
  void *collision_frame = pmm_alloc_page(); // Get a completely different frame
  map_page(collision_frame, crazy_virtual,
           0x03); // Try to map it to the same spot

  // Check if the original physical address is still the one sitting there
  if ((uint32_t)get_physaddr(crazy_virtual) == (uint32_t)fresh_frame) {
    printf("[5] Collision handled safely! (Original mapping protected)\n");
  } else {
    printf("[5] Collision FAILED! (Overwrote existing page)\n");
  }

  printf("--- MMU TESTS COMPLETE ---\n\n");
}

void kernel_main(void) {
  cli();
  init_gdt();
  init_PIC(0x20, 0x28);
  init_idt();
  init_pmm();
  init_tty();
  init_shell();
  init_drivers(1000);
  init_ext2();

  sti();

  printf("%s  %s \n", kernel_buffer, happy);

  // mmu_tests();
  test_disk();
  while (1) {

    run_shell();
    __asm__ volatile("hlt");
  }
}
