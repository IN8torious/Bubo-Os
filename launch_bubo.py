"""
BUBO OS — Unified Launcher
===========================
Starts the whole world.
Weather widget. Pull-down tray. All agents.
One command.

Usage:
    python launch_bubo.py              # Full GUI (weather + tray)
    python launch_bubo.py --kami       # Just Kami in the terminal
    python launch_bubo.py --tray       # Just the pull-down tray
    python launch_bubo.py --weather    # Just the weather widget

NO MAS DISADVANTAGED.
"""

import sys
import os
import threading
import subprocess
import time

ROOT = os.path.dirname(__file__)
sys.path.insert(0, ROOT)
sys.path.insert(0, os.path.join(ROOT, "agents", "world"))
sys.path.insert(0, os.path.join(ROOT, "agents", "kami"))

# ─────────────────────────────────────────────
# AGENT BACKGROUND THREADS
# ─────────────────────────────────────────────
def start_background_agents():
    """Start Mater and Batty as background monitoring threads."""
    try:
        from agents.world.world_bus import bus, registry
        import psutil

        def mater_loop():
            """Mater watches for build activity."""
            registry.update_status("Mater", "watching")
            while True:
                # Check if any build process is running
                build_procs = ["make", "cmake", "gcc", "g++", "ld", "ninja"]
                active = [p.name() for p in psutil.process_iter(["name"])
                          if p.info["name"] in build_procs]
                if active:
                    status = f"building: {active[0]}"
                    registry.update_status("Mater", status)
                    bus.publish("Mater", "mater.report", f"Build active: {', '.join(active)}")
                else:
                    registry.update_status("Mater", "watching")
                time.sleep(5)

        def batty_loop():
            """Batty monitors PC health."""
            registry.update_status("Batty", "healthy")
            while True:
                cpu = psutil.cpu_percent(interval=1)
                mem = psutil.virtual_memory().percent
                temp_str = ""
                try:
                    temps = psutil.sensors_temperatures()
                    if temps:
                        for name, entries in temps.items():
                            if entries:
                                temp_str = f" {entries[0].current:.0f}°C"
                                break
                except Exception:
                    pass

                if cpu > 85 or mem > 90:
                    status = f"⚠ CPU:{cpu:.0f}% MEM:{mem:.0f}%"
                    registry.update_status("Batty", status)
                    bus.publish("Batty", "batty.alert",
                                f"High load — CPU:{cpu:.0f}% MEM:{mem:.0f}%{temp_str}")
                else:
                    status = f"CPU:{cpu:.0f}% MEM:{mem:.0f}%{temp_str}"
                    registry.update_status("Batty", status)
                time.sleep(8)

        def corvus_loop():
            """Corvus is the AI brain — monitors bus for routing decisions."""
            registry.update_status("Corvus", "listening")
            while True:
                time.sleep(10)

        def vera_loop():
            """Vera routes intent — always listening."""
            registry.update_status("Vera", "routing")
            while True:
                time.sleep(10)

        threading.Thread(target=mater_loop, daemon=True, name="Mater").start()
        threading.Thread(target=batty_loop, daemon=True, name="Batty").start()
        threading.Thread(target=corvus_loop, daemon=True, name="Corvus").start()
        threading.Thread(target=vera_loop, daemon=True, name="Vera").start()

        print("[BUBO] Background agents started: Mater, Batty, Corvus, Vera")

    except Exception as e:
        print(f"[BUBO] Background agents failed to start: {e}")


# ─────────────────────────────────────────────
# LAUNCH MODES
# ─────────────────────────────────────────────
def launch_full_gui():
    """Launch weather widget + pull-down tray together."""
    import tkinter as tk
    from agents.world.weather_widget import WeatherWidget
    from agents.world.pull_tray import PullTray

    start_background_agents()
    time.sleep(0.5)

    # Weather widget in its own Tk window
    weather_root = tk.Tk()
    weather_app = WeatherWidget(weather_root)

    # Pull-down tray in a Toplevel
    tray_root = tk.Toplevel(weather_root)
    tray_app = PullTray(tray_root)

    print()
    print("=" * 60)
    print("  🦉 BUBO OS — SHARED WORLD ACTIVE")
    print("  Weather widget + Pull-down tray running.")
    print("  Ctrl+Shift+K  →  Summon Kami")
    print("  Click the red bar at top  →  Toggle tray")
    print("  NO MAS DISADVANTAGED")
    print("=" * 60)
    print()

    weather_root.mainloop()


def launch_kami_only():
    """Just Kami in the terminal."""
    start_background_agents()
    from agents.kami.kami import Kami
    kami = Kami()
    kami.run_interactive()


def launch_tray_only():
    """Just the pull-down tray."""
    import tkinter as tk
    from agents.world.pull_tray import PullTray
    start_background_agents()
    root = tk.Tk()
    app = PullTray(root)
    root.mainloop()


def launch_weather_only():
    """Just the weather widget."""
    import tkinter as tk
    from agents.world.weather_widget import WeatherWidget
    root = tk.Tk()
    app = WeatherWidget(root)
    root.mainloop()


# ─────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────
if __name__ == "__main__":
    args = sys.argv[1:]

    print()
    print("  🦉 BUBO OS — Starting...")
    print("  Built by Nathan Brown for Landon Pankuch.")
    print("  NO MAS DISADVANTAGED.")
    print()

    if "--kami" in args:
        launch_kami_only()
    elif "--tray" in args:
        launch_tray_only()
    elif "--weather" in args:
        launch_weather_only()
    else:
        launch_full_gui()
