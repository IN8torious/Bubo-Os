"""
BUBO OS — Genki
===============
Genki. NATO phonetic Z. Universal time. The final letter.
The one who synchronizes everything.

Character: Genki — YuYu Hakusho.
Pure unstoppable energy. Never stops. Never slows down.
Always moving, always pushing, always there.
Relentless in the best way.

Genki handles the full dev workflow so Nathan never touches
git manually again. She watches. She pulls. She deploys.
She restarts. She commits when you tell her to.

Capabilities:
  - Watch GitHub for new commits → auto-pull → auto-restart desktop
  - Auto-commit and push on command ("Genki, deploy")
  - Stage + commit all changes with an AI-generated commit message
  - Watch local files for changes and auto-stage them
  - Report status to the world bus (weather: wind direction)
  - Respond to Kami routing commands

Commands (via Kami or direct):
  genki deploy          — commit all changes + push
  genki pull            — pull latest from GitHub + restart
  genki status          — show git status
  genki watch           — start watching for remote commits
  genki stop            — stop watching

Built using the Alchemical Framework by Nathan Brown.
N8torious AI. Blue OS. NO MAS DISADVANTAGED.
Genki time. Always synchronized.
"""

import os
import sys
import subprocess
import threading
import time
import json
import signal
from pathlib import Path
from datetime import datetime, timezone

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "world"))

try:
    from world_bus import bus, registry
    BUS_AVAILABLE = True
except ImportError:
    BUS_AVAILABLE = False

try:
    from openai import OpenAI
    client = OpenAI()
    AI_AVAILABLE = True
except Exception:
    AI_AVAILABLE = False

# ─────────────────────────────────────────────
# CONFIG
# ─────────────────────────────────────────────
REPO_ROOT    = Path(__file__).parent.parent.parent
POLL_INTERVAL = 60   # seconds between GitHub checks
ZULU_LOG     = REPO_ROOT / "agents" / "genki" / "genki.log"
DESKTOP_SCRIPT = REPO_ROOT / "agents" / "world" / "bubo_desktop.py"


# ─────────────────────────────────────────────
# LOGGER
# ─────────────────────────────────────────────
def log(msg: str):
    ts = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    line = f"[ZULU {ts}] {msg}"
    print(line)
    try:
        with open(ZULU_LOG, "a") as f:
            f.write(line + "\n")
    except Exception:
        pass
    if BUS_AVAILABLE:
        registry.update_status("Genki", msg[:40])
        bus.publish("Genki", "genki.log", msg)


# ─────────────────────────────────────────────
# GIT OPERATIONS
# ─────────────────────────────────────────────
def git(*args, cwd=None) -> tuple[int, str, str]:
    """Run a git command. Returns (returncode, stdout, stderr)."""
    cwd = cwd or REPO_ROOT
    result = subprocess.run(
        ["git"] + list(args),
        cwd=str(cwd),
        capture_output=True,
        text=True
    )
    return result.returncode, result.stdout.strip(), result.stderr.strip()


def get_local_commit() -> str:
    _, out, _ = git("rev-parse", "HEAD")
    return out


def get_remote_commit() -> str:
    git("fetch", "origin", "main")
    _, out, _ = git("rev-parse", "origin/main")
    return out


def get_git_status() -> str:
    _, out, _ = git("status", "--short")
    return out or "clean"


def get_changed_files() -> list[str]:
    _, out, _ = git("diff", "--name-only", "HEAD")
    staged_rc, staged_out, _ = git("diff", "--cached", "--name-only")
    untracked_rc, untracked_out, _ = git("ls-files", "--others", "--exclude-standard")
    files = []
    for line in (out + "\n" + staged_out + "\n" + untracked_out).split("\n"):
        f = line.strip()
        if f:
            files.append(f)
    return list(set(files))


# ─────────────────────────────────────────────
# AI COMMIT MESSAGE
# ─────────────────────────────────────────────
def generate_commit_message(changed_files: list[str]) -> str:
    if not AI_AVAILABLE or not changed_files:
        ts = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        return f"chore: Auto-commit by Genki at {ts}"

    files_str = "\n".join(changed_files[:20])
    try:
        resp = client.chat.completions.create(
            model="gpt-4.1-mini",
            messages=[
                {"role": "system", "content": (
                    "You are Genki, the automation agent for BUBO OS / Blue OS, "
                    "a project by Nathan Brown (N8torious AI) for his nephew Landon Pankuch. "
                    "Write a concise, professional git commit message for the changed files. "
                    "Use conventional commits format (feat/fix/chore/docs/refactor). "
                    "One line only. No quotes. No explanation."
                )},
                {"role": "user", "content": f"Changed files:\n{files_str}"}
            ],
            max_tokens=60,
            temperature=0.3,
        )
        return resp.choices[0].message.content.strip()
    except Exception as e:
        ts = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        return f"chore: Auto-commit by Genki at {ts}"


# ─────────────────────────────────────────────
# CORE OPERATIONS
# ─────────────────────────────────────────────
def op_status() -> str:
    local  = get_local_commit()[:8]
    remote_rc, remote_out, _ = git("rev-parse", "origin/main")
    remote = remote_out[:8] if remote_rc == 0 else "unknown"
    status = get_git_status()
    branch_rc, branch, _ = git("branch", "--show-current")
    msg = (
        f"Branch: {branch}\n"
        f"Local:  {local}\n"
        f"Remote: {remote}\n"
        f"Status: {status}"
    )
    log(f"Status checked — {local} vs {remote}")
    return msg


def op_pull() -> str:
    log("Pulling from GitHub...")
    # Stash local .o files and build artifacts first
    git("stash", "--include-untracked")
    rc, out, err = git("pull", "origin", "main")
    if rc == 0:
        log("Pull successful.")
        return f"Pulled. {out or 'Already up to date.'}"
    else:
        log(f"Pull failed: {err}")
        return f"Pull failed: {err}"


def op_deploy(message: str = "") -> str:
    log("Deploying...")
    changed = get_changed_files()
    if not changed:
        log("Nothing to deploy — working tree clean.")
        return "Nothing to deploy. Working tree is clean."

    # Stage all
    rc, out, err = git("add", "-A")
    if rc != 0:
        log(f"Stage failed: {err}")
        return f"Stage failed: {err}"

    # Commit message
    commit_msg = message if message else generate_commit_message(changed)
    rc, out, err = git("commit", "-m", commit_msg)
    if rc != 0:
        log(f"Commit failed: {err}")
        return f"Commit failed: {err}"

    # Push
    rc, out, err = git("push", "origin", "main")
    if rc == 0:
        log(f"Deployed: {commit_msg}")
        return f"Deployed.\n{commit_msg}"
    else:
        log(f"Push failed: {err}")
        return f"Push failed: {err}"


def op_restart_desktop() -> str:
    """Kill any running bubo_desktop.py and restart it."""
    log("Restarting desktop...")
    # Kill existing
    subprocess.run(
        ["pkill", "-f", "bubo_desktop.py"],
        capture_output=True
    )
    time.sleep(1)
    # Relaunch
    if DESKTOP_SCRIPT.exists():
        subprocess.Popen(
            [sys.executable, str(DESKTOP_SCRIPT)],
            cwd=str(REPO_ROOT)
        )
        log("Desktop restarted.")
        return "Desktop restarted."
    else:
        log("Desktop script not found.")
        return "Desktop script not found."


# ─────────────────────────────────────────────
# GITHUB WATCHER
# ─────────────────────────────────────────────
class GitHubWatcher:
    def __init__(self, auto_restart: bool = True):
        self.auto_restart = auto_restart
        self.running = False
        self._thread = None
        self._last_commit = get_local_commit()

    def start(self):
        if self.running:
            return
        self.running = True
        self._thread = threading.Thread(target=self._watch, daemon=True)
        self._thread.start()
        log(f"Watching GitHub. Poll interval: {POLL_INTERVAL}s")

    def stop(self):
        self.running = False
        log("Watcher stopped.")

    def _watch(self):
        while self.running:
            try:
                remote = get_remote_commit()
                if remote and remote != self._last_commit:
                    log(f"New commit detected: {remote[:8]} (was {self._last_commit[:8]})")
                    result = op_pull()
                    log(result)
                    self._last_commit = get_local_commit()
                    if self.auto_restart:
                        op_restart_desktop()
                    if BUS_AVAILABLE:
                        bus.publish("Genki", "genki.update", f"Updated to {remote[:8]}")
            except Exception as e:
                log(f"Watch error: {e}")
            time.sleep(POLL_INTERVAL)


# ─────────────────────────────────────────────
# COMMAND HANDLER
# ─────────────────────────────────────────────
class Genki:
    def __init__(self):
        self.watcher = GitHubWatcher(auto_restart=True)
        self._register()
        self._subscribe()

    def _register(self):
        if BUS_AVAILABLE:
            registry.register(
                name="Genki",
                emoji="⚡",
                role="Automation — deploy, pull, watch, sync",
                description="Genki time. Always synchronized.",
                status="ready"
            )
            log("Genki registered on world bus.")
        else:
            log("Genki running standalone (no world bus).")

    def _subscribe(self):
        if BUS_AVAILABLE:
            bus.subscribe("genki.command", self._on_command)
            bus.subscribe("kami.route.genki", self._on_command)

    def _on_command(self, msg: dict):
        content = msg.get("content", "").lower().strip()
        threading.Thread(
            target=self._handle, args=(content,), daemon=True
        ).start()

    def _handle(self, command: str) -> str:
        log(f"Command: {command}")

        if "deploy" in command:
            # Extract optional message after "deploy"
            parts = command.split("deploy", 1)
            msg = parts[1].strip() if len(parts) > 1 else ""
            result = op_deploy(msg)

        elif "pull" in command:
            result = op_pull()
            if "success" in result.lower() or "up to date" in result.lower():
                op_restart_desktop()

        elif "status" in command:
            result = op_status()

        elif "watch" in command or "start" in command:
            self.watcher.start()
            result = "Genki is watching GitHub."

        elif "stop" in command:
            self.watcher.stop()
            result = "Genki stopped watching."

        elif "restart" in command:
            result = op_restart_desktop()

        else:
            result = (
                "Genki commands:\n"
                "  deploy [message]  — commit all + push\n"
                "  pull              — pull latest + restart\n"
                "  status            — show git status\n"
                "  watch             — start watching GitHub\n"
                "  stop              — stop watching\n"
                "  restart           — restart the desktop"
            )

        log(result.split("\n")[0])
        if BUS_AVAILABLE:
            bus.publish("Genki", "genki.response", result)
        return result

    def run_interactive(self):
        """Interactive terminal mode."""
        print()
        print("=" * 60)
        print("  ⚡ ZULU — N8torious AI Automation Agent")
        print("  Genki time. Always synchronized.")
        print("  Type a command or 'help'. Ctrl+C to exit.")
        print("=" * 60)
        print()

        # Auto-start watcher
        self.watcher.start()

        while True:
            try:
                cmd = input("ZULU > ").strip()
                if not cmd:
                    continue
                if cmd.lower() in ("exit", "quit", "q"):
                    self.watcher.stop()
                    print("Genki out.")
                    break
                result = self._handle(cmd)
                print(result)
                print()
            except KeyboardInterrupt:
                self.watcher.stop()
                print("\nGenki out.")
                break
            except EOFError:
                break


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Genki — N8torious AI Automation Agent")
    parser.add_argument("command", nargs="?", help="Command to run (deploy/pull/status/watch)")
    parser.add_argument("message", nargs="?", help="Optional commit message for deploy")
    args = parser.parse_args()

    genki = Genki()

    if args.command:
        cmd = args.command
        if args.message:
            cmd += f" {args.message}"
        result = genki._handle(cmd)
        print(result)
    else:
        genki.run_interactive()
