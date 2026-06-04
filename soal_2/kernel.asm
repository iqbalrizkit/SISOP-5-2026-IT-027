bits 16

global _start
global _putInMemory
global _getChar
extern _main

_start:

    cli

    mov ax, cs
    mov ds, ax
    mov es, ax

    sti

    call _main

.hang:
    jmp .hang


_putInMemory:
    push bp
    mov bp, sp

    push ds

    mov ax, [bp+4]
    mov si, [bp+6]
    mov cl, [bp+8]

    mov ds, ax
    mov [si], cl

    pop ds

    pop bp
    ret

_getChar:
    mov ah, 0x00        ; BIOS interrupt untuk membaca keyboard
    int 0x16            ; Panggil interupsi BIOS, hasil karakter masuk ke AL
    mov ah, 0x00        ; Bersihkan AH agar nilai kembalian murni AL
    ret
