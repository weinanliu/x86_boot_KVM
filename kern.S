#include "mmu.h"

.data

# page aligned
.p2align 12

.globl kern_stack
kern_stack:

.globl _start
_start:

/* Here 32-bit address OK */

movl $(entry_pgdir - KERNBASE), %eax
movl %eax, %cr3
movl %cr0, %eax
orl $(CR0_PE|CR0_PG|CR0_WP), %eax
movl %eax, %cr0

/* Page table loaded. Here virtual address space */

mov $relocated, %eax
jmp *%eax
relocated:

/* Control flow is on virtual address space NOW */

movl $kern_stack - 4, %esp
jmp kern_main
hlt

.globl kern_putc
kern_putc:
  mov $0x3f8, %dx
out %al, (%dx)
  ret

  .globl kern_hlt
  kern_hlt:
  hlt
  ret

#define TRAPHANDLER(name, num)                                          \
    .globl name;            /* define global symbol for 'name' */   \
  .type name, @function;  /* symbol type is function */           \
  .align 2;               /* align function definition */         \
  name:                   /* function starts here */              \
  pushl $(num);                                                   \
  jmp _alltraps

#define TRAPHANDLER_NOEC(name, num)                                     \
    .globl name;                                                    \
  .type name, @function;                                          \
  .align 2;                                                       \
  name:                                                           \
  pushl $0;                                                       \
  pushl $(num);                                                   \
  jmp _alltraps

  TRAPHANDLER_NOEC(trap_DIVIDE, T_DIVIDE)
  TRAPHANDLER_NOEC(trap_DEBUG, T_DEBUG)
  TRAPHANDLER_NOEC(trap_NMI, T_NMI)
  TRAPHANDLER_NOEC(trap_BRKPT, T_BRKPT)
  TRAPHANDLER_NOEC(trap_OFLOW, T_OFLOW)
  TRAPHANDLER_NOEC(trap_BOUND, T_BOUND)
  TRAPHANDLER_NOEC(trap_ILLOP, T_ILLOP)
  TRAPHANDLER_NOEC(trap_DEVICE, T_DEVICE)
TRAPHANDLER_NOEC(trap_DBLFLT, T_DBLFLT)
  //reserved for T_COPROC 9
  TRAPHANDLER(trap_TSS, T_TSS)
  TRAPHANDLER(trap_SEGNP, T_SEGNP)
  TRAPHANDLER(trap_STACK, T_STACK)
  TRAPHANDLER(trap_GPFLT, T_GPFLT)
TRAPHANDLER(trap_PGFLT, T_PGFLT)
  //reserved for T_RES 15
  TRAPHANDLER_NOEC(trap_FPERR, T_FPERR)
  TRAPHANDLER_NOEC(trap_ALIGN, T_ALIGN)
  TRAPHANDLER_NOEC(trap_MCHK, T_MCHK)
TRAPHANDLER_NOEC(trap_SIMDERR, T_SIMDERR)

  TRAPHANDLER_NOEC(trap_SYSCALL_PUTC, T_SYSCALL_PUTC)
TRAPHANDLER_NOEC(trap_SYSCALL_HLT, T_SYSCALL_HLT)

TRAPHANDLER_NOEC(trap_unknown, 0xffffffff)

  .globl _alltraps
  .type name, @function
  .align 2
  _alltraps:
  pushw $0
  pushw %ds
  pushw $0
  pushw %es
  pusha
  push %esp
  call trap

