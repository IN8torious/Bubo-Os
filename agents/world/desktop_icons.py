"""
BUBO OS — Desktop Icons
========================
Windows 11 style icons scattered on the rain canvas.
Double-click to launch. Single-click to select.
Drag to reposition. Positions saved between sessions.

Two columns on the left side — clean, not cluttered.
Each icon: emoji + label underneath, glow on select.

Built using the Alchemical Framework by Nathan Brown.
N8torious AI. Blue OS. NO MAS DISADVANTAGED.
"""

import tkinter as tk
import subprocess
import os
import sys
import json
import threading
import webbrowser
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))
from world_bus import bus, registry

# ─────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────
NEON_RED   = "#FF0A14"
NEON_BLUE  = "#00D4FF"
NEON_WHT   = "#E8E8F0"
NEON_GRN   = "#00FF88"
NEON_PUR   = "#9D00FF"
NEON_YEL   = "#FFD700"
GRAY       = "#555566"
BG         = "#050508"
SEL_COLOR  = "#FFFFFF22"

ICON_W     = 72
ICON_H     = 80
FONT_EMOJI = ("Segoe UI Emoji", 24)
FONT_LABEL = ("Segoe UI", 7)

# Positions save file
POSITIONS_FILE = Path(__file__).parent / "icon_positions.json"


# ─────────────────────────────────────────────
# DESKTOP ICON
# ─────────────────────────────────────────────
class DesktopIcon:
    def __init__(self, canvas: tk.Canvas, x: int, y: int,
                 emoji: str, label: str, color: str, action,
                 icon_id: str):
        self.canvas = canvas
        self.x = x
        self.y = y
        self.emoji = emoji
        self.label = label
        self.color = color
        self.action = action
        self.icon_id = icon_id
        self.selected = False
        self._drag_start = None
        self._items = []
        self._draw()

    def _draw(self):
        c = self.canvas
        # Remove old items
        for item in self._items:
            c.delete(item)
        self._items = []

        # Selection highlight
        if self.selected:
            sel = c.create_rectangle(
                self.x - 4, self.y - 4,
                self.x + ICON_W + 4, self.y + ICON_H + 4,
                fill="#FFFFFF", outline="",
                stipple="gray12", tags=("icon", self.icon_id)
            )
            self._items.append(sel)

        # Emoji
        emoji_item = c.create_text(
            self.x + ICON_W // 2, self.y + 28,
            text=self.emoji,
            font=FONT_EMOJI,
            fill=self.color,
            tags=("icon", self.icon_id)
        )
        self._items.append(emoji_item)

        # Label background (semi-dark pill)
        label_bg = c.create_rectangle(
            self.x + 2, self.y + 54,
            self.x + ICON_W - 2, self.y + ICON_H - 2,
            fill="#0A0A14", outline="",
            tags=("icon", self.icon_id)
        )
        self._items.append(label_bg)

        # Label text
        label_item = c.create_text(
            self.x + ICON_W // 2, self.y + 64,
            text=self.label,
            font=FONT_LABEL,
            fill=NEON_WHT if not self.selected else NEON_BLUE,
            width=ICON_W - 4,
            tags=("icon", self.icon_id)
        )
        self._items.append(label_item)

        # Bind events to all items
        for item in self._items:
            c.tag_bind(item, "<Button-1>",       self._on_click)
            c.tag_bind(item, "<Double-Button-1>", self._on_double_click)
            c.tag_bind(item, "<B1-Motion>",       self._on_drag)
            c.tag_bind(item, "<ButtonRelease-1>", self._on_release)

    def _on_click(self, event):
        # Deselect all others via canvas event
        self.canvas.event_generate("<<DeselectAll>>")
        self.selected = True
        self._draw()

    def _on_double_click(self, event):
        if self.action:
            threading.Thread(target=self.action, daemon=True).start()

    def _on_drag(self, event):
        if self._drag_start is None:
            self._drag_start = (event.x - self.x, event.y - self.y)
        dx, dy = self._drag_start
        self.x = event.x - dx
        self.y = event.y - dy
        # Clamp to canvas
        self.x = max(0, min(self.canvas.winfo_width() - ICON_W, self.x))
        self.y = max(30, min(self.canvas.winfo_height() - ICON_H - 80, self.y))
        self._draw()

    def _on_release(self, event):
        self._drag_start = None
        # Save positions
        self.canvas.event_generate("<<SavePositions>>")

    def deselect(self):
        if self.selected:
            self.selected = False
            self._draw()

    def get_position(self):
        return {"x": self.x, "y": self.y}


# ─────────────────────────────────────────────
# DESKTOP ICON MANAGER
# ─────────────────────────────────────────────
class DesktopIconManager:
    def __init__(self, canvas: tk.Canvas, summon_kami_fn=None):
        self.canvas = canvas
        self.summon_kami = summon_kami_fn
        self.icons = {}
        self._load_positions()
        self._create_icons()
        self._bind_canvas_events()

    def _default_positions(self):
        """Two columns of icons on the left side."""
        col1_x = 20
        col2_x = 20 + ICON_W + 16
        positions = {}
        icons_col1 = ["Kami", "Corvus", "Mater", "Batty", "Vera"]
        icons_col2 = ["Terminal", "Files", "Browser", "GitHub", "Docs"]
        for i, name in enumerate(icons_col1):
            positions[name] = {"x": col1_x, "y": 60 + i * (ICON_H + 12)}
        for i, name in enumerate(icons_col2):
            positions[name] = {"x": col2_x, "y": 60 + i * (ICON_H + 12)}
        return positions

    def _load_positions(self):
        try:
            if POSITIONS_FILE.exists():
                with open(POSITIONS_FILE) as f:
                    self._positions = json.load(f)
            else:
                self._positions = self._default_positions()
        except Exception:
            self._positions = self._default_positions()

    def _save_positions(self):
        try:
            data = {name: icon.get_position() for name, icon in self.icons.items()}
            with open(POSITIONS_FILE, "w") as f:
                json.dump(data, f, indent=2)
        except Exception:
            pass

    def _create_icons(self):
        icons_def = [
            # (id,        emoji, label,     color,     action)
            ("Kami",     "🏍️",  "Kami",     NEON_RED,  self._act_kami),
            ("Corvus",   "🐦‍⬛", "Corvus",   NEON_PUR,  self._act_corvus),
            ("Mater",    "🦅",  "Mater",    NEON_BLUE, self._act_mater),
            ("Batty",    "🦇",  "Batty",    NEON_GRN,  self._act_batty),
            ("Vera",     "🌊",  "Vera",     NEON_YEL,  self._act_vera),
            ("Terminal", "💻",  "Terminal", NEON_WHT,  self._act_terminal),
            ("Files",    "📁",  "Files",    NEON_WHT,  self._act_files),
            ("Browser",  "🌐",  "Browser",  NEON_BLUE, self._act_browser),
            ("GitHub",   "🐙",  "GitHub",   NEON_WHT,  self._act_github),
            ("Docs",     "📄",  "Docs",     GRAY,      self._act_docs),
        ]

        for icon_id, emoji, label, color, action in icons_def:
            pos = self._positions.get(icon_id, {"x": 20, "y": 60})
            icon = DesktopIcon(
                self.canvas,
                x=pos["x"], y=pos["y"],
                emoji=emoji, label=label,
                color=color, action=action,
                icon_id=icon_id
            )
            self.icons[icon_id] = icon

    def _bind_canvas_events(self):
        self.canvas.bind("<<DeselectAll>>", self._deselect_all)
        self.canvas.bind("<<SavePositions>>", lambda e: self._save_positions())
        # Click on blank canvas = deselect all
        self.canvas.bind("<Button-1>", self._canvas_click)

    def _canvas_click(self, event):
        # Only deselect if clicking on background (not an icon)
        items = self.canvas.find_overlapping(
            event.x - 2, event.y - 2, event.x + 2, event.y + 2
        )
        icon_items = [i for i in items if "icon" in self.canvas.gettags(i)]
        if not icon_items:
            self._deselect_all()

    def _deselect_all(self, event=None):
        for icon in self.icons.values():
            icon.deselect()

    # ── Actions ──────────────────────────────

    def _act_kami(self):
        if self.summon_kami:
            self.canvas.after(0, self.summon_kami)

    def _act_corvus(self):
        bus.publish("Desktop", "corvus.request", "Status report")

    def _act_mater(self):
        bus.publish("Desktop", "mater.request", "Build status")

    def _act_batty(self):
        bus.publish("Desktop", "batty.request", "Health check")

    def _act_vera(self):
        bus.publish("Desktop", "vera.input", "Status")

    def _act_terminal(self):
        try:
            if sys.platform == "win32":
                subprocess.Popen(["cmd.exe"])
            else:
                subprocess.Popen(["x-terminal-emulator"])
        except Exception:
            pass

    def _act_files(self):
        try:
            if sys.platform == "win32":
                subprocess.Popen(["explorer.exe"])
            else:
                subprocess.Popen(["nautilus"])
        except Exception:
            pass

    def _act_browser(self):
        webbrowser.open("https://www.google.com")

    def _act_github(self):
        webbrowser.open("https://github.com/IN8torious/Bubo-Os")

    def _act_docs(self):
        docs_path = Path(__file__).parent.parent.parent / "docs"
        if docs_path.exists():
            if sys.platform == "win32":
                subprocess.Popen(["explorer.exe", str(docs_path)])
            else:
                subprocess.Popen(["nautilus", str(docs_path)])
        else:
            webbrowser.open("https://github.com/IN8torious/Bubo-Os/tree/main/docs")


# ─────────────────────────────────────────────
# STANDALONE PREVIEW
# ─────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    root.title("Blue OS — Desktop Icons Preview")
    root.configure(bg=BG)
    root.geometry("1200x700")

    canvas = tk.Canvas(root, bg=BG, highlightthickness=0)
    canvas.pack(fill="both", expand=True)

    mgr = DesktopIconManager(canvas)
    root.mainloop()
