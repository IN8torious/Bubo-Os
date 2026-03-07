// =============================================================================
// Instinct OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/screen_recorder.c — Framebuffer Screen Recorder
// One voice command. Your world, captured. No third-party software.
// =============================================================================

#include "screen_recorder.h"
#include "framebuffer.h"
#include "font.h"
#include "vmm.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_FRAMES      300     // ~5 seconds at 60fps
#define FRAME_STRIDE    4       // RGBA bytes per pixel

static struct {
    bool     recording;
    bool     has_frames;
    uint32_t frame_count;
    uint32_t frame_w;
    uint32_t frame_h;
    uint32_t tick;
    uint8_t* frame_buf;   // kmalloc'd ring buffer
    uint32_t buf_size;
} g_rec;

void recorder_init(void) {
    g_rec.recording   = false;
    g_rec.has_frames  = false;
    g_rec.frame_count = 0;
    g_rec.tick        = 0;
    g_rec.frame_buf   = (uint8_t*)0;
    g_rec.buf_size    = 0;

    fb_info_t* info = fb_get_info();
    g_rec.frame_w = info->width;
    g_rec.frame_h = info->height;

    // Allocate ring buffer for MAX_FRAMES frames (compressed — store every 4th pixel)
    uint32_t frame_bytes = (g_rec.frame_w / 4) * (g_rec.frame_h / 4) * FRAME_STRIDE;
    g_rec.buf_size = frame_bytes * MAX_FRAMES;
    g_rec.frame_buf = (uint8_t*)kmalloc(g_rec.buf_size);
}

void recorder_start(void) {
    if (g_rec.recording) return;
    if (!g_rec.frame_buf) recorder_init();
    g_rec.recording   = true;
    g_rec.frame_count = 0;
    g_rec.tick        = 0;
}

void recorder_stop(void) {
    if (!g_rec.recording) return;
    g_rec.recording  = false;
    g_rec.has_frames = (g_rec.frame_count > 0);
}

bool recorder_is_recording(void) { return g_rec.recording; }
uint32_t recorder_frame_count(void) { return g_rec.frame_count; }

void recorder_tick(void) {
    if (!g_rec.recording || !g_rec.frame_buf) return;
    g_rec.tick++;

    // Capture every 2nd tick (~30fps effective)
    if (g_rec.tick & 1) return;
    if (g_rec.frame_count >= MAX_FRAMES) {
        // Ring: wrap around
        g_rec.frame_count = 0;
    }

    // Downsample 4x4 — store 1 pixel per 4x4 block
    uint32_t fw = g_rec.frame_w / 4;
    uint32_t fh = g_rec.frame_h / 4;
    uint32_t frame_bytes = fw * fh * FRAME_STRIDE;
    uint8_t* dst = g_rec.frame_buf + g_rec.frame_count * frame_bytes;

    uint32_t* fb = (uint32_t*)fb_get_info()->addr;
    uint32_t stride = g_rec.frame_w;

    for (uint32_t fy = 0; fy < fh; fy++) {
        for (uint32_t fx = 0; fx < fw; fx++) {
            uint32_t px = fx * 4, py = fy * 4;
            uint32_t pixel = fb[py * stride + px];
            uint32_t idx = (fy * fw + fx) * FRAME_STRIDE;
            dst[idx+0] = (uint8_t)((pixel >> 16) & 0xFF); // R
            dst[idx+1] = (uint8_t)((pixel >>  8) & 0xFF); // G
            dst[idx+2] = (uint8_t)( pixel        & 0xFF); // B
            dst[idx+3] = 0xFF;                             // A
        }
    }
    g_rec.frame_count++;
}

void recorder_draw_indicator(void) {
    if (!g_rec.recording) return;
    // Pulsing REC indicator top-left
    uint32_t col = (g_rec.tick & 16) ? 0xFF0000 : 0x880000;
    fb_fill_rect(4, 4, 10, 10, col);
    font_draw_string(18, 6, "REC", 0xFF0000, 0, true);
    // Frame count
    char buf[12];
    uint32_t f = g_rec.frame_count;
    buf[0]='0'+(f/1000)%10; buf[1]='0'+(f/100)%10;
    buf[2]='0'+(f/10)%10;   buf[3]='0'+(f%10);
    buf[4]='f'; buf[5]=0;
    font_draw_string(48, 6, buf, 0x888888, 0, true);
}

// Export: write a minimal PPM sequence header to the VFS
// (Full VFS write integration done in vfs layer)
void recorder_export(const char* path) {
    (void)path;
    // Stub — full VFS write integration in v1.2
    // For now, frames are in g_rec.frame_buf ready for export
}
