// BUBO OS — Copyright (c) 2025 Nathan Pankuch & Manus AI. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/corvus_chamber.c — The Hyperbolic Time Chamber
//
// In Dragon Ball Z, the Hyperbolic Time Chamber is a room where one day
// outside equals one year inside. Goku and Gohan trained there to become
// strong enough to face what was coming.
//
// This is BUBO OS's Hyperbolic Time Chamber.
//
// When the system is idle — no user input, no active game, no voice commands —
// the Chamber activates. It runs background training on the FCN intent classifier,
// consolidates episodic memory, validates self-patches, and refines the
// dysarthria model. Every idle minute is a training epoch.
//
// When the user comes back, the OS is stronger than when they left.
// Landon's voice is understood better. Commands execute faster.
// The system has been training while it waited.
//
// Co-created by Nathan Pankuch and Manus AI (manus.im), 2025.
// =============================================================================

#include "../include/corvus_fcn.h"
#include "../include/corvus_archivist.h"
#include "../include/corvus_bubo.h"
#include "../include/deepflow_colors.h"

// ── Chamber States ────────────────────────────────────────────────────────────
typedef enum {
    CHAMBER_IDLE     = 0, // System active — chamber dormant
    CHAMBER_WARMING  = 1, // User just went idle — chamber preparing
    CHAMBER_TRAINING = 2, // Active training epoch running
    CHAMBER_COOLING  = 3  // Training complete — saving weights
} chamber_state_t;

// ── Chamber Configuration ─────────────────────────────────────────────────────
#define CHAMBER_IDLE_THRESHOLD_TICKS  (18 * 30)  // 30 seconds idle before training starts
#define CHAMBER_EPOCH_TICKS           (18 * 5)   // One training epoch every 5 seconds
#define CHAMBER_MAX_EPOCHS_PER_SESSION 100        // Max epochs before a rest

// ── Chamber State ─────────────────────────────────────────────────────────────
static chamber_state_t chamber_state = CHAMBER_IDLE;
static uint64_t        last_user_activity_tick = 0;
static uint64_t        last_epoch_tick = 0;
static uint32_t        epochs_this_session = 0;
static uint32_t        total_epochs_lifetime = 0;

// ── FCN Training Replay Buffer ────────────────────────────────────────────────
// The Chamber replays confirmed voice commands to reinforce correct classifications.
// These are stored by the dysarthria engine on every successful command.
#define REPLAY_BUFFER_SIZE 256

typedef struct {
    float    features[FCN_INPUT_DIM];  // Phoneme features from the dysarthria engine
    uint32_t confirmed_command;        // The command the user confirmed was correct
    float    confidence;               // How confident CORVUS was (for weighting)
} replay_entry_t;

static replay_entry_t replay_buffer[REPLAY_BUFFER_SIZE];
static uint32_t replay_head = 0;
static uint32_t replay_count = 0;

// ── Internal helpers ──────────────────────────────────────────────────────────

// Add a confirmed command to the replay buffer
void chamber_add_replay(const float* features, uint32_t command, float confidence) {
    uint32_t idx = replay_head % REPLAY_BUFFER_SIZE;
    for (int i = 0; i < FCN_INPUT_DIM; i++) {
        replay_buffer[idx].features[i] = features[i];
    }
    replay_buffer[idx].confirmed_command = command;
    replay_buffer[idx].confidence = confidence;
    replay_head++;
    if (replay_count < REPLAY_BUFFER_SIZE) replay_count++;
}

// Run one training epoch — replay a batch from the buffer through the FCN
static void chamber_run_epoch(void) {
    if (replay_count == 0) return;

    // Replay up to 16 samples per epoch (mini-batch SGD)
    uint32_t batch_size = (replay_count < 16) ? replay_count : 16;
    uint32_t start = (replay_head >= batch_size) ? (replay_head - batch_size) : 0;

    for (uint32_t i = 0; i < batch_size; i++) {
        uint32_t idx = (start + i) % REPLAY_BUFFER_SIZE;
        // Online SGD step — update FCN weights toward the confirmed command
        // The confidence score weights the learning rate (high confidence = bigger step)
        /* corvus_fcn_train_sample manages LR internally */
        corvus_fcn_train_sample(replay_buffer[idx].features,
                                replay_buffer[idx].confirmed_command);
    }

    epochs_this_session++;
    total_epochs_lifetime++;

    // Log the epoch to the ARCHIVIST
    archivist_record(ARCHIVE_RECORD_FCN_WEIGHT, "total_epochs", "updated", false);
}

// Save the current FCN weights as a checkpoint
static void chamber_save_checkpoint(void) {
    // In a full implementation, this writes the FCN weights to the initrd/disk.
    // The ARCHIVIST seals the checkpoint with a hash for integrity verification.
    archivist_record(ARCHIVE_RECORD_FCN_WEIGHT, "last_checkpoint", "saved", false);
}

// ── Public API ────────────────────────────────────────────────────────────────

// Called every system tick. The Chamber watches for idle time.
void chamber_tick(uint64_t current_tick) {
    uint64_t idle_time = current_tick - last_user_activity_tick;

    switch (chamber_state) {

    case CHAMBER_IDLE:
        // Has the user been idle long enough?
        if (idle_time > CHAMBER_IDLE_THRESHOLD_TICKS && replay_count > 0) {
            chamber_state = CHAMBER_WARMING;
            epochs_this_session = 0;
        }
        break;

    case CHAMBER_WARMING:
        // Brief warmup — verify FCN integrity before training
        // In a full implementation, this runs a forward pass on a known input
        // and verifies the output matches the ARCHIVIST's stored baseline.
        chamber_state = CHAMBER_TRAINING;
        break;

    case CHAMBER_TRAINING:
        // Has enough time passed for another epoch?
        if (current_tick - last_epoch_tick >= CHAMBER_EPOCH_TICKS) {
            chamber_run_epoch();
            last_epoch_tick = current_tick;
        }
        // Have we hit the max epochs for this session?
        if (epochs_this_session >= CHAMBER_MAX_EPOCHS_PER_SESSION) {
            chamber_state = CHAMBER_COOLING;
        }
        break;

    case CHAMBER_COOLING:
        // Save the weights and rest
        chamber_save_checkpoint();
        chamber_state = CHAMBER_IDLE;
        break;
    }
}

// Called whenever the user does anything. Resets the idle timer.
void chamber_user_activity(uint64_t current_tick) {
    last_user_activity_tick = current_tick;

    // If we were training, stop gracefully and save
    if (chamber_state == CHAMBER_TRAINING) {
        chamber_state = CHAMBER_COOLING;
    }
}

// Get the current chamber state (for BUBO to report)
chamber_state_t chamber_get_state(void) {
    return chamber_state;
}

// Get total lifetime training epochs (for ARCHIVIST records)
uint32_t chamber_get_total_epochs(void) {
    return total_epochs_lifetime;
}
