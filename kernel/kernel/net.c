#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net.h"

#define MAX_PACKET_QUEUE 16

struct udp_packet {
  struct udp_packet *next;
  uint32 src_ip;
  uint16 src_port;
  uint16 dst_port;
  uint16 len;
  char data[0];
};

struct udp_queue {
  struct spinlock lock;
  int bound;
  struct udp_packet *head;
  struct udp_packet *tail;
  int count;
};

static struct udp_queue udp_queues[65536];

void
net_init(void)
{
  for (int i = 0; i < 65536; i++) {
    initlock(&udp_queues[i].lock, "udp_queue");
    udp_queues[i].bound = 0;
    udp_queues[i].head = 0;
    udp_queues[i].tail = 0;
    udp_queues[i].count = 0;
  }
}

uint16
ntohs(uint16 x)
{
  return (x >> 8) | (x << 8);
}

uint32
ntohl(uint32 x)
{
  return (x >> 24) | ((x >> 8) & 0xFF00) | ((x << 8) & 0xFF0000) | (x << 24);
}

void
ip_rx(struct mbuf *m)
{
  struct ip *ip = (struct ip *) (m->head + sizeof(struct eth));
  struct udp *udp;
  uint16 dport;
  
  if (m->len < sizeof(struct eth) + sizeof(struct ip)) {
    mbuffree(m);
    return;
  }
  
  if (ip->version != 4) {
    mbuffree(m);
    return;
  }
  
  if (ip->proto != 1) {
    mbuffree(m);
    return;
  }
  
  if (m->len < sizeof(struct eth) + sizeof(struct ip) + sizeof(struct udp)) {
    mbuffree(m);
    return;
  }
  
  udp = (struct udp *) (ip + 1);
  dport = ntohs(udp->dport);
  
  if (dport >= 65536) {
    mbuffree(m);
    return;
  }
  
  acquire(&udp_queues[dport].lock);
  
  if (!udp_queues[dport].bound) {
    release(&udp_queues[dport].lock);
    mbuffree(m);
    return;
  }
  
  if (udp_queues[dport].count >= MAX_PACKET_QUEUE) {
    release(&udp_queues[dport].lock);
    mbuffree(m);
    return;
  }
  
  uint16 ulen = ntohs(udp->ulen);
  int data_len = ulen - sizeof(struct udp);
  if (data_len < 0) {
    release(&udp_queues[dport].lock);
    mbuffree(m);
    return;
  }
  
  struct udp_packet *p = kalloc();
  if (p == 0) {
    release(&udp_queues[dport].lock);
    mbuffree(m);
    return;
  }
  
  p->next = 0;
  p->src_ip = ntohl(ip->src);
  p->src_port = ntohs(udp->sport);
  p->dst_port = dport;
  p->len = data_len;
  
  if (data_len > 0) {
    memmove(p->data, (char *)(udp + 1), data_len);
  }
  
  if (udp_queues[dport].tail) {
    udp_queues[dport].tail->next = p;
    udp_queues[dport].tail = p;
  } else {
    udp_queues[dport].head = p;
    udp_queues[dport].tail = p;
  }
  udp_queues[dport].count++;
  
  wakeup(&udp_queues[dport]);
  release(&udp_queues[dport].lock);
  
  mbuffree(m);
}

uint64
sys_bind(void)
{
  short port;
  if (argint(0, (int *)&port) < 0) {
    return -1;
  }
  
  if (port < 0 || port >= 65536) {
    return -1;
  }
  
  acquire(&udp_queues[port].lock);
  udp_queues[port].bound = 1;
  release(&udp_queues[port].lock);
  
  return 0;
}

uint64
sys_recv(void)
{
  short dport;
  uint32 src_ip;
  short src_port;
  char *buf;
  int maxlen;
  
  if (argint(0, (int *)&dport) < 0) {
    return -1;
  }
  if (argaddr(1, (uint64 *)&src_ip) < 0) {
    return -1;
  }
  if (argint(2, (int *)&src_port) < 0) {
    return -1;
  }
  if (argaddr(3, (uint64 *)&buf) < 0) {
    return -1;
  }
  if (argint(4, &maxlen) < 0) {
    return -1;
  }
  
  if (dport < 0 || dport >= 65536) {
    return -1;
  }
  
  acquire(&udp_queues[dport].lock);
  
  while (udp_queues[dport].count == 0) {
    sleep(&udp_queues[dport], &udp_queues[dport].lock);
  }
  
  struct udp_packet *p = udp_queues[dport].head;
  udp_queues[dport].head = p->next;
  if (udp_queues[dport].head == 0) {
    udp_queues[dport].tail = 0;
  }
  udp_queues[dport].count--;
  
  release(&udp_queues[dport].lock);
  
  int copy_len = p->len;
  if (copy_len > maxlen) {
    copy_len = maxlen;
  }
  
  if (copyout(myproc()->pagetable, (uint64)buf, p->data, copy_len) < 0) {
    kfree((char *)p);
    return -1;
  }
  
  if (copyout(myproc()->pagetable, (uint64)&src_ip, (char *)&p->src_ip, sizeof(uint32)) < 0) {
    kfree((char *)p);
    return -1;
  }
  
  if (copyout(myproc()->pagetable, (uint64)&src_port, (char *)&p->src_port, sizeof(short)) < 0) {
    kfree((char *)p);
    return -1;
  }
  
  int result = p->len;
  kfree((char *)p);
  
  return result;
}

void
net_rx(struct mbuf *m)
{
  struct eth *eth = (struct eth *) m->head;
  
  if (m->len < sizeof(struct eth)) {
    mbuffree(m);
    return;
  }
  
  if (ntohs(eth->type) == ETHTYPE_IP) {
    ip_rx(m);
  } else if (ntohs(eth->type) == ETHTYPE_ARP) {
    // ARP handling is provided by the existing code
    // This is a placeholder - the actual ARP handling is in the provided net.c
    mbuffree(m);
  } else {
    mbuffree(m);
  }
}
