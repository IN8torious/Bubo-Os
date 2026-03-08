// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
#include "racing_game.h"
#include "framebuffer.h"
#include "deepflow_colors.h"
#include "font.h"
#include "corvus.h"
#include <stdint.h>
#include <stdbool.h>

// ── Akatsuki color palette ────────────────────────────────────────────────────
#define COLOR_BLACK       0x00000000
#define COLOR_CRIMSON     0xCC0000FF
#define COLOR_DARK_RED    0x8B0000FF
#define COLOR_WHITE       DF_ERROR_VETO
#define COLOR_GREY        0x888888FF
#define COLOR_DARK_GREY   0x333333FF
#define COLOR_GOLD        0xFFD700FF
#define COLOR_DEMON_RED   0xDC143CFF   // Landon's Demon 170 — Sinamon Red
#define COLOR_SUPRA_BLACK 0x1A1A2EFF   // CORVUS Supra — Midnight Black
#define COLOR_ASPHALT     0x2C2C2CFF
#define COLOR_ROAD_LINE   0xFFFF00FF
#define COLOR_GRASS       0x1A3A1AFF
#define COLOR_SKY         0x0D0D1AFF   // Night sky — Akatsuki moon

// ── Screen layout ─────────────────────────────────────────────────────────────
#define SCREEN_W   1024
#define SCREEN_H   768
#define TRACK_TOP  150
#define TRACK_BOT  620
#define TRACK_LEFT  80
#define TRACK_RIGHT 944

// ── Car dimensions ────────────────────────────────────────────────────────────
#define CAR_W  60
#define CAR_H  110

// ── Physics constants ─────────────────────────────────────────────────────────
#define DEMON_MAX_SPEED    280   // 280 km/h — 1,400 HP Demon 170
#define SUPRA_MAX_SPEED    260   // 260 km/h — CORVUS Supra
#define DEMON_ACCEL        18    // Brutal launch — Demon launches 0-60 in 1.66s
#define SUPRA_ACCEL        12
#define BRAKE_FORCE        25
#define DRIFT_FACTOR       85    // Demon slides — RWD + 1,400 HP

// ── Voice command intents ─────────────────────────────────────────────────────
typedef enum {
    CMD_NONE = 0,
    CMD_GO_FASTER,       // "go faster" / "punch it" / "floor it"
    CMD_SLOW_DOWN,       // "slow down" / "brake" / "easy"
    CMD_DRIFT,           // "drift" / "slide" / "sideways"
    CMD_OVERTAKE,        // "pass him" / "overtake" / "get past"
    CMD_HOLD_LINE,       // "hold it" / "stay here" / "keep going"
    CMD_NITRO,           // "nitro" / "boost" / "all out"
    CMD_PIT,             // "pit" / "stop" / "come in"
} VoiceCommand;

// ── Car state ─────────────────────────────────────────────────────────────────
typedef struct {
    int32_t  x, y;           // Position on track
    int32_t  speed;          // Current speed km/h
    int32_t  target_speed;   // Target speed
    int32_t  lane;           // 0 = left, 1 = right
    bool     drifting;
    int32_t  drift_angle;    // Degrees off-axis
    uint32_t nitro;          // Nitro charge (0-100)
    uint32_t lap;
    uint32_t race_pos;       // 1st or 2nd
    const char* driver_name;
    uint32_t color;
    uint32_t roof_color;
} RaceCar;

// ── CORVUS AI driver state ─────────────────────────────────────────────────────
typedef struct {
    VoiceCommand last_command;
    uint32_t     command_timer;
    bool         aggressive_mode;
    bool         defending;
    const char*  last_response;
} CorvusDriver;

// ── Race state ────────────────────────────────────────────────────────────────
typedef struct {
    RaceCar      demon;        // Landon's car — CORVUS drives it
    RaceCar      supra;        // CORVUS opponent
    CorvusDriver corvus;
    uint32_t     lap_total;
    uint32_t     race_tick;
    bool         race_active;
    bool         race_finished;
    uint32_t     winner;       // 1 = Landon, 2 = CORVUS
    char         hud_message[64];
    uint32_t     hud_timer;
} RaceState;

static RaceState g_race;

// ── Simple string copy (no libc) ──────────────────────────────────────────────
static void str_copy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = 0;
}

// ── Set HUD message ───────────────────────────────────────────────────────────
static void set_hud(const char* msg) {
    str_copy(g_race.hud_message, msg, 64);
    g_race.hud_timer = 180; // show for 3 seconds at 60fps
}

// ── Initialize the race ───────────────────────────────────────────────────────
void racing_game_init(void) {
    // Landon's Demon 170 — Sinamon Red, CORVUS drives it
    g_race.demon.x           = TRACK_LEFT + 120;
    g_race.demon.y           = TRACK_BOT - 200;
    g_race.demon.speed       = 0;
    g_race.demon.target_speed = 0;
    g_race.demon.lane        = 0;
    g_race.demon.drifting    = false;
    g_race.demon.drift_angle = 0;
    g_race.demon.nitro       = 50;
    g_race.demon.lap         = 1;
    g_race.demon.race_pos    = 1;
    g_race.demon.driver_name = "LANDON";
    g_race.demon.color       = COLOR_DEMON_RED;
    g_race.demon.roof_color  = COLOR_DARK_RED;

    // CORVUS Supra — Midnight Black
    g_race.supra.x           = TRACK_LEFT + 120;
    g_race.supra.y           = TRACK_BOT - 80;
    g_race.supra.speed       = 0;
    g_race.supra.target_speed = 0;
    g_race.supra.lane        = 1;
    g_race.supra.drifting    = false;
    g_race.supra.drift_angle = 0;
    g_race.supra.nitro       = 50;
    g_race.supra.lap         = 1;
    g_race.supra.race_pos    = 2;
    g_race.supra.driver_name = "BUBO";
    g_race.supra.color       = COLOR_SUPRA_BLACK;
    g_race.supra.roof_color  = COLOR_CRIMSON;

    // CORVUS driver state
    g_race.corvus.last_command   = CMD_NONE;
    g_race.corvus.command_timer  = 0;
    g_race.corvus.aggressive_mode = false;
    g_race.corvus.defending      = false;
    g_race.corvus.last_response  = "Ready to race, Landon.";

    g_race.lap_total    = 3;
    g_race.race_tick    = 0;
    g_race.race_active  = true;
    g_race.race_finished = false;
    g_race.winner       = 0;

    set_hud("CORVUS: Say 'GO' to start!");
}

// ── Parse voice command from text ─────────────────────────────────────────────
VoiceCommand racing_parse_command(const char* text) {
    // Simple keyword matching — no libc needed
    // "go faster" / "floor it" / "punch it" / "go" / "faster"
    const char* go_words[]    = {"faster", "floor", "punch", "go", "speed", "accelerate", 0};
    const char* slow_words[]  = {"slow", "brake", "easy", "stop", "careful", 0};
    const char* drift_words[] = {"drift", "slide", "sideways", "spin", 0};
    const char* pass_words[]  = {"pass", "overtake", "get past", "beat", 0};
    const char* nitro_words[] = {"nitro", "boost", "all out", "max", "everything", 0};

    // Check each word list
    for (int i = 0; go_words[i]; i++) {
        const char* w = go_words[i];
        const char* t = text;
        while (*t) {
            const char* tw = t;
            const char* ww = w;
            while (*tw && *ww && *tw == *ww) { tw++; ww++; }
            if (!*ww) return CMD_GO_FASTER;
            t++;
        }
    }
    for (int i = 0; slow_words[i]; i++) {
        const char* w = slow_words[i];
        const char* t = text;
        while (*t) {
            const char* tw = t;
            const char* ww = w;
            while (*tw && *ww && *tw == *ww) { tw++; ww++; }
            if (!*ww) return CMD_SLOW_DOWN;
            t++;
        }
    }
    for (int i = 0; drift_words[i]; i++) {
        const char* w = drift_words[i];
        const char* t = text;
        while (*t) {
            const char* tw = t;
            const char* ww = w;
            while (*tw && *ww && *tw == *ww) { tw++; ww++; }
            if (!*ww) return CMD_DRIFT;
            t++;
        }
    }
    for (int i = 0; pass_words[i]; i++) {
        const char* w = pass_words[i];
        const char* t = text;
        while (*t) {
            const char* tw = t;
            const char* ww = w;
            while (*tw && *ww && *tw == *ww) { tw++; ww++; }
            if (!*ww) return CMD_OVERTAKE;
            t++;
        }
    }
    for (int i = 0; nitro_words[i]; i++) {
        const char* w = nitro_words[i];
        const char* t = text;
        while (*t) {
            const char* tw = t;
            const char* ww = w;
            while (*tw && *ww && *tw == *ww) { tw++; ww++; }
            if (!*ww) return CMD_NITRO;
            t++;
        }
    }
    return CMD_NONE;
}

// ── CORVUS executes Landon's command ─────────────────────────────────────────
void racing_execute_command(VoiceCommand cmd) {
    g_race.corvus.last_command  = cmd;
    g_race.corvus.command_timer = 120;

    switch (cmd) {
        case CMD_GO_FASTER:
            g_race.demon.target_speed = DEMON_MAX_SPEED;
            g_race.corvus.last_response = "Flooring it, Landon!";
            set_hud("CORVUS: Flooring it!");
            break;

        case CMD_SLOW_DOWN:
            g_race.demon.target_speed = g_race.demon.speed / 2;
            g_race.demon.drifting = false;
            g_race.corvus.last_response = "Braking hard.";
            set_hud("CORVUS: Braking hard.");
            break;

        case CMD_DRIFT:
            if (g_race.demon.speed > 80) {
                g_race.demon.drifting = true;
                g_race.demon.drift_angle = 35;
                g_race.corvus.last_response = "Drifting! Hold on!";
                set_hud("CORVUS: DRIFTING!");
            } else {
                g_race.corvus.last_response = "Need more speed to drift, Landon.";
                set_hud("CORVUS: Need more speed first!");
            }
            break;

        case CMD_OVERTAKE:
            g_race.demon.target_speed = DEMON_MAX_SPEED;
            g_race.demon.lane = (g_race.demon.lane == 0) ? 1 : 0;
            g_race.corvus.aggressive_mode = true;
            g_race.corvus.last_response = "Going for the overtake!";
            set_hud("CORVUS: Overtaking!");
            break;

        case CMD_NITRO:
            if (g_race.demon.nitro >= 25) {
                g_race.demon.nitro -= 25;
                g_race.demon.speed += 60;
                if (g_race.demon.speed > DEMON_MAX_SPEED + 60)
                    g_race.demon.speed = DEMON_MAX_SPEED + 60;
                g_race.corvus.last_response = "NITRO! 1400 horses, Landon!";
                set_hud("CORVUS: NITRO ENGAGED!");
            } else {
                g_race.corvus.last_response = "Nitro depleted. Recharging.";
                set_hud("CORVUS: No nitro left!");
            }
            break;

        case CMD_HOLD_LINE:
            g_race.demon.target_speed = g_race.demon.speed;
            g_race.corvus.last_response = "Holding this line.";
            set_hud("CORVUS: Holding line.");
            break;

        default:
            break;
    }
}

// ── CORVUS AI opponent logic ───────────────────────────────────────────────────
static void corvus_ai_tick(void) {
    RaceCar* s = &g_race.supra;
    RaceCar* d = &g_race.demon;

    // CORVUS tries to keep pace but lets Landon win sometimes
    int32_t gap = d->y - s->y;

    if (gap > 150) {
        // Landon is ahead — CORVUS pushes hard
        s->target_speed = SUPRA_MAX_SPEED;
    } else if (gap < -50) {
        // CORVUS is ahead — ease off slightly (let Landon have fun)
        s->target_speed = SUPRA_MAX_SPEED - 30;
    } else {
        // Side by side — race hard
        s->target_speed = SUPRA_MAX_SPEED - 10;
    }

    // CORVUS uses nitro if falling behind by a lot
    if (gap > 300 && s->nitro >= 25 && (g_race.race_tick % 200 == 0)) {
        s->nitro -= 25;
        s->speed += 40;
    }

    // Recharge nitro slowly
    if (g_race.race_tick % 60 == 0) {
        if (s->nitro < 100) s->nitro++;
        if (d->nitro < 100) d->nitro++;
    }
}

// ── Physics update ────────────────────────────────────────────────────────────
static void update_car(RaceCar* car, int32_t accel, int32_t max_speed) {
    // Accelerate toward target
    if (car->speed < car->target_speed) {
        car->speed += accel;
        if (car->speed > car->target_speed) car->speed = car->target_speed;
    } else if (car->speed > car->target_speed) {
        car->speed -= BRAKE_FORCE;
        if (car->speed < 0) car->speed = 0;
    }
    if (car->speed > max_speed + 60) car->speed = max_speed + 60;
    if (car->speed < 0) car->speed = 0;

    // Move car up the track (y decreases = moving forward)
    car->y -= car->speed / 30;

    // Wrap around track (simple oval)
    if (car->y < TRACK_TOP + 20) {
        car->y = TRACK_BOT - 20;
        car->lap++;
        if (car == &g_race.demon) {
            set_hud("CORVUS: Lap complete, Landon!");
        }
    }

    // Drift decay
    if (car->drifting) {
        car->drift_angle -= 2;
        if (car->drift_angle <= 0) {
            car->drift_angle = 0;
            car->drifting = false;
        }
    }
}

// ── Draw a car ────────────────────────────────────────────────────────────────
static void draw_car(RaceCar* car) {
    int32_t x = car->x;
    int32_t y = car->y;

    // Car body
    fb_fill_rect(x, y, CAR_W, CAR_H, car->color);

    // Roof / cabin
    fb_fill_rect(x + 10, y + 20, CAR_W - 20, CAR_H - 50, car->roof_color);

    // Windshield
    fb_fill_rect(x + 12, y + 22, CAR_W - 24, 20, 0x88CCFFFF);

    // Headlights
    fb_fill_rect(x + 5,  y + 5,  15, 8, COLOR_GOLD);
    fb_fill_rect(x + CAR_W - 20, y + 5, 15, 8, COLOR_GOLD);

    // Taillights
    fb_fill_rect(x + 5,  y + CAR_H - 12, 15, 8, COLOR_CRIMSON);
    fb_fill_rect(x + CAR_W - 20, y + CAR_H - 12, 15, 8, COLOR_CRIMSON);

    // Driver name on car
    font_draw_string(x + 5, y + CAR_H/2 - 4, car->driver_name, COLOR_WHITE, COLOR_BLACK, false);

    // Speed indicator
    if (car->drifting) {
        font_draw_string(x - 5, y - 16, "DRIFT!", COLOR_GOLD, COLOR_BLACK, false);
    }
}

// ── Draw the track ────────────────────────────────────────────────────────────
static void draw_track(void) {
    // Sky
    fb_fill_rect(0, 0, SCREEN_W, TRACK_TOP, COLOR_SKY);

    // Grass
    fb_fill_rect(0, TRACK_TOP, SCREEN_W, TRACK_BOT - TRACK_TOP, COLOR_GRASS);

    // Track surface
    fb_fill_rect(TRACK_LEFT, TRACK_TOP + 20, TRACK_RIGHT - TRACK_LEFT, TRACK_BOT - TRACK_TOP - 40, COLOR_ASPHALT);

    // Center line
    for (int y = TRACK_TOP + 30; y < TRACK_BOT - 30; y += 40) {
        fb_fill_rect(SCREEN_W/2 - 3, y, 6, 20, COLOR_ROAD_LINE);
    }

    // Track borders
    fb_draw_rect(TRACK_LEFT, TRACK_TOP + 20, TRACK_RIGHT - TRACK_LEFT, TRACK_BOT - TRACK_TOP - 40, 2, COLOR_WHITE);

    // Red cloud logo on track surface
    fb_fill_rect(SCREEN_W/2 - 30, TRACK_TOP + 60, 60, 20, COLOR_DARK_RED);
    font_draw_string(SCREEN_W/2 - 28, TRACK_TOP + 64, "RED CLOUD", COLOR_WHITE, COLOR_DARK_RED, false);
}

// ── Draw HUD ──────────────────────────────────────────────────────────────────
static void draw_hud(void) {
    // HUD background
    fb_fill_rect(0, 0, SCREEN_W, 48, 0x000000CC);

    // Landon's info
    fb_fill_rect(8, 6, 200, 36, COLOR_DARK_RED);
    font_draw_string(14, 10, "DEMON 170", COLOR_GOLD, COLOR_DARK_RED, false);

    // Speed
    char speed_str[32];
    int spd = g_race.demon.speed;
    speed_str[0] = 'S'; speed_str[1] = 'P'; speed_str[2] = 'D'; speed_str[3] = ':';
    speed_str[4] = ' ';
    speed_str[5] = '0' + (spd / 100) % 10;
    speed_str[6] = '0' + (spd / 10) % 10;
    speed_str[7] = '0' + spd % 10;
    speed_str[8] = ' '; speed_str[9] = 'k'; speed_str[10] = 'm'; speed_str[11] = '/';
    speed_str[12] = 'h'; speed_str[13] = 0;
    font_draw_string(14, 26, speed_str, COLOR_WHITE, COLOR_DARK_RED, false);

    // Lap counter
    char lap_str[16];
    lap_str[0] = 'L'; lap_str[1] = 'A'; lap_str[2] = 'P'; lap_str[3] = ':';
    lap_str[4] = ' ';
    lap_str[5] = '0' + g_race.demon.lap;
    lap_str[6] = '/';
    lap_str[7] = '0' + g_race.lap_total;
    lap_str[8] = 0;
    font_draw_string(220, 10, lap_str, COLOR_WHITE, COLOR_BLACK, false);

    // Nitro bar
    font_draw_string(220, 26, "NOS:", COLOR_CRIMSON, COLOR_BLACK, false);
    fb_fill_rect(260, 28, 80, 12, COLOR_DARK_GREY);
    fb_fill_rect(260, 28, (int32_t)(g_race.demon.nitro * 80 / 100), 12, COLOR_CRIMSON);

    // CORVUS response message
    if (g_race.hud_timer > 0) {
        fb_fill_rect(SCREEN_W/2 - 200, 8, 400, 32, 0x00000099);
        font_draw_string(SCREEN_W/2 - 195, 16, g_race.hud_message, COLOR_GOLD, COLOR_BLACK, false);
        g_race.hud_timer--;
    }

    // CORVUS Supra info (right side)
    fb_fill_rect(SCREEN_W - 210, 6, 200, 36, COLOR_SUPRA_BLACK);
    font_draw_string(SCREEN_W - 205, 10, "CORVUS SUPRA", COLOR_CRIMSON, COLOR_SUPRA_BLACK, false);
    int cspd = g_race.supra.speed;
    char cspeed_str[16];
    cspeed_str[0] = '0' + (cspd / 100) % 10;
    cspeed_str[1] = '0' + (cspd / 10) % 10;
    cspeed_str[2] = '0' + cspd % 10;
    cspeed_str[3] = ' '; cspeed_str[4] = 'k'; cspeed_str[5] = 'm';
    cspeed_str[6] = '/'; cspeed_str[7] = 'h'; cspeed_str[8] = 0;
    font_draw_string(SCREEN_W - 205, 26, cspeed_str, COLOR_WHITE, COLOR_SUPRA_BLACK, false);

    // Bottom command hint
    fb_fill_rect(0, SCREEN_H - 28, SCREEN_W, 28, 0x000000CC);
    font_draw_string(10, SCREEN_H - 20,
        "SAY: 'FASTER' | 'BRAKE' | 'DRIFT' | 'OVERTAKE' | 'NITRO'",
        COLOR_GREY, COLOR_BLACK, false);
}

// ── Draw finish screen ────────────────────────────────────────────────────────
static void draw_finish(void) {
    fb_fill_rect(0, 0, SCREEN_W, SCREEN_H, COLOR_BLACK);
    fb_fill_rect(SCREEN_W/2 - 300, SCREEN_H/2 - 100, 600, 200, COLOR_DARK_RED);
    fb_draw_rect(SCREEN_W/2 - 300, SCREEN_H/2 - 100, 600, 200, 2, COLOR_GOLD);

    if (g_race.winner == 1) {
        font_draw_string(SCREEN_W/2 - 140, SCREEN_H/2 - 60,
            "DRIVER WINS!", COLOR_GOLD, COLOR_DARK_RED, false);
        font_draw_string(SCREEN_W/2 - 180, SCREEN_H/2 - 20,
            "DEMON 170 TAKES THE CHECKERED FLAG!", COLOR_WHITE, COLOR_DARK_RED, false);
        font_draw_string(SCREEN_W/2 - 160, SCREEN_H/2 + 30,
            "CORVUS: That's your race, Landon!", COLOR_WHITE, COLOR_DARK_RED, false);
    } else {
        font_draw_string(SCREEN_W/2 - 120, SCREEN_H/2 - 60,
            "CORVUS WINS!", COLOR_CRIMSON, COLOR_DARK_RED, false);
        font_draw_string(SCREEN_W/2 - 160, SCREEN_H/2 - 20,
            "Supra takes it — rematch, Landon?", COLOR_WHITE, COLOR_DARK_RED, false);
    }
}

// ── Main game tick ────────────────────────────────────────────────────────────
void racing_game_tick(void) {
    if (!g_race.race_active) return;
    if (g_race.race_finished) {
        draw_finish();
        return;
    }

    g_race.race_tick++;

    // CORVUS AI drives the Supra
    corvus_ai_tick();

    // Update physics
    update_car(&g_race.demon, DEMON_ACCEL, DEMON_MAX_SPEED);
    update_car(&g_race.supra, SUPRA_ACCEL, SUPRA_MAX_SPEED);

    // Check win condition
    if (g_race.demon.lap > g_race.lap_total) {
        g_race.race_finished = true;
        g_race.winner = 1;
    } else if (g_race.supra.lap > g_race.lap_total) {
        g_race.race_finished = true;
        g_race.winner = 2;
    }

    // Draw everything
    draw_track();
    draw_car(&g_race.supra);   // Draw Supra first (behind)
    draw_car(&g_race.demon);   // Draw Demon on top (Landon's car)
    draw_hud();
}

// ── Called from shell when Landon speaks ─────────────────────────────────────
void racing_voice_input(const char* text) {
    VoiceCommand cmd = racing_parse_command(text);
    if (cmd != CMD_NONE) {
        racing_execute_command(cmd);
    } else {
        // CORVUS responds to anything
        set_hud("CORVUS: I'm listening, Landon.");
    }
}

// ── Get CORVUS's last spoken response ────────────────────────────────────────
const char* racing_get_corvus_response(void) {
    return g_race.corvus.last_response;
}
