#ifndef MMU_H
#define MMU_H

#define GD_KT     0x08     // kernel text
#define GD_KD     0x10     // kernel data
#define GD_UT     0x18     // user text
#define GD_UD     0x20     // user data
#define GD_TSS0   0x28     // Task segment selector for CPU 0

#ifdef __ASSEMBLER__

/*
 * Macros to build GDT entries in assembly.
 */
#define SEG_NULL                                                \
        .word 0, 0;                                             \
        .byte 0, 0, 0, 0
#define SEG(type,base,lim)                                      \
        .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);      \
        .byte (((base) >> 16) & 0xff), (0x90 | (type)),         \
                (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#else   // not __ASSEMBLER__

typedef __signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;
typedef uint32_t physaddr_t;


struct Pseudodesc {
        unsigned short pd_lim;                // Limit
        unsigned pd_base;               // Base address
} __attribute__ ((packed));

// Segment Descriptors
struct Segdesc {
        unsigned sd_lim_15_0 : 16;  // Low bits of segment limit
        unsigned sd_base_15_0 : 16; // Low bits of segment base address
        unsigned sd_base_23_16 : 8; // Middle bits of segment base address
        unsigned sd_type : 4;       // Segment type (see STS_ constants)
        unsigned sd_s : 1;          // 0 = system, 1 = application
        unsigned sd_dpl : 2;        // Descriptor Privilege Level
        unsigned sd_p : 1;          // Present
        unsigned sd_lim_19_16 : 4;  // High bits of segment limit
        unsigned sd_avl : 1;        // Unused (available for software use)
        unsigned sd_rsv1 : 1;       // Reserved
        unsigned sd_db : 1;         // 0 = 16-bit segment, 1 = 32-bit segment
        unsigned sd_g : 1;          // Granularity: limit scaled by 4K when set
        unsigned sd_base_31_24 : 8; // High bits of segment base address
};
// Null segment
#define SEG_NULL        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
// Segment that is loadable but faults when used
#define SEG_FAULT       { 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0 }
// Normal segment
#define SEG(type, base, lim, dpl)                                       \
{ ((lim) >> 12) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,       \
    type, 1, dpl, 1, (unsigned) (lim) >> 28, 0, 0, 1, 1,                \
    (unsigned) (base) >> 24 }
#define SEG16(type, base, lim, dpl) (struct Segdesc)                    \
{ (lim) & 0xffff, (base) & 0xffff, ((base) >> 16) & 0xff,               \
    type, 1, dpl, 1, (unsigned) (lim) >> 16, 0, 0, 1, 0,                \
    (unsigned) (base) >> 24 }

struct Taskstate {
        uint32_t ts_link;       // Old ts selector
        uintptr_t ts_esp0;      // Stack pointers and segment selectors
        uint16_t ts_ss0;        //   after an increase in privilege level
        uint16_t ts_padding1;
        uintptr_t ts_esp1;
        uint16_t ts_ss1;
        uint16_t ts_padding2;
        uintptr_t ts_esp2;
        uint16_t ts_ss2;
        uint16_t ts_padding3;
        physaddr_t ts_cr3;      // Page directory base
        uintptr_t ts_eip;       // Saved state from last task switch
        uint32_t ts_eflags;
        uint32_t ts_eax;        // More saved state (registers)
        uint32_t ts_ecx;
        uint32_t ts_edx;
        uint32_t ts_ebx;
        uintptr_t ts_esp;
        uintptr_t ts_ebp;
        uint32_t ts_esi;
        uint32_t ts_edi;
        uint16_t ts_es;         // Even more saved state (segment selectors)
        uint16_t ts_padding4;
        uint16_t ts_cs;
        uint16_t ts_padding5;
        uint16_t ts_ss;
        uint16_t ts_padding6;
        uint16_t ts_ds;
        uint16_t ts_padding7;
        uint16_t ts_fs;
        uint16_t ts_padding8;
        uint16_t ts_gs;
        uint16_t ts_padding9;
        uint16_t ts_ldt;
        uint16_t ts_padding10;
        uint16_t ts_t;          // Trap on task switch
        uint16_t ts_iomb;       // I/O map base address
};

struct Gatedesc {
        unsigned gd_off_15_0 : 16;   // low 16 bits of offset in segment
        unsigned gd_sel : 16;        // segment selector
        unsigned gd_args : 5;        // # args, 0 for interrupt/trap gates
        unsigned gd_rsv1 : 3;        // reserved(should be zero I guess)
        unsigned gd_type : 4;        // type(STS_{TG,IG32,TG32})
        unsigned gd_s : 1;           // must be 0 (system)
        unsigned gd_dpl : 2;         // descriptor(meaning new) privilege level
        unsigned gd_p : 1;           // Present
        unsigned gd_off_31_16 : 16;  // high bits of offset in segment
};

#define SETGATE(gate, istrap, sel, off, dpl)                    \
{                                                               \
        (gate).gd_off_15_0 = (unsigned long) (off) & 0xffff;         \
        (gate).gd_sel = (sel);                                  \
        (gate).gd_args = 0;                                     \
        (gate).gd_rsv1 = 0;                                     \
        (gate).gd_type = (istrap) ? STS_TG32 : STS_IG32;        \
        (gate).gd_s = 0;                                        \
        (gate).gd_dpl = (dpl);                                  \
        (gate).gd_p = 1;                                        \
        (gate).gd_off_31_16 = (unsigned long) (off) >> 16;           \
}

struct PushRegs {
        /* registers as pushed by pusha */
        uint32_t reg_edi;
        uint32_t reg_esi;
        uint32_t reg_ebp;
        uint32_t reg_oesp;              /* Useless */
        uint32_t reg_ebx;
        uint32_t reg_edx;
        uint32_t reg_ecx;
        uint32_t reg_eax;
} __attribute__((packed));

struct Trapframe {
        struct PushRegs tf_regs;
        uint16_t tf_es;
        uint16_t tf_padding1;
        uint16_t tf_ds;
        uint16_t tf_padding2;
        uint32_t tf_trapno;
        /* below here defined by x86 hardware */
        uint32_t tf_err;
        uintptr_t tf_eip;
        uint16_t tf_cs;
        uint16_t tf_padding3;
        uint32_t tf_eflags;
        /* below here only when crossing rings, such as from user to kernel */
        uintptr_t tf_esp;
        uint16_t tf_ss;
        uint16_t tf_padding4;
} __attribute__((packed));



#endif /* !__ASSEMBLER__ */




#define NPTENTRIES 1024
#define NPDENTRIES 1024

// All physical memory mapped at this address
#define KERNBASE 0xF0000000

#define PGSIZE 4096

#define STA_X 0x8    // Executable segment
#define STA_E 0x4    // Expand down (non-executable segments)
#define STA_C 0x4    // Conforming code segment (executable only)
#define STA_W 0x2    // Writeable (non-executable segments)
#define STA_R 0x2    // Readable (executable segments)
#define STA_A 0x1    // Accessed

#define STS_T16A        0x1         // Available 16-bit TSS
#define STS_LDT         0x2         // Local Descriptor Table
#define STS_T16B        0x3         // Busy 16-bit TSS
#define STS_CG16        0x4         // 16-bit Call Gate
#define STS_TG          0x5         // Task Gate / Coum Transmitions
#define STS_IG16        0x6         // 16-bit Interrupt Gate
#define STS_TG16        0x7         // 16-bit Trap Gate
#define STS_T32A        0x9         // Available 32-bit TSS
#define STS_T32B        0xB         // Busy 32-bit TSS
#define STS_CG32        0xC         // 32-bit Call Gate
#define STS_IG32        0xE         // 32-bit Interrupt Gate
#define STS_TG32        0xF         // 32-bit Trap Gate

#define FL_IF		0x00000200	// Interrupt Flag

#define PTE_P 0x001 // Present
#define PTE_W 0x002 // Writeable
#define PTE_U 0x004 // User

#define CR0_PE 0x00000001 // Protection Enable
#define CR0_MP 0x00000002 // Monitor coProcessor
#define CR0_EM 0x00000004 // Emulation
#define CR0_TS 0x00000008 // Task Switched
#define CR0_ET 0x00000010 // Extension Type
#define CR0_NE 0x00000020 // Numeric Errror
#define CR0_WP 0x00010000 // Write Protect
#define CR0_AM 0x00040000 // Alignment Mask
#define CR0_NW 0x20000000 // Not Writethrough
#define CR0_CD 0x40000000 // Cache Disable
#define CR0_PG 0x80000000 // Paging




#define T_DIVIDE     0          // divide error
#define T_DEBUG      1          // debug exception
#define T_NMI        2          // non-maskable interrupt
#define T_BRKPT      3          // breakpoint
#define T_OFLOW      4          // overflow
#define T_BOUND      5          // bounds check
#define T_ILLOP      6          // illegal opcode
#define T_DEVICE     7          // device not available
#define T_DBLFLT     8          // double fault
/* #define T_COPROC  9 */       // reserved (not generated by recent processors)
#define T_TSS       10          // invalid task switch segment
#define T_SEGNP     11          // segment not present
#define T_STACK     12          // stack exception
#define T_GPFLT     13          // general protection fault
#define T_PGFLT     14          // page fault
/* #define T_RES    15 */       // reserved
#define T_FPERR     16          // floating point error
#define T_ALIGN     17          // aligment check
#define T_MCHK      18          // machine check
#define T_SIMDERR   19          // SIMD floating point error

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_SYSCALL_PUTC   48          // system call
#define T_SYSCALL_HLT   49          // system call
#define T_ALL   50         // catchall

#endif

