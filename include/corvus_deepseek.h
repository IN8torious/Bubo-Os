// =============================================================================
// Instinct OS v1.2 — Dedicated to Landon Pankuch
// =============================================================================
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// include/corvus_deepseek.h — CORVUS DeepSeek Coder Research / Engineer Agent
//
// Architecture informed by:
//   DeepSeek-Coder: https://github.com/deepseek-ai/DeepSeek-Coder
//   HaskellCCEval:  https://github.com/AISE-TUDelft/HaskellCCEval
//   Mercury aider:  https://github.com/MercuryTechnologies/haskell_llm_benchmark
//
// Role in CORVUS Multi-Agent Council:
//   DeepSeek Coder = The Engineer / Researcher
//   - Deep code generation across 80+ languages including C, Haskell, Zig
//   - 16K context window for project-level code completion and infilling
//   - Fill-in-the-Middle (FIM): surgical insertion without disturbing context
//   - HumanEval-validated: must pass generated tests before any patch is accepted
//   - Receives plans from Claude Architect, returns aider-style edit-block diffs
//   - Validated on HumanEval, MultiPL-E, MBPP, DS-1000 benchmarks
// =============================================================================
#ifndef INSTINCT_CORVUS_DEEPSEEK_H
#define INSTINCT_CORVUS_DEEPSEEK_H

#include <stdint.h>
#include <stdbool.h>

// ── API constants ─────────────────────────────────────────────────────────────
#define DEEPSEEK_API_HOST    "api.deepseek.com"
#define DEEPSEEK_API_PORT    443
#define DEEPSEEK_API_PATH    "/v1/chat/completions"
#define DEEPSEEK_MODEL       "deepseek-coder"
#define DEEPSEEK_MAX_TOKENS  4096
#define DEEPSEEK_TIMEOUT_MS  45000

// ── Edit block format (aider SEARCH/REPLACE, from Mercury benchmark) ──────────
// DeepSeek returns code changes in this exact format.
// CORVUS parses and applies them surgically to source files in the VFS.
#define EDITBLOCK_SEARCH_MARKER   "<<<<<<< SEARCH"
#define EDITBLOCK_DIVIDER_MARKER  "======="
#define EDITBLOCK_REPLACE_MARKER  ">>>>>>> REPLACE"
#define EDITBLOCK_MAX_BLOCKS      32
#define EDITBLOCK_MAX_CONTENT     2048

typedef struct {
    char     filename[256];
    char     search[EDITBLOCK_MAX_CONTENT];
    char     replace[EDITBLOCK_MAX_CONTENT];
    uint32_t search_len;
    uint32_t replace_len;
    bool     applied;
    bool     verified;
} edit_block_t;

typedef struct {
    edit_block_t blocks[EDITBLOCK_MAX_BLOCKS];
    uint32_t     count;
    char         raw_response[16384];
    bool         parse_success;
} edit_diff_t;

// ── FIM (Fill-in-the-Middle) request ─────────────────────────────────────────
// DeepSeek's signature capability — given prefix and suffix, fill the middle.
// Used for surgical function insertion without disturbing surrounding code.
// DeepSeek-Coder uses special tokens: <|fim_begin|>, <|fim_hole|>, <|fim_end|>
typedef struct {
    char     prefix[4096];   // code before the insertion point
    char     suffix[4096];   // code after the insertion point
    char     filename[256];  // file context for language detection
    uint32_t max_tokens;
} fim_request_t;

// ── HumanEval test harness ────────────────────────────────────────────────────
// Ported from HaskellCCEval (TU Delft) and DeepSeek's own evaluation suite.
// Before any generated function is accepted, CORVUS generates unit tests for it
// and runs them in the constitutional sandbox. Must pass before Claude approves.
#define HUMANEVAL_MAX_TESTS     8
#define HUMANEVAL_MAX_TEST_LEN  512

typedef struct {
    char     function_name[128];
    char     test_cases[HUMANEVAL_MAX_TESTS][HUMANEVAL_MAX_TEST_LEN];
    uint32_t test_count;
    bool     results[HUMANEVAL_MAX_TESTS];
    uint32_t passed;
    uint32_t failed;
    float    pass_rate;   // 0.0 to 1.0 — must be >= 0.875 (7/8) to accept
} humaneval_result_t;

// ── Code generation modes ─────────────────────────────────────────────────────
typedef enum {
    DS_MODE_INSTRUCT  = 0,  // instruction-following (default)
    DS_MODE_FIM       = 1,  // fill-in-the-middle
    DS_MODE_COMPLETE  = 2   // raw completion
} deepseek_mode_t;

typedef struct {
    deepseek_mode_t  mode;
    char             prompt[8192];
    char             system_context[2048];  // repo map + relevant file contents
    char             language[32];          // "c", "haskell", "zig", etc.
    uint32_t         max_tokens;
    float            temperature;
    bool             validate_with_humaneval;
    bool             require_edit_blocks;   // force aider SEARCH/REPLACE format
} deepseek_request_t;

// ── Response ──────────────────────────────────────────────────────────────────
typedef struct {
    char               raw_code[8192];
    edit_diff_t        diff;              // parsed edit blocks
    humaneval_result_t validation;        // test results (if validation enabled)
    bool               claude_approved;   // set after Claude Architect reviews
    char               claude_feedback[512];
    uint32_t           input_tokens;
    uint32_t           output_tokens;
    bool               success;
    char               error[256];
} deepseek_response_t;

// ── Repo map (compressed codebase context, like aider's ctags map) ────────────
// A compact summary of all kernel symbols, function signatures, and struct
// definitions. Fits in DeepSeek's 16K context window even for large codebases.
#define REPOMAP_MAX_SYMBOLS  512
#define REPOMAP_MAX_SYM_LEN  128

typedef struct {
    char     filename[256];
    char     symbol[REPOMAP_MAX_SYM_LEN];
    char     signature[256];
    uint8_t  kind;   // 0=function, 1=struct, 2=enum, 3=define, 4=typedef
} repomap_symbol_t;

typedef struct {
    repomap_symbol_t symbols[REPOMAP_MAX_SYMBOLS];
    uint32_t         count;
    char             summary[4096];  // compressed text for LLM context
    uint32_t         last_updated;   // kernel tick of last refresh
} repomap_t;

// ── Agent state ───────────────────────────────────────────────────────────────
typedef struct {
    bool     initialized;
    char     api_key[128];
    repomap_t repomap;
    uint32_t  total_requests;
    uint32_t  patches_generated;
    uint32_t  patches_accepted;
    uint32_t  patches_rejected;
    uint32_t  humaneval_pass_total;
    uint32_t  humaneval_fail_total;
    float     lifetime_pass_rate;
} deepseek_agent_t;

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize the DeepSeek agent with API key
bool         corvus_deepseek_init(const char* api_key);

// Generate code from an instruction (instruct mode)
bool         corvus_deepseek_generate(const deepseek_request_t* req,
                                       deepseek_response_t* out);

// Fill-in-the-middle: insert code between prefix and suffix
bool         corvus_deepseek_fim(const fim_request_t* req,
                                  char* out_code, uint32_t max_len);

// Parse DeepSeek's raw response into edit blocks
bool         corvus_deepseek_parse_diff(const char* raw, edit_diff_t* out);

// Apply a parsed edit diff to the kernel's VFS
bool         corvus_deepseek_apply_diff(const edit_diff_t* diff);

// Run HumanEval-style validation on a generated function in the sandbox
bool         corvus_deepseek_validate(const char* code,
                                       const char* function_name,
                                       humaneval_result_t* out);

// Build or refresh the repo map from the kernel's VFS
void         corvus_deepseek_build_repomap(repomap_t* out);

// Full pipeline: receive Claude's architect plan → generate → validate → return
bool         corvus_deepseek_execute_plan(const char* architect_plan,
                                           deepseek_response_t* out);

// Get agent stats
const deepseek_agent_t* corvus_deepseek_get_state(void);

// Shutdown
void         corvus_deepseek_shutdown(void);

#endif // INSTINCT_CORVUS_DEEPSEEK_H
