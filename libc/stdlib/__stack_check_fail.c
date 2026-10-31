#include <stdint.h>
#include <stdlib.h>

#if defined(__is_libk)
#include <kernel/panic.h>
#endif

#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x186f8ae00cbe8f09
#endif

uintptr_t __stack_chk_guard =
    STACK_CHK_GUARD; // i think maybe this should be in the boot sequence? maybe
                     // ...

__attribute__((noreturn)) void __stack_chk_fail(void) {
#if defined(__is_libk)
  kerror("Stack smashing detected");
#else
  abort();
#endif
}
