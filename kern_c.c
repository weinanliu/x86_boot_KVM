#include "mmu.h"

typedef uint32_t pte_t;
typedef uint32_t pde_t;

// Entry 0 of the page table maps to physical page 0, entry 1 to
// physical page 1, etc.
__attribute__((__aligned__(PGSIZE)))
pte_t kernel_ptes[NPTENTRIES] = {
        0x00000000 | PTE_P | PTE_W,
        0x00001000 | PTE_P | PTE_W,
        0x00002000 | PTE_P | PTE_W,
        0x00003000 | PTE_P | PTE_W,
        0x00004000 | PTE_P | PTE_W,
        0x00005000 | PTE_P | PTE_W,
        0x00006000 | PTE_P | PTE_W,
        0x00007000 | PTE_P | PTE_W,
        0x00008000 | PTE_P | PTE_W,
        0x00009000 | PTE_P | PTE_W,
        0x0000a000 | PTE_P | PTE_W,
        0x0000b000 | PTE_P | PTE_W,
        0x0000c000 | PTE_P | PTE_W,
        0x0000d000 | PTE_P | PTE_W,
        0x0000e000 | PTE_P | PTE_W,
        0x0000f000 | PTE_P | PTE_W,
};

__attribute__((__aligned__(PGSIZE)))
pte_t user_ptes[NPTENTRIES] = {{0}};

__attribute__((__aligned__(PGSIZE)))
pde_t entry_pgdir[NPDENTRIES] = {
        // Map VA's [0, 4MB) to PA's [0, 4MB)
        [0]
                = ((uintptr_t)kernel_ptes - KERNBASE) + PTE_P,
        // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
        [KERNBASE>>22]
                = ((uintptr_t)kernel_ptes - KERNBASE) + PTE_P + PTE_W
};


static struct Segdesc gdt[] =
{
  // 0x0 - unused (always faults -- for trapping NULL far pointers)
  SEG_NULL,

  // 0x8 - kernel code segment
  [GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

  // 0x10 - kernel data segment
  [GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

  // 0x18 - user code segment
  [GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

  // 0x20 - user data segment
  [GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),

  // Per-CPU TSS descriptors (starting from GD_TSS0) are initialized
  // in trap_init_percpu()
  [GD_TSS0 >> 3] = SEG_NULL
};

struct Pseudodesc gdt_pd = {
    sizeof(gdt) - 1, (unsigned long) gdt
};


static struct Gatedesc idt[T_ALL] = { { 0 } };
struct Pseudodesc idt_pd = {
    sizeof(idt) - 1, (unsigned long) idt
};


extern void __attribute__((regparm(1)))
    kern_putc (char c);

extern void __attribute__((regparm(1)))
    kern_hlt ();

    static void puts_1(const char *str) {
	if (str[0] != '\0') {
	    kern_putc(str[0]);
	    puts_1(str+1);
	}
    }

void puts(const char *str) {
    puts_1(str);
    kern_putc('\n');
}

static struct Taskstate cpu_ts;

static const char *trapname(int trapno)
{
  static const char * const excnames[] = {
      "Divide error",
      "Debug",
      "Non-Maskable Interrupt",
      "Breakpoint",
      "Overflow",
      "BOUND Range Exceeded",
      "Invalid Opcode",
      "Device Not Available",
      "Double Fault",
      "Coprocessor Segment Overrun",
      "Invalid TSS",
      "Segment Not Present",
      "Stack Fault",
      "General Protection",
      "Page Fault",
      "(unknown trap)",
      "x87 FPU Floating-Point Error",
      "Alignment Check",
      "Machine-Check",
      "SIMD Floating-Point Exception"
  };

  if (trapno < sizeof(excnames)/sizeof(excnames[0]))
    return excnames[trapno];
  if (trapno == T_SYSCALL_PUTC)
    return "System call putc";
  if (trapno == T_SYSCALL_HLT)
    return "System call hlt";
  return "(unknown trap)";
}

void
run(struct Trapframe *tf)
{
  asm volatile("\tmovl %0,%%esp\n"
	       "\tpopal\n"
	       "\tpopl %%es\n"
	       "\tpopl %%ds\n"
	       "\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
	       "\tiret\n"
	       : : "g" (tf) : "memory");
}

void
trap(struct Trapframe *tf)
{
  if (tf->tf_trapno == T_SYSCALL_PUTC) {
      kern_putc(tf->tf_regs.reg_eax);
  } else if (tf->tf_trapno == T_SYSCALL_HLT) {
      kern_hlt();
  } else {
      if ((tf->tf_cs & 3) == 3) {
	  // Trapped from user mode.
	  puts("trap from user mode occures!");
      } else
	puts ("trap occures!");
      puts(trapname(tf->tf_trapno));
      kern_hlt();
  }

  run(tf);
}

static inline void
invlpg(void *addr)
{
        asm volatile("invlpg (%0)" : : "r" (addr) : "memory");
}

void kern_main() {

    // Load the IDT
    asm volatile("lidt (%0)" : : "r" (&idt_pd));

    asm volatile("lgdt (%0)" : : "r" (&gdt_pd));
    // The kernel never uses GS or FS, so we leave those set to
    // the user data segment.
    asm volatile("movw %%ax,%%gs" : : "a" (GD_UD|3));
    asm volatile("movw %%ax,%%fs" : : "a" (GD_UD|3));
    // The kernel does use ES, DS, and SS.  We'll change between
    // the kernel and user data segments as needed.
    asm volatile("movw %%ax,%%es" : : "a" (GD_KD));
    asm volatile("movw %%ax,%%ds" : : "a" (GD_KD));
    asm volatile("movw %%ax,%%ss" : : "a" (GD_KD));
    // Load the kernel text segment into CS.
    asm volatile("ljmp %0,$1f\n 1:\n" : : "i" (GD_KT));
    // For good measure, clear the local descriptor table (LDT),
    // since we don't use it.
    asm volatile("lldt %%ax" : : "a" (0));

    extern void trap_DIVIDE();
    extern void trap_DEBUG();
    extern void trap_NMI();
    extern void trap_BRKPT();
    extern void trap_OFLOW();
    extern void trap_BOUND();
    extern void trap_ILLOP();
    extern void trap_DEVICE();
    extern void trap_DBLFLT();
    extern void trap_TSS();
    extern void trap_SEGNP();
    extern void trap_STACK();
    extern void trap_GPFLT();
    extern void trap_PGFLT();
    extern void trap_FPERR();
    extern void trap_ALIGN();
    extern void trap_MCHK();
    extern void trap_SIMDERR();
    extern void trap_SYSCALL_PUTC();
    extern void trap_SYSCALL_HLT();

    SETGATE (idt[T_DIVIDE], 0, GD_KT, trap_DIVIDE,  0)
    SETGATE (idt[T_DEBUG],  0, GD_KT, trap_DEBUG,   0)
    SETGATE (idt[T_NMI],    0, GD_KT, trap_NMI,     0)
    SETGATE (idt[T_BRKPT],  0, GD_KT, trap_BRKPT,   3)
    SETGATE (idt[T_OFLOW],  0, GD_KT, trap_OFLOW,   0)
    SETGATE (idt[T_BOUND],  0, GD_KT, trap_BOUND,   0)
    SETGATE (idt[T_ILLOP],  0, GD_KT, trap_ILLOP,   0)
    SETGATE (idt[T_DEVICE], 0, GD_KT, trap_DEVICE,  0)
    SETGATE (idt[T_DBLFLT], 0, GD_KT, trap_DBLFLT,  0)
    SETGATE (idt[T_TSS],    0, GD_KT, trap_TSS,     0)
    SETGATE (idt[T_SEGNP],  0, GD_KT, trap_SEGNP,   0)
    SETGATE (idt[T_STACK],  0, GD_KT, trap_STACK,   0)
    SETGATE (idt[T_GPFLT],  0, GD_KT, trap_GPFLT,   0)
    SETGATE (idt[T_PGFLT],  0, GD_KT, trap_PGFLT,   0)
    SETGATE (idt[T_FPERR],  0, GD_KT, trap_FPERR,   0)
    SETGATE (idt[T_ALIGN],  0, GD_KT, trap_ALIGN,   0)
    SETGATE (idt[T_MCHK],   0, GD_KT, trap_MCHK,    0)
    SETGATE (idt[T_SIMDERR],        0, GD_KT, trap_SIMDERR, 0)
    SETGATE (idt[T_SYSCALL_PUTC],        0, GD_KT, trap_SYSCALL_PUTC, 3)
    SETGATE (idt[T_SYSCALL_HLT],        0, GD_KT, trap_SYSCALL_HLT, 3)


    extern char kern_stack[];
    cpu_ts.ts_esp0 = kern_stack;
    cpu_ts.ts_ss0 = GD_KD;
    cpu_ts.ts_iomb = sizeof(cpu_ts);


    gdt[(GD_TSS0 >> 3)] = SEG16(STS_T32A, (uint32_t) &cpu_ts,
				sizeof(struct Taskstate) - 1, 0);
    gdt[(GD_TSS0 >> 3)].sd_s = 0;

    // Load the TSS selector (like other segment selectors, the
    // bottom three bits are special; we leave them 0)
    asm volatile("ltr %%ax" : : "a" (GD_TSS0));

#define ROUND_UP(n, v) ((n) - 1 + (v) - ((n) - 1) % (v))
    extern char kernel_end[];
    physaddr_t kernel_end_pa = ROUND_UP((uintptr_t)kernel_end - KERNBASE, 4096);
    int i;
    uintptr_t user_va =  0x00010000;
    int user_pageN = 0x010;
    for(i = 0; i < 5;i++) {
	user_ptes[(user_va >> 12) & 0x3ff] = kernel_end_pa | PTE_P | PTE_W | PTE_U;
        asm volatile("invlpg (%0)" : : "r" ((void*)user_va) : "memory");
	kernel_end_pa += 4096;
	user_va += 4096;
    }
    entry_pgdir[user_va >> 22] = ((uintptr_t)user_ptes - KERNBASE) | PTE_P | PTE_W | PTE_U;

    struct Trapframe user;
    user.tf_eip = 0x00010000;
    user.tf_cs = GD_UT | 3;
    user.tf_es = GD_UD | 3;
    user.tf_ds = GD_UD | 3;
    user.tf_ss = GD_UD | 3;
    user.tf_eflags = FL_IF;

    puts("Let's run user!!!!");
    run(&user);

    puts("OVER!!");
    kern_hlt();
}


