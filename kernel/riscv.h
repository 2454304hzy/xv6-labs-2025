#ifndef __ASSEMBLER__

// which hart (core) is this?
static inline uint64
r_mhartid()
{
  uint64 x;
  asm volatile("csrr %0, mhartid" : "=r" (x) );
  return x;
}

// Machine Status Register, mstatus

#define MSTATUS_MPP_MASK (3L << 11) // previous mode.
#define MSTATUS_MPP_M (3L << 11)
#define MSTATUS_MPP_S (1L << 11)
#define MSTATUS_MPP_U (0L << 11)
#define MSTATUS_MIE (1L << 3)    // machine-mode interrupt enable.
#define PTE_V (1L << 0) // valid
#define PTE_R (1L << 1) // read
#define PTE_W (1L << 2) // write
#define PTE_X (1L << 3) // execute
#define PTE_U (1L << 4) // user can access
#define PTE_COW (1L << 8) // copy-on-write flag

static inline uint64
r_mstatus()
{
  uint64 x;
  asm volatile("csrr %0, mstatus" : "=r" (x) );
  return x;
}

static inline void 
w_mstatus(uint64 x)
{
  asm volatile("csrw mstatus, %0" : : "r" (x));
}

// machine exception program counter, holds the
// instruction address to which a return from
// exception should go.
static inline void 
w_mepc(uint64 x)
{
  asm volatile("csrw mepc, %0" : : "r" (x));
}

// Supervisor Status Register, sstatus

#define SSTATUS_SPP (1L << 8)  // Previous mode, 1=Supervisor, 0=User
#define SSTATUS_SPIE (1L << 5) // Supervisor Previous Interrupt Enable
#define SSTATUS_UPIE (1L << 4) // User Previous Interrupt Enable
#define SSTATUS_SIE (1L << 1)  // Supervisor Interrupt Enable
#define SSTATUS_UIE (1L << 0)  // User Interrupt Enable

static inline uint64
r_sstatus()
{
  uint64 x;
  asm volatile("csrr %0, sstatus" : "=r" (x) );
  return x;
}

static inline void 
w_sstatus(uint64 x)
{
  asm volatile("csrw sstatus, %0" : : "r" (x));
}

// Supervisor Interrupt Pending
static inline uint64
r_sip()
{
  uint64 x;
  asm volatile("csrr %0, sip" : "=r" (x) );
  return x;
}

static inline void 
w_sip(uint64 x)
{
  asm volatile("csrw sip, %0" : : "r" (x));
}

// Supervisor Interrupt Enable
static inline uint64
r_sie()
{
  uint64 x;
  asm volatile("csrr %0, sie" : "=r" (x) );
  return x;
}

static inline void 
w_sie(uint64 x)
{
  asm volatile("csrw sie, %0" : : "r" (x));
}

// Supervisor Exception Program Counter
static inline uint64
r_sepc()
{
  uint64 x;
  asm volatile("csrr %0, sepc" : "=r" (x) );
  return x;
}

static inline void 
w_sepc(uint64 x)
{
  asm volatile("csrw sepc, %0" : : "r" (x));
}

// Supervisor Cause
static inline uint64
r_scause()
{
  uint64 x;
  asm volatile("csrr %0, scause" : "=r" (x) );
  return x;
}

// Supervisor Trap Value
static inline uint64
r_stval()
{
  uint64 x;
  asm volatile("csrr %0, stval" : "=r" (x) );
  return x;
}

// Machine-mode Interrupt Enable
static inline uint64
r_mie()
{
  uint64 x;
  asm volatile("csrr %0, mie" : "=r" (x) );
  return x;
}

static inline void 
w_mie(uint64 x)
{
  asm volatile("csrw mie, %0" : : "r" (x));
}

// Machine-mode Exception Program Counter
static inline uint64
r_mepc()
{
  uint64 x;
  asm volatile("csrr %0, mepc" : "=r" (x) );
  return x;
}

// Machine-mode Cause
static inline uint64
r_mcause()
{
  uint64 x;
  asm volatile("csrr %0, mcause" : "=r" (x) );
  return x;
}

// Machine-mode Trap Value
static inline uint64
r_mtval()
{
  uint64 x;
  asm volatile("csrr %0, mtval" : "=r" (x) );
  return x;
}

// Machine-mode Scratch register, for early trap handler in machine mode.
static inline uint64
r_mscratch()
{
  uint64 x;
  asm volatile("csrr %0, mscratch" : "=r" (x) );
  return x;
}

static inline void 
w_mscratch(uint64 x)
{
  asm volatile("csrw mscratch, %0" : : "r" (x));
}

// Supervisor Trap Vector Base Address
static inline uint64
r_stvec()
{
  uint64 x;
  asm volatile("csrr %0, stvec" : "=r" (x) );
  return x;
}

static inline void
w_stvec(uint64 x)
{
  asm volatile("csrw stvec, %0" : : "r" (x));
}

// Machine-mode Trap Vector Base Address
static inline uint64
r_mtvec()
{
  uint64 x;
  asm volatile("csrr %0, mtvec" : "=r" (x) );
  return x;
}

static inline void
w_mtvec(uint64 x)
{
  asm volatile("csrw mtvec, %0" : : "r" (x));
}

static inline uint64
r_tp()
{
  uint64 x;
  asm volatile("mv %0, tp" : "=r" (x) );
  return x;
}

static inline void
w_tp(uint64 x)
{
  asm volatile("mv tp, %0" : : "r" (x));
}

static inline uint64
r_ra()
{
  uint64 x;
  asm volatile("mv %0, ra" : "=r" (x) );
  return x;
}

static inline uint64
r_fp()
{
  uint64 x;
  asm volatile("mv %0, s0" : "=r" (x) );
  return x;
}

static inline uint64
r_sp()
{
  uint64 x;
  asm volatile("mv %0, sp" : "=r" (x) );
  return x;
}

#endif // __ASSEMBLER__

#endif
