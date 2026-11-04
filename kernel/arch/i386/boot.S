# constanrs for memory managment
.set OFFSET, 0xC0000000 # the offset of the higher half of the kernel

# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Reserve a stack for the initial thread.
.section .bss
.align 16
stack_bottom:
.skip 65536 # 64 KiB
stack_top:

# And the page tables in the data section 
.section .data
.align 4096
page_directory:
    .space 4096
page_tables:
    .space 16384 # 4 contiguous page tables (4 * 4096) for 16MB

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
  movl $(stack_top - OFFSET), %esp
  movl %eax, %edi
  movl %ebx, %esi
  
  # Map 16MB (4096 pages) into our page tables
  movl $0, %eax # eax = page index (0 to 4095)
  movl $0, %ebx # ebx = physical address

  1:
    movl %ebx, %ecx
    orl $3, %ecx
    movl %ecx, (page_tables - OFFSET)(,%eax,4)
    incl %eax
    addl $4096, %ebx
    cmpl $4096, %eax # 4096 * 4KB = 16MB
    jne 1b

  # Link all 4 tables into the Page Directory
  movl $0, %eax # eax = table index (0 to 3)
  
  2:
    # ecx = page_tables_phys + (eax * 4096) | Present | Read/Write
    movl %eax, %ecx
    imull $4096, %ecx
    addl $(page_tables - OFFSET), %ecx
    orl $3, %ecx

    # Slot index offset = 768 * 4 = 3072
    movl %ecx, (page_directory - OFFSET + 3072)(,%eax,4)

    # Identity Map ONLY for the first 4 slots (0 to 3 -> lower 16MB)
    cmpl $4, %eax
    jge 3f
    movl %ecx, (page_directory - OFFSET)(,%eax,4)

  3:
    incl %eax
    cmpl $256, %eax
    jne 2b

    # Overwrite slot 1023 with recursive mapping
    movl $(page_directory - OFFSET), %eax
    orl $3, %eax
    movl %eax, (page_directory - OFFSET + 1023*4)

    # Load CR3 and enable paging
    movl $(page_directory - OFFSET), %eax
    movl %eax, %cr3
    
  # recursive mapping and cr3 load 
  movl $(page_directory - OFFSET), %eax
  orl $3, %eax 
  movl %eax, (page_directory - OFFSET) + 1023*4

  movl $(page_directory - OFFSET), %eax
  movl %eax, %cr3

  # Turn on Paging
  movl %cr0, %eax
  orl $0x80000001, %eax
  movl %eax, %cr0 

  lea higher_half, %eax
  jmp *%eax

higher_half:
    movl $stack_top, %esp

    # Preserve Multiboot magic (%edi) in %ebx before rep stosl overwrites %edi.
    movl %edi, %ebx

    # Zero out .bss section
    movl $sbss, %edi
    movl $ebss, %ecx
    subl %edi, %ecx
    shrl $2, %ecx

    xorl %eax, %eax
    cld
    rep stosl

    movl $0, page_directory + 0
    movl $0, page_directory + 4
    movl $0, page_directory + 8
    movl $0, page_directory + 12
    movl %cr3, %eax
    movl %eax, %cr3

    # Call global constructors
    call _init

    pushl %esi   # 2nd argument: Multiboot Info structure pointer
    pushl %ebx   # 1st argument: Multiboot Magic Number (0x2BADB002)

    # Transfer control to the main kernel
    call kernel_main

    cli
5:  hlt
    jmp 5b
