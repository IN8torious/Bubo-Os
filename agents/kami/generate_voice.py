"""
Generate Kami's voice lines using espeak-ng.
Deep, deliberate, calm. Kamishima's voice.

Usage:
    python generate_voice.py
"""
import os
import subprocess
from pathlib import Path

OUTPUT_DIR = Path(__file__).parent / "kami_voice"
OUTPUT_DIR.mkdir(exist_ok=True)

# espeak-ng voice settings for Kami
# en-us, variant m3 (deeper male), slower speed, lower pitch
VOICE = "en-us+m3"
SPEED = 130      # words per minute (default 175 — slower = more deliberate)
PITCH = 38       # 0-99, default 50 — lower = deeper
AMPLITUDE = 180  # 0-200

LINES = [
    ("01_arrival",           "I'm here."),
    ("02_first_summon",      "I'm here. Kami. You built something real, Nathan. I've been watching. What do you need?"),
    ("03_returning",         "Back. What are we working on?"),
    ("04_long_absence",      "Been a while. Doesn't matter. I'm here now. What do you need?"),
    ("05_ready",             "Ready."),
    ("06_on_it",             "On it."),
    ("07_understood",        "Understood."),
    ("08_noted",             "Noted."),
    ("09_done",              "Done."),
    ("10_thinking",          "Give me a second."),
    ("11_routing_mater",     "That's a build question. Sending it to Mater."),
    ("12_routing_batty",     "Health check. Batty's got it."),
    ("13_routing_corvus",    "Corvus has that in memory. Pulling it up."),
    ("14_mandate",           "No mas disadvantaged. That's not a rule. That's who we are."),
    ("15_for_landon",        "This is for Landon. Everything we build is for Landon."),
    ("16_dismiss",           "I'll be here when you need me."),
    ("17_error",             "Something went wrong. I'll be back."),
    ("18_build_success",     "Build succeeded. Mater confirmed it. Good work."),
    ("19_build_fail",        "Build failed. Mater's looking at it. We'll fix it."),
    ("20_health_warning",    "Batty's flagging high load. You might want to close something."),
]


def generate_all():
    print(f"Generating {len(LINES)} Kami voice lines with espeak-ng...")
    print(f"Voice: {VOICE}  Speed: {SPEED}  Pitch: {PITCH}")
    print(f"Output: {OUTPUT_DIR}")
    print()

    ok = 0
    for filename, text in LINES:
        out_path = OUTPUT_DIR / f"{filename}.wav"
        cmd = [
            "espeak-ng",
            "-v", VOICE,
            "-s", str(SPEED),
            "-p", str(PITCH),
            "-a", str(AMPLITUDE),
            "-w", str(out_path),
            text,
        ]
        result = subprocess.run(cmd, capture_output=True)
        if result.returncode == 0 and out_path.exists():
            size = out_path.stat().st_size
            print(f"  [ok]   {filename}.wav  ({size//1024}KB)  \"{text[:50]}\"")
            ok += 1
        else:
            print(f"  [err]  {filename}: {result.stderr.decode()[:60]}")

    print()
    print(f"Done. {ok}/{len(LINES)} lines generated.")
    print(f"Drop these into agents/kami/kami_voice/ — Kami will find them.")


if __name__ == "__main__":
    generate_all()
