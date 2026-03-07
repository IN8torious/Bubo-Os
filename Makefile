# =============================================================================
# Raven AOS — Build System
# Builds the full 7-layer kernel: boot → PMM → IDT → VMM → PIT → KBD → CORVUS
# =============================================================================

CC      = gcc
AS      = nasm
LD      = ld

# Cross-compile flags for bare-metal x86 32-bit
CFLAGS  = -m32 -ffreestanding -fno-stack-protector -fno-builtin \
          -nostdlib -nostdinc -Wall -Wextra -O2 \
          -I./include

ASFLAGS = -f elf32

LDFLAGS = -m elf_i386 -T kernel.ld --oformat elf32-i386

# ── Source files ──────────────────────────────────────────────────────────────
ASM_SRCS = boot/boot.asm \
            boot/interrupts.asm \
            boot/scheduler_asm.asm

C_SRCS   = kernel/kernel.c \
            kernel/pmm.c \
            kernel/idt.c \
            kernel/vmm.c \
            kernel/pit.c \
            kernel/keyboard.c \
            kernel/scheduler.c \
            kernel/corvus.c \
            kernel/corvus_brain.c \
            kernel/shell.c \
            drivers/vga.c \
            graphics/framebuffer.c \
            graphics/font.c \
            graphics/gui.c \
            engine/raven_engine.c

# ── Object files ──────────────────────────────────────────────────────────────
ASM_OBJS = $(ASM_SRCS:.asm=.o)
C_OBJS   = $(C_SRCS:.c=.o)
OBJS     = $(ASM_OBJS) $(C_OBJS)

KERNEL   = iso/boot/raven.kernel
ISO      = RavenOS.iso

.PHONY: all kernel iso run run-headless clean push

all: iso

# ── Compile ASM ───────────────────────────────────────────────────────────────
%.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

# ── Compile C ─────────────────────────────────────────────────────────────────
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── Link kernel ───────────────────────────────────────────────────────────────
kernel: $(OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)
	@echo "[OK] Kernel: $(KERNEL)"
	@file $(KERNEL)
	@size $(KERNEL)

# ── Build ISO ─────────────────────────────────────────────────────────────────
iso: kernel
	grub-mkrescue -o $(ISO) iso/ 2>/dev/null
	@echo "[OK] ISO: $(ISO)"
	@ls -lh $(ISO)

# ── Run in QEMU (with display) ────────────────────────────────────────────────
run: iso
	qemu-system-i386 -cdrom $(ISO) -m 256M -vga std

# ── Run headless for CI testing ───────────────────────────────────────────────
run-headless: iso
	qemu-system-i386 -cdrom $(ISO) -m 256M -nographic -no-reboot

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	@echo "[OK] Cleaned"

# ── Push to GitHub ────────────────────────────────────────────────────────────
push:
	git add -A
	git commit -m "Raven AOS v0.3 — CORVUS brain, Raven Engine, GUI, framebuffer, font renderer"
	git push origin main
