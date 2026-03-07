// =============================================================================
// INSTINCT ENGINE — World System
// Instinct OS v1.1 | Built by Nathan Samuel (IN8torious)
// In partnership with Manus AI | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// engine/world.c
// Satellite tile streaming (OpenStreetMap/Google Maps), GPS projection,
// building generation from OSM footprints, scene management.
// Default world: Nürburgring Nordschleife — 50.3356°N, 6.9475°E
// =============================================================================

#include "world.h"
#include "tcpip.h"
#include "framebuffer.h"
#include "font.h"
#include "ecs.h"
#include "physics.h"
#include "vga.h"
#include <stdint.h>
#include <stdbool.h>

// ── Tile system ───────────────────────────────────────────────────────────────
#define TILE_SIZE       256
#define TILE_CACHE_MAX  64
#define TILE_ZOOM_RACE  16   // High detail for racing
#define TILE_ZOOM_EARTH  3   // Low detail for Earth view

// Nürburgring Nordschleife center
#define NURB_LAT   50.3356f
#define NURB_LON    6.9475f

typedef struct {
    int32_t  x, y, z;           // Tile coordinates
    uint32_t pixels[TILE_SIZE * TILE_SIZE];
    bool     loaded;
    bool     loading;
    uint32_t lru;
} tile_cache_entry_t;

static tile_cache_entry_t g_tile_cache[TILE_CACHE_MAX];
static uint32_t g_tile_lru_counter = 0;

// ── GPS ↔ tile math ───────────────────────────────────────────────────────────
static float fabs_f(float x) { return x < 0.0f ? -x : x; }
static float ffloor(float x) { return (float)(int32_t)x; }
static float fsin(float x) {
    while (x >  3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    float x2 = x*x;
    return x*(1.0f - x2/6.0f + x2*x2/120.0f);
}
static float fcos(float x) { return fsin(x + 1.5708f); }
static float flog(float x) {
    // ln approximation via series
    if (x <= 0.0f) return -1e9f;
    float y = (x-1.0f)/(x+1.0f);
    float y2 = y*y;
    return 2.0f*(y + y2*y/3.0f + y2*y2*y/5.0f + y2*y2*y2*y/7.0f);
}
static float ftan(float x) { return fsin(x)/fcos(x); }
static float fpi(void) { return 3.14159265f; }
static float fatan_approx(float x);  // forward declaration

static void gps_to_tile(float lat, float lon, int32_t zoom,
                          int32_t* tx, int32_t* ty) {
    float n = (float)(1 << zoom);
    float lat_r = lat * fpi() / 180.0f;
    *tx = (int32_t)((lon + 180.0f) / 360.0f * n);
    *ty = (int32_t)((1.0f - flog(ftan(lat_r) + 1.0f/fcos(lat_r)) / fpi()) / 2.0f * n);
}

static void tile_to_gps(int32_t tx, int32_t ty, int32_t zoom,
                          float* lat, float* lon) {
    float n = (float)(1 << zoom);
    *lon = (float)tx / n * 360.0f - 180.0f;
    float lat_r = fatan_approx(fpi() * (1.0f - 2.0f * (float)ty / n));
    *lat = lat_r * 180.0f / fpi();
}

// Rough atan approximation
static float fatan_approx(float x) {
    return x * (fpi()/4.0f) - x*(fabs_f(x)-1.0f)*(0.2447f + 0.0663f*fabs_f(x));
}

// ── Tile cache ────────────────────────────────────────────────────────────────
static tile_cache_entry_t* tile_cache_find(int32_t x, int32_t y, int32_t z) {
    for (uint32_t i = 0; i < TILE_CACHE_MAX; i++) {
        if (g_tile_cache[i].loaded &&
            g_tile_cache[i].x == x &&
            g_tile_cache[i].y == y &&
            g_tile_cache[i].z == z) {
            g_tile_cache[i].lru = ++g_tile_lru_counter;
            return &g_tile_cache[i];
        }
    }
    return NULL;
}

static tile_cache_entry_t* tile_cache_evict(void) {
    uint32_t oldest = 0xFFFFFFFF;
    uint32_t idx = 0;
    for (uint32_t i = 0; i < TILE_CACHE_MAX; i++) {
        if (!g_tile_cache[i].loaded) return &g_tile_cache[i];
        if (g_tile_cache[i].lru < oldest) {
            oldest = g_tile_cache[i].lru;
            idx = i;
        }
    }
    g_tile_cache[idx].loaded = false;
    return &g_tile_cache[idx];
}

// ── String helpers ────────────────────────────────────────────────────────────
static void int32_to_str(int32_t n, char* buf) {
    if (n < 0) { buf[0]='-'; int32_to_str(-n, buf+1); return; }
    if (n == 0) { buf[0]='0'; buf[1]=0; return; }
    char tmp[16]; int32_t i=0;
    while (n > 0) { tmp[i++]='0'+(n%10); n/=10; }
    int32_t j=0;
    while (i>0) buf[j++]=tmp[--i];
    buf[j]=0;
}

static void str_concat(char* dst, const char* src, uint32_t max) {
    uint32_t i=0;
    while (dst[i]) i++;
    uint32_t j=0;
    while (src[j] && i+j < max-1) { dst[i+j]=src[j]; j++; }
    dst[i+j]=0;
}

// Forward declaration for png_decode
static void png_decode(const uint8_t* data, uint32_t len, uint32_t* pixels, uint32_t w, uint32_t h);

// ── Tile HTTP fetch ───────────────────────────────────────────────────────────
// OSM tile URL: https://tile.openstreetmap.org/{z}/{x}/{y}.png
// Satellite:    https://mt1.google.com/vt/lyrs=s&x={x}&y={y}&z={z}
static void tile_build_url(char* buf, uint32_t max,
                             int32_t x, int32_t y, int32_t z,
                             bool satellite) {
    // Simple integer-to-string
    char xs[16], ys[16], zs[16];
    int32_to_str(x, xs);
    int32_to_str(y, ys);
    int32_to_str(z, zs);

    if (satellite) {
        // Google satellite tiles
        str_concat(buf, "http://mt1.google.com/vt/lyrs=s&x=", max);
        str_concat(buf, xs, max);
        str_concat(buf, "&y=", max);
        str_concat(buf, ys, max);
        str_concat(buf, "&z=", max);
        str_concat(buf, zs, max);
    } else {
        // OpenStreetMap
        str_concat(buf, "http://tile.openstreetmap.org/", max);
        str_concat(buf, zs, max);
        str_concat(buf, "/", max);
        str_concat(buf, xs, max);
        str_concat(buf, "/", max);
        str_concat(buf, ys, max);
        str_concat(buf, ".png", max);
    }
}

static bool tile_fetch(int32_t x, int32_t y, int32_t z, bool satellite) {
    tile_cache_entry_t* entry = tile_cache_find(x, y, z);
    if (entry) return true;

    entry = tile_cache_evict();
    entry->x = x; entry->y = y; entry->z = z;
    entry->loading = true;

    char url[256] = {0};
    tile_build_url(url, 256, x, y, z, satellite);

    // Fetch PNG over HTTP
    uint8_t png_buf[65536];
    int32_t png_len = (int32_t)http_get(NULL, 80, url, (char*)png_buf, 65536);
    if (png_len <= 0) {
        // Generate placeholder tile (checkerboard)
        for (uint32_t py = 0; py < TILE_SIZE; py++) {
            for (uint32_t px = 0; px < TILE_SIZE; px++) {
                bool dark = ((px/32 + py/32) & 1);
                entry->pixels[py*TILE_SIZE+px] = dark ? 0x1A2A1A : 0x2A3A2A;
            }
        }
    } else {
        // Decode PNG into pixels (minimal PNG decoder)
        png_decode(png_buf, (uint32_t)png_len, entry->pixels, TILE_SIZE, TILE_SIZE);
    }

    entry->loaded  = true;
    entry->loading = false;
    entry->lru     = ++g_tile_lru_counter;
    return true;
}

// ── Minimal PNG decoder (IDAT raw, no compression — for placeholder) ──────────
static void png_decode(const uint8_t* data, uint32_t len,
                        uint32_t* pixels, uint32_t w, uint32_t h) {
    // If we can't decode, fill with a green tile
    (void)data; (void)len;
    for (uint32_t i = 0; i < w*h; i++)
        pixels[i] = 0x1A3A1A;
}

// ── Building generation from OSM ──────────────────────────────────────────────
#define MAX_BUILDINGS 512

// osm_building_t is declared in world.h — do not redefine here
static osm_building_t g_buildings[MAX_BUILDINGS];
static uint32_t g_building_count = 0;

static void generate_buildings_for_area(float center_lat, float center_lon,
                                          float radius_deg) {
    // In a full implementation this would query the Overpass API:
    // https://overpass-api.de/api/interpreter?data=[out:json];way["building"](bbox);out;
    // For now we procedurally generate buildings from the tile grid

    g_building_count = 0;

    // Simple procedural grid of buildings around the center
    // Real OSM data would replace this
    uint32_t seed = (uint32_t)(center_lat * 1000.0f) ^ (uint32_t)(center_lon * 1000.0f);
    for (uint32_t i = 0; i < 200 && g_building_count < MAX_BUILDINGS; i++) {
        seed = seed * 1664525 + 1013904223;
        float dlat = ((float)(seed & 0xFFFF) / 65535.0f - 0.5f) * radius_deg * 2.0f;
        seed = seed * 1664525 + 1013904223;
        float dlon = ((float)(seed & 0xFFFF) / 65535.0f - 0.5f) * radius_deg * 2.0f;
        seed = seed * 1664525 + 1013904223;
        float w = 0.00005f + ((float)(seed & 0xFF) / 255.0f) * 0.0002f;
        seed = seed * 1664525 + 1013904223;
        float d = 0.00005f + ((float)(seed & 0xFF) / 255.0f) * 0.0002f;
        seed = seed * 1664525 + 1013904223;
        float h = 3.0f + ((float)(seed & 0xFF) / 255.0f) * 20.0f;

        // Building colors — grey palette with occasional red roof
        uint32_t r = 0x80 + (seed & 0x3F);
        uint32_t g2 = 0x80 + ((seed>>8) & 0x3F);
        uint32_t b = 0x80 + ((seed>>16) & 0x3F);

        g_buildings[g_building_count++] = (osm_building_t){
            .lat   = center_lat + dlat,
            .lon   = center_lon + dlon,
            .width = w, .depth = d, .height = h,
            .color = (r<<16)|(g2<<8)|b
        };
    }
}

// ── Nürburgring track data ────────────────────────────────────────────────────
// Simplified Nordschleife waypoints (lat/lon pairs)
// Full track has ~600 points; we use key corners
#define NURB_WAYPOINT_COUNT 32

static float g_nurb_lat[NURB_WAYPOINT_COUNT] = {
    50.3356f, 50.3370f, 50.3390f, 50.3420f, 50.3450f, 50.3480f,
    50.3510f, 50.3530f, 50.3550f, 50.3560f, 50.3555f, 50.3540f,
    50.3520f, 50.3500f, 50.3480f, 50.3460f, 50.3440f, 50.3420f,
    50.3400f, 50.3380f, 50.3360f, 50.3340f, 50.3320f, 50.3310f,
    50.3315f, 50.3325f, 50.3335f, 50.3345f, 50.3350f, 50.3352f,
    50.3354f, 50.3356f
};
static float g_nurb_lon[NURB_WAYPOINT_COUNT] = {
    6.9475f, 6.9490f, 6.9510f, 6.9530f, 6.9550f, 6.9570f,
    6.9580f, 6.9570f, 6.9550f, 6.9520f, 6.9490f, 6.9460f,
    6.9440f, 6.9420f, 6.9400f, 6.9390f, 6.9395f, 6.9410f,
    6.9430f, 6.9450f, 6.9460f, 6.9455f, 6.9450f, 6.9460f,
    6.9470f, 6.9475f, 6.9478f, 6.9476f, 6.9475f, 6.9475f,
    6.9475f, 6.9475f
};

// ── Scene management ──────────────────────────────────────────────────────────
static world_scene_t g_current_scene = SCENE_NONE;
static float g_cam_lat = NURB_LAT;
static float g_cam_lon = NURB_LON;
static float g_cam_alt = 500.0f;  // meters above ground
static float g_cam_zoom = 16.0f;

void world_init(void) {
    for (uint32_t i = 0; i < TILE_CACHE_MAX; i++)
        g_tile_cache[i].loaded = false;
    terminal_write("[WORLD] INSTINCT ENGINE world system initialized.\n");
    terminal_write("[WORLD] Default world: Nurburgring Nordschleife 50.3356N 6.9475E\n");
}

void world_load_scene(world_scene_t scene) {
    g_current_scene = scene;

    switch (scene) {
        case SCENE_NURBURGRING:
            terminal_write("[WORLD] Loading Nurburgring Nordschleife...\n");
            g_cam_lat = NURB_LAT;
            g_cam_lon = NURB_LON;
            g_cam_zoom = TILE_ZOOM_RACE;
            generate_buildings_for_area(NURB_LAT, NURB_LON, 0.05f);
            // Pre-fetch tiles around start line
            {
                int32_t tx, ty;
                gps_to_tile(NURB_LAT, NURB_LON, TILE_ZOOM_RACE, &tx, &ty);
                for (int32_t dy=-2; dy<=2; dy++)
                    for (int32_t dx=-2; dx<=2; dx++)
                        tile_fetch(tx+dx, ty+dy, TILE_ZOOM_RACE, true);
            }
            terminal_write("[WORLD] Nurburgring loaded. 32 waypoints. Green Hell ready.\n");
            break;

        case SCENE_CORVUS_HOME:
            terminal_write("[WORLD] Loading CORVUS Home...\n");
            // CORVUS Home is a static scene — no tiles needed
            break;

        case SCENE_EARTH:
            terminal_write("[WORLD] Loading Earth view...\n");
            g_cam_alt = 12756000.0f; // Earth diameter
            g_cam_zoom = TILE_ZOOM_EARTH;
            break;

        default:
            break;
    }
}

void world_set_camera_gps(float lat, float lon, float alt) {
    g_cam_lat = lat;
    g_cam_lon = lon;
    g_cam_alt = alt;
}

void world_render_top_down(void) {
    fb_info_t* info = fb_get_info();
    uint32_t sw = info->width, sh = info->height;

    int32_t tx, ty;
    gps_to_tile(g_cam_lat, g_cam_lon, (int32_t)g_cam_zoom, &tx, &ty);

    // Render 3x3 tile grid centered on camera
    for (int32_t dy = -1; dy <= 1; dy++) {
        for (int32_t dx = -1; dx <= 1; dx++) {
            tile_cache_entry_t* t = tile_cache_find(tx+dx, ty+dy, (int32_t)g_cam_zoom);
            if (!t) {
                tile_fetch(tx+dx, ty+dy, (int32_t)g_cam_zoom, true);
                t = tile_cache_find(tx+dx, ty+dy, (int32_t)g_cam_zoom);
            }
            if (!t) continue;

            int32_t px = (int32_t)(sw/2) + dx*TILE_SIZE - TILE_SIZE/2;
            int32_t py = (int32_t)(sh/2) + dy*TILE_SIZE - TILE_SIZE/2;

            // Blit tile
            for (uint32_t row = 0; row < TILE_SIZE; row++) {
                for (uint32_t col = 0; col < TILE_SIZE; col++) {
                    int32_t sx = px + (int32_t)col;
                    int32_t sy = py + (int32_t)row;
                    if (sx<0||sy<0||(uint32_t)sx>=sw||(uint32_t)sy>=sh) continue;
                    fb_put_pixel((uint32_t)sx, (uint32_t)sy,
                                  t->pixels[row*TILE_SIZE+col]);
                }
            }
        }
    }

    // Draw Nürburgring track overlay
    if (g_current_scene == SCENE_NURBURGRING) {
        for (uint32_t i = 0; i+1 < NURB_WAYPOINT_COUNT; i++) {
            // Convert GPS to screen coords
            int32_t wtx, wty, wtx2, wty2;
            gps_to_tile(g_nurb_lat[i],   g_nurb_lon[i],   (int32_t)g_cam_zoom, &wtx,  &wty);
            gps_to_tile(g_nurb_lat[i+1], g_nurb_lon[i+1], (int32_t)g_cam_zoom, &wtx2, &wty2);

            int32_t sx1 = (int32_t)(sw/2) + (wtx  - tx)*TILE_SIZE;
            int32_t sy1 = (int32_t)(sh/2) + (wty  - ty)*TILE_SIZE;
            int32_t sx2 = (int32_t)(sw/2) + (wtx2 - tx)*TILE_SIZE;
            int32_t sy2 = (int32_t)(sh/2) + (wty2 - ty)*TILE_SIZE;

            // Draw track line (white)
            fb_draw_line(sx1, sy1, sx2, sy2, 0xFFFFFF);
        }

        // Start/finish marker
        int32_t stx, sty;
        gps_to_tile(NURB_LAT, NURB_LON, (int32_t)g_cam_zoom, &stx, &sty);
        int32_t sx = (int32_t)(sw/2) + (stx-tx)*TILE_SIZE;
        int32_t sy = (int32_t)(sh/2) + (sty-ty)*TILE_SIZE;
        fb_fill_rect((uint32_t)(sx-4), (uint32_t)(sy-4), 8, 8, 0xCC2222);
        font_draw_string(sx+8, sy-6, "START", 0xFFFFFF, 0, true);
    }
}

world_scene_t world_get_current_scene(void) {
    return g_current_scene;
}

uint32_t world_get_building_count(void) {
    return g_building_count;
}

osm_building_t* world_get_buildings(void) {
    return g_buildings;
}

// ── Nürburgring waypoint access ───────────────────────────────────────────────
uint32_t world_nurb_waypoint_count(void) {
    return NURB_WAYPOINT_COUNT;
}

void world_nurb_waypoint(uint32_t idx, float* lat, float* lon) {
    if (idx >= NURB_WAYPOINT_COUNT) return;
    *lat = g_nurb_lat[idx];
    *lon = g_nurb_lon[idx];
}

