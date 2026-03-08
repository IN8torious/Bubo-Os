// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
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
// Raven OS — CORVUS Shell
// Layer 7: Natural Language Interface
// User types intent → CORVUS processes → agents act
// =============================================================================

#include "shell.h"
#include "corvus.h"
#include "vga.h"
#include "pmm.h"
#include "pit.h"
#include <stdint.h>
#include <stdbool.h>

// ── String helpers ────────────────────────────────────────────────────────────
static bool str_eq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == '\0' && *b == '\0';
}

static bool str_starts(const char* s, const char* prefix) {
    while (*prefix) {
        if (*s != *prefix) return false;
        s++; prefix++;
    }
    return true;
}

// ── Print number ──────────────────────────────────────────────────────────────
static void print_num(uint32_t n) {
    char buf[12];
    if (!n) { terminal_putchar('0'); return; }
    char tmp[12]; int i = 0;
    while (n) { tmp[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) buf[11 - i] = tmp[--i];
    // find start
    int start = 0;
    while (start < 11 && !buf[start]) start++;
    for (int j = start; j < 12; j++) if (buf[j]) terminal_putchar(buf[j]);
}

// ── Shell command handlers ────────────────────────────────────────────────────

static void cmd_help(void) {
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,  VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,       VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY,   VGA_BLACK);

    terminal_setcolor(red);
    terminal_writeline("  ╔══════════════════════════════════════════════════════════════╗");
    terminal_writeline("  ║              CORVUS Shell — Command Reference               ║");
    terminal_writeline("  ╚══════════════════════════════════════════════════════════════╝");

    terminal_setcolor(white);
    terminal_writeline("  System:");
    terminal_setcolor(grey);
    terminal_writeline("    help          — This menu");
    terminal_writeline("    clear         — Clear the screen");
    terminal_writeline("    uptime        — Show system uptime in ticks");
    terminal_writeline("    reboot        — Reboot the system");
    terminal_writeline("    halt          — Halt the system");

    terminal_setcolor(white);
    terminal_writeline("  CORVUS:");
    terminal_setcolor(grey);
    terminal_writeline("    status        — CORVUS agent dashboard");
    terminal_writeline("    agents        — List all 10 agents");
    terminal_writeline("    memory        — Physical memory statistics");
    terminal_writeline("    security scan — Run security sweep");
    terminal_writeline("    heal          — Trigger healer agent");
    terminal_writeline("    vector        — Show vector memory log");

    terminal_setcolor(white);
    terminal_writeline("  Natural language (CORVUS interprets):");
    terminal_setcolor(grey);
    terminal_writeline("    Any other input is processed by CORVUS as intent.");
    terminal_writeline("    Example: \"optimize memory\" or \"check security\"");
}

static void cmd_clear(void) {
    terminal_clear();
    // Reprint mini header
    uint8_t red = vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);
    terminal_setcolor(red);
    terminal_writeline("  Deep Flow OS — CORVUS Shell  |  Type 'help' for commands");
    terminal_writeline("");
}

static void cmd_uptime(void) {
    uint8_t white = vga_entry_color(VGA_WHITE, VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY, VGA_BLACK);
    terminal_setcolor(white);
    terminal_write("  Uptime: ");
    terminal_setcolor(grey);
    print_num(pit_get_ticks());
    terminal_write(" ticks  |  CORVUS tick: ");
    print_num(g_corvus.tick);
    terminal_writeline("");
}

static void cmd_memory(void) {
    pmm_stats_t stats = pmm_get_stats();
    uint8_t white = vga_entry_color(VGA_WHITE,       VGA_BLACK);
    uint8_t green = vga_entry_color(VGA_YELLOW, VGA_BLACK);
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,   VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY,   VGA_BLACK);

    terminal_setcolor(white);
    terminal_writeline("  Physical Memory:");
    terminal_setcolor(grey);
    terminal_write("    Total pages : "); print_num(stats.total_pages);
    terminal_write("  ("); print_num(stats.total_pages * 4); terminal_writeline(" KiB)");
    terminal_setcolor(green);
    terminal_write("    Free  pages : "); print_num(stats.free_pages);
    terminal_write("  ("); print_num(stats.free_pages * 4); terminal_writeline(" KiB)");
    terminal_setcolor(red);
    terminal_write("    Used  pages : "); print_num(stats.used_pages);
    terminal_write("  ("); print_num(stats.used_pages * 4); terminal_writeline(" KiB)");
}

static void cmd_vector(void) {
    uint8_t white = vga_entry_color(VGA_WHITE,      VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY,  VGA_BLACK);
    uint8_t green = vga_entry_color(VGA_YELLOW,VGA_BLACK);

    terminal_setcolor(white);
    terminal_writeline("  CORVUS Vector Memory (last 8 entries):");

    const char* agent_names[] = {"Crow","Heal","Sec ","Priv","Perf","Sess","File","Ntif","Diag","A11y","SYS "};
    const char* tool_names[]  = {"PMM_ALLOC","PMM_FREE ","VMM_MAP  ","VMM_UNMAP",
                                  "SCHED_BST","SCHED_DEM","LOG_WRITE","SEC_ALERT",
                                  "HEAL_PTCH","SHELL_RSP"};

    uint32_t start = (g_corvus.vector_head + VECTOR_MEM_SIZE - 8) % VECTOR_MEM_SIZE;
    for (int i = 0; i < 8; i++) {
        uint32_t idx = (start + i) % VECTOR_MEM_SIZE;
        vector_entry_t* e = &g_corvus.vector_mem[idx];
        if (e->tick == 0 && i > 0) continue;

        terminal_setcolor(grey);
        terminal_write("    [T:");
        print_num(e->tick);
        terminal_write("] ");

        uint8_t aid = e->agent_id < 11 ? e->agent_id : 10;
        terminal_setcolor(green);
        terminal_write(agent_names[aid]);
        terminal_write(" → ");

        uint8_t tid = e->tool_id < 10 ? e->tool_id : 9;
        terminal_setcolor(white);
        terminal_write(tool_names[tid]);
        terminal_write(" | ");
        terminal_setcolor(e->outcome == 0 ? green : (uint8_t)vga_entry_color(VGA_LIGHT_RED, VGA_BLACK));
        terminal_write(e->outcome == 0 ? "OK  " : (e->outcome == 1 ? "FAIL" : "BLK "));
        terminal_write(" ");
        terminal_setcolor(grey);
        terminal_writeline(e->context);
    }
}

static void cmd_reboot(void) {
    uint8_t red = vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);
    terminal_setcolor(red);
    terminal_writeline("  [CORVUS] Initiating reboot sequence...");
    // Keyboard controller reset
    outb(0x64, 0xFE);
    // If that fails, triple fault
    __asm__ volatile("cli; hlt");
}

static void cmd_halt(void) {
    uint8_t red = vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);
    terminal_setcolor(red);
    terminal_writeline("  [CORVUS] System halted. Power off safely.");
    __asm__ volatile("cli; hlt");
}

// ── Shell submit — called by keyboard driver on Enter ─────────────────────────
void corvus_shell_submit(const char* line) {
    // Route known commands directly, pass rest to CORVUS intent engine
    if (str_eq(line, "help"))          { cmd_help(); }
    else if (str_eq(line, "clear"))    { cmd_clear(); }
    else if (str_eq(line, "uptime"))   { cmd_uptime(); }
    else if (str_eq(line, "memory"))   { cmd_memory(); }
    else if (str_eq(line, "vector"))   { cmd_vector(); }
    else if (str_eq(line, "reboot"))   { cmd_reboot(); }
    else if (str_eq(line, "halt"))     { cmd_halt(); }
    else {
        // Pass to CORVUS natural language processor
        corvus_process_intent(line);
    }
}

// ── Print the shell prompt ────────────────────────────────────────────────────
void shell_print_prompt(void) {
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED, VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,      VGA_BLACK);
    terminal_setcolor(red);
    terminal_write("corvus");
    terminal_setcolor(white);
    terminal_write("> ");
}

// ── Initialize shell ──────────────────────────────────────────────────────────
void shell_init(void) {
    uint8_t red   = vga_entry_color(VGA_LIGHT_RED,  VGA_BLACK);
    uint8_t white = vga_entry_color(VGA_WHITE,       VGA_BLACK);
    uint8_t grey  = vga_entry_color(VGA_DARK_GREY,   VGA_BLACK);

    terminal_writeline("");
    terminal_setcolor(red);
    terminal_writeline("  ████████████████████████████████████████████████████████████████");
    terminal_setcolor(white);
    terminal_writeline("  CORVUS Shell — Deep Flow OS Layer 7 — Natural Language Interface");
    terminal_setcolor(grey);
    terminal_writeline("  Type 'help' for commands. All input is processed by CORVUS.");
    terminal_setcolor(red);
    terminal_writeline("  ████████████████████████████████████████████████████████████████");
    terminal_writeline("");

    shell_print_prompt();
}
