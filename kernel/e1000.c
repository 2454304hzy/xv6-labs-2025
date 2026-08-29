#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h.h"
#include "net.h"
#include "e1000_dev.h"

#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

// Each descriptor has an address and length, plus status and command flags.
struct tx_desc {
  uint64 addr;
  uint16 length;
  uint8 cso;
  uint8 cmd;
  uint8 status;
  uint8 css;
  uint16 special;
};

struct rx_desc {
  uint64 addr;
  uint16 length;
  uint16 csum;
  uint8 status;
  uint8 errors;
  uint16 special;
};

static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));

static char *tx_mbufs[TX_RING_SIZE];
static char *rx_mbufs[RX_RING_SIZE];

// The pointer to the memory-mapped I/O registers of the E1000.
volatile uint32 *regs;

// Protects the transmit ring.
struct spinlock e1000_lock;

static int
e1000_rxd_ready(uint32 idx)
{
  return rx_ring[idx].status & E1000_RXD_STAT_DD;
}

void
e1000_init(void)
{
  int i;

  initlock(&e1000_lock, "e1000");

  regs = (volatile uint32 *) (void *) (uint64) E1000_BASE;

  // Reset the device.
  regs[E1000_IMS] = 0;      // disable interrupts
  regs[E1000_ICR] = 0xffffffff; // clear any pending interrupts

  // Initialize transmit descriptors.
  for(i = 0; i < TX_RING_SIZE; i++){
    tx_ring[i].status = E1000_TXD_STAT_DD;
    tx_mbufs[i] = 0;
  }
  regs[E1000_TDBAL] = (uint64) tx_ring;
  regs[E1000_TDBAH] = (uint64) tx_ring >> 32;
  regs[E1000_TDLEN] = sizeof(tx_ring);
  regs[E1000_TDH] = 0;
  regs[E1000_TDT] = 0;

  // Initialize receive descriptors.
  for(i = 0; i < RX_RING_SIZE; i++){
    rx_mbufs[i] = kalloc();
    if(rx_mbufs[i] == 0)
      panic("e1000_init: kalloc");
    rx_ring[i].addr = (uint64) rx_mbufs[i];
    rx_ring[i].status = 0;
  }
  regs[E1000_RDBAL] = (uint64) rx_ring;
  regs[E1000_RDBAH] = (uint64) rx_ring >> 32;
  regs[E1000_RDLEN] = sizeof(rx_ring);
  regs[E1000_RDH] = 0;
  regs[E1000_RDT] = RX_RING_SIZE - 1;

  // Configure receive.
  regs[E1000_RCTL] = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SZ_2048 |
                     E1000_RCTL_SECRC;

  // Configure transmit.
  regs[E1000_TCTL] = E1000_TCTL_EN | E1000_TCTL_PSP |
                     (E1000_TCTL_CT & (0x10 << 4)) |
                     (E1000_TCTL_COLD & (0x40 << 10));

  // Enable interrupts.
  regs[E1000_IMS] = E1000_IMS_RXT0;

  // Ask for an interrupt when a packet is received.
  regs[E1000_RDTR] = 0;
  regs[E1000_RADV] = 0;

  printf("e1000: init succeeded\n");
}

void
e1000_transmit(struct mbuf *m)
{
  acquire(&e1000_lock);
  
  int idx = regs[E1000_TDT];
  
  if ((tx_ring[idx].status & E1000_TXD_STAT_DD) == 0) {
    release(&e1000_lock);
    return;
  }
  
  if (tx_mbufs[idx]) {
    mbuffree(tx_mbufs[idx]);
    tx_mbufs[idx] = 0;
  }
  
  tx_ring[idx].addr = (uint64) m->head;
  tx_ring[idx].length = m->len;
  tx_ring[idx].cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;
  tx_ring[idx].status = 0;
  
  tx_mbufs[idx] = m;
  
  idx = (idx + 1) % TX_RING_SIZE;
  regs[E1000_TDT] = idx;
  
  release(&e1000_lock);
}

static void
e1000_receive(void)
{
  int idx = (regs[E1000_RDT] + 1) % RX_RING_SIZE;
  
  while (e1000_rxd_ready(idx)) {
    struct mbuf *m = mbufalloc(0);
    if (m == 0) {
      // drop the packet
      char *old = rx_mbufs[idx];
      rx_mbufs[idx] = kalloc();
      if (rx_mbufs[idx] == 0) {
        panic("e1000_recv: kalloc");
      }
      rx_ring[idx].addr = (uint64) rx_mbufs[idx];
      rx_ring[idx].status = 0;
      regs[E1000_RDT] = idx;
      idx = (idx + 1) % RX_RING_SIZE;
      continue;
    }
    
    memmove(m->head, rx_mbufs[idx], rx_ring[idx].length);
    m->len = rx_ring[idx].length;
    
    net_rx(m);
    
    char *old = rx_mbufs[idx];
    rx_mbufs[idx] = kalloc();
    if (rx_mbufs[idx] == 0) {
      panic("e1000_recv: kalloc");
    }
    rx_ring[idx].addr = (uint64) rx_mbufs[idx];
    rx_ring[idx].status = 0;
    
    regs[E1000_RDT] = idx;
    idx = (idx + 1) % RX_RING_SIZE;
  }
}

void
e1000_intr(void)
{
  uint32 status = regs[E1000_ICR];
  if (status & E1000_ICR_RXT0) {
    e1000_receive();
  }
}
