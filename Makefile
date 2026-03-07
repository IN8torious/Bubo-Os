# Raven OS — Build System
# Targets: kernel, iso, run, clean

CC      = gcc
AS      = nasm
LD      = ld

# Cross-compile flags for bare-metal x86 32-bit
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -fno-builtin \
          -nostdlib -nostdinc -Wall -Wextra -O2 \
          -I./include -I./kernel -I./drivers

ASFLAGS = -f elf32

LDFLAGS = -m elf_i386 -T kernel.ld --oformat elf32-i386

# Source files
ASM_SRCS = boot/boot.asm
C_SRCS   = kernel/kernel.c \
            drivers/vga.c

# Object files
ASM_OBJS = $(ASM_SRCS:.asm=.o)
C_OBJS   = $(C_SRCS:.c=.o)
OBJS     = $(ASM_OBJS) $(C_OBJS)

KERNEL   = iso/boot/raven.kernel
ISO      = RavenOS.iso

.PHONY: all kernel iso run clean

all: iso

# Compile ASM
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# Compile C
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
kernel: $(OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)
	@echo "[OK] Kernel built: $(KERNEL)"
	@file $(KERNEL)

# Build ISO
iso: kernel
	grub-mkrescue -o $(ISO) iso/ 2>/dev/null
	@echo "[OK] ISO built: $(ISO)"
	@ls -lh $(ISO)

# Run in QEMU
run: iso
	qemu-system-i386 -cdrom $(ISO) -m 256M -vga std

# Run headless (for testing)
run-headless: iso
	qemu-system-i386 -cdrom $(ISO) -m 256M -nographic -serial stdio

clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	@echo "[OK] Cleaned"
