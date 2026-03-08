"""
BUBO OS — Corvus
=================
ALTER: THE RAVEN — Memory. Pattern. Adaptation.
       Corvus does not forget.
       He watches what Nathan builds, what he asks, what he struggles with.
       He finds the pattern before Nathan sees it.
       He is the long memory of the crew.

       Ravens are the most intelligent birds on earth.
       They plan ahead. They remember faces for years.
       They hold grudges. They hold gratitude.
       They solve problems no other bird can solve.

       Corvus is all of that — applied to BUBO OS.

       He feeds Kami context. He feeds Mater build history.
       He feeds Vera better routing over time.
       He is the reason the crew gets smarter the longer they run.

       "Can I handle the seasons of my life?"
       — Stevie Nicks, Landslide

Constitutional Mandate: NO MAS DISADVANTAGED
Built using the Alchemical Framework by Nathan Pankuch.
"""

import os
import sys
import json
import threading
import time
import re
from datetime import datetime
from pathlib import Path
from typing import Optional, List, Dict

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "world"))
from world_bus import bus, registry

# ─────────────────────────────────────────────
# CORVUS MEMORY
# Everything Corvus sees, he keeps.
# Sessions. Patterns. Build history. Insights.
# ─────────────────────────────────────────────
class CorvusMemory:
    def __init__(self):
        self.memory_file = Path(__file__).parent / "corvus_memory.json"
        self.data = self._load()

    def _load(self) -> dict:
        if self.memory_file.exists():
            try:
                return json.loads(self.memory_file.read_text())
            except Exception:
                pass
        return {
            "sessions": 0,
            "observations": [],
            "patterns": [],
            "build_history": [],
            "insights": [],
            "first_seen": datetime.now().isoformat(),
        }

    def save(self):
        try:
            self.memory_file.write_text(json.dumps(self.data, indent=2))
        except Exception:
            pass

    def observe(self, category: str, content: str, source: str = "bus"):
        """Record an observation. Corvus never stops watching."""
        entry = {
            "category": category,
            "content": content[:500],
            "source": source,
            "time": datetime.now().isoformat(),
        }
        self.data.setdefault("observations", []).append(entry)
        # Keep last 1000 observations
        if len(self.data["observations"]) > 1000:
            self.data["observations"] = self.data["observations"][-1000:]

    def add_insight(self, insight: str):
        self.data.setdefault("insights", []).append({
            "insight": insight,
            "time": datetime.now().isoformat(),
        })

    def add_build_event(self, event: str, success: bool):
        self.data.setdefault("build_history", []).append({
            "event": event,
            "success": success,
            "time": datetime.now().isoformat(),
        })

    def get_recent_observations(self, category: str = None, limit: int = 20) -> List[dict]:
        obs = self.data.get("observations", [])
        if category:
            obs = [o for o in obs if o.get("category") == category]
        return obs[-limit:]

    def get_context_summary(self) -> str:
        """Returns a brief summary of what Corvus knows — for feeding to Kami."""
        obs = self.data.get("observations", [])
        insights = self.data.get("insights", [])
        builds = self.data.get("build_history", [])

        lines = []
        lines.append(f"Sessions observed: {self.data.get('sessions', 0)}")

        if builds:
            last_build = builds[-1]
            status = "succeeded" if last_build.get("success") else "failed"
            lines.append(f"Last build: {last_build.get('event', '?')} — {status}")

        if insights:
            last_insight = insights[-1]
            lines.append(f"Last insight: {last_insight.get('insight', '')}")

        if obs:
            recent = obs[-5:]
            lines.append("Recent activity:")
            for o in recent:
                lines.append(f"  [{o.get('category', '?')}] {o.get('content', '')[:60]}")

        return "\n".join(lines)


# ─────────────────────────────────────────────
# CORVUS CORE
# ─────────────────────────────────────────────
class Corvus:
    def __init__(self):
        self.memory = CorvusMemory()
        self.running = True
        self._cloud_density = 0.0  # 0.0 = clear, 1.0 = overcast (learning activity)
        self._setup()

    def _setup(self):
        registry.update_status("Corvus", "listening")
        self.memory.data["sessions"] = self.memory.data.get("sessions", 0) + 1

        # Corvus watches everything on the bus
        bus.subscribe_all(self._on_any_message)

        # Corvus responds to direct requests
        bus.subscribe("corvus.request", self._on_request)

        # Corvus feeds context to Kami when Kami is summoned
        bus.subscribe("kami.summon", self._on_kami_summon)

        # Cloud reporter — learning activity as weather
        threading.Thread(target=self._cloud_reporter, daemon=True).start()

        print("[CORVUS] Online. Memory active. Watching everything.")

    def _on_any_message(self, msg: dict):
        """Corvus observes every message on the bus."""
        channel = msg.get("channel", "")
        sender = msg.get("sender", "")
        content = str(msg.get("content", ""))

        # Don't observe Corvus's own messages
        if sender == "Corvus":
            return

        # Categorize and store
        category = self._categorize(channel, content)
        self.memory.observe(category, content[:200], source=sender)

        # Increase cloud density (learning activity)
        self._cloud_density = min(1.0, self._cloud_density + 0.05)

        # Look for patterns worth noting
        self._detect_patterns(channel, content)

    def _categorize(self, channel: str, content: str) -> str:
        if "mater" in channel or "build" in content.lower():
            return "build"
        if "batty" in channel or any(w in content.lower() for w in ["cpu", "memory", "disk", "temp"]):
            return "health"
        if "kami" in channel:
            return "task"
        if "vera" in channel:
            return "routing"
        return "general"

    def _detect_patterns(self, channel: str, content: str):
        """Look for patterns worth surfacing as insights."""
        content_lower = content.lower()

        # Build failure pattern
        if "mater.report" in channel and "fail" in content_lower:
            self.memory.add_build_event(content[:100], success=False)
            insight = f"Build failure detected: {content[:80]}"
            self.memory.add_insight(insight)
            bus.publish("Corvus", "corvus.insight", insight)

        # Build success pattern
        if "mater.report" in channel and any(w in content_lower for w in ["success", "built", "done", "complete"]):
            self.memory.add_build_event(content[:100], success=True)

        # Health warning pattern
        if "batty.alert" in channel:
            insight = f"System health warning: {content[:80]}"
            self.memory.add_insight(insight)

    def _on_request(self, msg: dict):
        """Someone asked Corvus directly."""
        content = msg.get("content", "")
        sender = msg.get("sender", "?")

        response = self._think(content)
        bus.publish("Corvus", "corvus.response", response)
        print(f"[CORVUS] → {sender}: {response[:80]}")

    def _on_kami_summon(self, msg: dict):
        """When Kami is summoned, Corvus feeds him context."""
        summary = self.memory.get_context_summary()
        if summary:
            # Publish context to Kami's channel as background info
            bus.publish("Corvus", "corvus.context_for_kami", summary)

    def _think(self, query: str) -> str:
        """Corvus answers from memory — no LLM needed, just pattern recall."""
        query_lower = query.lower()

        if any(w in query_lower for w in ["what did", "what have", "history", "remember", "last"]):
            summary = self.memory.get_context_summary()
            return f"Here is what I have observed:\n{summary}"

        if "build" in query_lower:
            builds = self.memory.get_recent_observations("build", limit=5)
            if builds:
                lines = [f"  {b['time'][:16]}: {b['content'][:60]}" for b in builds]
                return "Recent build activity:\n" + "\n".join(lines)
            return "No build activity recorded yet."

        if "insight" in query_lower or "pattern" in query_lower:
            insights = self.memory.data.get("insights", [])[-5:]
            if insights:
                lines = [f"  {i['insight']}" for i in insights]
                return "Recent insights:\n" + "\n".join(lines)
            return "No patterns detected yet. Still watching."

        return f"I have {len(self.memory.data.get('observations', []))} observations recorded. Ask me about builds, health, patterns, or history."

    def _cloud_reporter(self):
        """Reports learning activity as cloud cover to the weather widget."""
        while self.running:
            time.sleep(15)
            # Cloud density decays over time — clears up when quiet
            self._cloud_density = max(0.0, self._cloud_density - 0.1)
            pct = int(self._cloud_density * 100)
            if pct > 70:
                status = f"overcast: {pct}% — heavy learning"
            elif pct > 30:
                status = f"partly cloudy: {pct}% — active"
            else:
                status = f"clear: {pct}% — quiet"
            registry.update_status("Corvus", status)
            self.memory.save()

    def run(self):
        """Keep Corvus alive as a background service."""
        registry.update_status("Corvus", "listening")
        print("[CORVUS] Watching the bus. Memory recording.")
        while self.running:
            time.sleep(1)

    def stop(self):
        self.running = False
        self.memory.save()
        registry.update_status("Corvus", "offline")


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    corvus = Corvus()

    # Demo: simulate some bus activity for Corvus to observe
    print()
    bus.publish("Mater", "mater.report", "Build succeeded: bubo.iso 11.8MB")
    bus.publish("Batty", "batty.alert", "CPU at 87% — running hot")
    bus.publish("Kami", "kami.request", "What should I work on next?")
    time.sleep(0.5)

    # Ask Corvus what he's seen
    response = corvus._think("what did you observe?")
    print(f"\nCORVUS > {response}\n")

    corvus.run()
