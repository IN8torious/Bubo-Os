"""
BUBO OS — N8torious AI Home Hub
=================================
The Akatsuki home screen. This is where everything lives.

Layout:
  - Full screen, black background
  - N8torious AI owl logo centered at top
  - Akatsuki red voxel clouds floating across the bottom like thoughts
  - RTX voxel icons in a grid — agents + apps
  - Hover over an icon → name fades in below in Orbitron font, glowing red
  - Double-click → launches the agent or app
  - Right-click → context menu (Open, Pin to Dock, Remove)

Agents: Kami, Corvus, Mater, Batty, Vera, Genki, Bubo
Apps:   Terminal, Files, Browser, GitHub

Fonts: Orbitron Bold (hover labels), ShareTech Mono (status text)
Colors: Akatsuki black + crimson red + gold accents

Built by Nathan Brown. N8torious AI. Blue OS.
For Landon Pankuch. NO MAS DISADVANTAGED.
"""

import tkinter as tk
from tkinter import font as tkfont
import threading
import subprocess
import webbrowser
import sys
import os
import time
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))
from world_bus import bus, registry

# ─────────────────────────────────────────────
# PATHS
# ─────────────────────────────────────────────
ASSETS   = Path(__file__).parent.parent.parent / "assets"
AGENTS   = Path(__file__).parent.parent
FONTS    = ASSETS / "fonts"

# ─────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────
BG          = "#050508"
RED         = "#FF0A14"
RED_DARK    = "#8B0000"
GOLD        = "#FFD700"
WHITE       = "#E8E8F0"
GRAY        = "#333344"
CLOUD_RED   = "#CC0000"

# ─────────────────────────────────────────────
# ICON DEFINITIONS
# (id, label, icon_file, action_key)
# ─────────────────────────────────────────────
ICONS = [
    # Agents row
    ("kami",     "KAMI",     "icon_kami.png",     "agent"),
    ("corvus",   "CORVUS",   "icon_corvus.png",   "agent"),
    ("mater",    "MATER",    "icon_mater.png",    "agent"),
    ("batty",    "BATTY",    "icon_batty.png",    "agent"),
    ("vera",     "VERA",     "icon_vera.png",     "agent"),
    ("genki",    "GENKI",    "icon_genki.png",    "agent"),
    # Apps row
    ("terminal", "TERMINAL", "icon_terminal.png", "app"),
    ("files",    "FILES",    "icon_files.png",    "app"),
    ("browser",  "BROWSER",  "icon_browser.png",  "app"),
    ("github",   "GITHUB",   "icon_github.png",   "app"),
]

ICON_SIZE   = 96   # px on screen
ICON_PAD    = 20
COLS        = 5
LABEL_FADE_STEPS = 12
LABEL_FADE_MS    = 18


# ─────────────────────────────────────────────
# CLOUD PARTICLE
# ─────────────────────────────────────────────
class AkatsukiCloud:
    def __init__(self, canvas: tk.Canvas, screen_w: int, screen_h: int, offset_x: float = 0):
        self.canvas = canvas
        self.sw = screen_w
        self.sh = screen_h
        self.x = screen_w + offset_x
        self.y = screen_h - 80 - int(40 * (offset_x % 3) / 3)
        self.speed = 0.4 + (offset_x % 10) * 0.05   # slow drift
        self.scale = 0.6 + (offset_x % 5) * 0.1
        self.alpha_tag = f"cloud_{id(self)}"
        self._items = []
        self._draw()

    def _draw(self):
        for item in self._items:
            self.canvas.delete(item)
        self._items = []
        c = self.canvas
        s = self.scale
        x, y = self.x, self.y

        # Draw Akatsuki cloud shape from voxel-style rectangles
        # Main cloud body — three bumps
        blocks = [
            # (dx, dy, w, h)
            (0,   0,  30, 20),   # center base
            (-22, -8, 22, 18),   # left bump
            (20,  -10, 24, 20),  # right bump
            (-8,  -18, 18, 16),  # center top
            (8,   -14, 14, 12),  # right top
        ]
        for dx, dy, bw, bh in blocks:
            item = c.create_rectangle(
                x + dx * s, y + dy * s,
                x + (dx + bw) * s, y + (dy + bh) * s,
                fill=CLOUD_RED, outline="",
                tags=("cloud", self.alpha_tag)
            )
            self._items.append(item)

    def update(self):
        self.x -= self.speed
        if self.x < -200:
            self.x = self.sw + 100
        for item in self._items:
            self.canvas.move(item, -self.speed, 0)

    def is_offscreen(self):
        return self.x < -200


# ─────────────────────────────────────────────
# ICON TILE
# ─────────────────────────────────────────────
class IconTile:
    def __init__(self, canvas: tk.Canvas, x: int, y: int,
                 icon_id: str, label: str, image: tk.PhotoImage,
                 action_fn, right_click_fn,
                 font_orbitron, font_mono):
        self.canvas = canvas
        self.x = x
        self.y = y
        self.icon_id = icon_id
        self.label = label
        self.image = image
        self.action_fn = action_fn
        self.right_click_fn = right_click_fn
        self.font_orbitron = font_orbitron
        self.font_mono = font_mono

        self._label_alpha = 0   # 0 = invisible, 255 = full
        self._fade_job = None
        self._items = []
        self._label_item = None
        self._img_item = None
        self._draw()

    def _draw(self):
        c = self.canvas
        cx = self.x + ICON_SIZE // 2
        cy = self.y + ICON_SIZE // 2

        # Image
        self._img_item = c.create_image(
            cx, cy,
            image=self.image,
            tags=("icon", self.icon_id)
        )

        # Label (starts invisible — color matches bg)
        self._label_item = c.create_text(
            cx, self.y + ICON_SIZE + 14,
            text=self.label,
            font=self.font_orbitron,
            fill=BG,   # invisible initially
            tags=("icon_label", self.icon_id)
        )

        # Bind events
        for item in [self._img_item, self._label_item]:
            c.tag_bind(item, "<Enter>",           self._on_enter)
            c.tag_bind(item, "<Leave>",           self._on_leave)
            c.tag_bind(item, "<Double-Button-1>", self._on_double)
            c.tag_bind(item, "<Button-3>",        self._on_right)

    def _on_enter(self, event=None):
        self._stop_fade()
        self._fade_to(255)
        # Slight scale-up effect via image offset
        self.canvas.move(self._img_item, 0, -3)

    def _on_leave(self, event=None):
        self._stop_fade()
        self._fade_to(0)
        self.canvas.move(self._img_item, 0, 3)

    def _on_double(self, event=None):
        if self.action_fn:
            threading.Thread(target=self.action_fn, daemon=True).start()

    def _on_right(self, event=None):
        if self.right_click_fn:
            self.right_click_fn(event, self.icon_id, self.label)

    def _stop_fade(self):
        if self._fade_job:
            self.canvas.after_cancel(self._fade_job)
            self._fade_job = None

    def _fade_to(self, target: int):
        step = 255 // LABEL_FADE_STEPS
        if target > self._label_alpha:
            self._label_alpha = min(255, self._label_alpha + step * 2)
        else:
            self._label_alpha = max(0, self._label_alpha - step * 2)

        # Convert alpha to color blend between BG and RED
        t = self._label_alpha / 255.0
        r = int(int(BG[1:3], 16) * (1 - t) + int(RED[1:3], 16) * t)
        g = int(int(BG[3:5], 16) * (1 - t) + int(RED[3:5], 16) * t)
        b = int(int(BG[5:7], 16) * (1 - t) + int(RED[5:7], 16) * t)
        color = f"#{r:02x}{g:02x}{b:02x}"
        self.canvas.itemconfig(self._label_item, fill=color)

        if self._label_alpha != target:
            self._fade_job = self.canvas.after(
                LABEL_FADE_MS, lambda: self._fade_to(target)
            )


# ─────────────────────────────────────────────
# HOME HUB
# ─────────────────────────────────────────────
class HomeHub:
    def __init__(self, root: tk.Tk = None):
        self.standalone = root is None
        self.root = root or tk.Tk()
        self._setup_window()
        self._load_fonts()
        self._build_canvas()
        self._load_images()
        self._build_logo()
        self._build_icons()
        self._build_clouds()
        self._start_animation()
        self._bind_keys()

    def _setup_window(self):
        self.root.title("N8torious AI — Blue OS")
        self.root.configure(bg=BG)
        self.sw = self.root.winfo_screenwidth()
        self.sh = self.root.winfo_screenheight()
        self.root.geometry(f"{self.sw}x{self.sh}+0+0")
        if sys.platform == "win32":
            self.root.state("zoomed")
        else:
            self.root.attributes("-zoomed", True)

    def _load_fonts(self):
        # Register Orbitron if available
        orbitron_path = FONTS / "Orbitron-Bold.ttf"
        mono_path = FONTS / "ShareTechMono.ttf"

        # tkinter can't load custom fonts directly on all platforms
        # Use system fallbacks that match the aesthetic
        try:
            if sys.platform == "win32":
                import ctypes
                if orbitron_path.exists():
                    ctypes.windll.gdi32.AddFontResourceW(str(orbitron_path))
                self.font_label  = tkfont.Font(family="Orbitron", size=9, weight="bold")
                self.font_title  = tkfont.Font(family="Orbitron", size=22, weight="bold")
                self.font_status = tkfont.Font(family="Share Tech Mono", size=8)
            else:
                self.font_label  = tkfont.Font(family="Noto Sans", size=9, weight="bold")
                self.font_title  = tkfont.Font(family="Noto Sans", size=22, weight="bold")
                self.font_status = tkfont.Font(family="Noto Sans Mono", size=8)
        except Exception:
            self.font_label  = tkfont.Font(family="Courier", size=9, weight="bold")
            self.font_title  = tkfont.Font(family="Courier", size=22, weight="bold")
            self.font_status = tkfont.Font(family="Courier", size=8)

    def _build_canvas(self):
        self.canvas = tk.Canvas(
            self.root, bg=BG,
            highlightthickness=0,
            width=self.sw, height=self.sh
        )
        self.canvas.pack(fill="both", expand=True)

    def _load_images(self):
        from PIL import Image, ImageTk
        self._images = {}
        logo_path = ASSETS / "n8torious_logo.png"
        if logo_path.exists():
            img = Image.open(logo_path).resize((80, 80), Image.LANCZOS)
            self._images["logo"] = ImageTk.PhotoImage(img)

        for icon_id, label, icon_file, _ in ICONS:
            path = ASSETS / icon_file
            if path.exists():
                img = Image.open(path).resize((ICON_SIZE, ICON_SIZE), Image.LANCZOS)
                self._images[icon_id] = ImageTk.PhotoImage(img)

    def _build_logo(self):
        c = self.canvas
        cx = self.sw // 2

        # Logo image
        if "logo" in self._images:
            c.create_image(cx, 55, image=self._images["logo"], tags="logo")

        # N8torious AI title
        c.create_text(
            cx, 108,
            text="N8torious AI",
            font=self.font_title,
            fill=WHITE,
            tags="logo_text"
        )
        # Red N8 accent
        c.create_text(
            cx - 52, 108,
            text="N8",
            font=self.font_title,
            fill=RED,
            tags="logo_n8"
        )

        # Subtitle
        c.create_text(
            cx, 132,
            text="Blue OS  ·  NO MAS DISADVANTAGED",
            font=self.font_status,
            fill=RED_DARK,
            tags="logo_sub"
        )

        # Separator line
        c.create_line(
            cx - 200, 148, cx + 200, 148,
            fill=RED_DARK, width=1,
            tags="sep"
        )

    def _build_icons(self):
        self.tiles = {}
        total = len(ICONS)
        # Two rows: agents (6) then apps (4)
        agent_icons = [i for i in ICONS if i[3] == "agent"]
        app_icons   = [i for i in ICONS if i[3] == "app"]

        def row_x_start(count):
            total_w = count * (ICON_SIZE + ICON_PAD) - ICON_PAD
            return (self.sw - total_w) // 2

        # Row 1 — Agents
        row1_y = 185
        x_start = row_x_start(len(agent_icons))
        for i, (icon_id, label, icon_file, _) in enumerate(agent_icons):
            x = x_start + i * (ICON_SIZE + ICON_PAD)
            self._make_tile(icon_id, label, x, row1_y)

        # Row label
        self.canvas.create_text(
            self.sw // 2, row1_y - 18,
            text="— AGENT CREW —",
            font=self.font_status,
            fill=GRAY,
        )

        # Row 2 — Apps
        row2_y = row1_y + ICON_SIZE + 60
        x_start = row_x_start(len(app_icons))
        for i, (icon_id, label, icon_file, _) in enumerate(app_icons):
            x = x_start + i * (ICON_SIZE + ICON_PAD)
            self._make_tile(icon_id, label, x, row2_y)

        self.canvas.create_text(
            self.sw // 2, row2_y - 18,
            text="— APPS —",
            font=self.font_status,
            fill=GRAY,
        )

    def _make_tile(self, icon_id, label, x, y):
        image = self._images.get(icon_id)
        if not image:
            return
        tile = IconTile(
            canvas=self.canvas,
            x=x, y=y,
            icon_id=icon_id,
            label=label,
            image=image,
            action_fn=lambda iid=icon_id: self._launch(iid),
            right_click_fn=self._right_click_menu,
            font_orbitron=self.font_label,
            font_mono=self.font_status,
        )
        self.tiles[icon_id] = tile

    def _build_clouds(self):
        """Spawn Akatsuki clouds that drift across the bottom like thoughts."""
        self.clouds = []
        # Stagger clouds across the screen width
        for i in range(8):
            cloud = AkatsukiCloud(
                self.canvas, self.sw, self.sh,
                offset_x=i * (self.sw // 7)
            )
            self.clouds.append(cloud)

    def _start_animation(self):
        self._animate()

    def _animate(self):
        for cloud in self.clouds:
            cloud.update()
        self.root.after(33, self._animate)   # ~30fps, very light

    def _bind_keys(self):
        self.root.bind("<Escape>", lambda e: self._close())
        self.root.bind("<Control-k>", lambda e: self._launch("kami"))

    def _close(self):
        if self.standalone:
            self.root.destroy()
        else:
            # Hide the hub window instead of destroying
            self.root.withdraw()

    # ── LAUNCH ACTIONS ───────────────────────

    def _launch(self, icon_id: str):
        actions = {
            "kami":     self._launch_kami,
            "corvus":   self._launch_corvus,
            "mater":    self._launch_mater,
            "batty":    self._launch_batty,
            "vera":     self._launch_vera,
            "genki":    self._launch_genki,
            "terminal": self._launch_terminal,
            "files":    self._launch_files,
            "browser":  self._launch_browser,
            "github":   self._launch_github,
        }
        fn = actions.get(icon_id)
        if fn:
            fn()

    def _launch_kami(self):
        # Open Kami chat window inline
        from pathlib import Path
        import importlib.util
        kami_win_path = AGENTS / "kami" / "kami_window.py"
        if kami_win_path.exists():
            spec = importlib.util.spec_from_file_location("kami_window", kami_win_path)
            mod = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(mod)
            win = tk.Toplevel(self.root)
            win.attributes("-topmost", True)
            mod.KamiWindow(root=win)
        else:
            bus.publish("Hub", "kami.request", "Summon Kami")

    def _launch_corvus(self):
        script = AGENTS / "corvus" / "corvus.py"
        if script.exists():
            subprocess.Popen([sys.executable, str(script)])
        else:
            bus.publish("Hub", "corvus.request", "Status")

    def _launch_mater(self):
        bus.publish("Hub", "mater.request", "Build status")

    def _launch_batty(self):
        # Open a live health window
        self._open_batty_window()

    def _launch_vera(self):
        bus.publish("Hub", "vera.input", "Status")

    def _launch_genki(self):
        script = AGENTS / "genki" / "genki.py"
        if script.exists():
            subprocess.Popen([sys.executable, str(script)])

    def _launch_terminal(self):
        try:
            if sys.platform == "win32":
                subprocess.Popen(["cmd.exe"])
            else:
                for term in ["x-terminal-emulator", "gnome-terminal", "xterm", "konsole"]:
                    try:
                        subprocess.Popen([term])
                        return
                    except FileNotFoundError:
                        continue
        except Exception:
            pass

    def _launch_files(self):
        try:
            if sys.platform == "win32":
                subprocess.Popen(["explorer.exe"])
            else:
                for fm in ["nautilus", "thunar", "dolphin", "nemo"]:
                    try:
                        subprocess.Popen([fm])
                        return
                    except FileNotFoundError:
                        continue
        except Exception:
            pass

    def _launch_browser(self):
        webbrowser.open("https://www.google.com")

    def _launch_github(self):
        webbrowser.open("https://github.com/IN8torious/Bubo-Os")

    # ── BATTY HEALTH WINDOW ──────────────────

    def _open_batty_window(self):
        import psutil
        win = tk.Toplevel(self.root)
        win.title("Batty — PC Health")
        win.configure(bg=BG)
        win.geometry("380x320")
        win.attributes("-topmost", True)

        tk.Label(win, text="🦇 BATTY — PC HEALTH",
                 font=self.font_label, fg=RED, bg=BG).pack(pady=(16, 4))
        tk.Label(win, text="Real-time system status",
                 font=self.font_status, fg=GRAY, bg=BG).pack()

        frame = tk.Frame(win, bg=BG)
        frame.pack(fill="both", expand=True, padx=20, pady=10)

        labels = {}
        metrics = [
            ("CPU", "cpu"),
            ("RAM", "ram"),
            ("Disk", "disk"),
            ("Temp", "temp"),
        ]
        for name, key in metrics:
            row = tk.Frame(frame, bg=BG)
            row.pack(fill="x", pady=3)
            tk.Label(row, text=f"{name}:", font=self.font_label,
                     fg=GOLD, bg=BG, width=6, anchor="w").pack(side="left")
            lbl = tk.Label(row, text="...", font=self.font_label,
                           fg=WHITE, bg=BG, anchor="w")
            lbl.pack(side="left")
            labels[key] = lbl

        def update():
            try:
                labels["cpu"].config(
                    text=f"{psutil.cpu_percent(interval=0.5):.1f}%",
                    fg=RED if psutil.cpu_percent() > 80 else WHITE
                )
                ram = psutil.virtual_memory()
                labels["ram"].config(
                    text=f"{ram.percent:.1f}%  ({ram.used // (1024**3):.1f}GB / {ram.total // (1024**3):.1f}GB)",
                    fg=RED if ram.percent > 85 else WHITE
                )
                disk = psutil.disk_usage("/")
                labels["disk"].config(
                    text=f"{disk.percent:.1f}%  ({disk.used // (1024**3):.0f}GB / {disk.total // (1024**3):.0f}GB)",
                    fg=RED if disk.percent > 90 else WHITE
                )
                try:
                    temps = psutil.sensors_temperatures()
                    if temps:
                        t = list(temps.values())[0][0].current
                        labels["temp"].config(
                            text=f"{t:.1f}°C",
                            fg=RED if t > 80 else WHITE
                        )
                    else:
                        labels["temp"].config(text="N/A")
                except Exception:
                    labels["temp"].config(text="N/A")
            except Exception:
                pass
            if win.winfo_exists():
                win.after(2000, update)

        update()
        tk.Button(win, text="CLOSE", font=self.font_label,
                  fg=RED, bg=BG, bd=0, activeforeground=GOLD,
                  command=win.destroy).pack(pady=10)

    # ── RIGHT-CLICK CONTEXT MENU ─────────────

    def _right_click_menu(self, event, icon_id: str, label: str):
        menu = tk.Menu(self.root, tearoff=0,
                       bg=BG, fg=WHITE,
                       activebackground=RED, activeforeground=WHITE,
                       font=self.font_status, bd=0)
        menu.add_command(label=f"Open {label}",
                         command=lambda: self._launch(icon_id))
        menu.add_separator()
        menu.add_command(label="Pin to Dock",
                         command=lambda: bus.publish("Hub", "dock.pin", icon_id))
        menu.add_command(label="Agent Info",
                         command=lambda: self._show_info(icon_id, label))
        menu.add_separator()
        menu.add_command(label="Close Menu", command=menu.destroy)
        menu.tk_popup(event.x_root, event.y_root)

    def _show_info(self, icon_id: str, label: str):
        agent = registry.get(label.title())
        if not agent:
            return
        win = tk.Toplevel(self.root)
        win.title(f"{label} — Info")
        win.configure(bg=BG)
        win.geometry("320x200")
        win.attributes("-topmost", True)
        tk.Label(win, text=f"{agent.get('emoji','')} {label}",
                 font=self.font_label, fg=RED, bg=BG).pack(pady=(16, 4))
        tk.Label(win, text=agent.get("role", ""),
                 font=self.font_status, fg=WHITE, bg=BG, wraplength=280).pack()
        tk.Label(win, text=f"Character: {agent.get('character','')}",
                 font=self.font_status, fg=GOLD, bg=BG).pack(pady=4)
        tk.Label(win, text=f"Status: {agent.get('status','')}",
                 font=self.font_status, fg=GRAY, bg=BG).pack()
        tk.Button(win, text="CLOSE", font=self.font_label,
                  fg=RED, bg=BG, bd=0, command=win.destroy).pack(pady=12)

    def run(self):
        if self.standalone:
            self.root.mainloop()


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    hub = HomeHub()
    hub.run()
