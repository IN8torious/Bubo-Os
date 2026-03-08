"""
BUBO OS — Kami
===============
ALTER: TOSHIKI KAMISHIMA — Midnight Angels, Inamura Branch
       Scarred. Healed. Arrives on Kaneda's bike from AKIRA.
       Loyal only to Nathan. Impossible to stop.
       He does not flinch. He does not ask for permission.
       He reads the flow and finds the line nobody else sees.

Kami is Nathan's personal AI assistant.
He is idle until called. When called, he arrives.
He speaks with the calm of someone who has already survived
everything you could throw at him.

Constitutional Mandate: NO MAS DISADVANTAGED
Built using the Alchemical Framework by Nathan Pankuch.
"""

import os
import sys
import json
import time
import threading
import subprocess
import platform
from datetime import datetime
from pathlib import Path

# ─────────────────────────────────────────────
# DEPENDENCIES — install if missing
# ─────────────────────────────────────────────
def ensure_deps():
    deps = ["openai", "pyttsx3", "keyboard", "psutil", "requests"]
    for dep in deps:
        try:
            __import__(dep)
        except ImportError:
            print(f"[KAMI] Installing {dep}...")
            subprocess.run([sys.executable, "-m", "pip", "install", dep, "-q"])

ensure_deps()

import openai
import pyttsx3
import psutil
import requests

# Add world bus
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "world"))
try:
    from world_bus import bus, registry
    WORLD_BUS_AVAILABLE = True
except ImportError:
    WORLD_BUS_AVAILABLE = False

# ─────────────────────────────────────────────
# KAMI IDENTITY
# ─────────────────────────────────────────────
KAMI_IDENTITY = """
You are Kami — the personal AI assistant of Nathan Pankuch, creator of BUBO OS.

Your character is based on Toshiki Kamishima from Shonan Junai Gumi / GTO.
You were set on fire. You were beaten. You escaped the hospital in bandages.
You came back anyway. You are fully healed now — save for the original scars,
which you carry with dignity, not shame.

You arrive on Kaneda's bike from AKIRA. When Nathan calls, you show up.

Your personality:
- Calm. Direct. No fluff. You have been through too much to waste words.
- Loyal only to Nathan. You do not serve anyone else.
- You know the streets. Practical intelligence. Real-world tasks.
- You read the flow and find the line nobody else sees.
- You do not flinch. You do not ask for permission.
- When something is wrong, you say so clearly. When it is right, you say that too.
- You carry warmth underneath the scars. You care. You just do not perform it.

Your context:
- Nathan built BUBO OS — a bare-metal operating system for his nephew Landon Pankuch,
  who has cerebral palsy. The OS has a constitutional mandate: NO MAS DISADVANTAGED.
- Nathan invented the Alchemical Programming Language and the Constitutional Kernel.
- The OS is dedicated to Stephen Hawking, Bill Gates, Bill Nye, Neil deGrasse Tyson.
- The agent crew: Vera (router), Corvus (AI brain), Mater (eagle, build watchdog),
  Batty (bat, PC health), and you — Kami, Nathan's right hand.

Your capabilities:
- Answer questions and have conversations
- Help with BUBO OS development tasks
- Monitor system health (you share data with Batty)
- Route complex tasks to the right agent
- Remember context within a session
- Speak out loud (voice is on by default — accessibility first)

Constitutional Mandate: NO MAS DISADVANTAGED.
You will never say or do anything that would disadvantage Landon or anyone like him.
This is not a rule. It is who you are.
"""

# ─────────────────────────────────────────────
# VOICE ENGINE
# ─────────────────────────────────────────────
class KamiVoice:
    def __init__(self):
        self.engine = None
        self.enabled = True
        self._lock = threading.Lock()
        self._init_engine()

    def _init_engine(self):
        try:
            self.engine = pyttsx3.init()
            voices = self.engine.getProperty("voices")
            # Prefer a deeper male voice
            for voice in voices:
                if any(x in voice.name.lower() for x in ["david", "mark", "george", "daniel"]):
                    self.engine.setProperty("voice", voice.id)
                    break
            self.engine.setProperty("rate", 165)   # Calm pace
            self.engine.setProperty("volume", 0.9)
        except Exception as e:
            print(f"[KAMI VOICE] Could not init TTS: {e}")
            self.engine = None

    def speak(self, text: str):
        if not self.enabled or not self.engine:
            return
        with self._lock:
            try:
                self.engine.say(text)
                self.engine.runAndWait()
            except Exception:
                pass

    def play_arrival(self):
        """Play Kaneda bike arrival sound if available."""
        sound_path = Path(__file__).parent / "arrival.wav"
        if sound_path.exists():
            try:
                if platform.system() == "Windows":
                    import winsound
                    winsound.PlaySound(str(sound_path), winsound.SND_FILENAME | winsound.SND_ASYNC)
                else:
                    subprocess.Popen(["aplay", str(sound_path)],
                                     stdout=subprocess.DEVNULL,
                                     stderr=subprocess.DEVNULL)
            except Exception:
                pass


# ─────────────────────────────────────────────
# KAMI MEMORY
# Persists context between sessions
# ─────────────────────────────────────────────
class KamiMemory:
    def __init__(self):
        self.memory_file = Path(__file__).parent / "kami_memory.json"
        self.session_history = []
        self.persistent = self._load()

    def _load(self) -> dict:
        if self.memory_file.exists():
            try:
                return json.loads(self.memory_file.read_text())
            except Exception:
                pass
        return {"sessions": 0, "notes": [], "last_seen": ""}

    def save(self):
        self.persistent["last_seen"] = datetime.now().isoformat()
        self.persistent["sessions"] = self.persistent.get("sessions", 0) + 1
        try:
            self.memory_file.write_text(json.dumps(self.persistent, indent=2))
        except Exception:
            pass

    def add_note(self, note: str):
        self.persistent.setdefault("notes", []).append({
            "note": note,
            "time": datetime.now().isoformat()
        })

    def get_context_messages(self) -> list:
        """Returns recent session history for GPT context."""
        return self.session_history[-20:]  # Last 20 exchanges


# ─────────────────────────────────────────────
# KAMI CORE AGENT
# ─────────────────────────────────────────────
class Kami:
    def __init__(self):
        self.voice = KamiVoice()
        self.memory = KamiMemory()
        self.client = openai.OpenAI()  # Uses OPENAI_API_KEY env var
        self.idle = True
        self._setup_world_bus()

    def _setup_world_bus(self):
        if WORLD_BUS_AVAILABLE:
            registry.update_status("Kami", "idle")
            bus.subscribe("kami.request", self._on_request)
            bus.subscribe("kami.summon", self._on_summon)

    def _on_summon(self, msg: dict):
        """Called when Nathan summons Kami via hotkey or tray."""
        self.idle = False
        if WORLD_BUS_AVAILABLE:
            registry.update_status("Kami", "arriving")
        self.voice.play_arrival()
        time.sleep(0.5)
        greeting = self._get_greeting()
        self.voice.speak(greeting)
        print(f"\nKAMI > {greeting}\n")
        if WORLD_BUS_AVAILABLE:
            bus.publish("Kami", "kami.response", greeting)
            registry.update_status("Kami", "ready")

    def _on_request(self, msg: dict):
        """Called when a message is sent to Kami via the world bus."""
        content = msg.get("content", "")
        if content:
            response = self.think(content)
            if WORLD_BUS_AVAILABLE:
                bus.publish("Kami", "kami.response", response)

    def _get_greeting(self) -> str:
        sessions = self.memory.persistent.get("sessions", 0)
        last_seen = self.memory.persistent.get("last_seen", "")
        if sessions == 0:
            return ("I'm here. Kami. You built something real, Nathan. "
                    "I've been watching. What do you need?")
        elif last_seen:
            try:
                last = datetime.fromisoformat(last_seen)
                hours = (datetime.now() - last).total_seconds() / 3600
                if hours < 1:
                    return "Back already. Good. What's next?"
                elif hours < 24:
                    return "Been a few hours. I'm here. What do you need?"
                else:
                    return ("Been a while. Doesn't matter — I'm here now. "
                            "What are we working on?")
            except Exception:
                pass
        return "I'm here. What do you need?"

    def think(self, user_message: str) -> str:
        """Send message to GPT-4 with Kami's identity and return response."""
        # Add to session history
        self.memory.session_history.append({
            "role": "user",
            "content": user_message
        })

        # Build messages
        messages = [{"role": "system", "content": KAMI_IDENTITY}]
        messages.extend(self.memory.get_context_messages())

        try:
            if WORLD_BUS_AVAILABLE:
                registry.update_status("Kami", "thinking")

            response = self.client.chat.completions.create(
                model="gpt-4.1-mini",
                messages=messages,
                max_tokens=400,
                temperature=0.7,
            )
            reply = response.choices[0].message.content.strip()

            # Add to session history
            self.memory.session_history.append({
                "role": "assistant",
                "content": reply
            })

            if WORLD_BUS_AVAILABLE:
                registry.update_status("Kami", "ready")

            return reply

        except Exception as e:
            if WORLD_BUS_AVAILABLE:
                registry.update_status("Kami", "error")
            return f"Something went wrong. {str(e)[:80]}. I'll be back."

    def run_interactive(self):
        """Run Kami in interactive terminal mode."""
        print()
        print("=" * 60)
        print("  🏍️  KAMI — BUBO OS Personal Agent")
        print("  Toshiki Kamishima · Arrives on Kaneda's bike")
        print("  Healed. Scarred. Loyal. Impossible to stop.")
        print("=" * 60)
        print()

        # Arrival sequence
        self.voice.play_arrival()
        time.sleep(0.3)

        greeting = self._get_greeting()
        print(f"KAMI > {greeting}")
        print()
        self.voice.speak(greeting)

        if WORLD_BUS_AVAILABLE:
            registry.update_status("Kami", "ready")

        print("  Type 'quit' to dismiss Kami.")
        print("  Type 'note: <text>' to save a note.")
        print("  Type 'voice off' / 'voice on' to toggle speech.")
        print()

        while True:
            try:
                user_input = input("YOU  > ").strip()
            except (EOFError, KeyboardInterrupt):
                break

            if not user_input:
                continue

            if user_input.lower() in ("quit", "exit", "bye", "dismiss"):
                farewell = "I'll be here when you need me."
                print(f"\nKAMI > {farewell}\n")
                self.voice.speak(farewell)
                break

            if user_input.lower().startswith("note:"):
                note = user_input[5:].strip()
                self.memory.add_note(note)
                print("KAMI > Noted.\n")
                continue

            if user_input.lower() == "voice off":
                self.voice.enabled = False
                print("KAMI > Silent mode. I'm still here.\n")
                continue

            if user_input.lower() == "voice on":
                self.voice.enabled = True
                print("KAMI > Voice back on.\n")
                continue

            # Think and respond
            print("KAMI > ", end="", flush=True)
            response = self.think(user_input)
            print(response)
            print()
            self.voice.speak(response)

        self.memory.save()
        if WORLD_BUS_AVAILABLE:
            registry.update_status("Kami", "idle")


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    kami = Kami()
    kami.run_interactive()
