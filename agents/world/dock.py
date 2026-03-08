"""
BUBO OS — Floating Dock
========================
Mac energy. Centered at the bottom.
Not edge-to-edge. Not a Windows taskbar.
Minimal. Floating. Glows on hover.

Icons for the agent crew + common apps.
Click to launch. Hover to see the label pop up.

Built using the Alchemical Framework by Nathan Brown.
Blue OS. NO MAS DISADVANTAGED.
"""

import tkinter as tk
import subprocess
import os
import sys
import threading
import time

sys.path.insert(0, os.path.dirname(__file__))
from world_bus import bus, registry

# ─────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────
BG_DOCK   = "#0A0A14"
BG_HOVER  = "#1A1A2E"
NEON_RED  = "#FF0A14"
NEON_BLUE = "#00D4FF"
NEON_WHT  = "#E8E8F0"
GRAY      = "#444455"
BORDER    = "#2A2A3E"

DOCK_H    = 68
ICON_SIZE = 48
ICON_PAD  = 10


# ─────────────────────────────────────────────
# DOCK ITEM
# ─────────────────────────────────────────────
class DockItem:
    def __init__(self, frame: tk.Frame, emoji: str, label: str,
                 color: str, action, tooltip: str = ""):
        self.frame = frame
        self.emoji = emoji
        self.label = label
        self.color = color
        self.action = action
        self.tooltip = tooltip or label
        self._build()

    def _build(self):
        self.container = tk.Frame(
            self.frame, bg=BG_DOCK,
            width=ICON_SIZE + 8, height=DOCK_H - 8,
            cursor="hand2"
        )
        self.container.pack(side="left", padx=ICON_PAD // 2)
        self.container.pack_propagate(False)

        # Emoji icon
        self.icon_lbl = tk.Label(
            self.container,
            text=self.emoji,
            font=("Segoe UI Emoji", 22),
            fg=self.color, bg=BG_DOCK,
            cursor="hand2"
        )
        self.icon_lbl.pack(expand=True)

        # Dot indicator (active = colored, idle = gray)
        self.dot = tk.Label(
            self.container,
            text="●", font=("Courier", 6),
            fg=GRAY, bg=BG_DOCK
        )
        self.dot.pack()

        # Bind events
        for w in (self.container, self.icon_lbl, self.dot):
            w.bind("<Enter>", self._on_enter)
            w.bind("<Leave>", self._on_leave)
            w.bind("<Button-1>", self._on_click)

    def _on_enter(self, event=None):
        for w in (self.container, self.icon_lbl, self.dot):
            w.config(bg=BG_HOVER)
        # Scale up slightly
        self.icon_lbl.config(font=("Segoe UI Emoji", 26))

    def _on_leave(self, event=None):
        for w in (self.container, self.icon_lbl, self.dot):
            w.config(bg=BG_DOCK)
        self.icon_lbl.config(font=("Segoe UI Emoji", 22))

    def _on_click(self, event=None):
        if self.action:
            threading.Thread(target=self.action, daemon=True).start()

    def set_active(self, active: bool):
        self.dot.config(fg=self.color if active else GRAY)


# ─────────────────────────────────────────────
# FLOATING DOCK
# ─────────────────────────────────────────────
class FloatingDock:
    def __init__(self, root: tk.Tk, summon_kami_fn=None):
        self.root = root
        self.summon_kami = summon_kami_fn
        self.tooltip_win = None
        self._build()
        self._start_status_updater()

    def _build(self):
        sw = self.root.winfo_screenwidth()
        sh = self.root.winfo_screenheight()

        # Dock items definition
        self.dock_items_config = [
            # (emoji, label, color, action_fn)
            ("🏍️", "Kami",     "#FF0A14", self._launch_kami),
            ("🐦‍⬛", "Corvus",  "#9D00FF", self._launch_corvus),
            ("🦅", "Mater",    "#00D4FF", self._launch_mater),
            ("🦇", "Batty",    "#00FF88", self._launch_batty),
            ("🌊", "Vera",     "#FFD700",  self._launch_vera),
            ("⚡", "Genki",     "#FF6600",  self._launch_genki),
            ("─", "",          GRAY,      None),          # Separator
            ("💻", "Terminal", NEON_WHT,  self._launch_terminal),
            ("📁", "Files",    NEON_WHT,  self._launch_files),
            ("🌐", "Browser",  NEON_BLUE, self._launch_browser),
            ("⚙️", "Settings", GRAY,      self._launch_settings),
        ]

        # Calculate dock width
        n_items = len([x for x in self.dock_items_config if x[1]])
        dock_w = n_items * (ICON_SIZE + ICON_PAD * 2) + 24

        # Dock window — frameless, always on top
        self.dock_win = tk.Toplevel(self.root)
        self.dock_win.overrideredirect(True)
        self.dock_win.attributes("-topmost", True)
        self.dock_win.configure(bg=BG_DOCK)

        # Position: centered at bottom, above taskbar
        dock_x = (sw - dock_w) // 2
        dock_y = sh - DOCK_H - 48  # 48px above Windows taskbar
        self.dock_win.geometry(f"{dock_w}x{DOCK_H}+{dock_x}+{dock_y}")

        # Rounded-ish border frame
        outer = tk.Frame(self.dock_win, bg=BORDER, padx=1, pady=1)
        outer.pack(fill="both", expand=True)

        inner = tk.Frame(outer, bg=BG_DOCK, padx=8, pady=4)
        inner.pack(fill="both", expand=True)

        # Build items
        self.items = {}
        for emoji, label, color, action in self.dock_items_config:
            if not label:  # Separator
                sep = tk.Frame(inner, bg=BORDER, width=1, height=DOCK_H - 20)
                sep.pack(side="left", padx=6)
                continue
            item = DockItem(inner, emoji, label, color, action)
            self.items[label] = item

        # Tooltip label (shared, shown on hover)
        self.tooltip_lbl = tk.Label(
            self.dock_win,
            text="", font=("Courier", 8, "bold"),
            fg=NEON_WHT, bg="#1A1A2E",
            padx=6, pady=2
        )

        # Bind hover for tooltip
        self._bind_tooltips()

    def _bind_tooltips(self):
        for label, item in self.items.items():
            for w in (item.container, item.icon_lbl):
                w.bind("<Enter>", lambda e, l=label: self._show_tooltip(l))
                w.bind("<Leave>", lambda e: self._hide_tooltip())

    def _show_tooltip(self, label: str):
        self.tooltip_lbl.config(text=label)
        self.tooltip_lbl.place(relx=0.5, y=4, anchor="n")

    def _hide_tooltip(self):
        self.tooltip_lbl.place_forget()

    def _start_status_updater(self):
        def update():
            while True:
                for name, item in self.items.items():
                    agent = registry.get(name)
                    if agent:
                        active = agent.get("status", "offline") not in ("offline", "idle")
                        try:
                            self.root.after(0, lambda i=item, a=active: i.set_active(a))
                        except Exception:
                            pass
                time.sleep(3)
        threading.Thread(target=update, daemon=True).start()

    # ── Launch actions ──────────────────────────────────────

    def _launch_kami(self):
        if self.summon_kami:
            self.root.after(0, self.summon_kami)
        else:
            bus.publish("Dock", "kami.summon", "Dock icon clicked")

    def _launch_corvus(self):
        bus.publish("Dock", "corvus.request", "Status report")

    def _launch_mater(self):
        bus.publish("Dock", "mater.request", "Build status")

    def _launch_batty(self):
        bus.publish("Dock", "batty.request", "Health check")

    def _launch_vera(self):
        bus.publish("Dock", "vera.input", "Status")

    def _launch_genki(self):
        import subprocess, sys
        genki_path = os.path.join(
            os.path.dirname(__file__), "..", "genki", "genki.py"
        )
        subprocess.Popen([sys.executable, genki_path])

    def _launch_terminal(self):
        try:
            if sys.platform == "win32":
                subprocess.Popen(["cmd.exe"])
            else:
                subprocess.Popen(["x-terminal-emulator"])
        except Exception:
            pass

    def _launch_files(self):
        try:
            if sys.platform == "win32":
                subprocess.Popen(["explorer.exe"])
            else:
                subprocess.Popen(["nautilus"])
        except Exception:
            pass

    def _launch_browser(self):
        try:
            import webbrowser
            webbrowser.open("https://github.com/IN8torious/Bubo-Os")
        except Exception:
            pass

    def _launch_settings(self):
        try:
            if sys.platform == "win32":
                subprocess.Popen(["ms-settings:"], shell=True)
        except Exception:
            pass


# ─────────────────────────────────────────────
# STANDALONE PREVIEW
# ─────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    root.withdraw()  # Hide root — dock is a Toplevel

    # Black background to preview against
    bg = tk.Toplevel(root)
    bg.configure(bg="#050508")
    bg.geometry(f"{root.winfo_screenwidth()}x{root.winfo_screenheight()}+0+0")
    bg.attributes("-topmost", False)

    dock = FloatingDock(root)

    root.mainloop()
