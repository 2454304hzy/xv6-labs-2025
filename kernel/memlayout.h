
// Physical memory layout

// qemu -machine virt is set up like this,
// based on qemu's hw/riscv/virt.c:
//
// 00001000 -- boot ROM, provided by qemu
// 02000000 -- CLINT
// 0C000000 -- PLIC
// 10000000 -- uart0 
// 10001000 -- virtio disk 
// 80000000 -- boot ROM jumps here in machine mode
//             -kernel loads the kernel here
// unused RAM after 80000000.

// the kernel uses physical memory thus:
// 80000000 -- kernel image, loaded and linked at this address
// 80000000 -- kernel image entry point, also the address the kernel is loaded at (actually 0x80000000, but qemu aligns up)
// 80000000 + KERNBASE -- actually the kernel starts at 0x80000000
// 80000000 + KERNBASE + PHYSTOP -- physical memory top, currently 128 MB

#define PHYSTOP (128 * 1024 * 1024)  // 128 MB

// kernel.ld sets this to the start of the kernel.
extern char kernel_start[];  // defined by kernel.ld
extern char kernel_end[];    // defined by kernel.ld

// qemu puts UART registers here in physical memory.
#define UART0 0x10000000L
#define UART0_IRQ 10

// virtio mmio interface
#define VIRTIO0 0x10001000
#define VIRTIO0_IRQ 1

// core local interruptor (CLINT), which contains the timer.
#define CLINT 0x2000000L
#define CLINT_MTIMECMP (CLINT + 0x4000)
#define CLINT_MTIME (CLINT + 0xBFF8)

// qemu puts platform-level interrupt controller (PLIC) here.
#define PLIC 0x0C000000L
#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart) * 0x100)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart) * 0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart) * 0x2000)

// the kernel expects there to be RAM
// for use by the kernel and user pages
// from physical address 0x80000000 to PHYSTOP.
#define KERNBASE 0x80000000L
#define TRAMPOLINE (MAXVA - PGSIZE)

// map kernel stacks beneath the trampoline,
// each surrounded by invalid guard pages.
#define KSTACK(p) (TRAMPOLINE - ((p) + 1) * (2 * PGSIZE))

// User memory layout.
// Address zero first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
//   ...
//   TRAPFRAME (p->trapframe, used by the trampoline)
//   TRAMPOLINE (the same page as in the kernel)
#define TRAPFRAME (TRAMPOLINE - PGSIZE)
#define USYSCALL (TRAPFRAME - PGSIZE)

// struct for USYSCALL fast system call optimization
struct usyscall {
  int pid;
};
