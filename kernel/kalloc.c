#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};

static struct kmem kmems[NCPU];

void
kinit()
{
  for (int i = 0; i < NCPU; i++) {
    char name[8];
    snprintf(name, sizeof(name), "kmem_%d", i);
    initlock(&kmems[i].lock, name);
    kmems[i].freelist = 0;
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    int cpu = cpuid();
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int cpu = cpuid();
  struct kmem *kmem = &kmems[cpu];
  acquire(&kmem->lock);
  r->next = kmem->freelist;
  kmem->freelist = r;
  release(&kmem->lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r = 0;
  
  push_off();
  int cpu = cpuid();
  struct kmem *kmem = &kmems[cpu];
  
  acquire(&kmem->lock);
  r = kmem->freelist;
  if (r) {
    kmem->freelist = r->next;
  }
  release(&kmem->lock);
  
  if (r == 0) {
    // steal from another CPU
    for (int i = 0; i < NCPU; i++) {
      if (i == cpu) continue;
      struct kmem *other = &kmems[i];
      acquire(&other->lock);
      if (other->freelist) {
        // steal 16 pages at a time
        int steal_count = 16;
        struct run *steal_head = other->freelist;
        struct run *steal_tail = steal_head;
        while (steal_tail->next && steal_count > 1) {
          steal_tail = steal_tail->next;
          steal_count--;
        }
        other->freelist = steal_tail->next;
        steal_tail->next = 0;
        release(&other->lock);
        
        acquire(&kmem->lock);
        kmem->freelist = steal_head;
        release(&kmem->lock);
        
        // now allocate from our new list
        acquire(&kmem->lock);
        r = kmem->freelist;
        if (r) {
          kmem->freelist = r->next;
        }
        release(&kmem->lock);
        break;
      }
      release(&other->lock);
    }
  }
  
  pop_off();
  
  if (r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
