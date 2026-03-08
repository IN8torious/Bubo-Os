"""
BUBO OS — Pull-Down Tray
=========================
One click. The whole crew slides down.
All agents. Their status. Kami's chat.
Click again. It disappears.

Non-intrusive. Always there when you need it.
AKIRA theme. Neo-Tokyo neon on black.

Built using the Alchemical Framework by Nathan Brown.
NO MAS DISADVANTAGED.
"""

import tkinter as tk
from tkinter import scrolledtext
import threading
import time
import sys
import os
import subprocess
import psutil
from datetime import datetime

sys.path.insert(0, os.path.dirname(__file__))
from world_bus import bus, registry

# ─────────────────────────────────────────────
# AKIRA COLORS
# ─────────────────────────────────────────────
C = {
    "bg":       "#050508",
    "panel":    "#0A0A12",
    "card":     "#0F0F1A",
    "red":      "#FF0A14",
    "red_dim":  "#8B0000",
    "neon":     "#00D4FF",
    "neon_dim": "#004466",
    "yellow":   "#FFD700",
    "green":    "#00FF88",
    "white":    "#E8E8F0",
    "gray":     "#444455",
    "purple":   "#9D00FF",
    "border":   "#1A1A2E",
}

TRAY_HEIGHT = 380
TRAY_COLLAPSED = 4  # Just a thin red line when hidden


class PullTray:
    def __init__(self, root: tk.Tk):
        self.root = root
        self.expanded = False
        self.animating = False
        self._setup_window()
        self._build_ui()
        self._start_updates()
        # Global hotkey: Ctrl+Shift+K summons Kami
        self.root.bind_all("<Control-Shift-K>", lambda e: self._summon_kami())

    def _setup_window(self):
        self.root.title("BUBO OS Tray")
        self.root.configure(bg=C["red"])
        self.root.attributes("-topmost", True)
        self.root.attributes("-alpha", 0.95)
        self.root.overrideredirect(True)

        sw = self.root.winfo_screenwidth()
        self.tray_width = sw  # Full width
        self.root.geometry(f"{self.tray_width}x{TRAY_COLLAPSED}+0+0")

        # Click the thin red line to expand
        self.root.bind("<Button-1>", self._on_click)

    def _on_click(self, event):
        if not self.animating:
            if self.expanded:
                self._collapse()
            else:
                self._expand()

    def _expand(self):
        self.animating = True
        self.expanded = True
        self.root.geometry(f"{self.tray_width}x{TRAY_HEIGHT}+0+0")
        self.content_frame.pack(fill="both", expand=True)
        self.animating = False

    def _collapse(self):
        self.animating = True
        self.expanded = False
        self.content_frame.pack_forget()
        self.root.geometry(f"{self.tray_width}x{TRAY_COLLAPSED}+0+0")
        self.animating = False

    def _build_ui(self):
        # ── Thin red strip (always visible) ──
        strip = tk.Frame(self.root, bg=C["red"], height=TRAY_COLLAPSED)
        strip.pack(fill="x")
        strip.bind("<Button-1>", self._on_click)

        tk.Label(strip, text="▼ BUBO OS  ·  NO MAS DISADVANTAGED  ·  Ctrl+Shift+K = KAMI  ▼",
                 font=("Courier", 7, "bold"),
                 fg=C["bg"], bg=C["red"]).pack()

        # ── Content (hidden until expanded) ──
        self.content_frame = tk.Frame(self.root, bg=C["bg"])
        # Not packed yet — only shows when expanded

        # Header
        header = tk.Frame(self.content_frame, bg=C["panel"], pady=6)
        header.pack(fill="x")

        tk.Label(header, text="🦉 BUBO OS — AGENT CREW",
                 font=("Courier", 11, "bold"),
                 fg=C["red"], bg=C["panel"]).pack(side="left", padx=16)

        tk.Label(header, text=datetime.now().strftime("%Y-%m-%d"),
                 font=("Courier", 9),
                 fg=C["gray"], bg=C["panel"]).pack(side="right", padx=16)

        close_btn = tk.Label(header, text="  ✕  ",
                             font=("Courier", 11, "bold"),
                             fg=C["gray"], bg=C["panel"], cursor="hand2")
        close_btn.pack(side="right")
        close_btn.bind("<Button-1>", lambda e: self._collapse())

        # ── Two-column layout: agents left, Kami chat right ──
        columns = tk.Frame(self.content_frame, bg=C["bg"])
        columns.pack(fill="both", expand=True, padx=8, pady=4)

        # Left: Agent status cards
        left = tk.Frame(columns, bg=C["bg"], width=480)
        left.pack(side="left", fill="y", padx=(0, 8))
        left.pack_propagate(False)

        tk.Label(left, text="CREW STATUS",
                 font=("Courier", 8, "bold"),
                 fg=C["gray"], bg=C["bg"]).pack(anchor="w", pady=(4, 2))

        self.agent_cards = {}
        agents = [
            ("🦅", "Mater",  C["neon"],   "Build Watchdog — Eagle"),
            ("🦇", "Batty",  C["green"],  "PC Health — Bat"),
            ("🏍️", "Kami",   C["red"],    "Personal Agent — Kaneda's Bike"),
            ("🐦‍⬛", "Corvus", C["purple"], "AI Brain — Raven"),
            ("🌊", "Vera",   C["yellow"], "Intent Router"),
            ("🦉", "Bubo",   C["white"],  "The OS — Owl"),
        ]

        # Two rows of 3
        row_frame = None
        for i, (bird, name, color, desc) in enumerate(agents):
            if i % 3 == 0:
                row_frame = tk.Frame(left, bg=C["bg"])
                row_frame.pack(fill="x", pady=2)

            card = tk.Frame(row_frame, bg=C["card"],
                            relief="flat", bd=0, padx=8, pady=6)
            card.pack(side="left", fill="both", expand=True, padx=2)

            tk.Label(card, text=bird, font=("Segoe UI Emoji", 18),
                     fg=color, bg=C["card"]).pack()
            tk.Label(card, text=name, font=("Courier", 9, "bold"),
                     fg=color, bg=C["card"]).pack()

            status_lbl = tk.Label(card, text="idle",
                                  font=("Courier", 7),
                                  fg=C["gray"], bg=C["card"])
            status_lbl.pack()

            self.agent_cards[name] = status_lbl

        # Right: Kami chat
        right = tk.Frame(columns, bg=C["panel"])
        right.pack(side="left", fill="both", expand=True)

        kami_header = tk.Frame(right, bg=C["panel"])
        kami_header.pack(fill="x", padx=8, pady=(4, 0))

        tk.Label(kami_header, text="🏍️ KAMI",
                 font=("Courier", 10, "bold"),
                 fg=C["red"], bg=C["panel"]).pack(side="left")

        tk.Label(kami_header, text="healed · loyal · impossible to stop",
                 font=("Courier", 7),
                 fg=C["gray"], bg=C["panel"]).pack(side="left", padx=8)

        # Chat display
        self.chat_display = scrolledtext.ScrolledText(
            right,
            bg=C["card"], fg=C["white"],
            font=("Courier", 9),
            relief="flat", bd=0,
            height=10, wrap="word",
            state="disabled"
        )
        self.chat_display.pack(fill="both", expand=True, padx=8, pady=4)

        # Tag colors
        self.chat_display.tag_config("kami", foreground=C["red"])
        self.chat_display.tag_config("user", foreground=C["neon"])
        self.chat_display.tag_config("system", foreground=C["gray"])

        # Input row
        input_row = tk.Frame(right, bg=C["panel"])
        input_row.pack(fill="x", padx=8, pady=(0, 4))

        self.chat_input = tk.Entry(
            input_row,
            bg=C["card"], fg=C["white"],
            font=("Courier", 9),
            relief="flat", bd=4,
            insertbackground=C["red"]
        )
        self.chat_input.pack(side="left", fill="x", expand=True)
        self.chat_input.bind("<Return>", self._send_to_kami)

        send_btn = tk.Label(input_row, text=" ▶ ",
                            font=("Courier", 9, "bold"),
                            fg=C["red"], bg=C["card"],
                            cursor="hand2", padx=4)
        send_btn.pack(side="left")
        send_btn.bind("<Button-1>", self._send_to_kami)

        # Summon button
        summon_btn = tk.Label(right,
                              text="[ SUMMON KAMI — Ctrl+Shift+K ]",
                              font=("Courier", 8, "bold"),
                              fg=C["red"], bg=C["panel"],
                              cursor="hand2")
        summon_btn.pack(pady=2)
        summon_btn.bind("<Button-1>", lambda e: self._summon_kami())

        # Welcome message
        self._chat_append("system",
            "Kami is idle. Type a message or press Ctrl+Shift+K to summon him.\n")

    def _chat_append(self, tag: str, text: str):
        self.chat_display.config(state="normal")
        prefix = {"kami": "KAMI > ", "user": "YOU  > ", "system": "      "}.get(tag, "")
        self.chat_display.insert("end", prefix + text + "\n", tag)
        self.chat_display.see("end")
        self.chat_display.config(state="disabled")

    def _send_to_kami(self, event=None):
        text = self.chat_input.get().strip()
        if not text:
            return
        self.chat_input.delete(0, "end")
        self._chat_append("user", text)
        # Publish to world bus
        bus.publish("Tray", "kami.request", text)
        # Subscribe to kami response (one-shot)
        threading.Thread(target=self._wait_for_kami_response, daemon=True).start()

    def _wait_for_kami_response(self):
        """Wait for Kami to respond via the bus."""
        response_received = threading.Event()
        def on_response(msg):
            if msg["channel"] == "kami.response":
                self.root.after(0, lambda: self._chat_append("kami", msg["content"]))
                response_received.set()
        bus.subscribe("kami.response", on_response)
        # Timeout after 15 seconds
        if not response_received.wait(timeout=15):
            self.root.after(0, lambda: self._chat_append(
                "system", "Kami is on the road. Try again in a moment.\n"))

    def _summon_kami(self):
        """Summon Kami — expand tray and focus chat."""
        if not self.expanded:
            self._expand()
        self.chat_input.focus_set()
        self._chat_append("system", "Kami summoned. He arrives on Kaneda's bike.\n")
        bus.publish("Tray", "kami.summon", "Nathan summoned Kami")

    def _start_updates(self):
        threading.Thread(target=self._update_loop, daemon=True).start()
        # Subscribe to all bus messages for display
        bus.subscribe_all(self._on_bus_message)

    def _update_loop(self):
        while True:
            agents = registry.get_all()
            for agent in agents:
                name = agent["name"]
                status = agent["status"]
                if name in self.agent_cards:
                    try:
                        self.root.after(0, lambda n=name, s=status:
                            self.agent_cards[n].config(text=s))
                    except Exception:
                        pass
            time.sleep(3)

    def _on_bus_message(self, msg: dict):
        """Show important bus messages in the chat."""
        channel = msg.get("channel", "")
        if channel in ("kami.response", "mater.report", "batty.alert"):
            sender = msg.get("sender", "?")
            content = msg.get("content", "")
            tag = "kami" if "kami" in sender.lower() else "system"
            try:
                self.root.after(0, lambda: self._chat_append(tag, content))
            except Exception:
                pass


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    root = tk.Tk()
    app = PullTray(root)
    root.mainloop()
