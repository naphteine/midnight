# Declare constants used for creating a multiboot header
.set ALIGN,	1<<0		# Align loaded modules on page boundaries
.set MEMINFO,	1<<1		# Provide memory map
.set FLAGS,	ALIGN | MEMINFO	# This is the Multiboot 'flag' field
.set MAGIC,	0x1BADB002	# 'magic number' lets bootloader find the header
.set CHECKSUM,	-(MAGIC + FLAGS)# Checksum of above, to prove we are multiboot

# Declare a header as in the Multiboot Standard. Blah blah blah.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Our own stack
.section .bootstrap_stack, "aw", @nobits
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

.section .text
.global _start
.type _start, @function
_start:
	movl $stack_top, %esp
	call kernel_main
	cli
	hlt

.Lhang:
	jmp .Lhang

.size _start, . - _start
