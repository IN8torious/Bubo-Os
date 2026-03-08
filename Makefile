# =============================================================================
# BUBO OS — The Flask Build System
# Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
# Built for Landon Pankuch. Built for everyone who was told they couldn't.
#
# "To obtain something, something of equal value must be lost."
# The equivalent exchange: one `make` command in. One `bubo.iso` out.
# The entire world in a flask.
#
# make          — Build everything → bubo.iso
# make run      — Boot in QEMU (test before real hardware)
# make run-vmx  — Boot in QEMU with VMX/KVM enabled (hypervisor testing)
# make flash    — Write bubo.iso to USB drive (make flash DRIVE=/dev/sdX)
# make clean    — Remove all build artifacts
# make push     — Commit and push to GitHub
# =============================================================================

CC      = gcc
AS      = nasm
LD      = ld

# ── Compiler Flags ────────────────────────────────────────────────────────────
# Bare-metal x86-64: no stdlib, no runtime, no OS assumptions
CFLAGS  = -m64 -ffreestanding -fno-stack-protector -fno-builtin \
          -fno-pie -fno-pic -mno-red-zone -mcmodel=kernel \
          -nostdlib -nostdinc -Wall -Wextra -O2 \
          -I./include

ASFLAGS = -f elf64

LDFLAGS = -m elf_x86_64 -T kernel.ld --oformat elf64-x86-64 \
          -z max-page-size=0x1000

# ── Output Names ──────────────────────────────────────────────────────────────
KERNEL  = iso/boot/bubo.kernel
ISO     = bubo.iso

# ── Assembly Sources ──────────────────────────────────────────────────────────
ASM_SRCS = \
    boot/boot.asm \
    boot/interrupts.asm \
    boot/scheduler_asm.asm

# ── Kernel C Sources ──────────────────────────────────────────────────────────
# Every agent, every system, every piece of the soul — all in one flask.

KERNEL_SRCS = \
    kernel/kernel.c \
    kernel/pmm.c \
    kernel/idt.c \
    kernel/vmm.c \
    kernel/pit.c \
    kernel/keyboard.c \
    kernel/scheduler.c \
    kernel/vfs.c \
    kernel/initrd.c \
    kernel/usermode.c \
    kernel/shell.c \
    kernel/tcpip.c \
    kernel/llm.c

# ── CORVUS Agent System ───────────────────────────────────────────────────────
AGENT_SRCS = \
    kernel/corvus.c \
    kernel/corvus_brain.c \
    kernel/corvus_constitution.c \
    kernel/corvus_flow.c \
    kernel/corvus_fcn.c \
    kernel/corvus_vash.c \
    kernel/corvus_archivist.c \
    kernel/corvus_bubo.c \
    kernel/corvus_chamber.c

# ── Voice & Accessibility ─────────────────────────────────────────────────────
VOICE_SRCS = \
    kernel/dysarthria.c \
    kernel/voice.c \
    kernel/rinnegan.c

# ── VMX Hypervisor Layer ──────────────────────────────────────────────────────
# BUBO at ring -1. Windows as a guest. The constitution absolute.
VMX_SRCS = \
    kernel/vmx.c \
    kernel/vmx_guest.c \
    kernel/vmx_vash.c \
    kernel/vt_d.c

# ── Drivers ───────────────────────────────────────────────────────────────────
DRIVER_SRCS = \
    drivers/vga.c \
    drivers/sound.c \
    drivers/net.c

# ── Graphics & Desktop ────────────────────────────────────────────────────────
GRAPHICS_SRCS = \
    graphics/framebuffer.c \
    graphics/font.c \
    graphics/gui.c \
    graphics/corvus_display.c \
    graphics/desktop.c \
    graphics/polish.c

# ── Game Engine ───────────────────────────────────────────────────────────────
ENGINE_SRCS = \
    engine/deepflow_engine.c \
    engine/racing_game.c

# ── Applications ──────────────────────────────────────────────────────────────
APP_SRCS = \
    apps/corvus_dashboard.c \
    apps/landon_center.c \
    apps/terminal_app.c

# ── All Sources Together ──────────────────────────────────────────────────────
C_SRCS = \
    $(KERNEL_SRCS) \
    $(AGENT_SRCS) \
    $(VOICE_SRCS) \
    $(VMX_SRCS) \
    $(DRIVER_SRCS) \
    $(GRAPHICS_SRCS) \
    $(ENGINE_SRCS) \
    $(APP_SRCS)

# ── Object Files ──────────────────────────────────────────────────────────────
ASM_OBJS = $(ASM_SRCS:.asm=.o)
C_OBJS   = $(C_SRCS:.c=.o)
OBJS     = $(ASM_OBJS) $(C_OBJS)

# ── Build Rules ───────────────────────────────────────────────────────────────
.PHONY: all kernel iso run run-vmx flash clean push

all: iso
	@echo ""
	@echo "  ╔══════════════════════════════════════════════════════╗"
	@echo "  ║           BUBO OS — The Flask is Sealed              ║"
	@echo "  ║                                                      ║"
	@echo "  ║   bubo.iso is ready.                                 ║"
	@echo "  ║   Write it to USB: make flash DRIVE=/dev/sdX         ║"
	@echo "  ║   Test in QEMU:    make run                          ║"
	@echo "  ║                                                      ║"
	@echo "  ║   Built for Landon. Built for everyone who           ║"
	@echo "  ║   was told they couldn't.                            ║"
	@echo "  ╚══════════════════════════════════════════════════════╝"
	@echo ""

# ── Compile Assembly ──────────────────────────────────────────────────────────
%.o: %.asm
	@echo "[ASM] $<"
	@$(AS) $(ASFLAGS) $< -o $@

# ── Compile C ─────────────────────────────────────────────────────────────────
%.o: %.c
	@echo "[CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# ── Link the Kernel ───────────────────────────────────────────────────────────
kernel: $(OBJS)
	@echo "[LD]  Linking BUBO kernel..."
	@$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)
	@echo "[OK]  Kernel: $(KERNEL)"
	@size $(KERNEL)

# ── Pack the Flask — Build the ISO ───────────────────────────────────────────
iso: kernel
	@echo "[ISO] Sealing the Flask..."
	@grub-mkrescue -o $(ISO) iso/ 2>/dev/null
	@echo "[OK]  $(ISO) is ready"
	@ls -lh $(ISO)

# ── Test in QEMU (standard) ───────────────────────────────────────────────────
run: iso
	@echo "[QEMU] Booting BUBO OS..."
	qemu-system-x86_64 \
	    -cdrom $(ISO) \
	    -m 2G \
	    -vga std \
	    -device ac97 \
	    -net nic,model=e1000 -net user \
	    -usb -device usb-ehci \
	    -serial stdio \
	    -no-reboot

# ── Test in QEMU with VMX/KVM (hypervisor testing) ────────────────────────────
run-vmx: iso
	@echo "[QEMU] Booting BUBO OS with VMX enabled..."
	qemu-system-x86_64 \
	    -cdrom $(ISO) \
	    -m 8G \
	    -enable-kvm \
	    -cpu host,+vmx \
	    -vga std \
	    -device ac97 \
	    -net nic,model=e1000 -net user \
	    -usb -device usb-ehci \
	    -serial stdio \
	    -no-reboot

# ── Flash to USB Drive ────────────────────────────────────────────────────────
# Usage: make flash DRIVE=/dev/sdX
# WARNING: This will ERASE the target drive. Double-check DRIVE= before running.
DRIVE ?= UNSET
flash: iso
	@if [ "$(DRIVE)" = "UNSET" ]; then \
	    echo ""; \
	    echo "  ERROR: Specify the USB drive."; \
	    echo "  Usage: make flash DRIVE=/dev/sdX"; \
	    echo ""; \
	    exit 1; \
	fi
	@echo "[FLASH] Writing bubo.iso to $(DRIVE)..."
	@echo "[FLASH] WARNING: This will erase $(DRIVE). Ctrl+C to cancel. (5 seconds)"
	@sleep 5
	sudo dd if=$(ISO) of=$(DRIVE) bs=4M status=progress oflag=sync
	@echo "[OK]  BUBO OS written to $(DRIVE). Plug it in and boot."

# ── Clean ─────────────────────────────────────────────────────────────────────
clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -f $(OBJS) $(KERNEL) $(ISO)
	@echo "[OK]  Clean"

# ── Push to GitHub ────────────────────────────────────────────────────────────
push:
	git add -A
	git commit -m "BUBO OS — The Flask is sealed. Built for Landon. — Nathan Pankuch & Manus AI"
	git push origin main
