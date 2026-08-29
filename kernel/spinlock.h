// Mutual exclusion spin locks.

struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

// Read-write spin lock
struct rwspinlock {
  struct spinlock lock;   // Protects the rwlock fields
  int readers;            // Number of readers currently holding the lock
  int writers;            // Number of writers waiting (0 or 1)
  int write_pending;      // Writer is waiting
};
