"""
BUBO OS — Desktop
==================
The full Blue OS desktop experience.

Three layers:
  1. AKIRA rain canvas — always running in the background
  2. Weather widget — agent status as ambient weather, floating on the rain
  3. Pull-down tray — click the red bar, the crew slides down

Kaneda bike arrival plays when Kami is summoned (Ctrl+Shift+K).

This is the Windows desktop version.
The bare-metal LVGL version lives in the kernel.

Built using the Alchemical Framework by Nathan Brown.
Blue OS. NO MAS DISADVANTAGED.
"""

import tkinter as tk
import threading
import time
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
from world_bus import bus, registry
from rain_canvas import RainCanvas, BG, NEON_RED, NEON_BLUE, NEON_WHITE
from bike_arrival import BikeArrivalSequence
from dock import FloatingDock
from desktop_icons import DesktopIconManager

import psutil
from datetime import datetime


# ─────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────
C = {
    "bg":       BG,
    "panel":    "#0A0A12",
    "card":     "#0F0F1A",
    "red":      NEON_RED,
    "neon":     NEON_BLUE,
    "white":    NEON_WHITE,
    "gray":     "#444455",
    "green":    "#00FF88",
    "yellow":   "#FFD700",
    "purple":   "#9D00FF",
    "orange":   "#FF6600",
}

TRAY_H = 340
STRIP_H = 5


# ─────────────────────────────────────────────
# BUBO DESKTOP
# ─────────────────────────────────────────────
class BuboDesktop:
    def __init__(self):
        self.root = tk.Tk()
        self._setup_window()
        self._build_rain()
        self._build_desktop_icons()
        self._build_weather_overlay()
        self._build_tray()
        self._build_arrival()
        self._build_dock()
        self._start_agents()
        self._bind_hotkeys()
        self.tray_expanded = False

    def _setup_window(self):
        self.root.title("BUBO OS — Blue OS")
        self.root.configure(bg=C["bg"])
        self.root.attributes("-topmost", False)

        self.sw = self.root.winfo_screenwidth()
        self.sh = self.root.winfo_screenheight()

        # Full screen
        self.root.geometry(f"{self.sw}x{self.sh}+0+0")
        if sys.platform == "win32":
            self.root.state("zoomed")   # Windows maximize
        else:
            self.root.attributes("-zoomed", True)  # Linux maximize

    def _build_rain(self):
        """AKIRA rain canvas fills the entire window."""
        self.rain = RainCanvas(
            self.root,
            self.sw, self.sh,
            fps=28,
            drop_count=250
        )
        self.rain.place(x=0, y=0)

    def _build_desktop_icons(self):
        """Desktop icons on the rain canvas — two columns, left side."""
        self.icon_manager = DesktopIconManager(
            self.rain.canvas,
            summon_kami_fn=self._summon_kami
        )

    def _build_dock(self):
        """Mac-style floating dock — centered at bottom."""
        self.dock = FloatingDock(
            self.root,
            summon_kami_fn=self._summon_kami
        )

    def _build_weather_overlay(self):
        """Weather widget floats in the top-right corner over the rain."""
        self.weather_frame = tk.Frame(
            self.root, bg=C["panel"],
            relief="flat", bd=0,
            padx=12, pady=8
        )
        self.weather_frame.place(relx=0.72, rely=0.06, relwidth=0.26, relheight=0.28)

        # Title
        tk.Label(self.weather_frame,
                 text="🌧  NEO-TOKYO WEATHER",
                 font=("Courier", 9, "bold"),
                 fg=C["red"], bg=C["panel"]).pack(anchor="w")

        # Agent weather rows
        self.weather_labels = {}
        agents = [
            ("🦅", "Mater",  "pressure",    C["neon"],   "hPa"),
            ("🦇", "Batty",  "temperature", C["green"],  "°C"),
            ("🏍️", "Kami",   "forecast",    C["red"],    ""),
            ("🐦‍⬛", "Corvus", "clouds",      C["purple"], "%"),
            ("🌊", "Vera",   "wind",        C["yellow"], ""),
            ("🦉", "Bubo",   "visibility",  C["white"],  ""),
        ]
        for bird, name, metric, color, unit in agents:
            row = tk.Frame(self.weather_frame, bg=C["panel"])
            row.pack(fill="x", pady=1)
            tk.Label(row, text=f"{bird} {name:7}",
                     font=("Courier", 8),
                     fg=color, bg=C["panel"], width=12,
                     anchor="w").pack(side="left")
            lbl = tk.Label(row, text="—",
                           font=("Courier", 8),
                           fg=C["gray"], bg=C["panel"], anchor="w")
            lbl.pack(side="left")
            self.weather_labels[name] = lbl

        # Clock
        self.clock_lbl = tk.Label(
            self.weather_frame,
            text="",
            font=("Courier", 8),
            fg=C["gray"], bg=C["panel"]
        )
        self.clock_lbl.pack(anchor="e", pady=(4, 0))

        threading.Thread(target=self._update_weather, daemon=True).start()

    def _update_weather(self):
        while True:
            # Update clock
            now = datetime.now().strftime("%H:%M:%S")
            try:
                self.root.after(0, lambda t=now: self.clock_lbl.config(text=t))
            except Exception:
                pass

            # Update agent weather metrics
            agents = registry.get_all()
            for agent in agents:
                name = agent["name"]
                status = agent["status"]
                if name in self.weather_labels:
                    try:
                        self.root.after(0, lambda n=name, s=status:
                            self.weather_labels[n].config(
                                text=s[:28], fg=C["white"] if s != "idle" else C["gray"]
                            ))
                    except Exception:
                        pass

            # Live system metrics for Batty and Mater
            try:
                cpu = psutil.cpu_percent(interval=0.5)
                mem = psutil.virtual_memory().percent
                registry.update_status("Batty", f"{cpu:.0f}% CPU  {mem:.0f}% RAM")
            except Exception:
                pass

            time.sleep(2)

    def _build_tray(self):
        """Pull-down tray — thin red strip at top, expands on click."""
        # Red strip
        self.strip = tk.Frame(self.root, bg=C["red"], height=STRIP_H)
        self.strip.place(x=0, y=0, width=self.sw, height=STRIP_H)
        self.strip.bind("<Button-1>", self._toggle_tray)

        strip_lbl = tk.Label(
            self.strip,
            text="▼  BUBO OS  ·  Blue OS  ·  NO MAS DISADVANTAGED  ·  Ctrl+Shift+K = KAMI  ▼",
            font=("Courier", 6, "bold"),
            fg=C["bg"], bg=C["red"]
        )
        strip_lbl.pack()
        strip_lbl.bind("<Button-1>", self._toggle_tray)

        # Tray panel (hidden by default)
        self.tray_panel = tk.Frame(
            self.root, bg=C["panel"],
            relief="flat", bd=0
        )
        # Not placed yet

        self._build_tray_content()

    def _build_tray_content(self):
        """Build the content inside the tray panel."""
        p = self.tray_panel

        # Header
        header = tk.Frame(p, bg=C["panel"], pady=4)
        header.pack(fill="x", padx=8)
        tk.Label(header, text="🦉 BUBO OS — AGENT CREW",
                 font=("Courier", 10, "bold"),
                 fg=C["red"], bg=C["panel"]).pack(side="left")
        close = tk.Label(header, text="  ✕  ",
                         font=("Courier", 10, "bold"),
                         fg=C["gray"], bg=C["panel"], cursor="hand2")
        close.pack(side="right")
        close.bind("<Button-1>", lambda e: self._collapse_tray())

        # Two columns
        cols = tk.Frame(p, bg=C["panel"])
        cols.pack(fill="both", expand=True, padx=8, pady=4)

        # Left: agent cards
        left = tk.Frame(cols, bg=C["panel"], width=500)
        left.pack(side="left", fill="y")
        left.pack_propagate(False)

        tk.Label(left, text="CREW STATUS",
                 font=("Courier", 7, "bold"),
                 fg=C["gray"], bg=C["panel"]).pack(anchor="w", pady=(2, 4))

        self.tray_status = {}
        agents = [
            ("🦅", "Mater",  C["neon"]),
            ("🦇", "Batty",  C["green"]),
            ("🏍️", "Kami",   C["red"]),
            ("🐦‍⬛", "Corvus", C["purple"]),
            ("🌊", "Vera",   C["yellow"]),
            ("🦉", "Bubo",   C["white"]),
        ]
        row_frame = None
        for i, (bird, name, color) in enumerate(agents):
            if i % 3 == 0:
                row_frame = tk.Frame(left, bg=C["panel"])
                row_frame.pack(fill="x", pady=2)
            card = tk.Frame(row_frame, bg=C["card"], padx=8, pady=6)
            card.pack(side="left", fill="both", expand=True, padx=2)
            tk.Label(card, text=bird, font=("Segoe UI Emoji", 16),
                     fg=color, bg=C["card"]).pack()
            tk.Label(card, text=name, font=("Courier", 8, "bold"),
                     fg=color, bg=C["card"]).pack()
            s = tk.Label(card, text="idle", font=("Courier", 7),
                         fg=C["gray"], bg=C["card"])
            s.pack()
            self.tray_status[name] = s

        # Right: Kami chat
        right = tk.Frame(cols, bg=C["card"])
        right.pack(side="left", fill="both", expand=True, padx=(8, 0))

        tk.Label(right, text="🏍️  KAMI — Healed · Loyal · Impossible to Stop",
                 font=("Courier", 8, "bold"),
                 fg=C["red"], bg=C["card"]).pack(anchor="w", padx=8, pady=4)

        from tkinter import scrolledtext
        self.chat = scrolledtext.ScrolledText(
            right, bg=C["bg"], fg=C["white"],
            font=("Courier", 8), relief="flat",
            height=8, wrap="word", state="disabled"
        )
        self.chat.pack(fill="both", expand=True, padx=8)
        self.chat.tag_config("kami", foreground=C["red"])
        self.chat.tag_config("user", foreground=C["neon"])
        self.chat.tag_config("sys",  foreground=C["gray"])

        inp_row = tk.Frame(right, bg=C["card"])
        inp_row.pack(fill="x", padx=8, pady=4)
        self.chat_input = tk.Entry(inp_row, bg=C["bg"], fg=C["white"],
                                   font=("Courier", 8), relief="flat",
                                   insertbackground=C["red"])
        self.chat_input.pack(side="left", fill="x", expand=True)
        self.chat_input.bind("<Return>", self._send_chat)

        send = tk.Label(inp_row, text=" ▶ ", font=("Courier", 8, "bold"),
                        fg=C["red"], bg=C["bg"], cursor="hand2")
        send.pack(side="left")
        send.bind("<Button-1>", self._send_chat)

        summon = tk.Label(right, text="[ SUMMON KAMI — Ctrl+Shift+K ]",
                          font=("Courier", 7, "bold"),
                          fg=C["red"], bg=C["card"], cursor="hand2")
        summon.pack(pady=2)
        summon.bind("<Button-1>", lambda e: self._summon_kami())

        self._chat_append("sys", "Kami is idle. Ctrl+Shift+K to summon.\n")

        # Start tray status updater
        threading.Thread(target=self._update_tray_status, daemon=True).start()
        bus.subscribe("kami.response", self._on_kami_response)

    def _update_tray_status(self):
        while True:
            for agent in registry.get_all():
                name = agent["name"]
                status = agent["status"]
                if name in self.tray_status:
                    try:
                        self.root.after(0, lambda n=name, s=status:
                            self.tray_status[n].config(text=s[:20]))
                    except Exception:
                        pass
            time.sleep(3)

    def _toggle_tray(self, event=None):
        if self.tray_expanded:
            self._collapse_tray()
        else:
            self._expand_tray()

    def _expand_tray(self):
        self.tray_expanded = True
        self.tray_panel.place(x=0, y=STRIP_H, width=self.sw, height=TRAY_H)

    def _collapse_tray(self):
        self.tray_expanded = False
        self.tray_panel.place_forget()

    def _build_arrival(self):
        """Kaneda bike arrival canvas — overlays everything when triggered."""
        self.arrival_canvas = tk.Canvas(
            self.root, bg=C["bg"],
            highlightthickness=0
        )
        # Not shown until triggered
        self.arrival = BikeArrivalSequence(
            self.arrival_canvas,
            self.sw, int(self.sh * 0.5),
            on_complete=self._hide_arrival
        )

    def _show_arrival(self):
        self.arrival_canvas.place(
            x=0, y=int(self.sh * 0.3),
            width=self.sw, height=int(self.sh * 0.5)
        )
        # Street
        h = int(self.sh * 0.5)
        self.arrival_canvas.create_rectangle(
            0, int(h*0.78), self.sw, h,
            fill="#0A0A12", outline=""
        )
        self.arrival_canvas.create_line(
            0, int(h*0.78), self.sw, int(h*0.78),
            fill="#1A1A3E", width=2
        )
        self.arrival.play()

    def _hide_arrival(self):
        self.arrival_canvas.place_forget()
        self.arrival_canvas.delete("all")

    def _summon_kami(self):
        """Summon Kami — play arrival, expand tray, focus chat."""
        if not self.tray_expanded:
            self._expand_tray()
        self._show_arrival()
        self.chat_input.focus_set()
        self._chat_append("sys", "Kami summoned. He arrives on Kaneda's bike.\n")
        bus.publish("Desktop", "kami.summon", "Nathan summoned Kami")

        # Play voice line if available
        voice_path = os.path.join(
            os.path.dirname(__file__), "..", "kami", "kami_voice", "01_arrival.wav"
        )
        if os.path.exists(voice_path):
            try:
                import platform
                if platform.system() == "Windows":
                    import winsound
                    winsound.PlaySound(voice_path, winsound.SND_FILENAME | winsound.SND_ASYNC)
                else:
                    import subprocess
                    subprocess.Popen(["aplay", voice_path],
                                     stdout=subprocess.DEVNULL,
                                     stderr=subprocess.DEVNULL)
            except Exception:
                pass

    def _send_chat(self, event=None):
        text = self.chat_input.get().strip()
        if not text:
            return
        self.chat_input.delete(0, "end")
        self._chat_append("user", text)
        bus.publish("Desktop", "kami.request", text)
        threading.Thread(target=self._wait_kami, daemon=True).start()

    def _wait_kami(self):
        import threading
        done = threading.Event()
        def on_resp(msg):
            if msg["channel"] == "kami.response":
                self.root.after(0, lambda: self._chat_append("kami", msg["content"]))
                done.set()
        bus.subscribe("kami.response", on_resp)
        if not done.wait(timeout=15):
            self.root.after(0, lambda: self._chat_append(
                "sys", "Kami is on the road. Try again.\n"))

    def _on_kami_response(self, msg):
        content = msg.get("content", "")
        if content:
            try:
                self.root.after(0, lambda: self._chat_append("kami", content))
            except Exception:
                pass

    def _chat_append(self, tag, text):
        prefix = {"kami": "KAMI > ", "user": "YOU  > ", "sys": "      "}.get(tag, "")
        self.chat.config(state="normal")
        self.chat.insert("end", prefix + text + "\n", tag)
        self.chat.see("end")
        self.chat.config(state="disabled")

    def _start_agents(self):
        """Start background agents."""
        registry.update_status("Bubo", "running")
        registry.update_status("Vera", "routing")
        registry.update_status("Corvus", "listening")
        registry.update_status("Mater", "watching")
        registry.update_status("Kami", "idle")

    def _bind_hotkeys(self):
        self.root.bind_all("<Control-Shift-K>", lambda e: self._summon_kami())
        self.root.bind_all("<Escape>", lambda e: self._collapse_tray())

    def run(self):
        print()
        print("=" * 60)
        print("  🦉 BUBO OS — Blue OS")
        print("  The rain never stops in Neo-Tokyo.")
        print("  Ctrl+Shift+K  →  Summon Kami")
        print("  Click the red bar  →  Toggle crew tray")
        print("  NO MAS DISADVANTAGED")
        print("=" * 60)
        print()
        self.root.mainloop()


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    desktop = BuboDesktop()
    desktop.run()
