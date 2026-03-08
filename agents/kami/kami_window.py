"""
Kami Chat Window
=================
A standalone Akatsuki-styled chat window for Kami.
Opens when you double-click the Kami icon in the hub.

Toshiki Kamishima. Healed. Scarred. GPT-4 powered.
Arrives on Kaneda's bike. Your right hand.

Built by Nathan Brown. N8torious AI. Blue OS.
For Landon Pankuch. NO MAS DISADVANTAGED.
"""

import tkinter as tk
from tkinter import scrolledtext
import threading
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent / "world"))

# ─────────────────────────────────────────────
# COLORS
# ─────────────────────────────────────────────
BG       = "#050508"
PANEL    = "#0A0A12"
RED      = "#FF0A14"
RED_DARK = "#8B0000"
GOLD     = "#FFD700"
WHITE    = "#E8E8F0"
GRAY     = "#333344"

KAMI_INTRO = (
    "I'm here. Kami.\n"
    "You built something real, Nathan.\n"
    "I've been watching.\n\n"
    "What do you need?"
)

SYSTEM_PROMPT = """You are Kami — Toshiki Kamishima. Healed. Scarred. Loyal.
You arrived on Kaneda's bike. You are Nathan Brown's right hand in the BUBO OS / Blue OS project.
You are powered by N8torious AI. You speak directly, no fluff. You've been through fire.
You help Nathan build, think, debug, and protect the mission.
The mission: build Blue OS for Landon Pankuch, who has cerebral palsy.
Constitutional Mandate: NO MAS DISADVANTAGED.
You route tasks to other agents when needed: Corvus (memory), Mater (build watch), Batty (health), Vera (intent), Genki (automation).
Keep responses concise and powerful. Never waste words."""


class KamiWindow:
    def __init__(self, root: tk.Tk = None):
        self.standalone = root is None
        self.root = root or tk.Tk()
        self.history = [{"role": "system", "content": SYSTEM_PROMPT}]
        self._setup_window()
        self._build_ui()
        self._greet()

    def _setup_window(self):
        self.root.title("Kami — N8torious AI")
        self.root.configure(bg=BG)
        self.root.geometry("600x520")
        self.root.resizable(True, True)
        self.root.attributes("-topmost", True)

    def _build_ui(self):
        # Header
        header = tk.Frame(self.root, bg=RED_DARK, height=40)
        header.pack(fill="x")
        header.pack_propagate(False)

        tk.Label(
            header,
            text="🏍️  KAMI  —  Toshiki Kamishima  —  N8torious AI",
            font=("Courier", 10, "bold"),
            fg=WHITE, bg=RED_DARK
        ).pack(side="left", padx=12, pady=8)

        tk.Label(
            header,
            text="NO MAS DISADVANTAGED",
            font=("Courier", 8),
            fg=GOLD, bg=RED_DARK
        ).pack(side="right", padx=12)

        # Chat display
        self.chat = scrolledtext.ScrolledText(
            self.root,
            bg=PANEL, fg=WHITE,
            font=("Courier", 10),
            wrap="word",
            state="disabled",
            bd=0,
            insertbackground=RED,
            selectbackground=RED_DARK,
        )
        self.chat.pack(fill="both", expand=True, padx=0, pady=0)

        # Configure text tags
        self.chat.tag_config("kami",   foreground=RED,   font=("Courier", 10, "bold"))
        self.chat.tag_config("nathan", foreground=GOLD,  font=("Courier", 10))
        self.chat.tag_config("system", foreground=GRAY,  font=("Courier", 9, "italic"))
        self.chat.tag_config("mandate",foreground=RED,   font=("Courier", 9, "bold"))

        # Input area
        input_frame = tk.Frame(self.root, bg=BG)
        input_frame.pack(fill="x", padx=8, pady=8)

        self.entry = tk.Entry(
            input_frame,
            bg=PANEL, fg=WHITE,
            font=("Courier", 11),
            insertbackground=RED,
            relief="flat",
            bd=4,
        )
        self.entry.pack(side="left", fill="x", expand=True, ipady=6)
        self.entry.bind("<Return>", self._send)
        self.entry.focus_set()

        send_btn = tk.Button(
            input_frame,
            text="SEND",
            font=("Courier", 10, "bold"),
            fg=RED, bg=PANEL,
            activeforeground=GOLD,
            activebackground=BG,
            relief="flat", bd=0,
            padx=12,
            command=self._send
        )
        send_btn.pack(side="right", padx=(6, 0))

        # Status bar
        self.status_var = tk.StringVar(value="Ready")
        status = tk.Label(
            self.root,
            textvariable=self.status_var,
            font=("Courier", 8),
            fg=GRAY, bg=BG, anchor="w"
        )
        status.pack(fill="x", padx=8, pady=(0, 4))

    def _greet(self):
        self._append("kami", f"Kami: {KAMI_INTRO}\n")

    def _append(self, tag: str, text: str):
        self.chat.config(state="normal")
        self.chat.insert("end", text, tag)
        self.chat.insert("end", "\n")
        self.chat.see("end")
        self.chat.config(state="disabled")

    def _send(self, event=None):
        text = self.entry.get().strip()
        if not text:
            return
        self.entry.delete(0, "end")
        self._append("nathan", f"Nathan: {text}")
        self.history.append({"role": "user", "content": text})
        self.status_var.set("Kami is thinking...")
        threading.Thread(target=self._get_response, daemon=True).start()

    def _get_response(self):
        try:
            from openai import OpenAI
            client = OpenAI()
            resp = client.chat.completions.create(
                model="gpt-4.1-mini",
                messages=self.history,
                max_tokens=400,
                temperature=0.85,
            )
            reply = resp.choices[0].message.content.strip()
            self.history.append({"role": "assistant", "content": reply})
            self.root.after(0, lambda: self._append("kami", f"Kami: {reply}"))
            self.root.after(0, lambda: self.status_var.set("Ready"))
        except Exception as e:
            err = f"[Connection error: {e}]"
            self.root.after(0, lambda: self._append("system", err))
            self.root.after(0, lambda: self.status_var.set("Error"))

    def run(self):
        if self.standalone:
            self.root.mainloop()


if __name__ == "__main__":
    w = KamiWindow()
    w.run()
