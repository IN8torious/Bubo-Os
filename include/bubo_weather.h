/*
 * bubo_weather.h — BUBO OS Live Weather System
 * ─────────────────────────────────────────────────────────────────────────────
 * Real-time weather for Brunswick, Ohio 44212 (and any location).
 * Powered by OpenWeatherMap (free, open source friendly).
 *
 * The desktop is alive. The sky reflects the world outside.
 * When Boo is working — the storm intensifies.
 * When she is done — the dawn returns.
 *
 * Credits (they all earned it):
 *   OpenWeatherMap  — live weather data
 *   NASA            — satellite imagery
 *   Google          — mapping infrastructure
 *   Epic Games      — particle/weather visual philosophy
 *   Qt Foundation   — UI framework
 *   Linux           — kernel foundation
 *
 * NO MAS DISADVANTAGED
 * ─────────────────────────────────────────────────────────────────────────────
 */

#ifndef BUBO_WEATHER_H
#define BUBO_WEATHER_H

#include <stdint.h>
#include "bubo_input_map.h"

/* ── Weather state ─────────────────────────────────────────────────────────── */
typedef enum {
    WEATHER_CLEAR       = 0,   /* Calm dawn — Boo is idle, sky is clear        */
    WEATHER_CLOUDS      = 1,   /* Overcast — light cloud cover                 */
    WEATHER_DRIZZLE     = 2,   /* Light rain — gentle drops on the ocean       */
    WEATHER_RAIN        = 3,   /* Rain — ocean surface alive with ripples      */
    WEATHER_THUNDERSTORM= 4,   /* Storm — lightning, heavy rain, Boo is busy   */
    WEATHER_SNOW        = 5,   /* Snow — rare but beautiful                    */
    WEATHER_MIST        = 6,   /* Fog/mist — mysterious, low visibility        */
    WEATHER_UNKNOWN     = 7
} bubo_weather_state_t;

/* ── Task intensity overlay (from Vera Workflow) ───────────────────────────── */
typedef enum {
    TASK_INTENSITY_IDLE     = 0,   /* No active tasks — weather is real only   */
    TASK_INTENSITY_LIGHT    = 1,   /* Light task — subtle wind on the water    */
    TASK_INTENSITY_MODERATE = 2,   /* Moderate task — clouds darken            */
    TASK_INTENSITY_HEAVY    = 3,   /* Heavy task — full storm, lightning       */
    TASK_INTENSITY_CRITICAL = 4    /* Critical task — apocalyptic storm        */
} bubo_task_intensity_t;

/* ── Weather data structure ────────────────────────────────────────────────── */
typedef struct {
    bubo_weather_state_t    state;          /* Current weather condition        */
    bubo_task_intensity_t   task_intensity; /* Vera Workflow task overlay       */
    float                   temp_f;         /* Temperature in Fahrenheit        */
    float                   temp_c;         /* Temperature in Celsius           */
    float                   humidity;       /* Humidity percentage              */
    float                   wind_speed_mph; /* Wind speed in MPH                */
    float                   wind_dir_deg;   /* Wind direction in degrees        */
    char                    condition[64];  /* Human readable condition string  */
    char                    city[64];       /* City name                        */
    char                    icon_code[8];   /* OpenWeatherMap icon code         */
    uint32_t                last_updated;   /* Unix timestamp of last fetch     */
    int                     valid;          /* 1 if data is valid, 0 if stale   */
} bubo_weather_t;

/* ── Visual intensity (combined weather + task) ────────────────────────────── */
typedef struct {
    float   rain_density;       /* 0.0 = no rain, 1.0 = torrential             */
    float   lightning_freq;     /* 0.0 = none, 1.0 = constant flicker          */
    float   cloud_darkness;     /* 0.0 = clear dawn, 1.0 = pitch black         */
    float   wind_intensity;     /* 0.0 = calm, 1.0 = hurricane                 */
    float   fog_density;        /* 0.0 = clear, 1.0 = zero visibility          */
    uint8_t r, g, b;            /* Sky tint color for current conditions        */
} bubo_visual_intensity_t;

/* ── Default location: Brunswick, Ohio 44212 ───────────────────────────────── */
#define BUBO_DEFAULT_LAT        "41.2378"
#define BUBO_DEFAULT_LON        "-81.8418"
#define BUBO_DEFAULT_CITY       "Brunswick, Ohio"
#define BUBO_DEFAULT_ZIP        "44212"
#define BUBO_DEFAULT_COUNTRY    "US"

/* ── OpenWeatherMap API ────────────────────────────────────────────────────── */
#define BUBO_OWM_BASE_URL       "https://api.openweathermap.org/data/2.5/weather"
#define BUBO_OWM_UPDATE_SEC     600     /* Refresh every 10 minutes             */

/* ── Function declarations ─────────────────────────────────────────────────── */

/**
 * bubo_weather_init — Initialize the weather system
 * Sets default location to Brunswick, Ohio 44212.
 * Call once at boot after network is available.
 */
int bubo_weather_init(const char *api_key);

/**
 * bubo_weather_fetch — Fetch current weather from OpenWeatherMap
 * Populates the global weather state.
 * Returns 0 on success, -1 on failure (uses cached data if available).
 */
int bubo_weather_fetch(bubo_weather_t *out);

/**
 * bubo_weather_set_location — Change the weather location
 * Accepts zip code or lat/lon coordinates.
 */
void bubo_weather_set_location(const char *zip, const char *country);

/**
 * bubo_weather_set_task_intensity — Called by Vera Workflow when tasks start/end
 * Drives the storm overlay on top of real weather.
 */
void bubo_weather_set_task_intensity(bubo_task_intensity_t intensity);

/**
 * bubo_weather_get_visual — Compute combined visual intensity
 * Merges real weather + task intensity into particle system parameters.
 */
bubo_visual_intensity_t bubo_weather_get_visual(const bubo_weather_t *weather);

/**
 * bubo_weather_get_state_name — Human readable weather state string
 */
const char *bubo_weather_get_state_name(bubo_weather_state_t state);

/**
 * bubo_weather_format_taskbar — Format weather string for taskbar display
 * e.g. "Brunswick, OH  ⛈  52°F  Thunderstorm"
 */
void bubo_weather_format_taskbar(const bubo_weather_t *w, char *out, int max_len);

/* ── Global weather state (read-only from other modules) ───────────────────── */
extern bubo_weather_t g_bubo_weather;
extern bubo_task_intensity_t g_bubo_task_intensity;

#endif /* BUBO_WEATHER_H */
