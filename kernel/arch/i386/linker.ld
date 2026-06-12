/* The bootloader will look at this image and start execution at the symbol
   designated at the entry point. */
ENTRY(_start)

/* Tell where the various sections of the object files will be put in the final
   kernel image. */
SECTIONS
{
    /* The kernel will live at 3GB + 1MB in the virtual address space (VMA),
       but it will be physically loaded at 1MB (LMA) by the bootloader. */
    . = 0xC0100000;

    /* First put the multiboot header, as it is required to be put very early
       in the image or the bootloader won't recognize the file format.
       We use AT() to calculate the physical Load Memory Address. */
    .text ALIGN(4K) : AT(ADDR(.text) - 0xC0000000)
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data. */
    .rodata ALIGN(4K) : AT(ADDR(.rodata) - 0xC0000000)
    {
        *(.rodata)
    }

    /* Read-write data (initialized) */
    .data ALIGN(4K) : AT(ADDR(.data) - 0xC0000000)
    {
        *(.data)
    }

    /* Read-write data (uninitialized) and stack */
    .bss ALIGN(4K) : AT(ADDR(.bss) - 0xC0000000)
    {
        *(COMMON)
        *(.bss)
    }

    /* The compiler may produce other sections, put them in the proper place in
       in this file, if you'd like to include them in the final kernel. */
}
