# Corvus OS — Raven AOS

> **"The chef who runs the kitchen."**

A 7-layer Agentic Operating System written in C and Assembly, with **CORVUS** as the embedded reasoning engine at its core.

---

## Architecture

| Layer | Name | Description |
|-------|------|-------------|
| 1 | HAL | Hardware Abstraction Layer — ports, MMIO, CPU control |
| 2 | PMM | Physical Memory Manager — bitmap allocator, 4KiB pages |
| 3 | VMM | Virtual Memory Manager — page tables, kernel mapping |
| 4 | Sensory | Telemetry ring buffer — all kernel events → AI tokens |
| 5 | **CORVUS** | Reasoning engine — goals, agents, tool calls, governance |
| 6 | Governance | Constitutional C++ guard — AI cannot override safety rules |
| 7 | Interface | Natural language shell — type intent, not commands |

---

## CORVUS — Chief Orchestration & Reasoning Via Unified Systems

CORVUS is the brain of Raven OS. It coordinates 10 kernel agents using a BDI (Belief-Desire-Intention) model, enforces constitutional governance rules, and maintains a vector memory for system learning.

### 10 Kernel Agents

| Agent | Role |
|-------|------|
| Crow | Watchdog & self-heal |
| Healer | Auto-repair corrupted state |
| Security | Threat detection & syscall monitoring |
| Privacy | Tracker & telemetry blocker |
| Performance | Memory & CPU optimizer |
| Session | Session & auth manager |
| FileGuard | File integrity monitor |
| Notifier | Smart notification filter |
| Diagnostics | System health diagnostics |
| Accessibility | A11y & input assistance |

---

## Build

```bash
# Requirements: nasm, gcc (multilib), ld, grub-mkrescue, xorriso
make        # builds kernel + ISO
make run    # boots in QEMU
make clean  # clean build artifacts
```

---

## Status

- [x] Multiboot2 bootloader (ASM)
- [x] VGA text mode driver (Akatsuki theme — crimson on black)
- [x] Physical Memory Manager (bitmap allocator + telemetry)
- [x] CORVUS orchestration engine (10 agents, governance, vector memory)
- [x] IDT header (interrupt descriptor table)
- [ ] VMM (page tables) — in progress
- [ ] PIT timer + scheduler — next
- [ ] Keyboard driver (perception module) — next
- [ ] Natural language shell — planned
- [ ] llama.cpp SLM integration — planned

---

*Built with CORVUS. Authored by Raven.*
