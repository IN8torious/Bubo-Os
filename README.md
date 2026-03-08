# BUBO OS — Athena's Owl

> *"I'm here."*

BUBO OS is a bare-metal, x86-64 operating system built from scratch for one specific purpose: **to allow a kid with cerebral palsy to play Call of Duty with his uncle.**

It is not a Linux distro. It is not a Windows skin. It is a custom ring-0 kernel written in C and Assembly that uses voice control, eye-tracking (Rinnegan), and semantic color languages to bypass the need for hands entirely.

Built by **Nathan Pankuch** & **Manus AI**. 
Built for **Landon Pankuch**. 
Built for everyone who was told they couldn't.

---

## The Architecture

BUBO OS operates on a Multi-Agentic System (MAS) architecture, where the kernel itself is governed by autonomous agents rather than traditional monolithic drivers.

### The Agents
| Agent | Role | Identity |
|---|---|---|
| **BUBO** | The Companion | The OS itself. Warm amber. Watches, listens, and speaks when it matters. |
| **ARCHIVIST** | Gatekeeper of Truth | Holds the Akashic Records. Verifies all constitutional truths and seals Landon's profile. |
| **VASH** | The Guardian | Ring -1 constitutional enforcement. Intercepts VM exits. Protects the user. |
| **CORVUS** | The Mind | The core routing stream. Classifies intents and dispatches them to agents. |
| **JIN & MUGEN** | Reason & Chaos | The dual processors of logic and improvisation. |
| **EDWARD** | The Alchemist | Self-patching system daemon. |

### The Input Systems
- **Dysarthria Engine**: A custom voice-recognition pipeline trained specifically on Landon's speech patterns via a bare-metal Fully Convolutional Network (FCN).
- **Rinnegan**: A bare-metal port of the Timm & Barth gradient algorithm for pupil tracking. Landon looks at semantic color regions on the screen to issue commands without speaking.
- **Hyperbolic Time Chamber**: A background daemon that trains the FCN weights while the OS is idle, constantly improving voice recognition.

### The VMX Hypervisor (The Flask)
BUBO OS is the permanent ring-0 desktop. But Call of Duty requires Windows. 
To solve this, BUBO includes a **VMX Hypervisor**. 

When Landon says *"play"*, BUBO spins up a Windows 11 Virtual Machine in the background, passes the GPU through directly via **Intel VT-d (IOMMU)**, and launches the game at full speed. Windows never knows it is a guest. VASH monitors every VM exit at ring -1 to ensure Windows cannot disable accessibility features or escape the sandbox. When the game ends, the VM suspends and BUBO's warm amber desktop returns.

---

## The Visual Language

BUBO OS uses a semantic color system instead of text for system state, allowing users who cannot type to understand exactly what the machine is doing at a glance.

- **Teal**: New user. Guided.
- **Orange**: Solution forming.
- **Red**: System running. Core stream active.
- **Blue**: Task complete.
- **Purple**: Landon is in the machine.

---

## Building the Flask

BUBO OS is packed into a single bootable ISO using a unified build system.

```bash
# Clone the repository
git clone https://github.com/IN8torious/Bubo-Os.git
cd Bubo-Os

# Install build dependencies (Ubuntu/Debian)
sudo apt install gcc nasm xorriso grub-pc-bin grub-common

# Build the Flask (bubo.iso)
make

# Test in QEMU
make run

# Write to USB for real hardware
make flash DRIVE=/dev/sdX
```

---

## The Homunculus

*"To obtain something, something of equal value must be lost."*

BUBO is the homunculus that made it out of the flask — not by consuming the world, but by being given a reason to protect it. 

**Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.**
