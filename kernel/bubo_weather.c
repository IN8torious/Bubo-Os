/*
 * bubo_weather.c — BUBO OS Live Weather System Implementation
 * ─────────────────────────────────────────────────────────────────────────────
 * Fetches real weather from OpenWeatherMap for Brunswick, Ohio 44212.
 * Merges real weather with Vera Workflow task intensity to drive the
 * desktop particle system — rain, lightning, storm, calm dawn.
 *
 * The sky reflects the world. The storm reflects the work.
 *
 * NO MAS DISADVANTAGED
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "../include/bubo_weather.h"
#include <stddef.h>   /* must precede stdlib.h — defines wchar_t */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── Global state ──────────────────────────────────────────────────────────── */
bubo_weather_t       g_bubo_weather       = {0};
bubo_task_intensity_t g_bubo_task_intensity = TASK_INTENSITY_IDLE;

static char s_api_key[64]   = {0};
static char s_zip[16]       = BUBO_DEFAULT_ZIP;
static char s_country[8]    = BUBO_DEFAULT_COUNTRY;
static char s_lat[16]       = BUBO_DEFAULT_LAT;
static char s_lon[16]       = BUBO_DEFAULT_LON;

/* ── Init ──────────────────────────────────────────────────────────────────── */
int bubo_weather_init(const char *api_key)
{
    if (!api_key || api_key[0] == '\0') {
        /* No API key — weather widget shows "--" but system still runs */
        g_bubo_weather.valid = 0;
        snprintf(g_bubo_weather.city, sizeof(g_bubo_weather.city),
                 "%s", BUBO_DEFAULT_CITY);
        return -1;
    }

    snprintf(s_api_key, sizeof(s_api_key), "%s", api_key);
    snprintf(g_bubo_weather.city, sizeof(g_bubo_weather.city),
             "%s", BUBO_DEFAULT_CITY);
    return 0;
}

/* ── Set location ──────────────────────────────────────────────────────────── */
void bubo_weather_set_location(const char *zip, const char *country)
{
    if (zip)     snprintf(s_zip,     sizeof(s_zip),     "%s", zip);
    if (country) snprintf(s_country, sizeof(s_country), "%s", country);
}

/* ── Set task intensity (called by Vera Workflow) ──────────────────────────── */
void bubo_weather_set_task_intensity(bubo_task_intensity_t intensity)
{
    g_bubo_task_intensity = intensity;
}

/* ── Parse OWM weather ID to our state ────────────────────────────────────── */
static bubo_weather_state_t owm_id_to_state(int owm_id)
{
    if (owm_id >= 200 && owm_id < 300) return WEATHER_THUNDERSTORM;
    if (owm_id >= 300 && owm_id < 400) return WEATHER_DRIZZLE;
    if (owm_id >= 500 && owm_id < 600) return WEATHER_RAIN;
    if (owm_id >= 600 && owm_id < 700) return WEATHER_SNOW;
    if (owm_id >= 700 && owm_id < 800) return WEATHER_MIST;
    if (owm_id == 800)                  return WEATHER_CLEAR;
    if (owm_id > 800)                   return WEATHER_CLOUDS;
    return WEATHER_UNKNOWN;
}

/* ── Fetch weather (HTTP GET via libcurl or kernel HTTP stack) ─────────────── */
int bubo_weather_fetch(bubo_weather_t *out)
{
    if (!out) return -1;

    /*
     * In the full kernel build this uses the BUBO HTTP stack.
     * In userspace / development this shells out to curl.
     *
     * URL format:
     * https://api.openweathermap.org/data/2.5/weather
     *   ?zip=44212,US&appid=<KEY>&units=imperial
     */

    if (s_api_key[0] == '\0') {
        /* No key — populate with placeholder data so UI does not crash */
        out->state          = WEATHER_CLEAR;
        out->task_intensity = g_bubo_task_intensity;
        out->temp_f         = 0.0f;
        out->temp_c         = 0.0f;
        out->humidity       = 0.0f;
        out->wind_speed_mph = 0.0f;
        snprintf(out->condition, sizeof(out->condition), "No API Key");
        snprintf(out->city,      sizeof(out->city),      "%s", BUBO_DEFAULT_CITY);
        out->valid          = 0;
        if (g_bubo_weather.valid == 0) *out = g_bubo_weather;
        return -1;
    }

    /*
     * Build the request URL
     * Production: replace with kernel HTTP GET call
     */
    char url[512];
    snprintf(url, sizeof(url),
        "%s?zip=%s,%s&appid=%s&units=imperial",
        BUBO_OWM_BASE_URL, s_zip, s_country, s_api_key);

    /*
     * TODO: Replace with kernel HTTP stack call
     * For now, this is a stub that returns the last known good data.
     * The Qt layer in userspace handles the actual HTTP fetch and
     * calls bubo_weather_update_from_json() with the response.
     */
    (void)url;

    /* Return last known good data */
    *out = g_bubo_weather;
    return g_bubo_weather.valid ? 0 : -1;
}

/* ── Update from parsed JSON (called by Qt weather thread) ────────────────── */
void bubo_weather_update_from_json(
    int owm_id,
    float temp_f,
    float humidity,
    float wind_mph,
    float wind_deg,
    const char *condition,
    const char *city,
    const char *icon_code,
    uint32_t timestamp)
{
    g_bubo_weather.state          = owm_id_to_state(owm_id);
    g_bubo_weather.task_intensity = g_bubo_task_intensity;
    g_bubo_weather.temp_f         = temp_f;
    g_bubo_weather.temp_c         = (temp_f - 32.0f) * 5.0f / 9.0f;
    g_bubo_weather.humidity       = humidity;
    g_bubo_weather.wind_speed_mph = wind_mph;
    g_bubo_weather.wind_dir_deg   = wind_deg;
    g_bubo_weather.last_updated   = timestamp;
    g_bubo_weather.valid          = 1;

    if (condition) snprintf(g_bubo_weather.condition, sizeof(g_bubo_weather.condition), "%s", condition);
    if (city)      snprintf(g_bubo_weather.city,      sizeof(g_bubo_weather.city),      "%s", city);
    if (icon_code) snprintf(g_bubo_weather.icon_code, sizeof(g_bubo_weather.icon_code), "%s", icon_code);
}

/* ── Compute visual intensity ──────────────────────────────────────────────── */
bubo_visual_intensity_t bubo_weather_get_visual(const bubo_weather_t *w)
{
    bubo_visual_intensity_t v = {0};

    if (!w || !w->valid) {
        /* Default: calm Akatsuki dawn */
        v.rain_density    = 0.0f;
        v.lightning_freq  = 0.0f;
        v.cloud_darkness  = 0.2f;
        v.wind_intensity  = 0.05f;
        v.fog_density     = 0.0f;
        v.r = 180; v.g = 40; v.b = 20;  /* Deep crimson dawn */
        return v;
    }

    /* Base values from real weather */
    switch (w->state) {
        case WEATHER_CLEAR:
            v.rain_density   = 0.0f;
            v.lightning_freq = 0.0f;
            v.cloud_darkness = 0.1f;
            v.wind_intensity = 0.05f;
            v.fog_density    = 0.0f;
            v.r = 180; v.g = 50; v.b = 20;   /* Warm crimson dawn */
            break;

        case WEATHER_CLOUDS:
            v.rain_density   = 0.0f;
            v.lightning_freq = 0.0f;
            v.cloud_darkness = 0.4f;
            v.wind_intensity = 0.15f;
            v.fog_density    = 0.0f;
            v.r = 120; v.g = 30; v.b = 30;   /* Darker, moodier */
            break;

        case WEATHER_DRIZZLE:
            v.rain_density   = 0.2f;
            v.lightning_freq = 0.0f;
            v.cloud_darkness = 0.5f;
            v.wind_intensity = 0.2f;
            v.fog_density    = 0.1f;
            v.r = 80; v.g = 20; v.b = 40;
            break;

        case WEATHER_RAIN:
            v.rain_density   = 0.55f;
            v.lightning_freq = 0.0f;
            v.cloud_darkness = 0.65f;
            v.wind_intensity = 0.4f;
            v.fog_density    = 0.05f;
            v.r = 60; v.g = 15; v.b = 50;
            break;

        case WEATHER_THUNDERSTORM:
            v.rain_density   = 0.8f;
            v.lightning_freq = 0.4f;
            v.cloud_darkness = 0.85f;
            v.wind_intensity = 0.75f;
            v.fog_density    = 0.0f;
            v.r = 40; v.g = 5; v.b = 60;    /* Deep purple-black storm */
            break;

        case WEATHER_SNOW:
            v.rain_density   = 0.3f;   /* Snow particles reuse rain system */
            v.lightning_freq = 0.0f;
            v.cloud_darkness = 0.5f;
            v.wind_intensity = 0.3f;
            v.fog_density    = 0.2f;
            v.r = 100; v.g = 80; v.b = 120;  /* Cold blue-purple */
            break;

        case WEATHER_MIST:
            v.rain_density   = 0.0f;
            v.lightning_freq = 0.0f;
            v.cloud_darkness = 0.3f;
            v.wind_intensity = 0.05f;
            v.fog_density    = 0.6f;
            v.r = 80; v.g = 60; v.b = 80;
            break;

        default:
            v.cloud_darkness = 0.2f;
            v.r = 150; v.g = 40; v.b = 20;
            break;
    }

    /* ── Vera Workflow task intensity overlay ──────────────────────────────── */
    float task_boost = 0.0f;
    switch (w->task_intensity) {
        case TASK_INTENSITY_IDLE:     task_boost = 0.0f;  break;
        case TASK_INTENSITY_LIGHT:    task_boost = 0.1f;  break;
        case TASK_INTENSITY_MODERATE: task_boost = 0.25f; break;
        case TASK_INTENSITY_HEAVY:    task_boost = 0.5f;  break;
        case TASK_INTENSITY_CRITICAL: task_boost = 0.8f;  break;
    }

    /* Task boost amplifies everything — Boo working hard = full storm */
    v.rain_density   = v.rain_density   + task_boost * (1.0f - v.rain_density);
    v.lightning_freq = v.lightning_freq + task_boost * (1.0f - v.lightning_freq);
    v.cloud_darkness = v.cloud_darkness + task_boost * 0.4f;
    v.wind_intensity = v.wind_intensity + task_boost * 0.5f;

    /* Clamp to [0, 1] */
    if (v.rain_density   > 1.0f) v.rain_density   = 1.0f;
    if (v.lightning_freq > 1.0f) v.lightning_freq = 1.0f;
    if (v.cloud_darkness > 1.0f) v.cloud_darkness = 1.0f;
    if (v.wind_intensity > 1.0f) v.wind_intensity = 1.0f;

    return v;
}

/* ── State name ────────────────────────────────────────────────────────────── */
const char *bubo_weather_get_state_name(bubo_weather_state_t state)
{
    switch (state) {
        case WEATHER_CLEAR:        return "Clear";
        case WEATHER_CLOUDS:       return "Cloudy";
        case WEATHER_DRIZZLE:      return "Drizzle";
        case WEATHER_RAIN:         return "Rain";
        case WEATHER_THUNDERSTORM: return "Thunderstorm";
        case WEATHER_SNOW:         return "Snow";
        case WEATHER_MIST:         return "Mist";
        default:                   return "Unknown";
    }
}

/* ── Taskbar format ────────────────────────────────────────────────────────── */
void bubo_weather_format_taskbar(const bubo_weather_t *w, char *out, int max_len)
{
    if (!w || !w->valid) {
        snprintf(out, max_len, "Brunswick, OH  --°F");
        return;
    }

    const char *icon = "";
    switch (w->state) {
        case WEATHER_CLEAR:        icon = "☀";  break;
        case WEATHER_CLOUDS:       icon = "☁";  break;
        case WEATHER_DRIZZLE:      icon = "🌦"; break;
        case WEATHER_RAIN:         icon = "🌧"; break;
        case WEATHER_THUNDERSTORM: icon = "⛈";  break;
        case WEATHER_SNOW:         icon = "❄";  break;
        case WEATHER_MIST:         icon = "🌫"; break;
        default:                   icon = "?";  break;
    }

    snprintf(out, max_len,
        "Brunswick, OH  %s  %.0f°F  %s",
        icon, w->temp_f, w->condition);
}
