.data
.p2align 12
_start:
movl $user_stack - 4, %esp
jmp main

.globl hlt
hlt:
int $0x31

.globl putc
putc:
int $0x30
ret

.p2align 12
user_stack:
