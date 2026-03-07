// =============================================================================
// Raven AOS — Dedicated to Landon Pankuch
// =============================================================================
// Built by IN8torious | Copyright (c) 2025 | MIT License
//
// This software was created for Landon Pankuch, who has cerebral palsy,
// so that he may drive, race, and command his world with his voice alone.
//
// Built by a person with manic depression, for a person with cerebral palsy,
// for every person who has ever been told their disability makes them less.
// It does not. You are not less. This machine was built to serve you.
//
// Constitutional Mandate: "NO MAS DISADVANTAGED"
// MAS = Multi-Agentic Systems — Sovereign Intelligence, not corporate AI
//
// MIT License — Free for Landon. Free for everyone. Especially those who
// need it most. Accessibility features must remain free in all derivatives.
// See LICENSE file for full terms and the permanent dedication.
// =============================================================================

// =============================================================================
// Raven AOS — CORVUS Dysarthria Adaptation Engine
//
// Built for Landon Pankuch. Built for anyone whose brain moves faster
// than their mouth. Built for anyone who has ever been told a machine
// couldn't understand them.
//
// This engine does NOT require perfect speech.
//
// WHAT IT HANDLES:
//   - Slurred consonants:    "FASTER" → "FASHER", "FASSER", "FASDER"
//   - Dropped finals:        "BRAKE"  → "BRAK", "BRAY", "BRACE"
//   - Vowel distortion:      "NITRO"  → "NETRO", "NEETRO", "NUTRO"
//   - Merged words:          "TURN LEFT" → "TERNLEF", "TUNLEF", "TLEFT"
//   - Partial words:         "NIT"    → NITRO (if unambiguous)
//   - Rapid-fire truncation: "FAS"    → FASTER (context-aware)
//   - Voiced/unvoiced swap:  "DRIFT"  → "DRIFF", "DRIFFT"
//   - Syllable reduction:    "OVERTAKE" → "OVTAK", "OVERT"
//
// APPROACH (inspired by jmaczan/asr-dysarthria wav2vec2 fine-tuning):
//   1. Phonetic normalization — map input to phoneme classes
//   2. Fuzzy edit distance — Levenshtein with phoneme-aware costs
//   3. Prefix matching — partial words matched to unambiguous commands
//   4. Context weighting — recent commands get higher prior
//   5. Personal calibration — CORVUS learns Landon's specific patterns
//
// "NO MAS DISADVANTAGED" — the machine adapts to the human.
// =============================================================================
#include "dysarthria.h"
#include "vga.h"

// ── Static engine state ───────────────────────────────────────────────────────
static dysarthria_state_t g_state;

// ── Command table with slur variants ─────────────────────────────────────────
// Each entry has the canonical form, command ID, and known dysarthric variants.
// Variants are generated from common dysarthria patterns:
//   - Consonant cluster reduction
//   - Final consonant deletion
//   - Vowel centralization
//   - Syllable deletion
static dysarthria_cmd_t g_commands[DYSARTHRIA_MAX_COMMANDS] = {
    {
        "FASTER", 0, // VCMD_FASTER
        { "FASHER", "FASSER", "FASDER", "FASER", "FASTE", "FAST",
          "FASH", "FAS", "FASTR", "FASTER", "FAASTR", "FAASTER" },
        12, 3, 0
    },
    {
        "BRAKE", 1, // VCMD_BRAKE
        { "BRAK", "BRAY", "BRACE", "BRАК", "BRAYK", "BREK",
          "BREYK", "BRK", "BRAK", "BRAКЕ", "BRAYKE", "BRKE" },
        12, 3, 0
    },
    {
        "DRIFT", 2, // VCMD_DRIFT
        { "DRIFF", "DRIFFT", "DRIF", "DRFT", "DRFT", "DRIFF",
          "DRIT", "DRIF", "DRIFT", "DRIFFT", "DRIFF", "DRF" },
        12, 3, 0
    },
    {
        "NITRO", 3, // VCMD_NITRO
        { "NETRO", "NEETRO", "NUTRO", "NITR", "NIT", "NITROW",
          "NEETROW", "NITROH", "NITROE", "NEATRO", "NITROO", "NITRO" },
        12, 3, 0
    },
    {
        "OVERTAKE", 4, // VCMD_OVERTAKE
        { "OVTAK", "OVERT", "OVRTAKE", "OVERTAYK", "OVRTAYK",
          "OVERTK", "OVRTAK", "OVTAKE", "OVRTK", "OVERTAYKE",
          "OVETAKE", "OVRTAKE" },
        12, 4, 0
    },
    {
        "TURN LEFT", 5, // VCMD_TURN_LEFT
        { "TERNLEF", "TUNLEF", "TLEFT", "TRNLFT", "TURNLEF",
          "TERNLEFT", "TUNLEFT", "TRNLEFT", "TERLEFT", "TURNLFT",
          "TNLEFT", "TRNLEF" },
        12, 4, 0
    },
    {
        "TURN RIGHT", 6, // VCMD_TURN_RIGHT
        { "TERNRIT", "TUNRIT", "TRIGHT", "TRNRIT", "TURNRIT",
          "TERNRIGHT", "TUNRIGHT", "TRNRIGHT", "TERRIGHT", "TURNRIT",
          "TNRIGHT", "TRNRIT" },
        12, 4, 0
    },
    {
        "PIT STOP", 7, // VCMD_PIT_STOP
        { "PITSTOP", "PITSTAP", "PITSTOB", "PITSOP", "PITSTOB",
          "PITSTAP", "PITSTOPP", "PITSTAHP", "PITSTAHB", "PITSTUP",
          "PITSTOAP", "PITSTOB" },
        12, 3, 0
    },
    {
        "STATUS", 8, // VCMD_STATUS
        { "STATIS", "STATISS", "STATOS", "STATS", "STATU",
          "STATUZ", "STATUSS", "STATIS", "STAYTUS", "STAYTOS",
          "STATOOS", "STATUZ" },
        12, 4, 0
    },
    {
        "STOP", 9, // VCMD_STOP
        { "STAP", "STOB", "STOPP", "STAHP", "STAHB",
          "STAWP", "STUP", "STP", "STOAP", "STOB",
          "STAHPP", "STAPP" },
        12, 3, 0
    },
    {
        "LAUNCH", 10, // VCMD_LAUNCH
        { "LONCH", "LAWNCH", "LAUCH", "LANCH", "LNCH",
          "LAUNSH", "LAWNSH", "LAUUNCH", "LAWNCHE", "LAUNCHE",
          "LONSH", "LNSH" },
        12, 3, 0
    },
    {
        "MUSIC", 11, // VCMD_MUSIC
        { "MEWSIC", "MUSICK", "MUZIC", "MEWZIC", "MUSIK",
          "MUZIK", "MEWSIK", "MEWZIK", "MUSEC", "MUZEC",
          "MEWSEC", "MEWZEC" },
        12, 3, 0
    },
    {
        "HELP", 12, // VCMD_HELP
        { "HEP", "HELB", "HELLP", "HEPP", "HELPP",
          "HLP", "HEELP", "HELLPP", "HEPPP", "HELB",
          "HELBP", "HLEP" },
        12, 3, 0
    },
    {
        "CORVUS", 13, // VCMD_CORVUS
        { "CORVIS", "CORVISS", "CORVUS", "CORBUS", "CORBIS",
          "CORVS", "CORV", "CORBUS", "CORVISS", "CORVIS",
          "CORBISS", "CORVUHS" },
        12, 4, 0
    },
    {
        "MANDATE", 14, // VCMD_MANDATE
        { "MANDAYT", "MANDAIT", "MANDAT", "MANDAET", "MANDAYTE",
          "MNDAYT", "MNDATE", "MANDAIT", "MANDAET", "MANDAYTE",
          "MNDAIT", "MANDAYT" },
        12, 4, 0
    },
};

static const uint32_t g_cmd_count = 15;

// ── Phoneme mapping table ─────────────────────────────────────────────────────
// Maps each ASCII letter to its phoneme class.
// This allows "BRAKE" and "BRAYKE" to have lower distance than "BRAKE" and "NITRO"
static const phoneme_class_t g_phoneme_map[26] = {
    PHON_VOWEL,   // A
    PHON_STOP,    // B
    PHON_FRIC,    // C
    PHON_STOP,    // D
    PHON_VOWEL,   // E
    PHON_FRIC,    // F
    PHON_STOP,    // G
    PHON_FRIC,    // H
    PHON_VOWEL,   // I
    PHON_FRIC,    // J
    PHON_STOP,    // K
    PHON_LIQUID,  // L
    PHON_NASAL,   // M
    PHON_NASAL,   // N
    PHON_VOWEL,   // O
    PHON_STOP,    // P
    PHON_FRIC,    // Q
    PHON_LIQUID,  // R
    PHON_FRIC,    // S
    PHON_STOP,    // T
    PHON_VOWEL,   // U
    PHON_FRIC,    // V
    PHON_APPROX,  // W
    PHON_FRIC,    // X
    PHON_APPROX,  // Y
    PHON_FRIC,    // Z
};

// ── String utilities (bare-metal — no stdlib) ─────────────────────────────────
static uint32_t dys_strlen(const char* s) {
    uint32_t n = 0;
    while (s[n]) n++;
    return n;
}

static void dys_strcpy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static bool dys_streq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}

static char dys_toupper(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

// ── Normalization ─────────────────────────────────────────────────────────────
// Convert raw input to uppercase, strip punctuation, collapse spaces
void dysarthria_normalize(const char* input, char* output, uint32_t max_len) {
    uint32_t j = 0;
    bool last_space = false;
    for (uint32_t i = 0; input[i] && j < max_len - 1; i++) {
        char c = dys_toupper(input[i]);
        if (c >= 'A' && c <= 'Z') {
            output[j++] = c;
            last_space = false;
        } else if (c == ' ' || c == '\t') {
            if (!last_space && j > 0) {
                output[j++] = ' ';
                last_space = true;
            }
        }
        // Drop all other characters (punctuation, numbers in speech context)
    }
    // Trim trailing space
    while (j > 0 && output[j-1] == ' ') j--;
    output[j] = 0;
}

// ── Phoneme class of a character ─────────────────────────────────────────────
static phoneme_class_t get_phoneme(char c) {
    if (c >= 'A' && c <= 'Z') return g_phoneme_map[c - 'A'];
    if (c == ' ') return PHON_UNKNOWN;
    return PHON_UNKNOWN;
}

// ── Phoneme substitution cost ─────────────────────────────────────────────────
// Same phoneme class = low cost (likely a dysarthric substitution)
// Adjacent phoneme class = medium cost
// Distant phoneme class = high cost
static uint32_t phoneme_cost(char a, char b) {
    if (a == b) return 0;
    phoneme_class_t pa = get_phoneme(a);
    phoneme_class_t pb = get_phoneme(b);
    if (pa == pb) return 1;          // same class: voiced/unvoiced swap, etc.
    // Adjacent classes (common dysarthric confusions)
    if ((pa == PHON_STOP && pb == PHON_FRIC) ||
        (pa == PHON_FRIC && pb == PHON_STOP)) return 1;
    if ((pa == PHON_LIQUID && pb == PHON_APPROX) ||
        (pa == PHON_APPROX && pb == PHON_LIQUID)) return 1;
    if ((pa == PHON_VOWEL && pb == PHON_APPROX) ||
        (pa == PHON_APPROX && pb == PHON_VOWEL)) return 2;
    return 3;                        // distant classes: high cost
}

// ── Levenshtein edit distance (phoneme-aware) ─────────────────────────────────
// Standard Levenshtein but substitution cost is phoneme-based, not binary.
// This means "FASHER" is much closer to "FASTER" than "NITRO" is.
#define DYS_MAX_LEN 32
uint32_t dysarthria_edit_distance(const char* a, const char* b) {
    uint32_t la = dys_strlen(a);
    uint32_t lb = dys_strlen(b);
    if (la > DYS_MAX_LEN) la = DYS_MAX_LEN;
    if (lb > DYS_MAX_LEN) lb = DYS_MAX_LEN;

    // dp[i][j] = edit distance between a[0..i-1] and b[0..j-1]
    static uint32_t dp[DYS_MAX_LEN+1][DYS_MAX_LEN+1];

    for (uint32_t i = 0; i <= la; i++) dp[i][0] = i;
    for (uint32_t j = 0; j <= lb; j++) dp[0][j] = j;

    for (uint32_t i = 1; i <= la; i++) {
        for (uint32_t j = 1; j <= lb; j++) {
            uint32_t cost = phoneme_cost(a[i-1], b[j-1]);
            uint32_t del  = dp[i-1][j] + 1;
            uint32_t ins  = dp[i][j-1] + 1;
            uint32_t sub  = dp[i-1][j-1] + cost;
            dp[i][j] = del < ins ? del : ins;
            if (sub < dp[i][j]) dp[i][j] = sub;
        }
    }
    return dp[la][lb];
}

// ── Phonetic distance (simplified Soundex-style) ──────────────────────────────
// Reduces both strings to their phoneme class sequence, then compares.
// "FASHER" → FRIC-VOWEL-FRIC-FRIC-LIQUID → same as "FASTER"
uint32_t dysarthria_phonetic_distance(const char* a, const char* b) {
    char pa[DYS_MAX_LEN], pb[DYS_MAX_LEN];
    uint32_t ia = 0, ib = 0;
    // Build phoneme class string for a
    for (uint32_t i = 0; a[i] && ia < DYS_MAX_LEN-1; i++) {
        char cls = '0' + (char)get_phoneme(a[i]);
        if (ia == 0 || pa[ia-1] != cls) pa[ia++] = cls; // collapse repeats
    }
    pa[ia] = 0;
    // Build phoneme class string for b
    for (uint32_t i = 0; b[i] && ib < DYS_MAX_LEN-1; i++) {
        char cls = '0' + (char)get_phoneme(b[i]);
        if (ib == 0 || pb[ib-1] != cls) pb[ib++] = cls;
    }
    pb[ib] = 0;
    return dysarthria_edit_distance(pa, pb);
}

// ── Prefix match ──────────────────────────────────────────────────────────────
// Returns true if input is a prefix of target with at least min_len chars.
// Used for truncated speech: "NIT" → NITRO
bool dysarthria_is_prefix_match(const char* input, const char* target, uint32_t min_len) {
    uint32_t li = dys_strlen(input);
    if (li < min_len) return false;
    for (uint32_t i = 0; i < li; i++) {
        if (!target[i]) return false;
        if (input[i] != target[i]) return false;
    }
    return true;
}

// ── Context prior ─────────────────────────────────────────────────────────────
// Commands used recently get a confidence boost.
// If Landon just said FASTER twice, a slurred "FAS" is very likely FASTER again.
static uint32_t context_boost(uint32_t cmd_id) {
    uint32_t boost = 0;
    uint32_t head = g_state.history_head;
    for (uint32_t i = 0; i < DYSARTHRIA_HISTORY_SIZE; i++) {
        uint32_t idx = (head + DYSARTHRIA_HISTORY_SIZE - 1 - i) % DYSARTHRIA_HISTORY_SIZE;
        if (g_state.history[idx] == cmd_id) {
            // More recent = more boost, but diminishing
            boost += (DYSARTHRIA_HISTORY_SIZE - i) / 4;
        }
    }
    return boost > 15 ? 15 : boost; // cap at 15 points
}

// ── Personal profile correction ───────────────────────────────────────────────
// Apply Landon's learned substitution patterns before matching.
// If CORVUS has learned that Landon says "SH" when he means "ST",
// it pre-corrects the input before running edit distance.
static void apply_profile(const char* input, char* output, uint32_t max_len) {
    if (!g_state.profile.calibrated) {
        dys_strcpy(output, input, max_len);
        return;
    }
    uint32_t j = 0;
    for (uint32_t i = 0; input[i] && j < max_len - 1; i++) {
        char c = input[i];
        if (c >= 'A' && c <= 'Z') {
            uint8_t conf = g_state.profile.sub_confidence[c - 'A'];
            if (conf > 70 && g_state.profile.substitutions[c - 'A'][0]) {
                // High confidence substitution — apply it
                const char* sub = g_state.profile.substitutions[c - 'A'];
                for (uint32_t k = 0; sub[k] && j < max_len - 1; k++) {
                    output[j++] = sub[k];
                }
                continue;
            }
        }
        output[j++] = c;
    }
    output[j] = 0;
}

// ── Core matching function ────────────────────────────────────────────────────
dysarthria_match_t dysarthria_match(const char* raw_input) {
    dysarthria_match_t result = {
        .cmd_id       = 15, // VCMD_UNKNOWN
        .confidence   = 0,
        .canonical    = "UNKNOWN",
        .original     = raw_input,
        .was_corrected = false,
        .needs_confirm = false
    };

    if (!raw_input || !raw_input[0]) return result;

    // Step 1: Normalize input
    char normalized[DYS_MAX_LEN];
    dysarthria_normalize(raw_input, normalized, DYS_MAX_LEN);
    if (!normalized[0]) return result;

    // Step 2: Apply personal profile corrections
    char corrected[DYS_MAX_LEN];
    apply_profile(normalized, corrected, DYS_MAX_LEN);

    // Step 3: Score every command
    uint32_t best_cmd    = 15; // VCMD_UNKNOWN
    uint32_t best_score  = 0;
    uint32_t second_score = 0;
    const char* best_canonical = "UNKNOWN";

    for (uint32_t ci = 0; ci < g_cmd_count; ci++) {
        dysarthria_cmd_t* cmd = &g_commands[ci];
        uint32_t score = 0;

        // A: Exact match on canonical
        if (dys_streq(corrected, cmd->canonical)) {
            score = 100;
        }

        // B: Exact match on any variant
        if (score < 100) {
            for (uint32_t vi = 0; vi < cmd->variant_count; vi++) {
                if (cmd->variants[vi] && dys_streq(corrected, cmd->variants[vi])) {
                    score = 95;
                    break;
                }
            }
        }

        // C: Prefix match (truncated speech)
        if (score < 90) {
            if (dysarthria_is_prefix_match(corrected, cmd->canonical, cmd->min_match_len)) {
                uint32_t li = dys_strlen(corrected);
                uint32_t lc = dys_strlen(cmd->canonical);
                // Score based on how much of the word was said
                score = 70 + (li * 20) / lc;
                if (score > 88) score = 88;
            }
        }

        // D: Phoneme-aware edit distance on canonical
        if (score < 70) {
            uint32_t ed = dysarthria_edit_distance(corrected, cmd->canonical);
            uint32_t lc = dys_strlen(cmd->canonical);
            if (ed == 0) score = 100;
            else if (ed == 1) score = 90;
            else if (ed <= lc / 3) score = 75 - (ed * 5);
            else if (ed <= lc / 2) score = 55 - (ed * 3);
            else score = 0;
        }

        // E: Phonetic distance (deeper slur tolerance)
        if (score < 60) {
            uint32_t pd = dysarthria_phonetic_distance(corrected, cmd->canonical);
            if (pd == 0) score = 80;
            else if (pd == 1) score = 65;
            else if (pd == 2) score = 50;
        }

        // F: Variant edit distance
        if (score < 50) {
            for (uint32_t vi = 0; vi < cmd->variant_count; vi++) {
                if (!cmd->variants[vi]) continue;
                uint32_t ed = dysarthria_edit_distance(corrected, cmd->variants[vi]);
                uint32_t lv = dys_strlen(cmd->variants[vi]);
                uint32_t vs = 0;
                if (ed == 0) vs = 95;
                else if (ed == 1) vs = 80;
                else if (ed <= lv / 3) vs = 65;
                if (vs > score) score = vs;
            }
        }

        // G: Context boost (recently used commands score higher)
        score += context_boost(cmd->cmd_id);
        if (score > 100) score = 100;

        // Track best and second-best
        if (score > best_score) {
            second_score   = best_score;
            best_score     = score;
            best_cmd       = cmd->cmd_id;
            best_canonical = cmd->canonical;
        } else if (score > second_score) {
            second_score = score;
        }
    }

    // Step 4: Build result
    result.cmd_id       = best_cmd;
    result.confidence   = (uint8_t)(best_score > 100 ? 100 : best_score);
    result.canonical    = best_canonical;
    result.was_corrected = !dys_streq(normalized, best_canonical);

    // If two commands are close in score, ask for confirmation
    if (best_score - second_score < 15 && best_score < DYSARTHRIA_CONFIDENCE_HIGH) {
        result.needs_confirm = true;
    }

    // Low confidence = unknown
    if (best_score < DYSARTHRIA_CONFIDENCE_LOW) {
        result.cmd_id    = 15; // VCMD_UNKNOWN
        result.canonical = "UNKNOWN";
    }

    // Step 5: Update history
    if (result.cmd_id != 15) {
        g_state.history[g_state.history_head] = result.cmd_id;
        g_state.history_head = (g_state.history_head + 1) % DYSARTHRIA_HISTORY_SIZE;
        g_commands[result.cmd_id].hit_count++;
    }

    // Step 6: Update stats
    g_state.total_processed++;
    if (result.was_corrected) g_state.total_corrected++;
    if (result.cmd_id == 15) g_state.total_failed++;

    return result;
}

// ── Learning: confirmed correction ───────────────────────────────────────────
void dysarthria_confirm(const char* raw_input, uint32_t correct_cmd_id) {
    if (!raw_input || correct_cmd_id >= g_cmd_count) return;

    char normalized[DYS_MAX_LEN];
    dysarthria_normalize(raw_input, normalized, DYS_MAX_LEN);

    // Learn character-level substitutions from this correction
    const char* canonical = g_commands[correct_cmd_id].canonical;
    uint32_t li = dys_strlen(normalized);
    uint32_t lc = dys_strlen(canonical);
    uint32_t min_len = li < lc ? li : lc;

    for (uint32_t i = 0; i < min_len; i++) {
        char said   = normalized[i];
        char target = canonical[i];
        if (said != target && said >= 'A' && said <= 'Z') {
            uint32_t idx = said - 'A';
            // Update substitution with exponential moving average
            g_state.profile.substitutions[idx][0] = target;
            g_state.profile.substitutions[idx][1] = 0;
            if (g_state.profile.sub_confidence[idx] < 90) {
                g_state.profile.sub_confidence[idx] += 5;
            }
        }
    }

    g_state.profile.total_corrections++;
    g_state.profile.session_corrections++;

    // Mark as calibrated after enough samples
    if (g_state.profile.total_corrections >= DYSARTHRIA_CALIBRATION_SAMPLES) {
        g_state.profile.calibrated = true;
    }
}

// ── Learning: rejection ───────────────────────────────────────────────────────
void dysarthria_reject(const char* raw_input) {
    (void)raw_input;
    // Decrease confidence in recent substitutions slightly
    for (uint32_t i = 0; i < 26; i++) {
        if (g_state.profile.sub_confidence[i] > 5) {
            g_state.profile.sub_confidence[i] -= 2;
        }
    }
}

// ── Init ──────────────────────────────────────────────────────────────────────
void dysarthria_init(void) {
    // Zero the state
    for (uint32_t i = 0; i < sizeof(dysarthria_state_t); i++) {
        ((uint8_t*)&g_state)[i] = 0;
    }
    g_state.active        = true;
    g_state.learning_mode = true;

    // Pre-load known patterns for cerebral palsy dysarthria
    // Based on TORGO and UASpeech dataset patterns
    // (jmaczan/asr-dysarthria: WER 0.182 on dysarthric speech)
    //
    // Common CP dysarthria substitutions:
    //   S → SH (fricative place shift)
    //   T → TH (stop → fricative)
    //   R → W  (liquid → approximant, very common in CP)
    //   K → T  (velar → alveolar backing)
    //   Final consonants often deleted

    // R → W substitution (very common in CP)
    g_state.profile.substitutions['R' - 'A'][0] = 'W';
    g_state.profile.sub_confidence['R' - 'A']   = 50;

    // S → SH: represented as S→S (same letter, different sound — handled by phoneme class)
    // T → TH: T→T (handled by phoneme class STOP)
    // These are handled by phoneme_cost() returning low cost for same-class substitutions

    terminal_write("[DYSARTHRIA] Engine initialized — learning mode active\n");
    terminal_write("[DYSARTHRIA] Phoneme-aware fuzzy matching ready\n");
}

// ── Accessors ─────────────────────────────────────────────────────────────────
dysarthria_state_t* dysarthria_get_state(void) {
    return &g_state;
}

void dysarthria_print_stats(void) {
    terminal_write("[DYSARTHRIA STATS]\n");
    terminal_write("  Processed: ");
    // Simple number print
    char buf[16];
    uint32_t n = g_state.total_processed;
    uint32_t i = 15; buf[i] = 0;
    if (n == 0) { buf[--i] = '0'; }
    while (n > 0) { buf[--i] = '0' + (n % 10); n /= 10; }
    terminal_write(buf + i);
    terminal_write("\n  Corrected: ");
    n = g_state.total_corrected; i = 15; buf[i] = 0;
    if (n == 0) { buf[--i] = '0'; }
    while (n > 0) { buf[--i] = '0' + (n % 10); n /= 10; }
    terminal_write(buf + i);
    terminal_write("\n  Calibrated: ");
    terminal_write(g_state.profile.calibrated ? "YES" : "NO (still learning)");
    terminal_write("\n");
}

void dysarthria_reset_profile(void) {
    for (uint32_t i = 0; i < 26; i++) {
        g_state.profile.substitutions[i][0] = 0;
        g_state.profile.sub_confidence[i]   = 0;
    }
    g_state.profile.calibrated          = false;
    g_state.profile.total_corrections   = 0;
    g_state.profile.session_corrections = 0;
    terminal_write("[DYSARTHRIA] Profile reset — starting fresh calibration\n");
}
