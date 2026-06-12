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
.skip 16384 # 16 KiB
stack_top:

.align 4096
page_directory:
    .space 4096
table_768:
    .space 4096

# The kernel entry point.
.section .text
.global _start
.type _start, @function
_start:
	movl $(stack_top - OFFSET), %esp
  
  # initializing paging before calling the kernel
  movl $0, %eax #mov eax, 0
  movl $0, %ebx #mov ebx, 0

  1:
    movl %ebx, %ecx #mov ecx, ebx
    orl $3, %ecx #or ecx, 3
    
    # table_768[eax] = ecx
    movl %ecx, (table_768 - OFFSET)(,%eax,4)
    
    incl %eax #inc eax
    addl $4096, %ebx # add ebx, 4096
    cmpl $1024, %eax # cmp eax, 1024
    jne 1b

  movl $(table_768 - OFFSET), %eax
  orl $3, %eax
    
  # Slot 0
  movl %eax, (page_directory - OFFSET)
  # Slot 768 (768 * 4 = 3072)
  movl %eax, (page_directory - OFFSET) + 3072
    
  # make the last PDE point to the page directory and load page_directory into cr3
  movl $(page_directory - OFFSET), %eax
  movl %eax, %cr3
  orl $3, %eax # turn on last two bits which are for read/write permession, and present
  movl %eax, (page_directory - OFFSET) + 1023*4

  # Turn on Paging
  movl %cr0, %eax
  orl $0x80000001, %eax
  movl %eax, %cr0
  
  lea higher_half, %eax
  jmp *%eax

higher_half:
  movl $stack_top, %esp


  movl $0, page_directory + 0
  movl %cr3, %eax
  movl %eax, %cr3


	# Call the global constructors.
	call _init

	# Transfer control to the main kernel.
	call kernel_main

	# Hang if kernel_main unexpectedly returns.
	cli
5:	hlt
	jmp 5b
.size _start, . - _start
