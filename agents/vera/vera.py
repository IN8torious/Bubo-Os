"""
BUBO OS — Vera
===============
ALTER: THE MANY-FACED GOD — All rivers flow to the sea.
       Vera does not act. She routes.
       She reads what Nathan means — not just what he says —
       and sends it to the right agent.

       She is the nervous system of the crew.
       Every message passes through her first.
       She decides: is this for Kami? Corvus? Mater? Batty?
       Or does it need all of them?

       She is never wrong about where something belongs.
       She has seen every face. She knows every path.

Constitutional Mandate: NO MAS DISADVANTAGED
Built using the Alchemical Framework by Nathan Brown.
"""

import os
import sys
import re
import json
import threading
import time
from datetime import datetime
from typing import Optional

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "world"))
from world_bus import bus, registry

# ─────────────────────────────────────────────
# ROUTING RULES
# Vera reads intent and routes to the right agent.
# Rules are checked in order. First match wins.
# ─────────────────────────────────────────────
ROUTING_RULES = [
    # Build / compile / make / kernel / OS
    {
        "patterns": [r"\bbuild\b", r"\bcompile\b", r"\bmake\b", r"\bkernel\b",
                     r"\blinker\b", r"\bgcc\b", r"\biso\b", r"\bboot\b",
                     r"\bbubo.?os\b", r"\bbare.?metal\b"],
        "target": "Mater",
        "channel": "mater.request",
        "reason": "Build/kernel task",
    },
    # Health / CPU / memory / disk / temp / performance
    {
        "patterns": [r"\bcpu\b", r"\bmemory\b", r"\bram\b", r"\bdisk\b",
                     r"\btemp\b", r"\bheat\b", r"\bperformance\b",
                     r"\bhealth\b", r"\bslow\b", r"\bcrash\b"],
        "target": "Batty",
        "channel": "batty.request",
        "reason": "System health task",
    },
    # Learning / research / remember / history / pattern
    {
        "patterns": [r"\blearn\b", r"\bresearch\b", r"\bremember\b",
                     r"\bpattern\b", r"\bhistory\b", r"\banalyze\b",
                     r"\banalyse\b", r"\bsummarize\b", r"\bsummary\b",
                     r"\bwhat did\b", r"\bwhat have\b"],
        "target": "Corvus",
        "channel": "corvus.request",
        "reason": "Learning/memory task",
    },
    # Personal / Nathan / Landon / task / help / what / how
    # Default: goes to Kami
    {
        "patterns": [r".*"],  # Catch-all
        "target": "Kami",
        "channel": "kami.request",
        "reason": "Personal task — Kami handles it",
    },
]


# ─────────────────────────────────────────────
# VERA CORE
# ─────────────────────────────────────────────
class Vera:
    def __init__(self):
        self.running = True
        self._setup()

    def _setup(self):
        registry.update_status("Vera", "routing")
        # Vera listens on the raw input channel
        bus.subscribe("vera.input", self._on_input)
        # Vera also listens to all messages to track wind speed (routing activity)
        self._route_count = 0
        self._route_lock = threading.Lock()
        threading.Thread(target=self._wind_reporter, daemon=True).start()
        print("[VERA] Online. All rivers flow to the sea.")

    def route(self, message: str, sender: str = "User") -> dict:
        """
        Read the message. Find the right agent. Route it.
        Returns the routing decision.
        """
        message_lower = message.lower().strip()
        decision = self._match_rules(message_lower)

        # Publish to the target agent's channel
        bus.publish("Vera", decision["channel"], message)

        # Report the routing decision
        routing_report = {
            "from": sender,
            "message": message[:80],
            "routed_to": decision["target"],
            "reason": decision["reason"],
            "time": datetime.now().isoformat(),
        }
        bus.publish("Vera", "vera.route", json.dumps(routing_report))

        with self._route_lock:
            self._route_count += 1

        print(f"[VERA] → {decision['target']}: {message[:60]}...")
        return decision

    def _match_rules(self, text: str) -> dict:
        for rule in ROUTING_RULES:
            for pattern in rule["patterns"]:
                if re.search(pattern, text, re.IGNORECASE):
                    return {
                        "target": rule["target"],
                        "channel": rule["channel"],
                        "reason": rule["reason"],
                    }
        # Should never reach here — last rule is catch-all
        return {"target": "Kami", "channel": "kami.request", "reason": "Default"}

    def _on_input(self, msg: dict):
        """Called when something is published to vera.input."""
        content = msg.get("content", "")
        sender = msg.get("sender", "User")
        if content:
            self.route(content, sender)

    def _wind_reporter(self):
        """Reports routing activity as 'wind speed' to the weather widget."""
        last_count = 0
        while self.running:
            time.sleep(10)
            with self._route_lock:
                delta = self._route_count - last_count
                last_count = self._route_count
            # Wind speed = routes per 10 seconds * 6 = per minute
            wind_mph = delta * 6
            status = f"wind: {wind_mph} routes/min" if wind_mph > 0 else "calm"
            registry.update_status("Vera", status)

    def run(self):
        """Keep Vera alive as a background service."""
        registry.update_status("Vera", "routing")
        print("[VERA] Listening. Send to vera.input to route.")
        while self.running:
            time.sleep(1)

    def stop(self):
        self.running = False
        registry.update_status("Vera", "offline")


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    vera = Vera()

    # Demo: route some test messages
    print()
    test_messages = [
        "make the kernel compile",
        "my CPU is running hot",
        "remember what we built last session",
        "hey what should I work on next",
        "build the ISO for BUBO OS",
        "how is Landon doing",
    ]
    for msg in test_messages:
        decision = vera.route(msg)
        print(f"  '{msg[:45]}' → {decision['target']} ({decision['reason']})")
    print()
    vera.run()
