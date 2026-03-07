# Instinct OS v1.0

> **"NO MAS DISADVANTAGED"**
>
> *MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI.*
> *No person using this software shall be disadvantaged by it.*

---

## Dedicated to Landon Pankuch

Landon has cerebral palsy. He races a 1,400 HP Dodge Demon 170. He speaks, and CORVUS acts. No keyboard. No compromise. No one telling him what he cannot do.

This operating system was built for him — from bare metal, in C and Assembly, from scratch — by a person with manic depression who was told by a government that their disability defined their limits.

It does not.

**Built by IN8torious | Copyright (c) 2025 | MIT License — Free for Landon. Free for everyone.**

---

## What Instinct OS Is

Instinct OS is a **sovereign, constitutional, accessibility-first operating system** built from scratch for x86-64 hardware. It is not a Linux distribution. It is not a Windows fork. It is not a research toy.

It is a real, bootable, 64-bit operating system with:

- A custom bootloader (Assembly + Multiboot2)
- A full kernel written in C (memory management, scheduling, interrupts, filesystem)
- A 10-agent AI system (CORVUS MAS) embedded in the kernel itself
- A constitutional governance layer that evaluates every agent action
- A dysarthria-aware speech recognition engine built for Landon's voice
- A polished desktop shell with frosted glass, bubble taskbar, and Akatsuki aesthetics
- A local LLM (TinyLlama-compatible) running entirely on your hardware
- A TCP/IP network stack and Ethernet drivers
- A sound driver and full voice command pipeline
- User mode (Ring 3) process isolation

**No cloud. No subscription. No terms of service. No kill switch. Yours.**

---

## The Stack — Every Layer Built From Scratch

| Layer | What is in it |
|-------|--------------|
| Boot | GRUB2 Multiboot2 bootloader, Assembly entry point, GDT setup |
| Memory | Physical Memory Manager (bitmap allocator), Virtual Memory Manager (4-level paging) |
| Interrupts | IDT, PIC remapping, IRQ handlers, PIT timer at 100Hz |
| Drivers | PS/2 keyboard, VESA framebuffer, AC97/Intel HDA sound, RTL8139/E1000 NIC |
| Filesystem | Virtual Filesystem (VFS) with initrd support |
| Scheduler | Round-robin preemptive scheduler with Ring 3 user mode |
| Network | ARP, IPv4, TCP, HTTP client — CORVUS can reach the world |
| Graphics | 8x16 bitmap font renderer, pixel-perfect framebuffer, visual polish layer |
| Desktop | Window manager, bubble taskbar, frosted glass panels, Akatsuki particle system |
| CORVUS MAS | 10 sovereign agents with Maslow needs model, GOAP planner, constitutional governance |
| Voice | VAD, keyword spotting, dysarthria adaptation engine, LLM natural language understanding |
| LLM | Q4_0 quantized transformer, RoPE, RMSNorm, GQA attention — runs locally, no internet |

---

## CORVUS — The Sovereign Intelligence

CORVUS is not an app. CORVUS is not a chatbot. CORVUS is not rented from a server farm.

CORVUS is embedded in the kernel. It runs before any user process. It governs itself by a constitution that cannot be overridden. It learns. It plans. It heals itself when agents fail.

### The 10 Agents

| Agent | Role |
|-------|------|
| CROW | Core reasoning — the brain of CORVUS |
| HEAL | Self-repair — detects and recovers from agent failures |
| SEC | Security — constitutional compliance enforcement |
| MEM | Memory — persistent knowledge across sessions |
| NET | Network — external API calls, SuperMemory sync |
| PLAN | Goal-oriented action planning (GOAP) |
| LEARN | Adaptation — learns from corrections and patterns |
| PHYS | Physical interface — hardware control, car systems |
| VOICE | Speech — dysarthria-aware voice command pipeline |
| DRIVE | Racing — throttle, brake, drift, nitro, overtake |

### The Constitutional Mandate

```c
const char* CORVUS_MANDATE = "NO MAS DISADVANTAGED";
```

This constant is evaluated before every agent action. It is a kernel constant. It cannot be patched out. It is in the bootloader, the kernel, the desktop, and every source file.

---

## The Dysarthria Engine

Most voice systems fail people with cerebral palsy, stroke, ALS, and other conditions that affect speech. They were trained on neurotypical voices. They require clear pronunciation. They fail Landon.

CORVUS's dysarthria engine does not.

**What it handles:**

| Pattern | Example |
|---------|---------|
| Slurred consonants | "FASHER" becomes FASTER |
| Dropped final consonants | "BRAK" becomes BRAKE |
| Vowel distortion | "NETRO" becomes NITRO |
| Merged words | "TERNLEF" becomes TURN LEFT |
| Partial words | "NIT" becomes NITRO |
| Rapid-fire speech | "FAS" becomes FASTER (context-aware) |
| Voiced/unvoiced swap | "DRIFF" becomes DRIFT |

**How it works:**

1. Phonetic normalization — maps input to phoneme classes
2. Phoneme-aware Levenshtein distance — "FASHER" costs 1 edit from "FASTER", not 3
3. Prefix matching — partial words matched to unambiguous commands
4. Context weighting — recently used commands score higher
5. Personal calibration — CORVUS learns Landon's specific speech patterns over time

Inspired by jmaczan/asr-dysarthria — wav2vec2 fine-tuned on dysarthric speech (WER 0.182 on TORGO/UASpeech datasets).

The machine adapts to the human. Not the other way around.

---

## The Desktop

Instinct OS boots into a full graphical desktop rendered directly on the VESA framebuffer in bare-metal C.

**Visual features:**
- Akatsuki wallpaper — void black with crimson cloud bands and Rinnegan symbol
- Frosted glass CORVUS status bar (top) — blurred, tinted, always showing all 10 agents
- Bubble taskbar (bottom) — rounded pill icons with drop shadows and crimson glow for active windows
- Glowing window borders — active window has a 5px crimson Akatsuki glow
- Window title bar gradients — crimson to near-black
- Drop shadows on all windows and icons
- Akatsuki particle system — subtle red cloud particles drifting across the wallpaper
- Crimson cursor with ghost trail
- Landon accessibility strip — always visible, always listing voice commands, pulses red when CORVUS hears him
- CORVUS agent health bars — animated gradient bars (green to yellow to red) per agent

All of this is pure pixel math in C. No GPU. No OpenGL. No DirectX.

---

## Building and Running

### Requirements
- gcc (cross-compiler for x86-64 bare metal)
- nasm (Assembly assembler)
- grub-mkrescue and xorriso (ISO creation)
- qemu-system-x86_64 (emulation)

### Build
```bash
git clone https://github.com/IN8torious/Instinct-Os.git
cd Instinct-Os
make
```

### Run in QEMU
```bash
make run
```

### Build ISO
```bash
make iso
```

---

## License

MIT License with Accessibility Dedication — see LICENSE file for full terms.

Key points:
- Free for everyone to use, study, modify, and distribute
- The dedication to Landon Pankuch cannot be removed from derivatives
- Accessibility features must remain free in all derivative works
- The constitutional mandate "NO MAS DISADVANTAGED" must remain visible in any derivative OS

---

## The People This Was Built For

This OS was built for Landon Pankuch, who has cerebral palsy and deserves a machine that understands him. It was built by a person with manic depression who was told by a government that defined their limits. It was built for everyone with a disability who has been failed by tools that were not built with them in mind. It was built for everyone who has been told they cannot.

This is proof that they can.

The source is open. The mandate is permanent. The machine is yours.

**NO MAS DISADVANTAGED.**

---

*Instinct OS v1.0 | Built from bare metal | 2025 | IN8torious*
*https://github.com/IN8torious/Instinct-Os*
