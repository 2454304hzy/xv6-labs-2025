#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquire(struct spinlock *lk)
{
  push_off(); // disable interrupts to avoid deadlock.
  if(holding(lk))
    panic("acquire");

  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  //   a5 = 1
  //   s1 = &lk->locked
  //   amoswap.w.aq a5, a5, (s1)
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();

  // Record info about lock acquisition for holding() and debugging.
  lk->cpu = mycpu();
}

// Release the lock.
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  __sync_lock_release(&lk->locked);

  pop_off();
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s, and intr_off()/intr_on()
// cannot be nested.
void
push_off(void)
{
  int old = intr_get();

  intr_off();
  if(mycpu()->noff == 0)
    mycpu()->intena = old;
  mycpu()->noff += 1;
}

void
pop_off(void)
{
  struct cpu *c = mycpu();
  if(intr_get())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if(c->noff == 0 && c->intena)
    intr_on();
}


// Read-write spinlock implementation

void
initrwlock(struct rwspinlock *rw)
{
  initlock(&rw->lock, "rwlock");
  rw->readers = 0;
  rw->writers = 0;
  rw->write_pending = 0;
}

void
read_acquire(struct rwspinlock *rw)
{
  acquire(&rw->lock);
  while (rw->write_pending || rw->writers > 0) {
    sleep(&rw->readers, &rw->lock);
  }
  rw->readers++;
  release(&rw->lock);
}

void
read_release(struct rwspinlock *rw)
{
  acquire(&rw->lock);
  rw->readers--;
  if (rw->readers == 0) {
    wakeup(&rw->writers);
  }
  release(&rw->lock);
}

void
write_acquire(struct rwspinlock *rw)
{
  acquire(&rw->lock);
  rw->writers++;
  rw->write_pending = 1;
  while (rw->readers > 0) {
    sleep(&rw->writers, &rw->lock);
  }
  release(&rw->lock);
}

void
write_release(struct rwspinlock *rw)
{
  acquire(&rw->lock);
  rw->writers--;
  rw->write_pending = 0;
  wakeup(&rw->readers);
  release(&rw->lock);
}
