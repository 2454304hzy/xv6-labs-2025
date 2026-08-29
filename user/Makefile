# To compile and run with a lab solution, set the lab name in conf/lab.mk
# (e.g., LAB=util). Run make grade to test solution with the lab's
# grade script (e.g., grade-lab-util).

include conf/lab.mk

K=kernel
U=user

OBJS = \
  $K/entry.o \
  $K/kalloc.o \
  $K/string.o \
  $K/main.o \
  $K/vm.o \
  $K/proc.o \
  $K/swtch.o \
  $K/trampoline.o \
  $K/trap.o \
  $K/syscall.o \
  $K/sysproc.o \
  $K/bio.o \
  $K/fs.o \
  $K/log.o \
  $K/sleeplock.o \
  $K/file.o \
  $K/pipe.o \
  $K/exec.o \
  $K/sysfile.o \
  $K/kernelvec.o \
  $K/plic.o \
  $K/virtio_disk.o \
  $K/start.o \
  $K/console.o \
  $K/printf.o \
  $K/uart.o \
  $K/spinlock.o

# Kernel asm
ASM = $(OBJS:.o=.asm)

KERNEL_OBJS = $(OBJS) $K/entry.o

# User programs
UPROGS = \
	$U/_cat \
	$U/_echo \
	$U/_forktest \
	$U/_grep \
	$U/_init \
	$U/_kill \
	$U/_ln \
	$U/_ls \
	$U/_mkdir \
	$U/_rm \
	$U/_sh \
	$U/_stressfs \
	$U/_usertests \
	$U/_grind \
	$U/_wc \
	$U/_zombie \
	$U/_logstress \
	$U/_forphan \
	$U/_dorphan \
	$U/_sleep \
	$U/_sixfive \
	$U/_memdump \
	$U/_find \
	$U/_sandbox \
	$U/_alarmtest \
	$U/_symlinktest \
	$U/_mmaptest

# Default target
all: $K/kernel fs.img

# Build kernel
$K/kernel: $(KERNEL_OBJS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(KERNEL_OBJS)
	$(OBJDUMP) -S $K/kernel > $K/kernel.asm
	$(OBJDUMP) -t $K/kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $K/kernel.sym

# Build user programs
$U/_%: $U/%.o $U/ulib.o $U/usys.o $U/printf.o $U/umalloc.o
	$(LD) $(LDFLAGS) -T $U/user.ld -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

# Build file system image
fs.img: mkfs/mkfs $(UPROGS)
	mkfs/mkfs fs.img README $(UPROGS)

# Build mkfs
mkfs/mkfs: mkfs/mkfs.c $K/fs.h
	gcc -Werror -Wall -I. -o $@ $<

# QEMU
QEMU = qemu-system-riscv64
QEMUOPTS = -machine virt -bios none -kernel $K/kernel -m 128M -smp $(CPUS) -nographic -global virtio-mmio.force-legacy=false -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

qemu: fs.img $K/kernel
	$(QEMU) $(QEMUOPTS)

# Clean
clean:
	rm -f *.o *.d *.asm *.sym fs.img fs.img.bk mkfs/mkfs
	rm -f $K/*.o $K/*.d $K/*.asm $K/*.sym
	rm -f $U/*.o $U/*.d $U/*.asm $U/*.sym
	rm -f $U/_*

# Grade
grade:
	./grade-lab-$(LAB)

.PHONY: all qemu clean grade
