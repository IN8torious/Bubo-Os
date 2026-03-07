// =============================================================================
// Instinct OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/corvus_browser.h — Header for CORVUS Browser
// =============================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Maximum length for URL and title
#define CORVUS_BROWSER_MAX_URL_LEN 256
#define CORVUS_BROWSER_MAX_TITLE_LEN 64
#define CORVUS_BROWSER_MAX_CONTENT_BUFFER_SIZE 4096 // Example size, adjust as needed

// Structure to represent a single browser tab
typedef struct {
    char url[CORVUS_BROWSER_MAX_URL_LEN];
    char title[CORVUS_BROWSER_MAX_TITLE_LEN];
    char content_buffer[CORVUS_BROWSER_MAX_CONTENT_BUFFER_SIZE]; // Buffer for plain text content
    uint32_t scroll_offset; // Current scroll position
    bool loading; // Is the tab currently loading content?
} CorvusBrowserTab;

// Public functions for the CORVUS Browser
void browser_init();
void browser_open_url(const char* url);
void browser_tick();
void browser_draw();
void browser_handle_key(uint8_t key);
void browser_voice_cmd(uint32_t cmd_id);
