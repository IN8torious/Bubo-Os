// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/screen_recorder.h — Header for CORVUS Screen Recorder
// =============================================================================

#pragma once

#include <stdbool.h>

// Initializes the screen recorder system.
// Sets up the ring buffer and prepares for recording.
void recorder_init(void);

// Starts the screen recording process.
// Captures framebuffer frames into the internal ring buffer.
void recorder_start(void);

// Stops the screen recording process.
// Writes the recorded frames to VFS at /home/user/recordings/.
void recorder_stop(void);

// Performs periodic tasks for the screen recorder.
// This function should be called regularly, e.g., at 30fps, to capture frames.
void recorder_tick(void);

// Checks if the screen recorder is currently active.
// Returns true if recording, false otherwise.
bool recorder_is_recording(void);
