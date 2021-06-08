ORG 0
BITS 16

_start:
    jmp short start
    nop

times 33 db 0

start:
    jmp 0x7C0:next

zero_interrupt:
    mov ah, 0eh
    mov al, '0'
    mov bx, 0x00
    int 0x10
    iret

one_interrupt:
    mov ah, 0eh
    mov al, '1'
    mov bx, 0x00
    int 0x10
    iret

next:
    cli
    mov ax, 0x7C0
    mov ds, ax
    mov es, ax

    mov ax, 0x00
    mov ss, ax
    mov sp, 0x7C00

    sti

    mov word[ss:0x00], zero_interrupt
    mov word[ss:0x02], 0x7C0

    int 0

    mov word[ss:0x04], one_interrupt
    mov word[ss:0x06], 0x7C0

    int 1

    mov si, message
    call print
    jmp $ 

print:
    mov bx, 0
.loop:
    lodsb

    cmp al, 0
    je .done

    call printc
    jmp .loop

.done:
    ret

printc:
    mov ah, 0eh
    int 0x10
    ret

message:
    db 'Hello World', 0

times 510-($-$$) db 0
dw 0xAA55