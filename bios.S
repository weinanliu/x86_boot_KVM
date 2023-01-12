#include "mmu.h"


.data

# page aligned
.p2align 12

.globl _start
_start:

.code16
cli
cld


xorw %ax, %ax
movw %ax, %ds
movw %ax, %es
movw %ax, %ss

lgdt gdtdesc
movl %cr0, %eax
orl $CR0_PE, %eax
movl %eax, %cr0

ljmp $GD_KT, $protcseg


.code32
protcseg:
  movw    $GD_KD, %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS
  movw    %ax, %ss                # -> SS: Stack Segment

jmp _next_kern_start


.p2align 2                                # force 4 byte alignment
gdt:
  SEG_NULL # null seg
  SEG(STA_X|STA_R, 0x0, 0xffffffff) # code seg
  SEG(STA_W, 0x0, 0xffffffff) # data seg

gdtdesc:
  .word   0x17                            # sizeof(gdt) - 1
  .long   gdt                             # address gdt

.p2align 12
_next_kern_start:
