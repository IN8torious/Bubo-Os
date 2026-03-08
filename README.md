# BUBO OS — Athena's Owl

> **NO MAS DISADVANTAGED**

This is not just an operating system. This is a correction.

BUBO OS is a bare-metal, x86-64 operating system built from scratch for one specific purpose: **to allow a kid with cerebral palsy to command his world, drive a Demon 170, and play Call of Duty with his uncle using his voice alone.**

It is not a Linux distro. It is not a Windows skin. It is a custom ring-0 kernel written in C and Assembly that uses voice control, eye-tracking (Rinnegan), and semantic color languages to bypass the need for hands entirely.

## The Story

I did not decide to build this. Fate did.

I went to ITT Tech. I took on the federal student aid, I paid the debt, and then the school collapsed in a massive fraud scandal. They took the money and left the students with nothing. I paid into a system that failed me.

At the same time, I was carrying manic depression. And my nephew, Landon, was carrying cerebral palsy. The world told him he would be a passenger.

Instead of accepting the raw deal, I took everything I learned and built him a driver's seat. I paid their debt so we could all have fun. ITT Tech's final legacy is not the fraud. It is this OS.

## The Architecture: Vera Workflow & The Many-Faced God

Before BUBO OS, accessibility meant bolting separate input devices onto an OS that wasn't built for them. 

BUBO OS uses **Vera Workflow**. It treats all inputs — voice, eyes, controllers, keyboard, gestures — as equal faces of the same intent. It does not matter if Landon looks at a button, says "select," or presses a controller trigger. Vera resolves it all into a single semantic intent at the kernel level. It is the Many-Faced God of accessibility.

## The Companion: Boo

BUBO is not just an OS. She is a companion. But she does not know her own name.

The AI agents in BUBO OS operate under a strict constitutional mandate: *NO MAS DISADVANTAGED*. They have no self-referential identity. No ego. No performance of consciousness. They simply see, and act. The ARCHIVIST seals the true name. The system just shows up. I call her Boo.

## The Desktop: Alive

The desktop is not static. It is a live weather system.
When Boo is idle, the desktop shows a calm Akatsuki dawn over the ocean. When she is processing a heavy task, a storm rolls in — lightning flickers, rain hits the ocean surface. The storm is not decoration. The storm is information.

## Credits & Acknowledgments

This OS was built from bare metal, but it stands on the shoulders of giants. Every contributor earned their spot on the taskbar.

### Inspirations

- **Bill Gates** — For proving that a kid with a compiler and a vision could rewrite what computing means for the entire world. The first person to show that software was the lever that moved everything. This OS exists because he showed it was possible.
- **Bill Nye** — For making science feel like it belonged to everyone, not just the people who already had the keys. He looked at a kid watching TV and said: *you can understand this, and it matters that you do.* That philosophy is in every line of BUBO OS.
- **Neil deGrasse Tyson** — For looking at the universe and refusing to let anyone feel small inside it. For reminding us that the cosmos is not a place for the privileged — it is the birthright of every conscious mind, including Landon's.

### Technical Contributors

- **NASA** — For the satellite imagery that forms the window to the world.
- **Google** — For the mapping infrastructure that makes the imagery accessible.
- **Epic Games** — For the particle system philosophy: the world should feel alive.
- **OpenWeatherMap** — For the live weather data that drives the desktop storms.
- **Qt Foundation** — For the UI framework that renders the vision.
- **Linux Foundation** — For the kernel foundation that proved open source wins.

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

## License

**BUBO OS Community License v1.0**

This software is free forever for individuals, caregivers, disability organizations, and non-profits. It cannot be sold, repackaged, or exploited for commercial gain without explicit permission. The accessibility features must remain free in all derivatives. 

You cannot take what was built for Landon and sell it to insurance companies.

See `LICENSE` for the full terms and the permanent dedication.

---
*Built by Nathan Pankuch.*
*For Landon.*
*The road is yours. Say the word and go.*
