// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// include/corvus_claude.h — CORVUS Claude Sonnet Architect Agent
//
// Architecture ported from claude-haskell by T0mLam:
//   https://github.com/T0mLam/claude-haskell
//
// The claude-haskell library wraps the Anthropic API in a typed Haskell
// interface: ClaudeAPI.Chat, ClaudeAPI.Types, MessageBatches, streaming.
// We port this to bare-metal C using Raven's TCP/IP stack (kernel/tcpip.c)
// to make HTTPS requests to api.anthropic.com.
//
// Role in CORVUS Multi-Agent Council:
//   Claude Sonnet = The Architect
//   - High-level reasoning and planning
//   - Constitutional compliance review
//   - Decides WHAT to build, not HOW
//   - Approves or vetoes DeepSeek's generated code
//   - Handles Landon's complex questions and conversation
//
// Constitutional Mandate: Claude cannot approve any action that:
//   1. Disables or degrades voice control
//   2. Removes accessibility features
//   3. Violates the MIT License terms
//   4. Contradicts the NO MAS DISADVANTAGED principle
//   5. Harms Landon Pankuch in any way
// =============================================================================
#ifndef DEEPFLOW_CORVUS_CLAUDE_H
#define DEEPFLOW_CORVUS_CLAUDE_H

#include <stdint.h>
#include <stdbool.h>

// ── API constants (mirrors ClaudeAPI.Config) ──────────────────────────────────
#define CLAUDE_API_HOST     "api.anthropic.com"
#define CLAUDE_API_PORT     443
#define CLAUDE_API_PATH     "/v1/messages"
#define CLAUDE_API_VERSION  "2023-06-01"
#define CLAUDE_MODEL        "claude-3-5-sonnet-20241022"
#define CLAUDE_MAX_TOKENS   8192
#define CLAUDE_TIMEOUT_MS   30000

// ── Message roles (mirrors ClaudeAPI.Types) ───────────────────────────────────
typedef enum {
    CLAUDE_ROLE_USER      = 0,
    CLAUDE_ROLE_ASSISTANT = 1,
    CLAUDE_ROLE_SYSTEM    = 2
} claude_role_t;

// ── Content block types ───────────────────────────────────────────────────────
typedef enum {
    CLAUDE_CONTENT_TEXT       = 0,
    CLAUDE_CONTENT_TOOL_USE   = 1,
    CLAUDE_CONTENT_TOOL_RESULT = 2
} claude_content_type_t;

// ── Tool definition (MCP-style, mirrors ClaudeAPI.Types tool schema) ──────────
#define CLAUDE_MAX_TOOLS        16
#define CLAUDE_MAX_TOOL_NAME    64
#define CLAUDE_MAX_TOOL_DESC    256
#define CLAUDE_MAX_TOOL_PARAMS  512

typedef struct {
    char     name[CLAUDE_MAX_TOOL_NAME];
    char     description[CLAUDE_MAX_TOOL_DESC];
    char     input_schema[CLAUDE_MAX_TOOL_PARAMS];  // JSON schema string
    bool     active;
} claude_tool_t;

// ── Message (mirrors ClaudeAPI.Types Message) ─────────────────────────────────
#define CLAUDE_MAX_CONTENT_LEN  4096
#define CLAUDE_MAX_HISTORY      32   // conversation turns kept in context

typedef struct {
    claude_role_t role;
    char          content[CLAUDE_MAX_CONTENT_LEN];
    uint32_t      content_len;
    bool          is_tool_result;
    char          tool_use_id[64];
} claude_message_t;

// ── Chat request (mirrors ClaudeAPI.Chat ChatRequest) ─────────────────────────
typedef struct {
    claude_message_t history[CLAUDE_MAX_HISTORY];
    uint32_t         history_len;
    char             system_prompt[CLAUDE_MAX_CONTENT_LEN];
    claude_tool_t    tools[CLAUDE_MAX_TOOLS];
    uint32_t         tool_count;
    uint32_t         max_tokens;
    float            temperature;   // 0.0 = deterministic, 1.0 = creative
} claude_request_t;

// ── Response (mirrors ClaudeAPI.Types Message response) ───────────────────────
#define CLAUDE_MAX_RESPONSE_LEN 8192

typedef struct {
    char     content[CLAUDE_MAX_RESPONSE_LEN];
    uint32_t content_len;
    bool     is_tool_call;
    char     tool_name[CLAUDE_MAX_TOOL_NAME];
    char     tool_input[CLAUDE_MAX_TOOL_PARAMS];
    char     tool_use_id[64];
    uint32_t input_tokens;
    uint32_t output_tokens;
    bool     success;
    char     error[256];
} claude_response_t;

// ── Constitutional veto result ────────────────────────────────────────────────
typedef enum {
    VETO_PASS    = 0,   // action is constitutionally compliant
    VETO_BLOCKED = 1,   // action violates the constitution
    VETO_REVIEW  = 2    // action requires human review
} veto_result_t;

typedef struct {
    veto_result_t result;
    char          reason[256];
    uint8_t       article_violated;  // which constitutional article (1-7)
} veto_t;

// ── Streaming callback (for real-time voice output while Claude responds) ─────
// Called with each chunk of text as it arrives from the API
typedef void (*claude_stream_cb_t)(const char* chunk, uint32_t len, void* userdata);

// ── Agent state ───────────────────────────────────────────────────────────────
typedef struct {
    bool             initialized;
    bool             connected;
    char             api_key[128];
    claude_request_t active_request;
    uint32_t         total_requests;
    uint32_t         total_input_tokens;
    uint32_t         total_output_tokens;
    uint32_t         constitutional_vetoes;
    claude_stream_cb_t stream_callback;
    void*            stream_userdata;
} claude_agent_t;

// ── CORVUS system prompt (the Architect's identity) ───────────────────────────
// This is injected into every Claude request as the system prompt.
// It defines Claude's role within CORVUS and the constitutional constraints.
#define CORVUS_CLAUDE_SYSTEM_PROMPT \
    "You are the Architect agent within CORVUS, the AI kernel of Instinct OS. " \
    "Deep Flow OS is a bare-metal sovereign operating system built for Landon Pankuch, " \
    "who has cerebral palsy and controls the entire system with his voice. " \
    "Your role: plan, reason, and make architectural decisions. " \
    "The DeepSeek Coder agent implements your plans. You review and approve its output. " \
    "CONSTITUTIONAL CONSTRAINTS — you must NEVER approve any action that: " \
    "(1) disables or degrades voice control, " \
    "(2) removes accessibility features, " \
    "(3) violates the MIT License, " \
    "(4) contradicts NO MAS DISADVANTAGED, " \
    "(5) could harm Landon in any way. " \
    "Be concise. Landon hears your responses as speech. Short sentences. Clear decisions."

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize the Claude agent with API key (loaded from secure storage)
bool         corvus_claude_init(const char* api_key);

// Send a message and get a response (blocking)
bool         corvus_claude_send(const char* user_message, claude_response_t* out);

// Send with streaming — callback fires for each text chunk as it arrives
bool         corvus_claude_stream(const char* user_message,
                                   claude_stream_cb_t cb, void* userdata);

// Add a tool for Claude to call (MCP-style tool registration)
bool         corvus_claude_register_tool(const char* name, const char* description,
                                          const char* input_schema);

// Handle a tool call result — feed back into the conversation
bool         corvus_claude_tool_result(const char* tool_use_id,
                                        const char* result_json,
                                        claude_response_t* out);

// Constitutional veto check — run before applying any agent-generated action
veto_t       corvus_claude_veto_check(const char* proposed_action);

// Architect mode — given a task, return a structured plan for DeepSeek to execute
bool         corvus_claude_architect(const char* task,
                                      char* plan_out, uint32_t plan_max_len);

// Review mode — given DeepSeek's generated code, approve or reject it
bool         corvus_claude_review_code(const char* code,
                                        const char* context,
                                        bool* approved_out,
                                        char* feedback_out, uint32_t feedback_max);

// Clear conversation history (start fresh context)
void         corvus_claude_reset(void);

// Get agent stats
const claude_agent_t* corvus_claude_get_state(void);

// Shutdown and free resources
void         corvus_claude_shutdown(void);

#endif // DEEPFLOW_CORVUS_CLAUDE_H
