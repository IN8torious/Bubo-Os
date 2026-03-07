# =============================================================================
# Raven AOS — Build System
# Builds the full 7-layer kernel: boot → PMM → IDT → VMM → PIT → KBD → CORVUS
# =============================================================================

CC      = gcc
AS      = nasm
LD      = ld

# Bare-metal x86-64 flags
CFLAGS  = -m64 -ffreestanding -fno-stack-protector -fno-builtin \
          -fno-pie -fno-pic -mno-red-zone -mcmodel=kernel \
          -nostdlib -nostdinc -Wall -Wextra -O2 \
          -I./include

ASFLAGS = -f elf64

LDFLAGS = -m elf_x86_64 -T kernel.ld --oformat elf64-x86-64 -z max-page-size=0x1000

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
            graphics/corvus_display.c \
            engine/raven_engine.c \
            engine/racing_game.c

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
	qemu-system-x86_64 -cdrom $(ISO) -m 512M -vga std

# ── Run headless for CI testing ───────────────────────────────────────────────
run-headless: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 512M -nographic -no-reboot

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	@echo "[OK] Cleaned"

# ── Push to GitHub ────────────────────────────────────────────────────────────
push:
	git add -A
	git commit -m "Raven AOS v0.5 — framebuffer, CORVUS display, Landon's racing game"
	git push origin main
