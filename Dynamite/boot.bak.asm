BITS 16
org 0x7c00

segments_clear:
	cli
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0x7c00
	mov bp, sp
	sti

start:
	mov ax, 0x13
	int 0x10

read_disk:
	mov ah, 0x42
	mov si, bootdap
	int 0x13

bootdap:
	db 0x10
	db 0
	dw (filled - image) / 512 + 1
	dw 0x000, 0xA000
	dq 1

boot_error:
	mov ah, 0x3e
	mov al, "X"
	int 0x10
	jmp stop

stop:
	hlt

times 512 - ($-$$) db 0
dw 0xaa55

image: incbin "dynamite.vad"
filled: times 512 - ($-$$) % 512 db 0