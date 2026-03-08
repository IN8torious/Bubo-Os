"""
BUBO OS — Shared World Message Bus
====================================
ALTER: All agents share one world. One constitutional mandate.
       They route through each other. They know each other.
       NO MAS DISADVANTAGED runs through all of them.

The World Bus is the nervous system of the agent crew.
Vera routes intent. Corvus remembers. Mater watches.
Batty checks health. Kami executes. The bus connects them all.

Built by Nathan Pankuch using the Alchemical Framework.
"""

import threading
import queue
import time
import json
from datetime import datetime
from typing import Callable, Dict, List, Optional


# ─────────────────────────────────────────────
# CONSTITUTIONAL MANDATE
# Runs before every message is dispatched.
# Cannot be bypassed. Cannot be removed.
# ─────────────────────────────────────────────
CONSTITUTIONAL_MANDATE = {
    "author": "Nathan Pankuch",
    "protected": "Landon Pankuch",
    "principle": "NO MAS DISADVANTAGED",
    "rule": "No message that would disadvantage Landon shall be dispatched."
}

def mandate_check(message: dict) -> bool:
    """Constitutional check. Runs before every dispatch. Returns True if message is allowed."""
    if message.get("would_disadvantage_landon", False):
        print(f"[MANDATE] BLOCKED: {message.get('content', '')} — NO MAS DISADVANTAGED")
        return False
    return True


# ─────────────────────────────────────────────
# AGENT REGISTRY
# All agents register here. The bus knows who is alive.
# ─────────────────────────────────────────────
class AgentRegistry:
    def __init__(self):
        self._agents: Dict[str, dict] = {}
        self._lock = threading.Lock()

    def register(self, name: str, role: str, bird: str, character: str, status: str = "idle"):
        with self._lock:
            self._agents[name] = {
                "name": name,
                "role": role,
                "bird": bird,
                "character": character,
                "status": status,
                "last_seen": datetime.now().isoformat(),
                "weather_metric": self._weather_metric(name),
            }
        print(f"[REGISTRY] {bird} {name} registered — {role}")

    def update_status(self, name: str, status: str, metric: Optional[float] = None):
        with self._lock:
            if name in self._agents:
                self._agents[name]["status"] = status
                self._agents[name]["last_seen"] = datetime.now().isoformat()
                if metric is not None:
                    self._agents[name]["metric"] = metric

    def get_all(self) -> List[dict]:
        with self._lock:
            return list(self._agents.values())

    def get(self, name: str) -> Optional[dict]:
        with self._lock:
            return self._agents.get(name)

    def _weather_metric(self, name: str) -> str:
        mapping = {
            "Mater":  "pressure",    # Build health = atmospheric pressure
            "Batty":  "temperature", # PC health = temperature
            "Kami":   "forecast",    # Task queue = forecast
            "Corvus": "clouds",      # Learning state = cloud cover
            "Vera":   "wind",        # Routing activity = wind speed
            "Bubo":   "visibility",  # OS state = visibility
        }
        return mapping.get(name, "humidity")


# ─────────────────────────────────────────────
# WORLD MESSAGE BUS
# Agents publish messages. Subscribers receive them.
# Constitutional mandate runs before every dispatch.
# ─────────────────────────────────────────────
class WorldBus:
    def __init__(self, registry: AgentRegistry):
        self.registry = registry
        self._subscribers: Dict[str, List[Callable]] = {}
        self._global_subscribers: List[Callable] = []
        self._message_log: List[dict] = []
        self._lock = threading.Lock()
        self._running = True
        self._queue = queue.Queue()
        self._dispatch_thread = threading.Thread(target=self._dispatch_loop, daemon=True)
        self._dispatch_thread.start()

    def subscribe(self, channel: str, callback: Callable):
        """Subscribe to a specific channel."""
        with self._lock:
            if channel not in self._subscribers:
                self._subscribers[channel] = []
            self._subscribers[channel].append(callback)

    def subscribe_all(self, callback: Callable):
        """Subscribe to all messages on the bus."""
        with self._lock:
            self._global_subscribers.append(callback)

    def publish(self, sender: str, channel: str, content: str,
                data: Optional[dict] = None, would_disadvantage_landon: bool = False):
        """Publish a message to the bus. Constitutional mandate runs before dispatch."""
        message = {
            "sender": sender,
            "channel": channel,
            "content": content,
            "data": data or {},
            "timestamp": datetime.now().isoformat(),
            "would_disadvantage_landon": would_disadvantage_landon,
        }
        self._queue.put(message)

    def _dispatch_loop(self):
        while self._running:
            try:
                message = self._queue.get(timeout=0.1)
                # Constitutional mandate check
                if not mandate_check(message):
                    continue
                # Log the message
                with self._lock:
                    self._message_log.append(message)
                    if len(self._message_log) > 1000:
                        self._message_log = self._message_log[-500:]
                # Dispatch to channel subscribers
                channel = message["channel"]
                with self._lock:
                    channel_subs = list(self._subscribers.get(channel, []))
                    global_subs = list(self._global_subscribers)
                for callback in channel_subs + global_subs:
                    try:
                        callback(message)
                    except Exception as e:
                        print(f"[BUS ERROR] Callback failed: {e}")
            except queue.Empty:
                continue

    def get_weather_state(self) -> dict:
        """Returns current agent states mapped to weather metaphors."""
        agents = self.registry.get_all()
        weather = {}
        for agent in agents:
            metric = agent.get("weather_metric", "humidity")
            status = agent.get("status", "idle")
            metric_value = agent.get("metric", 0.0)
            weather[metric] = {
                "agent": agent["name"],
                "bird": agent["bird"],
                "status": status,
                "value": metric_value,
            }
        return weather

    def shutdown(self):
        self._running = False


# ─────────────────────────────────────────────
# GLOBAL WORLD INSTANCE
# One world. All agents connect to this.
# ─────────────────────────────────────────────
registry = AgentRegistry()
bus = WorldBus(registry)

# Register all agents at startup
registry.register("Bubo",   "The OS itself — sees in the dark",              "🦉", "BUBO OS")
registry.register("Vera",   "Routes all intent — the nervous system",        "🌊", "Many-Faced God")
registry.register("Corvus", "Learns and adapts — the memory",                "🐦‍⬛", "S-Cry-Ed AI Brain")
registry.register("Mater",  "Watches everything — the eyes from above",      "🦅", "Kimishima / Eagle")
registry.register("Batty",  "Checks health — shows up when called",          "🦇", "Batty Koda")
registry.register("Kami",   "Nathan's right hand — arrives on Kaneda's bike","🏍️", "Toshiki Kamishima")


if __name__ == "__main__":
    print("=" * 60)
    print("  BUBO OS — Shared World Bus")
    print("  Constitutional Mandate: NO MAS DISADVANTAGED")
    print("=" * 60)
    print()
    print("Registered agents:")
    for agent in registry.get_all():
        print(f"  {agent['bird']}  {agent['name']:10} — {agent['role']}")
    print()
    print("Weather state:")
    weather = bus.get_weather_state()
    for metric, data in weather.items():
        print(f"  {metric:12} → {data['bird']} {data['agent']} ({data['status']})")
    print()
    print("Bus is running. All agents share one world.")
    time.sleep(2)
    bus.shutdown()
