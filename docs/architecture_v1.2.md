# Instinct OS v1.2 Architecture Document

**Mission:** "NO MAS DISADVANTAGED"
**Target:** Enable simultaneous multiplayer gaming — Landon Pankuch playing via CORVUS voice commands, while another player uses a standard USB controller. The system must support native games (Zelda-style adventure, sports hub) and pave the way for Windows/Linux games (like Call of Duty) via a bare-metal hypervisor.

---

## 1. Native Game Engine Extension (`game_engine.c`)

The existing INSTINCT ENGINE provides an Entity Component System (ECS), physics, and a 3D renderer. The v1.2 extension adds a dedicated 2D/2.5D game framework on top of this foundation.

### Core Systems
*   **Tilemap Engine:** A grid-based rendering system for overworlds and dungeons. It supports multiple layers (background, collision, foreground) and animated tiles.
*   **Sprite & Animation System:** A component-based animation controller tied to the ECS. It handles frame sequencing, state transitions (idle, walk, attack), and directional facing.
*   **Collision Detection:** A 2D AABB (Axis-Aligned Bounding Box) system integrated with the existing physics engine. It prevents entities from walking through solid tiles or other solid entities.
*   **Camera System:** A viewport controller that tracks the player entity, keeping them centered while clamping to map boundaries.
*   **Dialogue & Inventory:** A state machine for handling NPC interactions, text boxes, and item management.

### Voice Integration
The engine exposes a `game_handle_voice_cmd(uint32_t cmd_id)` interface. When CORVUS recognizes a command (e.g., "swing", "throw", "shoot"), it triggers the corresponding action in the player entity's state machine.

---

## 2. Native Games

### `zelda_game.c` (Adventure)
*   **Hero:** Landon.
*   **Mechanics:** Overworld exploration, dungeon crawling, sword combat, item usage (bow, bombs).
*   **Voice Commands:** "Walk north", "Swing sword", "Use bow", "Block".

### `sports_game.c` (Sports Hub)
*   **Hub:** A central menu to select between Basketball, Football, and Baseball.
*   **Basketball:** Voice commands like "Pass", "Shoot", "Defend".
*   **Football:** Voice commands like "Hike", "Pass left", "Run right", "Tackle".
*   **Baseball:** Voice commands like "Pitch fast", "Swing", "Steal base".

---

## 3. USB HID Controller Driver (`usb_hid.c`)

To support simultaneous multiplayer, Instinct OS requires a USB Human Interface Device (HID) driver.

### Architecture
*   **USB Stack Integration:** Hooks into the bare-metal USB controller driver (XHCI/EHCI) to detect device insertion.
*   **HID Parsing:** Parses the HID report descriptor to understand the controller's layout (buttons, axes, D-pad).
*   **Gamepad Mapping:** Normalizes input from various controllers (Xbox, PS5, generic USB) into a standard `gamepad_state_t` struct.
*   **Polling Loop:** Continuously polls the USB interrupt endpoint to update the gamepad state.

### Multiplayer Synergy
The game loop reads both the `gamepad_state_t` (for player 2) and the CORVUS voice command queue (for Landon). Both inputs are processed in the same tick, allowing seamless co-op or versus play.

---

## 4. VMX Hypervisor Layer (`vmx.c` & `raven_vm.c`)

This is the foundation for running Windows/Linux games (like Call of Duty) natively within Instinct OS. It acts as a Type-1 (bare-metal) hypervisor.

### Architecture
*   **Intel VT-x Enablement:** Checks CPU support, allocates VMXON and VMCS (Virtual Machine Control Structure) regions, and executes `VMXON`.
*   **VMCS Configuration:** Sets up the guest state (registers, segments, CR0/CR3/CR4), host state (where to return on VM exits), and execution controls.
*   **EPT (Extended Page Tables):** Maps guest physical addresses to host physical addresses, isolating the VM's memory from Instinct OS.
*   **VM Exits (`raven_vm.c`):** The handler that intercepts guest actions (e.g., CPUID, I/O port access, external interrupts). This is where Instinct OS emulates hardware or injects input.

### Input Injection (The CoD Bridge)
When the guest OS (Windows running CoD) is active, Instinct OS intercepts the VM's input polling. It takes Landon's voice commands (via CORVUS) and translates them into emulated keyboard/mouse or virtual gamepad inputs injected directly into the guest OS. Player 2's USB controller input is passed through directly.
