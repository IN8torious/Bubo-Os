// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// include/corvus_archivist.h — The Gatekeeper of Truth
//
// ARCHIVIST is the librarian and single source of truth for the entire OS.
// He does not create. He does not destroy. He records, verifies, and protects.
// No agent can act without his verification. No record can be altered without
// his seal. He is the Akashic Records of Deep Flow OS.
//
// "I do not create. I do not destroy. I record what is, what was, and what must remain.
//  Come to me with a question and I will give you the truth.
//  Come to me with a lie and I will show you the record."
// =============================================================================

#ifndef CORVUS_ARCHIVIST_H
#define CORVUS_ARCHIVIST_H

#include <stdint.h>
#include <stdbool.h>

// ── The Archivist's Color ─────────────────────────────────────────────────────
// Deep Indigo — The color of the archive at midnight. The color of ink.
#define DF_AGENT_ARCHIVIST ((uint32_t)0xFF3D0070)

// ── Record Types ──────────────────────────────────────────────────────────────
typedef enum {
    ARCHIVE_RECORD_CONSTITUTION = 0, // Immutable OS rules
    ARCHIVE_RECORD_AGENT_STATE  = 1, // Current status of an agent
    ARCHIVE_RECORD_USER_PROFILE = 2, // User identity and journey stage
    ARCHIVE_RECORD_VOICE_CMD    = 3, // History of voice commands and resolutions
    ARCHIVE_RECORD_FCN_WEIGHT   = 4, // Neural network checkpoints
    ARCHIVE_RECORD_PATCH_LOG    = 5, // Record of self-applied patches
    ARCHIVE_RECORD_TRUTH_SEAL   = 6  // A verified, immutable fact
} archive_record_type_t;

// ── The Record Structure ──────────────────────────────────────────────────────
#define ARCHIVE_MAX_KEY_LEN 64
#define ARCHIVE_MAX_VAL_LEN 256

typedef struct {
    uint32_t id;                            // Unique record ID
    archive_record_type_t type;             // What kind of record is this?
    uint64_t timestamp;                     // When was this recorded?
    char key[ARCHIVE_MAX_KEY_LEN];          // The lookup key (e.g., "rule_01", "landon_profile")
    char value[ARCHIVE_MAX_VAL_LEN];        // The truth value
    bool is_sealed;                         // If true, can NEVER be modified, even by ARCHIVIST
    uint32_t hash;                          // Verification hash
} archive_record_t;

// ── Initialization ────────────────────────────────────────────────────────────
// Wake the Archivist. Load the Akashic records from disk/memory.
void archivist_init(void);

// ── The Gatekeeper API ────────────────────────────────────────────────────────

// Ask the Archivist for the truth.
// Returns a pointer to the record, or NULL if the record does not exist.
const archive_record_t* archivist_query(archive_record_type_t type, const char* key);

// Ask the Archivist to record a new truth.
// Returns true if recorded, false if rejected (e.g., trying to overwrite a sealed record).
bool archivist_record(archive_record_type_t type, const char* key, const char* value, bool seal);

// Verify a truth. Returns true only if the record exists, matches the value, and the hash is valid.
bool archivist_verify(archive_record_type_t type, const char* key, const char* value);

// ── Specific Truth Domains ────────────────────────────────────────────────────

// Check if a proposed action violates the Constitution.
// ARCHIVIST consults the sealed constitutional records. VASH enforces the result.
bool archivist_check_constitution(const char* action_intent);

// Verify user identity. Landon's profile is sacred and sealed.
bool archivist_verify_user(const char* voice_signature, bool* is_landon_out);

// Log an agent's state change.
void archivist_log_agent_state(const char* agent_name, const char* state);

// Dump the archive to the terminal (for debugging/visualizing the library).
void archivist_dump_records(void);

#endif // CORVUS_ARCHIVIST_H
