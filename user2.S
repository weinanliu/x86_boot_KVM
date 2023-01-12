.data
.p2align 12
.globl page2_fun
page2_fun:
mov $'P', %eax
call putc
mov $'A', %eax
call putc
mov $'G', %eax
call putc
mov $'E', %eax
call putc
mov $'2', %eax
call putc
mov $'O', %eax
call putc
mov $'K', %eax
call putc
mov $'\n', %eax
call putc
ret


.p2align 12
page2_end:
