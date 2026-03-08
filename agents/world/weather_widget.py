"""
BUBO OS — Weather Widget
=========================
The agents live inside the weather. Non-intrusive. Always ambient.
You glance at the weather. The crew is already there.

  Mater  → Atmospheric Pressure  (build health)
  Batty  → Temperature           (PC health)
  Kami   → Forecast              (task queue)
  Corvus → Cloud Cover           (learning state)
  Vera   → Wind Speed            (routing activity)
  Bubo   → Visibility            (OS state)

Built using the Alchemical Framework by Nathan Pankuch.
Arrives on Kaneda's bike. NO MAS DISADVANTAGED.
"""

import tkinter as tk
from tkinter import font as tkfont
import threading
import time
import sys
import os
import subprocess
import platform
import psutil
import requests
from datetime import datetime

# Add world bus to path
sys.path.insert(0, os.path.dirname(__file__))
from world_bus import bus, registry

# ─────────────────────────────────────────────
# AKIRA COLOR PALETTE
# Neo-Tokyo. Rain-slicked streets. Neon on black.
# ─────────────────────────────────────────────
COLORS = {
    "bg":           "#050508",      # Deep Neo-Tokyo black
    "bg_panel":     "#0A0A12",      # Panel background
    "bg_card":      "#0F0F1A",      # Card background
    "red":          "#FF0A14",      # Kaneda red
    "red_dim":      "#8B0000",      # Dim red
    "blue_neon":    "#00D4FF",      # Neo-Tokyo neon blue
    "blue_dim":     "#004466",      # Dim blue
    "yellow":       "#FFD700",      # Warning yellow
    "green":        "#00FF88",      # Healthy green
    "white":        "#E8E8F0",      # Off-white text
    "gray":         "#444455",      # Dim text
    "purple":       "#9D00FF",      # Rinnegan purple
    "border":       "#1A1A2E",      # Subtle border
    "rain":         "#0D1B2A",      # Rain color
}

# ─────────────────────────────────────────────
# WEATHER DATA COLLECTOR
# Gets real weather + agent metrics
# ─────────────────────────────────────────────
class WeatherData:
    def __init__(self):
        self.temp_c = 0.0
        self.temp_f = 0.0
        self.condition = "Clear"
        self.city = "Neo-Tokyo"
        self.humidity = 0
        self.wind_kph = 0.0
        self.pressure_mb = 1013.0
        self.icon = "🌙"
        self.last_updated = ""
        self._lock = threading.Lock()

    def fetch(self):
        """Fetch real weather from wttr.in (no API key needed)."""
        try:
            r = requests.get(
                "https://wttr.in/?format=j1",
                timeout=5
            )
            if r.status_code == 200:
                data = r.json()
                current = data["current_condition"][0]
                with self._lock:
                    self.temp_c = float(current["temp_C"])
                    self.temp_f = float(current["temp_F"])
                    self.condition = current["weatherDesc"][0]["value"]
                    self.humidity = int(current["humidity"])
                    self.wind_kph = float(current["windspeedKmph"])
                    self.pressure_mb = float(current["pressure"])
                    self.last_updated = datetime.now().strftime("%H:%M")
                    self.icon = self._weather_icon(self.condition)
        except Exception:
            # Offline — use agent metrics as weather
            pass

    def _weather_icon(self, condition: str) -> str:
        c = condition.lower()
        if "thunder" in c: return "⛈️"
        if "rain" in c or "drizzle" in c: return "🌧️"
        if "snow" in c: return "❄️"
        if "cloud" in c or "overcast" in c: return "☁️"
        if "fog" in c or "mist" in c: return "🌫️"
        if "clear" in c or "sunny" in c: return "☀️"
        return "🌙"

    def get_agent_pressure(self) -> float:
        """Mater's build health as pressure (0-100)."""
        # 100 = perfect build, lower = issues
        return 98.0  # Updated by Mater when it runs

    def get_agent_temp(self) -> float:
        """Batty's PC health as temperature."""
        try:
            cpu = psutil.cpu_percent(interval=0.1)
            return cpu
        except Exception:
            return 0.0

    def get_agent_wind(self) -> float:
        """Vera's routing activity as wind speed."""
        try:
            net = psutil.net_io_counters()
            return round((net.bytes_sent + net.bytes_recv) / 1024 / 1024, 1)
        except Exception:
            return 0.0


# ─────────────────────────────────────────────
# WEATHER WIDGET
# ─────────────────────────────────────────────
class WeatherWidget:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.weather = WeatherData()
        self._setup_window()
        self._build_ui()
        self._start_updates()

    def _setup_window(self):
        self.root.title("BUBO OS — World")
        self.root.configure(bg=COLORS["bg"])
        self.root.attributes("-topmost", True)
        self.root.attributes("-alpha", 0.92)
        self.root.overrideredirect(True)  # No title bar

        # Position top-right corner
        sw = self.root.winfo_screenwidth()
        w, h = 320, 420
        x = sw - w - 20
        y = 20
        self.root.geometry(f"{w}x{h}+{x}+{y}")

        # Drag support
        self.root.bind("<Button-1>", self._start_drag)
        self.root.bind("<B1-Motion>", self._drag)
        self._drag_x = 0
        self._drag_y = 0

    def _start_drag(self, e):
        self._drag_x = e.x
        self._drag_y = e.y

    def _drag(self, e):
        x = self.root.winfo_x() + e.x - self._drag_x
        y = self.root.winfo_y() + e.y - self._drag_y
        self.root.geometry(f"+{x}+{y}")

    def _build_ui(self):
        # ── Header ──
        header = tk.Frame(self.root, bg=COLORS["bg"], pady=8)
        header.pack(fill="x", padx=12)

        tk.Label(header, text="BUBO OS", font=("Courier", 9, "bold"),
                 fg=COLORS["red"], bg=COLORS["bg"]).pack(side="left")
        tk.Label(header, text="NO MAS DISADVANTAGED", font=("Courier", 7),
                 fg=COLORS["gray"], bg=COLORS["bg"]).pack(side="right")

        # ── Main weather display ──
        main_frame = tk.Frame(self.root, bg=COLORS["bg_card"],
                              relief="flat", bd=0)
        main_frame.pack(fill="x", padx=12, pady=4)

        self.icon_label = tk.Label(main_frame, text="🌙",
                                   font=("Segoe UI Emoji", 32),
                                   fg=COLORS["white"], bg=COLORS["bg_card"])
        self.icon_label.pack(side="left", padx=12, pady=8)

        temp_frame = tk.Frame(main_frame, bg=COLORS["bg_card"])
        temp_frame.pack(side="left", pady=8)

        self.temp_label = tk.Label(temp_frame, text="--°F",
                                   font=("Courier", 24, "bold"),
                                   fg=COLORS["blue_neon"], bg=COLORS["bg_card"])
        self.temp_label.pack(anchor="w")

        self.condition_label = tk.Label(temp_frame, text="Connecting...",
                                        font=("Courier", 9),
                                        fg=COLORS["gray"], bg=COLORS["bg_card"])
        self.condition_label.pack(anchor="w")

        self.city_label = tk.Label(temp_frame, text="Neo-Tokyo",
                                   font=("Courier", 8),
                                   fg=COLORS["red_dim"], bg=COLORS["bg_card"])
        self.city_label.pack(anchor="w")

        # ── Divider ──
        tk.Frame(self.root, bg=COLORS["border"], height=1).pack(
            fill="x", padx=12, pady=4)

        # ── Agent weather metrics ──
        agents_label = tk.Label(self.root, text="CREW STATUS",
                                font=("Courier", 8, "bold"),
                                fg=COLORS["gray"], bg=COLORS["bg"])
        agents_label.pack(anchor="w", padx=14)

        self.agent_frame = tk.Frame(self.root, bg=COLORS["bg"])
        self.agent_frame.pack(fill="x", padx=12, pady=4)

        self.agent_rows = {}
        agents = [
            ("🦅", "Mater",  "pressure",    "Build Health",    COLORS["blue_neon"]),
            ("🦇", "Batty",  "temperature", "PC Health",       COLORS["green"]),
            ("🏍️", "Kami",   "forecast",    "Task Queue",      COLORS["red"]),
            ("🐦‍⬛", "Corvus", "clouds",      "Learning",        COLORS["purple"]),
            ("🌊", "Vera",   "wind",        "Routing",         COLORS["yellow"]),
            ("🦉", "Bubo",   "visibility",  "OS State",        COLORS["white"]),
        ]

        for bird, name, metric, label, color in agents:
            row = tk.Frame(self.agent_frame, bg=COLORS["bg"], pady=2)
            row.pack(fill="x")

            tk.Label(row, text=f"{bird} {name:8}", font=("Courier", 9),
                     fg=color, bg=COLORS["bg"], width=12, anchor="w").pack(side="left")

            bar_frame = tk.Frame(row, bg=COLORS["bg_panel"],
                                 width=120, height=6)
            bar_frame.pack(side="left", padx=4)
            bar_frame.pack_propagate(False)

            bar = tk.Frame(bar_frame, bg=color, height=6)
            bar.place(x=0, y=0, relwidth=0.8, height=6)

            val_label = tk.Label(row, text="--", font=("Courier", 8),
                                 fg=COLORS["gray"], bg=COLORS["bg"], width=8)
            val_label.pack(side="left")

            self.agent_rows[name] = {"bar": bar, "val": val_label, "color": color}

        # ── Divider ──
        tk.Frame(self.root, bg=COLORS["border"], height=1).pack(
            fill="x", padx=12, pady=4)

        # ── Bottom bar ──
        bottom = tk.Frame(self.root, bg=COLORS["bg"])
        bottom.pack(fill="x", padx=12, pady=4)

        self.time_label = tk.Label(bottom, text="",
                                   font=("Courier", 8),
                                   fg=COLORS["gray"], bg=COLORS["bg"])
        self.time_label.pack(side="left")

        # Summon Kami button
        kami_btn = tk.Label(bottom, text="[ SUMMON KAMI ]",
                            font=("Courier", 8, "bold"),
                            fg=COLORS["red"], bg=COLORS["bg"],
                            cursor="hand2")
        kami_btn.pack(side="right")
        kami_btn.bind("<Button-1>", lambda e: self._summon_kami())

        # Close button
        close_btn = tk.Label(bottom, text="✕",
                             font=("Courier", 10),
                             fg=COLORS["gray"], bg=COLORS["bg"],
                             cursor="hand2")
        close_btn.pack(side="right", padx=8)
        close_btn.bind("<Button-1>", lambda e: self.root.destroy())

    def _summon_kami(self):
        """Summon Kami — plays bike sound then opens chat."""
        kami_path = os.path.join(os.path.dirname(__file__), "..", "kami", "kami.py")
        if os.path.exists(kami_path):
            subprocess.Popen([sys.executable, kami_path])
        else:
            print("[WORLD] Kami not found — build kami.py first")

    def _start_updates(self):
        """Start background update threads."""
        threading.Thread(target=self._weather_loop, daemon=True).start()
        threading.Thread(target=self._agent_loop, daemon=True).start()
        self._tick()

    def _weather_loop(self):
        while True:
            self.weather.fetch()
            time.sleep(300)  # Refresh every 5 minutes

    def _agent_loop(self):
        while True:
            self._update_agent_metrics()
            time.sleep(5)

    def _update_agent_metrics(self):
        """Update agent metric bars with live data."""
        cpu = self.weather.get_agent_temp()
        wind = self.weather.get_agent_wind()
        pressure = self.weather.get_agent_pressure()

        metrics = {
            "Mater":  (pressure, f"{pressure:.0f}%"),
            "Batty":  (cpu,      f"{cpu:.0f}%"),
            "Kami":   (50.0,     "ready"),
            "Corvus": (30.0,     "learning"),
            "Vera":   (min(wind, 100), f"{wind:.1f}MB"),
            "Bubo":   (95.0,     "running"),
        }

        for name, (value, display) in metrics.items():
            if name in self.agent_rows:
                row = self.agent_rows[name]
                pct = max(0.0, min(1.0, value / 100.0))
                try:
                    row["bar"].place(x=0, y=0, relwidth=pct, height=6)
                    row["val"].config(text=display)
                    # Color shifts red if critical
                    if value > 85:
                        row["bar"].config(bg=COLORS["red"])
                    elif value > 60:
                        row["bar"].config(bg=COLORS["yellow"])
                    else:
                        row["bar"].config(bg=row["color"])
                except Exception:
                    pass

    def _tick(self):
        """Update clock and weather display every second."""
        now = datetime.now()
        self.time_label.config(text=now.strftime("%H:%M:%S"))

        # Update weather display
        with self.weather._lock:
            if self.weather.temp_f:
                self.temp_label.config(text=f"{self.weather.temp_f:.0f}°F")
                self.condition_label.config(text=self.weather.condition)
                self.icon_label.config(text=self.weather.icon)

        self.root.after(1000, self._tick)


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    app = WeatherWidget(root)
    root.mainloop()
