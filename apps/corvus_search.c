// =============================================================================
// Deep Flow OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/corvus_search.c — CORVUS Universal Search Bar
// Searches apps, files, agents, commands, and the web in one bar.
// =============================================================================

#include "corvus_search.h"
#include "framebuffer.h"
#include "font.h"
#include "polish.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_RESULTS  8
#define QUERY_LEN   64

typedef struct {
    char     label[48];
    char     subtitle[32];
    uint8_t  type;   // 0=app 1=file 2=agent 3=cmd 4=web
} search_result_t;

static struct {
    bool            open;
    char            query[QUERY_LEN];
    uint32_t        query_len;
    search_result_t results[MAX_RESULTS];
    uint32_t        result_count;
    int32_t         selected;
    int32_t         bar_x, bar_y, bar_w;
} g_search;

// Built-in app list
static const char* app_names[] = {
    "BUBO Dashboard", "Accessibility Hub", "Terminal",
    "File Manager", "Settings", "CORVUS Browser",
    "Screen Recorder", "CORVUS Live Mode", 0
};

static uint32_t str_len(const char* s) {
    uint32_t n = 0; while (s[n]) n++; return n;
}
static bool str_contains(const char* hay, const char* needle) {
    uint32_t hl = str_len(hay), nl = str_len(needle);
    if (nl > hl) return false;
    for (uint32_t i = 0; i <= hl - nl; i++) {
        bool match = true;
        for (uint32_t j = 0; j < nl; j++) {
            char a = hay[i+j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a += 32;
            if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) { match = false; break; }
        }
        if (match) return true;
    }
    return false;
}
static void str_copy(char* dst, const char* src, uint32_t max) {
    uint32_t i = 0;
    while (i < max - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

static void do_search(void) {
    g_search.result_count = 0;
    if (g_search.query_len == 0) return;

    // Search apps
    for (uint32_t i = 0; app_names[i] != 0 && g_search.result_count < MAX_RESULTS; i++) {
        if (str_contains(app_names[i], g_search.query)) {
            search_result_t* r = &g_search.results[g_search.result_count++];
            str_copy(r->label, app_names[i], 48);
            str_copy(r->subtitle, "Application", 32);
            r->type = 0;
        }
    }

    // Search CORVUS agents
    static const char* agents[] = {
        "CROW Agent","HEAL Agent","SEC Agent","MEM Agent","NET Agent",
        "PLAN Agent","LEARN Agent","PHYS Agent","VOICE Agent","DRIVE Agent", 0
    };
    for (uint32_t i = 0; agents[i] != 0 && g_search.result_count < MAX_RESULTS; i++) {
        if (str_contains(agents[i], g_search.query)) {
            search_result_t* r = &g_search.results[g_search.result_count++];
            str_copy(r->label, agents[i], 48);
            str_copy(r->subtitle, "CORVUS Agent", 32);
            r->type = 2;
        }
    }

    // Always add web search as last result
    if (g_search.result_count < MAX_RESULTS) {
        search_result_t* r = &g_search.results[g_search.result_count++];
        str_copy(r->label, "Search the web: ", 48);
        // Append query
        uint32_t ll = str_len(r->label);
        for (uint32_t i = 0; i < g_search.query_len && ll < 47; i++)
            r->label[ll++] = g_search.query[i];
        r->label[ll] = 0;
        str_copy(r->subtitle, "Open in CORVUS Browser", 32);
        r->type = 4;
    }
}

void corvus_search_init(int32_t sw, int32_t sh) {
    g_search.open        = false;
    g_search.query_len   = 0;
    g_search.query[0]    = 0;
    g_search.result_count= 0;
    g_search.selected    = 0;
    g_search.bar_w       = 600;
    g_search.bar_x       = (sw - g_search.bar_w) / 2;
    g_search.bar_y       = sh / 4;
}

void corvus_search_open(void) {
    g_search.open      = true;
    g_search.query_len = 0;
    g_search.query[0]  = 0;
    g_search.selected  = 0;
    do_search();
}

void corvus_search_close(void) {
    g_search.open = false;
}

bool corvus_search_is_open(void) {
    return g_search.open;
}

void corvus_search_voice(const char* query) {
    corvus_search_open();
    uint32_t i = 0;
    while (query[i] && i < QUERY_LEN - 1) {
        g_search.query[i] = query[i]; i++;
    }
    g_search.query[i] = 0;
    g_search.query_len = i;
    do_search();
}

void corvus_search_handle_key(uint8_t key) {
    if (!g_search.open) return;
    if (key == 0x1B) { corvus_search_close(); return; }  // ESC
    if (key == 0x0D) { corvus_search_close(); return; }  // Enter — launch selected
    if (key == 0x48) {  // Up arrow
        if (g_search.selected > 0) g_search.selected--;
        return;
    }
    if (key == 0x50) {  // Down arrow
        if (g_search.selected < (int32_t)g_search.result_count - 1)
            g_search.selected++;
        return;
    }
    if (key == 0x08) {  // Backspace
        if (g_search.query_len > 0) {
            g_search.query[--g_search.query_len] = 0;
            do_search();
        }
        return;
    }
    if (key >= 0x20 && key < 0x7F && g_search.query_len < QUERY_LEN - 1) {
        g_search.query[g_search.query_len++] = (char)key;
        g_search.query[g_search.query_len]   = 0;
        do_search();
    }
}

void corvus_search_draw(void) {
    if (!g_search.open) return;
    int32_t x = g_search.bar_x;
    int32_t y = g_search.bar_y;
    int32_t w = g_search.bar_w;
    int32_t row_h = 36;
    int32_t total_h = 52 + (int32_t)g_search.result_count * row_h + 8;

    // Backdrop shadow
    fb_fill_rect((uint32_t)(x-4), (uint32_t)(y-4),
                 (uint32_t)(w+8), (uint32_t)(total_h+8), 0x00000080);

    // Main bar body — frosted dark
    polish_glass_rect(x, y, w, total_h, 0x0D0820, 220);

    // Search input row
    fb_fill_rect((uint32_t)(x+8), (uint32_t)(y+8),
                 (uint32_t)(w-16), 36, 0x1A1030);
    // Magnifying glass icon
    font_draw_string(x + 16, y + 16, ">", 0xFFFFFF, 0, true);
    // Query text
    font_draw_string(x + 36, y + 16, g_search.query, 0xFFFFFF, 0, true);
    // Cursor blink
    uint32_t cx = (uint32_t)(x + 36 + (int32_t)g_search.query_len * 8);
    fb_fill_rect(cx, (uint32_t)(y + 14), 2, 20, 0xFF0000);

    // Results
    static const uint32_t type_colors[] = {
        0x4488FF, 0xAABBCC, 0x00DD66, 0xFFAA00, 0x44CCFF
    };
    for (uint32_t i = 0; i < g_search.result_count; i++) {
        int32_t ry = y + 52 + (int32_t)i * row_h;
        bool sel = ((int32_t)i == g_search.selected);
        if (sel)
            fb_fill_rect((uint32_t)(x+4), (uint32_t)ry,
                         (uint32_t)(w-8), (uint32_t)row_h, 0x2A1850);
        uint8_t t = g_search.results[i].type;
        uint32_t tc = (t < 5) ? type_colors[t] : 0xFFFFFF;
        font_draw_string(x + 16, ry + 4,  g_search.results[i].label,    tc, 0, true);
        font_draw_string(x + 16, ry + 20, g_search.results[i].subtitle, 0x888888, 0, true);
    }
}
