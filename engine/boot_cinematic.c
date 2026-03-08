// =============================================================================
// INSTINCT ENGINE — Boot Cinematic
// Deep Flow OS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// engine/boot_cinematic.c
// Phase 1: Earth sphere renders on black screen (3s)
// Phase 2: Camera zooms toward Europe → Germany → Nürburgring (4s)
// Phase 3: Satellite tiles load, track appears (2s)
// Phase 4: Drop into cockpit (1s)
// Phase 5: Minimize to taskbar, desktop appears
// =============================================================================

#include "boot_cinematic.h"
#include "framebuffer.h"
#include "font.h"
#include "polish.h"
#include "world.h"
#include "vga.h"
#include "pit.h"
#include <stdint.h>
#include <stdbool.h>

// ── Math helpers ──────────────────────────────────────────────────────────────
static float fsin(float x) {
    while (x >  3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    float x2=x*x;
    return x*(1.0f - x2/6.0f + x2*x2/120.0f);
}
static float fcos(float x) { return fsin(x+1.5708f); }
static float fsqrt(float x) {
    if (x<=0) return 0;
    float r=x; for(int i=0;i<16;i++) r=(r+x/r)*0.5f;
    return r;
}
static float flerp(float a, float b, float t) { return a+(b-a)*t; }
static float fclamp(float x, float mn, float mx) {
    return x<mn?mn:(x>mx?mx:x);
}
static float fabs_f(float x) { return x<0?-x:x; }
static float fpi(void) { return 3.14159265f; }

// ── Cinematic state ───────────────────────────────────────────────────────────
typedef enum {
    CIN_PHASE_EARTH    = 0,  // 0–3s: Earth sphere
    CIN_PHASE_ZOOM     = 1,  // 3–7s: Zoom to Nürburgring
    CIN_PHASE_TILES    = 2,  // 7–9s: Satellite tiles load
    CIN_PHASE_COCKPIT  = 3,  // 9–10s: Drop into cockpit
    CIN_PHASE_DONE     = 4,  // Minimize, show desktop
} cin_phase_t;

static cin_phase_t g_phase      = CIN_PHASE_EARTH;
static float       g_phase_time = 0.0f;
static float       g_total_time = 0.0f;
static bool        g_done       = false;

// ── Earth sphere renderer ─────────────────────────────────────────────────────
// Ray-sphere intersection, simple lat/lon texture lookup
// Continent colors from a hardcoded palette

static uint32_t earth_color(float lat_deg, float lon_deg) {
    // Very simplified continent detection
    // North America
    if (lat_deg > 15 && lat_deg < 75 && lon_deg > -170 && lon_deg < -50)
        return 0x2A5A2A;  // Green land
    // South America
    if (lat_deg > -60 && lat_deg < 15 && lon_deg > -85 && lon_deg < -30)
        return 0x3A6A2A;
    // Europe
    if (lat_deg > 35 && lat_deg < 72 && lon_deg > -12 && lon_deg < 45)
        return 0x4A7A3A;
    // Africa
    if (lat_deg > -40 && lat_deg < 38 && lon_deg > -20 && lon_deg < 55)
        return 0x8A7A3A;  // Tan/brown
    // Asia
    if (lat_deg > 0 && lat_deg < 75 && lon_deg > 45 && lon_deg < 150)
        return 0x3A6A3A;
    // Australia
    if (lat_deg > -45 && lat_deg < -10 && lon_deg > 110 && lon_deg < 155)
        return 0x9A8A4A;
    // Antarctica
    if (lat_deg < -65) return 0xDDEEFF;
    // Ocean
    return 0x1A3A6A;
}

static void render_earth(float cx, float cy, float radius,
                           float rotation, float zoom_t) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    float r2 = radius * radius;

    for (uint32_t py = 0; py < sh; py++) {
        for (uint32_t px = 0; px < sw; px++) {
            float dx = (float)px - cx;
            float dy = (float)py - cy;
            float d2 = dx*dx + dy*dy;

            if (d2 > r2) {
                // Space — star field
                uint32_t seed = px*1664525 + py*1013904223;
                if ((seed & 0x3FF) == 0) {
                    uint8_t brightness = 0xAA + (seed>>24 & 0x55);
                    fb_put_pixel(px, py, (brightness<<16)|(brightness<<8)|brightness);
                } else {
                    fb_put_pixel(px, py, 0x000008);
                }
                continue;
            }

            // Point on sphere
            float dz = fsqrt(r2 - d2);
            // Normal vector
            float nx = dx/radius, ny = dy/radius, nz = dz/radius;

            // Convert to lat/lon
            float lat = fsin(ny) * 90.0f;  // simplified
            float lon = fsin(nx / fcos(lat * fpi() / 180.0f)) * 180.0f;
            lon += rotation * 180.0f / fpi();
            while (lon > 180.0f)  lon -= 360.0f;
            while (lon < -180.0f) lon += 360.0f;

            uint32_t base_color = earth_color(lat, lon);

            // Atmosphere glow at limb
            float limb = 1.0f - nz;
            uint32_t r = (base_color>>16)&0xFF;
            uint32_t g = (base_color>>8)&0xFF;
            uint32_t b = base_color&0xFF;

            // Add blue atmosphere
            b = (uint32_t)fclamp((float)b + limb*80.0f, 0, 255);
            r = (uint32_t)fclamp((float)r * (1.0f - limb*0.5f), 0, 255);
            g = (uint32_t)fclamp((float)g * (1.0f - limb*0.3f), 0, 255);

            // Lighting — sun from top-right
            float sun_x = 0.6f, sun_y = 0.6f, sun_z = 0.5f;
            float dot = nx*sun_x + ny*sun_y + nz*sun_z;
            dot = fclamp(dot, 0.1f, 1.0f);
            r = (uint32_t)((float)r * dot);
            g = (uint32_t)((float)g * dot);
            b = (uint32_t)((float)b * dot);

            // Cloud layer
            float cloud_noise = fsin(lat*0.3f + lon*0.2f + rotation*3.0f) *
                                fcos(lat*0.5f - lon*0.1f);
            if (cloud_noise > 0.5f) {
                float cloud_alpha = (cloud_noise - 0.5f) * 2.0f * 0.7f;
                r = (uint32_t)flerp((float)r, 220.0f, cloud_alpha);
                g = (uint32_t)flerp((float)g, 230.0f, cloud_alpha);
                b = (uint32_t)flerp((float)b, 255.0f, cloud_alpha);
            }

            fb_put_pixel(px, py, (r<<16)|(g<<8)|b);
        }
    }

    // Deep Flow OS logo
    font_draw_string(20, 20, "DEEP FLOW OS", 0xFF2020, 0, true);
    font_draw_string(20, 36, "THIS IS THE WAY", 0xFF6600, 0, true);
}

// ── Zoom phase ────────────────────────────────────────────────────────────────
static void render_zoom(float t) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    // t: 0=full Earth, 1=close on Nürburgring
    float radius = flerp((float)(sh/2 - 20), (float)(sh*4), t);
    // Camera rotates to center on Nürburgring (50.3°N, 6.9°E)
    float target_rot = -6.9f * fpi() / 180.0f;
    float rotation   = flerp(0.3f, target_rot, t);

    float cx = flerp((float)(sw/2), (float)(sw/2), t);
    float cy = flerp((float)(sh/2), (float)(sh/2 + sh*0.1f*t), t);

    render_earth(cx, cy, radius, rotation, t);

    // Crosshair on Nürburgring when close
    if (t > 0.6f) {
        float alpha = (t - 0.6f) / 0.4f;
        int32_t mx = (int32_t)(sw/2);
        int32_t my = (int32_t)(sh/2);
        uint32_t col = (uint32_t)(0xCC * alpha) << 16;
        fb_draw_line(mx-20, my, mx+20, my, col | 0x2222);
        fb_draw_line(mx, my-20, mx, my+20, col | 0x2222);
        if (alpha > 0.5f)
            font_draw_string(mx+24, my-6, "NURBURGRING", 0xFF6600, 0, true);
    }
}

// ── Tile loading phase ────────────────────────────────────────────────────────
static void render_tile_load(float t) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    // Black background fading in
    fb_fill_rect(0, 0, sw, sh, 0x050510);

    // Loading bar
    uint32_t bar_w = (uint32_t)((float)(sw-100) * t);
    fb_fill_rect(50, (int32_t)(sh/2 - 8), bar_w, 16, 0xCC2222);
    fb_draw_rect(50, (int32_t)(sh/2 - 8), sw-100, 16, 1, 0x444444);

    font_draw_string((int32_t)(sw/2 - 80), (int32_t)(sh/2 + 20),
                     "LOADING NURBURGRING NORDSCHLEIFE", 0xFF4444, 0, true);
    font_draw_string((int32_t)(sw/2 - 60), (int32_t)(sh/2 + 40),
                     "THE GREEN HELL â 20.832 KM", 0xFF6600, 0, true);

    // Render world top-down as it loads
    if (t > 0.3f) {
        world_render_top_down();
    }
}

// ── Cockpit drop phase ────────────────────────────────────────────────────────
static void render_cockpit_drop(float t) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    // Fade from top-down to cockpit view
    world_render_top_down();

    // Vignette darkening
    float alpha = t;
    uint32_t vig = (uint32_t)(alpha * 200);
    polish_vignette(sw, sh, vig);

    // CORVUS HUD fading in
    if (t > 0.5f) {
        float hud_alpha = (t - 0.5f) * 2.0f;
        uint32_t hud_col = (uint32_t)(hud_alpha * 0xCC);
        font_draw_string(20, 20, "CORVUS v1.0", (hud_col<<16)|(hud_col<<8)|hud_col, 0, true);
        font_draw_string(20, 40, "ALL SYSTEMS SOVEREIGN", (hud_col<<16)|0, 0, true);
        font_draw_string(20, 60, "SAY LAUNCH TO BEGIN", 0, (hud_col<<8)|0, true);
    }
}

// ── Public API ────────────────────────────────────────────────────────────────
void boot_cinematic_init(void) {
    g_phase      = CIN_PHASE_EARTH;
    g_phase_time = 0.0f;
    g_total_time = 0.0f;
    g_done       = false;
    terminal_write("[CIN] Boot cinematic initialized.\n");
}

bool boot_cinematic_done(void) {
    return g_done;
}

void boot_cinematic_tick(float dt) {
    if (g_done) return;

    g_phase_time += dt;
    g_total_time += dt;

    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    switch (g_phase) {
        case CIN_PHASE_EARTH: {
            float rotation = g_phase_time * 0.15f;
            render_earth((float)(sw/2), (float)(sh/2),
                          (float)(sh/2 - 20), rotation, 0.0f);
            if (g_phase_time >= 3.0f) {
                g_phase = CIN_PHASE_ZOOM;
                g_phase_time = 0.0f;
                world_load_scene(SCENE_NURBURGRING);
            }
            break;
        }

        case CIN_PHASE_ZOOM: {
            float t = fclamp(g_phase_time / 4.0f, 0.0f, 1.0f);
            render_zoom(t);
            if (g_phase_time >= 4.0f) {
                g_phase = CIN_PHASE_TILES;
                g_phase_time = 0.0f;
            }
            break;
        }

        case CIN_PHASE_TILES: {
            float t = fclamp(g_phase_time / 2.0f, 0.0f, 1.0f);
            render_tile_load(t);
            if (g_phase_time >= 2.0f) {
                g_phase = CIN_PHASE_COCKPIT;
                g_phase_time = 0.0f;
            }
            break;
        }

        case CIN_PHASE_COCKPIT: {
            float t = fclamp(g_phase_time / 1.0f, 0.0f, 1.0f);
            render_cockpit_drop(t);
            if (g_phase_time >= 1.0f) {
                g_phase = CIN_PHASE_DONE;
                g_done  = true;
                terminal_write("[CIN] Boot cinematic complete. Minimizing to taskbar.\n");
                terminal_write("[CIN] CORVUS: All systems sovereign. NO MAS DISADVANTAGED.\n");
                terminal_write("[CIN] CORVUS: Welcome back, Nathan. Say LAUNCH to begin.\n");
            }
            break;
        }

        case CIN_PHASE_DONE:
            g_done = true;
            break;
    }
}

void boot_cinematic_skip(void) {
    g_done = true;
    world_load_scene(SCENE_NURBURGRING);
    terminal_write("[CIN] Boot cinematic skipped.\n");
}
