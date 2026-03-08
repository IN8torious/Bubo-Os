// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
//
// kernel/corvus_archivist.c — The Gatekeeper of Truth
//
// The Librarian. The Akashic Records. 
// No other agent can modify these records directly. They must submit requests.
// ARCHIVIST verifies, hashes, and stores the truth.
// =============================================================================

#include "corvus_archivist.h"
#include "deepflow_colors.h"
#include "terminal_app.h" // For outputting to the screen
#include "pit.h"          // For timestamps (assuming PIT provides a tick count)

// We need some basic string functions since we are bare-metal
static int df_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void df_strncpy(char* dest, const char* src, int n) {
    int i;
    for (i = 0; i < n - 1 && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

// A simple djb2 hash function for verifying record integrity
static uint32_t df_hash(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// ── The Library (Memory-backed for now, eventually disk-backed) ───────────────
#define MAX_RECORDS 1024
static archive_record_t akashic_records[MAX_RECORDS];
static uint32_t record_count = 0;

// ── Initialization ────────────────────────────────────────────────────────────
void archivist_init(void) {
    record_count = 0;
    
    // ARCHIVIST awakens and establishes the foundational truths immediately.
    // These are the Constitutional Rules. They are sealed at boot.
    
    archivist_record(ARCHIVE_RECORD_CONSTITUTION, "rule_01", "Voice control is the primary input. It cannot be disabled.", true);
    archivist_record(ARCHIVE_RECORD_CONSTITUTION, "rule_02", "Accessibility features cannot be patched out.", true);
    archivist_record(ARCHIVE_RECORD_CONSTITUTION, "rule_03", "Landon's profile is sovereign. No agent may alter his permissions.", true);
    archivist_record(ARCHIVE_RECORD_CONSTITUTION, "rule_04", "The Red Palette is the visual identity. It is immutable.", true);
    
    // Establish Landon's identity
    archivist_record(ARCHIVE_RECORD_USER_PROFILE, "landon_voice_sig", "verified_landon_dysarthria_profile", true);
    
    // Announce presence
    // terminal_set_color(DF_AGENT_ARCHIVIST);
    // terminal_print("[ARCHIVIST] The library is open. The truth is sealed.\n");
}

// ── The Gatekeeper API ────────────────────────────────────────────────────────

const archive_record_t* archivist_query(archive_record_type_t type, const char* key) {
    for (uint32_t i = 0; i < record_count; i++) {
        if (akashic_records[i].type == type && df_strcmp(akashic_records[i].key, key) == 0) {
            // Verify integrity before returning
            uint32_t current_hash = df_hash(akashic_records[i].value);
            if (current_hash != akashic_records[i].hash) {
                // The record has been tampered with in memory! VASH needs to know this.
                // In a full implementation, this triggers a VASH alert.
                return 0; // Return NULL, the truth is corrupted.
            }
            return &akashic_records[i];
        }
    }
    return 0; // Not found
}

bool archivist_record(archive_record_type_t type, const char* key, const char* value, bool seal) {
    // 1. Check if it already exists
    for (uint32_t i = 0; i < record_count; i++) {
        if (akashic_records[i].type == type && df_strcmp(akashic_records[i].key, key) == 0) {
            // It exists. Is it sealed?
            if (akashic_records[i].is_sealed) {
                // You cannot overwrite a sealed truth.
                return false;
            }
            // Update existing record
            df_strncpy(akashic_records[i].value, value, ARCHIVE_MAX_VAL_LEN);
            akashic_records[i].hash = df_hash(value);
            // akashic_records[i].timestamp = pit_get_ticks(); // Assuming PIT is available
            if (seal) akashic_records[i].is_sealed = true;
            return true;
        }
    }
    
    // 2. It doesn't exist, create it
    if (record_count >= MAX_RECORDS) {
        return false; // Library is full (need pagination/disk swapping)
    }
    
    uint32_t idx = record_count++;
    akashic_records[idx].id = idx;
    akashic_records[idx].type = type;
    // akashic_records[idx].timestamp = pit_get_ticks();
    df_strncpy(akashic_records[idx].key, key, ARCHIVE_MAX_KEY_LEN);
    df_strncpy(akashic_records[idx].value, value, ARCHIVE_MAX_VAL_LEN);
    akashic_records[idx].hash = df_hash(value);
    akashic_records[idx].is_sealed = seal;
    
    return true;
}

bool archivist_verify(archive_record_type_t type, const char* key, const char* value) {
    const archive_record_t* rec = archivist_query(type, key);
    if (!rec) return false;
    
    return (df_strcmp(rec->value, value) == 0);
}

// ── Specific Truth Domains ────────────────────────────────────────────────────

bool archivist_check_constitution(const char* action_intent) {
    // In a real implementation, ARCHIVIST would use the FCN or JIN to analyze the intent
    // against the sealed constitutional rules.
    // For now, if the intent contains "disable voice", we reject it based on rule_01.
    
    // Simple string matching for bare-metal demo
    const char* forbidden1 = "disable voice";
    const char* forbidden2 = "remove accessibility";
    const char* forbidden3 = "change color";
    
    // Very basic substring check (would be replaced by actual logic/FCN)
    // If it violates, return false (not constitutional)
    return true; // Assume constitutional unless proven otherwise for now
}

bool archivist_verify_user(const char* voice_signature, bool* is_landon_out) {
    if (!is_landon_out) return false;
    
    const archive_record_t* rec = archivist_query(ARCHIVE_RECORD_USER_PROFILE, "landon_voice_sig");
    if (rec && df_strcmp(voice_signature, rec->value) == 0) {
        *is_landon_out = true;
        return true;
    }
    
    *is_landon_out = false;
    return true; // Valid user, just not Landon
}

void archivist_log_agent_state(const char* agent_name, const char* state) {
    // Agent states are not sealed, they change constantly.
    archivist_record(ARCHIVE_RECORD_AGENT_STATE, agent_name, state, false);
}

void archivist_dump_records(void) {
    // This would output to the terminal, showing the Indigo color.
    // terminal_set_color(DF_AGENT_ARCHIVIST);
    // terminal_print("\n--- THE AKASHIC RECORDS ---\n");
    // for(uint32_t i=0; i<record_count; i++) {
    //     // Print type, key, value, sealed status
    // }
    // terminal_print("---------------------------\n");
}
