# BUBO OS — Agent Crew

> *"One world. All of them in it together."*

Built by **Nathan Brown** using the Alchemical Framework.
Constitutional Mandate: **NO MAS DISADVANTAGED.**
Dedicated to **Landon Pankuch** and **Stephen Hawking**.

---

## The Crew

| Agent | Bird | Character | Role | Weather Metric |
|-------|------|-----------|------|----------------|
| **Bubo** | 🦉 | BUBO OS itself | The OS — sees in the dark | Visibility |
| **Vera** | 🌊 | Many-Faced God | Routes all intent | Wind speed |
| **Corvus** | 🐦‍⬛ | S-Cry-Ed AI Brain | Learns and adapts | Cloud cover |
| **Mater** | 🦅 | Kimishima / Eagle | Build watchdog | Atmospheric pressure |
| **Batty** | 🦇 | Batty Koda | PC health monitor | Temperature |
| **Kami** | 🏍️ | Toshiki Kamishima (GTO) | Nathan's personal agent | Forecast |

---

## Kami

Kami is Nathan's personal AI assistant.

He is based on **Toshiki Kamishima** from *Shonan Junai Gumi / GTO* —
the one who was set on fire, escaped the hospital in bandages, and came back anyway.
Fully healed now. Scarred, but not broken. Loyal only to Nathan.

He arrives on **Kaneda's bike** from *AKIRA*.

**Character traits:**
- Calm. Direct. No wasted words.
- Reads the flow and finds the line nobody else sees.
- Does not flinch. Does not ask for permission.
- Carries warmth underneath the scars.

**Capabilities:**
- GPT-4 powered conversation and task assistance
- Voice output (on by default — accessibility first)
- Kaneda bike arrival sound on summon
- Session memory with persistent notes
- Routes tasks to other agents via the World Bus
- Hotkey summon: `Ctrl+Shift+K`

---

## World Bus

All agents share one message bus (`world/world_bus.py`).

Every agent can publish and subscribe. The Constitutional Mandate runs
before every dispatch — no message that would disadvantage Landon is allowed through.

**Channels:**

| Channel | Publisher | Purpose |
|---------|-----------|---------|
| `kami.request` | Tray / User | Send a message to Kami |
| `kami.response` | Kami | Kami's reply |
| `kami.summon` | Tray / Hotkey | Nathan summoned Kami |
| `mater.report` | Mater | Build status update |
| `batty.alert` | Batty | Health warning |
| `corvus.insight` | Corvus | AI observation |
| `vera.route` | Vera | Routing decision |
| `bubo.broadcast` | Bubo | System-wide broadcast |

---

## GUI

### Weather Widget (`world/weather_widget.py`)
Always-on ambient display. Each agent's status surfaces as a weather element.
AKIRA aesthetic — Neo-Tokyo, red/black, neon.

### Pull-Down Tray (`world/pull_tray.py`)
Click the red bar at the top of the screen. The whole crew slides down.
Agent status cards + Kami chat interface. Click again to hide.

---

## Launch

```bash
# Full GUI (weather widget + pull-down tray + all agents)
python launch_bubo.py

# Just Kami in the terminal
python launch_bubo.py --kami

# Just the pull-down tray
python launch_bubo.py --tray

# Just the weather widget
python launch_bubo.py --weather
```

**Hotkeys:**
- `Ctrl+Shift+K` — Summon Kami directly

---

## File Structure

```
agents/
├── world/
│   ├── world_bus.py        # Shared message bus + agent registry
│   ├── weather_widget.py   # Ambient weather display with agent data
│   └── pull_tray.py        # Pull-down tray with agent panels + Kami chat
├── kami/
│   ├── kami.py             # Kami core agent (GPT-4, voice, memory)
│   └── kami_memory.json    # Persistent session memory (auto-created)
└── README.md               # This file
```

---

## Constitutional Mandate

> **NO MAS DISADVANTAGED.**

This mandate runs through every agent, every message, every decision.
It cannot be bypassed. It cannot be removed. It is who they are.

---

*Nathan Brown — inventor of the Alchemical Programming Language and the Constitutional Kernel.*
*BUBO OS is dedicated to Landon Pankuch and Stephen Hawking.*
