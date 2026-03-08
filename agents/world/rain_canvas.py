"""
BUBO OS — AKIRA Rain Canvas
=============================
Neo-Tokyo. Rain-slicked streets. Neon on black.
The rain never stops in Akira's Tokyo.
It runs behind everything — ambient, alive, relentless.

This canvas runs as the background of the BUBO OS GUI.
The weather widget and pull-down tray float on top of it.

Pure tkinter. No dependencies. Runs on Windows, Linux, macOS.

Built using the Alchemical Framework by Nathan Pankuch.
Blue OS. NO MAS DISADVANTAGED.
"""

import tkinter as tk
import random
import math
import time
import threading


# ─────────────────────────────────────────────
# AKIRA COLORS
# ─────────────────────────────────────────────
BG          = "#050508"
RAIN_COLORS = ["#0A2A4A", "#0D3B6E", "#1A5276", "#003344", "#004466"]
NEON_RED    = "#FF0A14"
NEON_BLUE   = "#00D4FF"
NEON_WHITE  = "#E8E8F0"
NEON_PURPLE = "#9D00FF"
NEON_YELLOW = "#FFD700"
STREET_GRAY = "#0A0A12"


# ─────────────────────────────────────────────
# RAIN DROP
# ─────────────────────────────────────────────
class RainDrop:
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.reset(start_anywhere=True)

    def reset(self, start_anywhere=False):
        self.x = random.randint(0, self.width)
        self.y = random.randint(0, self.height) if start_anywhere else random.randint(-80, -5)
        self.length = random.randint(8, 35)
        self.speed = random.uniform(4, 14)
        self.angle = random.uniform(-0.15, 0.05)  # Slight lean — wind
        self.alpha = random.uniform(0.3, 1.0)
        self.color = random.choice(RAIN_COLORS)
        self.width_px = random.choice([1, 1, 1, 2])  # Mostly thin

    def update(self):
        self.y += self.speed
        self.x += self.angle * self.speed
        if self.y > self.height + self.length:
            self.reset()

    def draw(self, canvas: tk.Canvas):
        x2 = self.x + math.sin(self.angle) * self.length
        y2 = self.y + self.length
        canvas.create_line(
            self.x, self.y, x2, y2,
            fill=self.color,
            width=self.width_px,
            tags="rain"
        )


# ─────────────────────────────────────────────
# NEON REFLECTION
# Puddle reflections on the street — AKIRA signature
# ─────────────────────────────────────────────
class NeonReflection:
    def __init__(self, width: int, height: int):
        self.width = width
        self.height = height
        self.reflections = []
        self._generate()

    def _generate(self):
        colors = [NEON_RED, NEON_BLUE, NEON_PURPLE, NEON_YELLOW]
        for _ in range(8):
            self.reflections.append({
                "x": random.randint(50, self.width - 50),
                "y": self.height - random.randint(20, 80),
                "w": random.randint(40, 200),
                "h": random.randint(3, 12),
                "color": random.choice(colors),
                "flicker": random.uniform(0.7, 1.0),
                "flicker_speed": random.uniform(0.02, 0.08),
                "phase": random.uniform(0, math.pi * 2),
            })

    def update(self):
        t = time.time()
        for r in self.reflections:
            r["flicker"] = 0.5 + 0.5 * math.sin(t * r["flicker_speed"] * 10 + r["phase"])

    def draw(self, canvas: tk.Canvas):
        for r in self.reflections:
            alpha = int(r["flicker"] * 60)
            # Simulate transparency with a dim version of the color
            color = r["color"]
            # Draw a blurred oval reflection
            canvas.create_oval(
                r["x"] - r["w"]//2, r["y"] - r["h"]//2,
                r["x"] + r["w"]//2, r["y"] + r["h"]//2,
                fill=color, outline="",
                tags="rain",
                stipple="gray25" if r["flicker"] < 0.5 else "gray50",
            )


# ─────────────────────────────────────────────
# RAIN CANVAS
# ─────────────────────────────────────────────
class RainCanvas:
    def __init__(self, parent: tk.Widget, width: int, height: int,
                 fps: int = 30, drop_count: int = 180):
        self.width = width
        self.height = height
        self.fps = fps
        self.running = True

        self.canvas = tk.Canvas(
            parent,
            width=width, height=height,
            bg=BG, highlightthickness=0,
        )

        # Street / ground line
        self._draw_street()

        # Rain drops
        self.drops = [RainDrop(width, height) for _ in range(drop_count)]

        # Neon reflections
        self.reflections = NeonReflection(width, height)

        # Start animation loop
        self._animate()

    def _draw_street(self):
        """Draw the Neo-Tokyo street — wet asphalt, horizon line."""
        street_y = int(self.height * 0.82)
        # Sky gradient (dark)
        self.canvas.create_rectangle(
            0, 0, self.width, street_y,
            fill=BG, outline="", tags="bg"
        )
        # Street (slightly lighter)
        self.canvas.create_rectangle(
            0, street_y, self.width, self.height,
            fill=STREET_GRAY, outline="", tags="bg"
        )
        # Horizon line — faint neon glow
        self.canvas.create_line(
            0, street_y, self.width, street_y,
            fill="#1A1A3E", width=2, tags="bg"
        )

    def _animate(self):
        if not self.running:
            return
        # Clear rain layer
        self.canvas.delete("rain")
        # Update and draw reflections
        self.reflections.update()
        self.reflections.draw(self.canvas)
        # Update and draw drops
        for drop in self.drops:
            drop.update()
            drop.draw(self.canvas)
        # Schedule next frame
        delay = int(1000 / self.fps)
        self.canvas.after(delay, self._animate)

    def stop(self):
        self.running = False

    def pack(self, **kwargs):
        self.canvas.pack(**kwargs)

    def place(self, **kwargs):
        self.canvas.place(**kwargs)

    def grid(self, **kwargs):
        self.canvas.grid(**kwargs)


# ─────────────────────────────────────────────
# STANDALONE PREVIEW
# ─────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    root.title("BUBO OS — AKIRA Rain")
    root.configure(bg=BG)
    root.attributes("-topmost", True)

    W, H = 900, 500
    root.geometry(f"{W}x{H}")

    rain = RainCanvas(root, W, H, fps=30, drop_count=220)
    rain.pack(fill="both", expand=True)

    # Overlay title
    title = tk.Label(
        root,
        text="🦉 BUBO OS  ·  Blue OS  ·  NO MAS DISADVANTAGED",
        font=("Courier", 11, "bold"),
        fg=NEON_RED, bg=BG
    )
    title.place(x=20, y=20)

    root.mainloop()
