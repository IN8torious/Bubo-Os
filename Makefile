# =============================================================================
# Raven AOS v1.0 — Build System
#
# "NO MAS DISADVANTAGED"
# MAS = Multi-Agentic Systems — Sovereign Intelligence
#
# Full v1.0 stack:
#   boot → PMM → IDT → VMM → PIT → KBD → Scheduler → VFS → User Mode
#   → Sound → Voice → Network → TCP/IP → LLM
#   → CORVUS Constitution → CORVUS Agents → Desktop → Landon Center → Apps
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
            kernel/corvus_constitution.c \
            kernel/shell.c \
            kernel/vfs.c \
            kernel/initrd.c \
            kernel/usermode.c \
            kernel/voice.c \
            kernel/tcpip.c \
            kernel/llm.c \
            drivers/vga.c \
            drivers/sound.c \
            drivers/net.c \
            graphics/framebuffer.c \
            graphics/font.c \
            graphics/gui.c \
            graphics/corvus_display.c \
            graphics/desktop.c \
            engine/raven_engine.c \
            engine/racing_game.c \
            apps/corvus_dashboard.c \
            apps/landon_center.c \
            apps/terminal_app.c

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
	qemu-system-x86_64 \
	    -cdrom $(ISO) \
	    -m 512M \
	    -vga std \
	    -soundhw ac97 \
	    -net nic,model=e1000 -net user \
	    -serial stdio

# ── Run headless for CI testing ───────────────────────────────────────────────
run-headless: iso
	qemu-system-x86_64 \
	    -cdrom $(ISO) \
	    -m 512M \
	    -nographic \
	    -no-reboot \
	    -serial stdio \
	    -no-shutdown

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	@echo "[OK] Cleaned"

# ── Push to GitHub ────────────────────────────────────────────────────────────
push:
	git add -A
	git commit -m "Raven AOS v1.0 — Voice + LLM + Network + Sound + Ring3 — NO MAS DISADVANTAGED"
	git push origin main
