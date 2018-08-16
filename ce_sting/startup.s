	.globl	_main
	.globl	__pbase
	.globl  __pgmsize
	.globl	___main

| ------------------------------------------------------	
	.text
| ------------------------------------------------------
	move.l	-4(sp),d0				| address to basepage

| the following code should run when driver was run from TOS (not bootsector)

	move.l	4(sp),a5				| address to basepage
	move.l	0x0c(a5),d0				| length of text segment
	add.l	0x14(a5),d0				| length of data segment
	add.l	0x1c(a5),d0				| length of bss segment
	add.l	#0x1000+0x100,d0		| length of stack+basepage
	move.l  d0,__pgmsize			| store the prog size
	move.l	a5,d1					| address to basepage
	move.l	a5,__pbase				|
	add.l	d0,d1					| end of program
	and.b	#0xf0,d1				| align stack
	move.l	d1,sp					| new stackspace

	move.l	d0,-(sp)				| mshrink()
	move.l	a5,-(sp)				|
	clr.w	-(sp)					|
	move.w	#0x4a,-(sp)				|
	trap	#1						|
	lea.l	12(sp),sp				|

	jsr		_main

	clr.w	-(sp)
	trap	#1

___main:
	rts
	

| ------------------------------------------------------
	.bss
| ------------------------------------------------------	
__pbase:				.ds.l	1
__pgmsize:				.ds.l	1

