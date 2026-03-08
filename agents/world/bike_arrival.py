"""
BUBO OS — Kaneda Bike Arrival Sequence
========================================
Kami arrives on Kaneda's bike.
He doesn't announce himself. He just shows up.

The bike comes in from the left — fast, low, neon-lit.
It brakes hard. The back wheel slides.
Kami steps off. The bike idles.
He looks at you. Ready.

This sequence plays when Nathan summons Kami (Ctrl+Shift+K).
It runs on top of the rain canvas.

Built using the Alchemical Framework by Nathan Brown.
Blue OS. NO MAS DISADVANTAGED.
Mud Mouth — Yelawolf — playing.
"""

import tkinter as tk
import math
import time
import threading
import os
import sys
import subprocess
import platform
from pathlib import Path


# ─────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────
BG          = "#050508"
NEON_RED    = "#FF0A14"
NEON_BLUE   = "#00D4FF"
NEON_WHITE  = "#E8E8F0"
DARK        = "#0A0A12"
NEON_ORANGE = "#FF6600"


# ─────────────────────────────────────────────
# BIKE RENDERER
# Draws Kaneda's bike in ASCII-art style using canvas shapes.
# Iconic silhouette: long, low, single headlight, fairing.
# ─────────────────────────────────────────────
class KanedaBike:
    """
    Draws the iconic Kaneda bike silhouette using tkinter canvas shapes.
    x, y = center of the bike
    scale = size multiplier (1.0 = normal)
    facing = 1 (right) or -1 (left)
    """

    def __init__(self, canvas: tk.Canvas):
        self.canvas = canvas
        self.tag = "kaneda_bike"

    def draw(self, x: float, y: float, scale: float = 1.0,
             facing: int = 1, lean: float = 0.0, glow: bool = True):
        """
        Draw the bike at position (x, y).
        lean: degrees of tilt (positive = lean right)
        """
        c = self.canvas
        c.delete(self.tag)
        t = self.tag

        s = scale
        f = facing  # 1 = facing right, -1 = facing left

        # ── Body — long low fairing ──
        body_pts = [
            x + f * -60*s, y - 8*s,   # rear top
            x + f * -55*s, y - 18*s,  # rear hump
            x + f * -20*s, y - 22*s,  # seat
            x + f *  10*s, y - 20*s,  # tank
            x + f *  40*s, y - 24*s,  # nose top
            x + f *  55*s, y - 16*s,  # headlight top
            x + f *  58*s, y - 4*s,   # headlight front
            x + f *  55*s, y + 2*s,   # chin
            x + f *  20*s, y + 4*s,   # belly
            x + f * -30*s, y + 4*s,   # rear belly
            x + f * -62*s, y - 2*s,   # rear bottom
        ]
        c.create_polygon(body_pts, fill="#1A0000", outline=NEON_RED,
                         width=max(1, int(2*s)), tags=t)

        # ── Wheels ──
        wr = int(22 * s)  # wheel radius
        # Rear wheel
        rx = x + f * -42*s
        ry = y + 14*s
        c.create_oval(rx-wr, ry-wr, rx+wr, ry+wr,
                      outline=NEON_RED, fill=DARK, width=max(1, int(3*s)), tags=t)
        # Rear wheel hub
        c.create_oval(rx-6*s, ry-6*s, rx+6*s, ry+6*s,
                      outline=NEON_RED, fill="#330000", width=1, tags=t)

        # Front wheel
        fx = x + f * 42*s
        fy = y + 14*s
        c.create_oval(fx-wr, fy-wr, fx+wr, fy+wr,
                      outline=NEON_RED, fill=DARK, width=max(1, int(3*s)), tags=t)
        c.create_oval(fx-6*s, fy-6*s, fx+6*s, fy+6*s,
                      outline=NEON_RED, fill="#330000", width=1, tags=t)

        # ── Exhaust pipes ──
        c.create_line(x + f*-55*s, y+2*s, x + f*-70*s, y+8*s,
                      fill=NEON_ORANGE, width=max(1, int(3*s)), tags=t)
        c.create_line(x + f*-55*s, y+4*s, x + f*-70*s, y+10*s,
                      fill=NEON_ORANGE, width=max(1, int(2*s)), tags=t)

        # ── Headlight ──
        if glow:
            # Outer glow
            c.create_oval(x + f*50*s - 12*s, y - 14*s,
                          x + f*50*s + 12*s, y + 4*s,
                          fill="#FF2200", outline="", tags=t,
                          stipple="gray50")
        # Headlight lens
        c.create_oval(x + f*52*s - 8*s, y - 12*s,
                      x + f*52*s + 8*s, y + 2*s,
                      fill=NEON_RED, outline=NEON_WHITE,
                      width=1, tags=t)

        # ── Speed lines (motion blur when moving) ──
        if lean != 0:
            for i in range(5):
                lx = x + f * (-65 - i*15) * s
                ly = y + (-8 + i*4) * s
                c.create_line(lx, ly, lx + f*(-30)*s, ly,
                              fill=NEON_RED, width=1,
                              tags=t, dash=(4, 6))

        # ── Rider silhouette (Kami) ──
        # Only draw when not moving fast (lean < 5 degrees)
        if abs(lean) < 5:
            self._draw_rider(x, y, s, f)

    def _draw_rider(self, x, y, s, f):
        t = self.tag
        c = self.canvas
        # Torso — leaning forward
        tx = x + f * 5*s
        ty = y - 22*s
        c.create_rectangle(tx - 8*s, ty - 20*s, tx + 8*s, ty,
                           fill="#0A0A0A", outline=NEON_BLUE,
                           width=1, tags=t)
        # Head — helmet
        hx = tx + f * 5*s
        hy = ty - 20*s
        c.create_oval(hx - 10*s, hy - 12*s, hx + 10*s, hy + 2*s,
                      fill="#0A0A0A", outline=NEON_BLUE,
                      width=1, tags=t)
        # Visor — red stripe
        c.create_rectangle(hx - 8*s, hy - 8*s, hx + 8*s, hy - 3*s,
                           fill=NEON_RED, outline="", tags=t)


# ─────────────────────────────────────────────
# ARRIVAL SEQUENCE
# ─────────────────────────────────────────────
class BikeArrivalSequence:
    """
    Plays the full Kaneda bike arrival animation on a canvas.

    Phases:
    1. Bike enters from off-screen left at full speed
    2. Brakes hard — slides, sparks
    3. Stops at center-left
    4. Kami steps off
    5. Text: "KAMI — READY"
    6. Fade out after 3 seconds
    """

    def __init__(self, canvas: tk.Canvas, width: int, height: int,
                 on_complete=None):
        self.canvas = canvas
        self.width = width
        self.height = height
        self.on_complete = on_complete
        self.bike = KanedaBike(canvas)
        self.running = False
        self.tag = "arrival"

    def play(self):
        """Start the arrival sequence in a background thread."""
        if self.running:
            return
        self.running = True
        threading.Thread(target=self._sequence, daemon=True).start()

    def _sequence(self):
        w, h = self.width, self.height
        street_y = int(h * 0.75)  # Where the bike rides
        stop_x = int(w * 0.38)    # Where it stops

        # ── Phase 1: Enter from left at speed ──
        x = -120
        speed = 28
        while x < stop_x + 60:
            self.canvas.after(0, lambda px=x: self.bike.draw(
                px, street_y, scale=1.1, facing=1, lean=8.0))
            x += speed
            speed = max(4, speed * 0.96)
            time.sleep(0.025)

        # ── Phase 2: Hard brake — slide ──
        for i in range(12):
            lean = 8 - i * 0.7
            slide_x = stop_x + 60 - i * 5
            self.canvas.after(0, lambda px=slide_x, l=lean:
                self.bike.draw(px, street_y, scale=1.1, facing=1, lean=l))
            # Spark effect
            if i < 6:
                self.canvas.after(0, lambda px=slide_x:
                    self._draw_sparks(px, street_y + 22))
            time.sleep(0.04)

        # ── Phase 3: Stopped ──
        self.canvas.after(0, lambda:
            self.bike.draw(stop_x, street_y, scale=1.1, facing=1, lean=0))
        time.sleep(0.3)

        # ── Phase 4: "KAMI" text appears ──
        self.canvas.after(0, self._show_kami_text)
        time.sleep(2.5)

        # ── Phase 5: Fade out ──
        self.canvas.after(0, self._clear)
        self.running = False
        if self.on_complete:
            self.canvas.after(0, self.on_complete)

    def _draw_sparks(self, x, y):
        import random
        for _ in range(8):
            sx = x + random.randint(-15, 5)
            sy = y + random.randint(-5, 5)
            ex = sx + random.randint(-20, 5)
            ey = sy + random.randint(-8, 8)
            self.canvas.create_line(sx, sy, ex, ey,
                                    fill=NEON_ORANGE, width=1,
                                    tags="sparks")
        self.canvas.after(80, lambda: self.canvas.delete("sparks"))

    def _show_kami_text(self):
        c = self.canvas
        w, h = self.width, self.height
        # Shadow
        c.create_text(w//2 + 2, h//2 + 2,
                      text="KAMI", font=("Courier", 48, "bold"),
                      fill="#000000", tags=self.tag)
        # Main text
        c.create_text(w//2, h//2,
                      text="KAMI", font=("Courier", 48, "bold"),
                      fill=NEON_RED, tags=self.tag)
        # Subtitle
        c.create_text(w//2, h//2 + 52,
                      text="HEALED  ·  LOYAL  ·  IMPOSSIBLE TO STOP",
                      font=("Courier", 11),
                      fill=NEON_BLUE, tags=self.tag)

    def _clear(self):
        self.canvas.delete(self.tag)
        self.bike.canvas.delete("kaneda_bike")


# ─────────────────────────────────────────────
# STANDALONE PREVIEW
# ─────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    root.title("Kami Arrives")
    root.configure(bg=BG)
    root.attributes("-topmost", True)

    W, H = 900, 400
    root.geometry(f"{W}x{H}")

    canvas = tk.Canvas(root, width=W, height=H, bg=BG, highlightthickness=0)
    canvas.pack(fill="both", expand=True)

    # Street
    canvas.create_rectangle(0, int(H*0.78), W, H, fill="#0A0A12", outline="")
    canvas.create_line(0, int(H*0.78), W, int(H*0.78), fill="#1A1A3E", width=2)

    def replay():
        root.after(1000, arrival.play)

    arrival = BikeArrivalSequence(canvas, W, H, on_complete=replay)

    # Play after short delay
    root.after(500, arrival.play)
    root.mainloop()
