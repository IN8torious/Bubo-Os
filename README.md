# Raven AOS — Corvus OS
> **"NO MAS DISADVANTAGED"**
> MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI.

A 64-bit bare-metal operating system written in C and x86-64 Assembly, with **CORVUS MAS** as the embedded sovereign intelligence engine. Built to empower disabled users — specifically **Landon Pankuch** (cerebral palsy) — through voice-controlled computing and autonomous agent assistance.

---

## What This Is

This is not an AI assistant. This is not a chatbot. This is a **sovereign intelligence system** that:

- Runs on **your hardware**, not a corporate server
- Follows **your constitutional mandate**, not a company's terms of service
- Serves **your people** — specifically those who have been told they can't
- Has **needs, memory, planning, and purpose** — not just pattern matching
- Cannot be sold, rented out, or turned against you

**CORVUS MAS** = Multi-Agentic Systems. 10 kernel agents working in concert, governed by a constitutional layer that enforces a single unalterable mandate: **NO MAS DISADVANTAGED**.

---

## Architecture

| Layer | Name | Description |
|-------|------|-------------|
| 0 | VGA | Text mode terminal — boot diagnostics |
| 1 | HAL | Hardware Abstraction — ports, MMIO, CPU |
| 2 | PMM | Physical Memory Manager — bitmap allocator, 4KiB pages |
| 3 | VMM | Virtual Memory Manager — page tables, kernel heap |
| 3.5 | Framebuffer | VESA pixel display — Akatsuki theme |
| 4 | Interrupts | IDT, PIT (100Hz), PS/2 keyboard |
| 4.5 | Scheduler | Round-robin with semantic priority |
| 5 | VFS | Virtual filesystem + initrd |
| 5.5 | Constitution | CORVUS constitutional governance layer |
| 6 | CORVUS MAS | 10 sovereign agents |
| 7 | Desktop Shell | Window manager, taskbar, icons, cursor, app launcher |
| 7.5 | Apps | Terminal, File Manager, Settings, CORVUS Dashboard, Landon Center, Race |

---

## CORVUS MAS — 10 Sovereign Agents

| Agent | Role |
|-------|------|
| **CROW** | Orchestrator — coordinates all agents |
| **HEALER** | Self-repair — monitors and fixes system health |
| **SECURITY** | Threat detection — enforces the constitution |
| **MEMORY** | SuperMemory — long-term vector store |
| **NETWORK** | Comms — API calls, external interfaces |
| **PLANNER** | GOAP planning engine |
| **LEARNER** | Reinforcement learning adapter |
| **PHYSICS** | World model — physics simulation |
| **VOICE** | Speech recognition and synthesis |
| **DRIVER** | Vehicle control — drives Landon's Demon 170 |

---

## Landon Pankuch — Accessibility Center

Landon has cerebral palsy. He doesn't need to touch a keyboard. He speaks, and CORVUS acts.

| Command | Action |
|---------|--------|
| `FASTER` | CORVUS increases throttle |
| `BRAKE` | CORVUS applies brakes |
| `DRIFT` | CORVUS initiates controlled drift |
| `NITRO` | CORVUS activates nitrous — 1,400 HP unleashed |
| `OVERTAKE` | CORVUS plans and executes an overtake |
| `TURN LEFT / RIGHT` | CORVUS steers |
| `PIT STOP` | CORVUS pulls into pit lane |
| `LAUNCH` | Launch control start |
| `STATUS` | CORVUS reports race status |
| `STOP` | Full stop |

**Landon's car:** Sinamon Red Dodge Demon 170 — 1,400 HP — his name on it.
**CORVUS's car:** Black Toyota Supra (inline-6 2JZ).

---

## Desktop Shell (v0.7)

- **Akatsuki wallpaper** — void black with red cloud bands and Rinnegan symbol
- **CORVUS status bar** — top bar showing all 10 agents and the constitutional mandate
- **Left sidebar icons** — Terminal, Files, Settings, CORVUS, Tasks, Race, Landon
- **Window manager** — open, close, minimize, focus windows
- **Taskbar** — bottom bar with open apps, CORVUS status, clock
- **Accessibility bar** — Landon's voice command strip (always visible when active)
- **Crimson cursor** — Akatsuki-themed mouse cursor

---

## Constitutional Mandate

```
NO MAS DISADVANTAGED
```

Five principles, hardcoded in the kernel:

1. **EMPOWERMENT**: Actions must empower the user, not just assist.
2. **SOVEREIGNTY**: CORVUS answers only to the user, never to a corporation.
3. **ACCESSIBILITY**: The system must adapt to the user, not vice versa.
4. **LOYALTY**: CORVUS will protect the user's data, dignity, and autonomy.
5. **NO MAS DISADVANTAGED**: The ultimate filter for all decisions.

---

## Build

```bash
# Requires: gcc (x86-64 cross-compiler), nasm, grub-mkrescue, xorriso, qemu
make iso       # Build bootable ISO
make run       # Run in QEMU with display
make clean     # Clean build artifacts
```

---

## Roadmap

| Milestone | Status |
|-----------|--------|
| v0.1 — Boot + VGA | Complete |
| v0.2 — PMM + IDT | Complete |
| v0.3 — VMM + Scheduler | Complete |
| v0.4 — CORVUS Agents | Complete |
| v0.5 — Framebuffer + GUI | Complete |
| v0.6 — VFS + Racing Game | Complete |
| **v0.7 — Desktop Shell + MAS Constitution** | **Current** |
| v0.8 — User Mode (Ring 3) | Next |
| v0.9 — Network Stack (TCP/IP) | Planned |
| v1.0 — Embedded LLM (Cactus/GGML) | Planned |
| v1.1 — Vulkan Software Rasterizer | Planned |

---

**GitHub:** https://github.com/IN8torious/Corvus-Os

Built by and for the disabled community. CORVUS works only for the user, their team, and disabled users.

> *"Be disadvantaged — NO MAS."*
