#include "../commands.h"
#include <kernel/mmu.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void heap(char *args[], int argc) {
  if (argc >= 1) {
    if (argc == 1) {
      if (strcmp("info", args[0]) == 0) {
        size_t used_size = get_heap_size_u();
        int pages_used = get_pages_u();
        printf("used size = %d\n", used_size);
        printf("pages used = %d\n", pages_used);
        return;
      } else {
        printf("not supported yet\n");
        return;
      }
    }
  }
  return;
}
