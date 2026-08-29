#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern uint64 sstvec();

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set the kernel's trap vector to kernelvec
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    if(p->killed)
      exit(-1);
    p->trapframe->epc += 4;
    intr_on();
    syscall();
  } else if(r_scause() == 15 || r_scause() == 13) {
    uint64 va = r_stval();
    
    struct vma *vma = 0;
    for(int i = 0; i < 16; i++) {
      if(p->vmas[i] && p->vmas[i]->valid &&
         va >= p->vmas[i]->addr && va < p->vmas[i]->addr + p->vmas[i]->len) {
        vma = p->vmas[i];
        break;
      }
    }
    
    if(vma) {
      uint64 offset = va - vma->addr;
      uint64 pa = (uint64)kalloc();
      if(pa == 0) {
        p->killed = 1;
      } else {
        memset((void*)pa, 0, PGSIZE);
        struct inode *ip = vma->f->ip;
        ilock(ip);
        readi(ip, 1, pa, vma->offset + offset, PGSIZE);
        iunlock(ip);
        
        int perm = PTE_U | PTE_V;
        if(vma->prot & PROT_READ) perm |= PTE_R;
        if(vma->prot & PROT_WRITE) perm |= PTE_W;
        if(vma->prot & PROT_EXEC) perm |= PTE_X;
        
        if(mappages(p->pagetable, PGROUNDDOWN(va), PGSIZE, pa, perm) != 0) {
          kfree((void*)pa);
          p->killed = 1;
        }
      }
    } else {
      p->killed = 1;
    }
  } else if((which_dev = devintr()) != 0){
    if(which_dev == 2) {
      if(p->alarm_interval > 0 && p->alarm_handling == 0) {
        p->alarm_ticks--;
        if(p->alarm_ticks == 0) {
          p->alarm_ticks = p->alarm_interval;
          p->alarm_handling = 1;
          p->alarm_trapframe = *(p->trapframe);
          p->trapframe->epc = (uint64)p->alarm_handler;
        }
      }
      yield();
    }
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    p->killed = 1;
  }

  if(p->killed)
    exit(-1);

  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  intr_off();

  w_stvec(TRAMPOLINE + (uservec - trampoline));

  p->trapframe->kernel_satp = r_satp();
  p->trapframe->kernel_sp = p->kstack + PGSIZE;
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();

  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP;
  x |= SSTATUS_SPIE;
  w_sstatus(x);

  w_sepc(p->trapframe->epc);

  uint64 satp = MAKE_SATP(p->pagetable);

  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64, uint64))fn)(TRAPFRAME, satp);
}

void
kerneltrap()
{
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    if(irq)
      plic_complete(irq);

  } else if(scause == 0x8000000000000001L){
    if(cpuid() == 0){
      clockintr();
    }
    
    w_sip(r_sip() & ~2);

  } else {
    printf("kerneltrap: unexpected scause %p\n", scause);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  if(which_dev == 2)
    yield();

  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    if(cpuid() == 0){
      clockintr();
    }
    
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}
