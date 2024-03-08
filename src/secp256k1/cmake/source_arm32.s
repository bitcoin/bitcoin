.syntax unified
.eabi_attribute 24, 1
.eabi_attribute 25, 1
.text
.global main
main:
	ldr	r0, =0x002A
	mov	r7, #1
	swi	0   
