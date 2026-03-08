// Deep Flow OS — Copyright (c) 2025 IN8torious. MIT License.
// Built for Landon Pankuch. Built for everyone who was told they couldn't.
// https://github.com/IN8torious/Deep-Flow-OS
//
// kernel/corvus_fcn.c — CORVUS Fully-Connected Neural Network
//
// Ports the Haskell_ML FCN architecture (capn-freako/Haskell_ML) to
// bare-metal C. No GHC. No runtime. No ConCat. Just the math.
//
// "Do or do not. There is no try." — Yoda / CORVUS
// =============================================================================

#include "corvus_fcn.h"
#include "vfs.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ── Static state ──────────────────────────────────────────────────────────────
static fcn_params_t  g_params;
static fcn_trainer_t g_trainer;
static bool          g_initialized = false;

// ── Math helpers (no libm) ───────────────────────────────────────────────────

static float fcn_expf(float x) {
    // Fast exp approximation using Taylor series — good enough for softmax
    // Clamp to avoid overflow
    if (x >  88.0f) return 3.40282347e+38f;
    if (x < -88.0f) return 0.0f;
    // Decompose: e^x = e^(n*ln2) * e^(x - n*ln2)
    // Use integer part for range reduction
    int n = (int)(x * 1.4426950408f); // x / ln(2)
    float r = x - (float)n * 0.6931471806f;
    // Pade approximant for e^r, |r| <= 0.5*ln(2)
    float r2 = r * r;
    float num = 1.0f + r + r2 * (0.5f + r * (0.166666667f + r * 0.041666667f));
    // Scale by 2^n using bit manipulation
    union { float f; uint32_t i; } u;
    u.i = (uint32_t)((n + 127) << 23);
    return num * u.f;
}

static float fcn_relu(float x) {
    return x > 0.0f ? x : 0.0f;
}

static float fcn_absf(float x) {
    return x < 0.0f ? -x : x;
}

// ── Xavier weight initialization ─────────────────────────────────────────────
// Mirrors Haskell_ML's random weight initialization.
// Uses a simple LCG PRNG seeded from a compile-time constant.

static uint32_t g_rng_state = 0xDEEF100Cu;

static uint32_t fcn_rng_next(void) {
    g_rng_state ^= g_rng_state << 13;
    g_rng_state ^= g_rng_state >> 17;
    g_rng_state ^= g_rng_state << 5;
    return g_rng_state;
}

static float fcn_rng_float(void) {
    // Returns float in [-1.0, 1.0]
    uint32_t r = fcn_rng_next();
    float f = (float)(r & 0x7FFFFF) / (float)0x7FFFFF;
    return (r & 0x800000) ? -f : f;
}

static void fcn_xavier_init(float* w, uint32_t fan_in, uint32_t fan_out, uint32_t count) {
    // Xavier: scale = sqrt(2.0 / (fan_in + fan_out))
    float scale = 1.0f;
    uint32_t denom = fan_in + fan_out;
    if (denom > 0) {
        // Approximate sqrt via Newton-Raphson
        float x = 2.0f / (float)denom;
        float s = x;
        for (int i = 0; i < 8; i++) {
            s = 0.5f * (s + x / s);
        }
        scale = s;
    }
    for (uint32_t i = 0; i < count; i++) {
        w[i] = fcn_rng_float() * scale;
    }
}

// ── Forward pass ─────────────────────────────────────────────────────────────
// Mirrors Haskell_ML's FCN forward pass:
//   layer0: input  → hidden1 (ReLU)
//   layer1: hidden1 → hidden2 (ReLU)
//   layer2: hidden2 → output  (Softmax)

static void fcn_forward(const float* input, fcn_activations_t* act) {
    // Layer 0: input[FCN_INPUT_DIM] → h0[FCN_HIDDEN1]
    for (int j = 0; j < FCN_HIDDEN1; j++) {
        float sum = g_params.b0[j];
        for (int i = 0; i < FCN_INPUT_DIM; i++) {
            sum += g_params.w0[j][i] * input[i];
        }
        act->h0[j] = fcn_relu(sum);
    }

    // Layer 1: h0[FCN_HIDDEN1] → h1[FCN_HIDDEN2]
    for (int j = 0; j < FCN_HIDDEN2; j++) {
        float sum = g_params.b1[j];
        for (int i = 0; i < FCN_HIDDEN1; i++) {
            sum += g_params.w1[j][i] * act->h0[i];
        }
        act->h1[j] = fcn_relu(sum);
    }

    // Layer 2: h1[FCN_HIDDEN2] → out[FCN_OUTPUT_DIM] (pre-softmax logits)
    for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
        float sum = g_params.b2[j];
        for (int i = 0; i < FCN_HIDDEN2; i++) {
            sum += g_params.w2[j][i] * act->h1[i];
        }
        act->out[j] = sum;
    }

    // Softmax: normalize logits to probability distribution
    // Numerically stable: subtract max before exp
    float max_logit = act->out[0];
    for (int j = 1; j < FCN_OUTPUT_DIM; j++) {
        if (act->out[j] > max_logit) max_logit = act->out[j];
    }
    float sum_exp = 0.0f;
    for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
        act->out[j] = fcn_expf(act->out[j] - max_logit);
        sum_exp += act->out[j];
    }
    if (sum_exp > 0.0f) {
        for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
            act->out[j] /= sum_exp;
        }
    }
}

// ── Top-1 and Top-2 extraction ────────────────────────────────────────────────
static void fcn_top2(const fcn_activations_t* act, fcn_result_t* result) {
    result->cmd_id          = 0;
    result->confidence      = act->out[0];
    result->second_cmd_id   = 1;
    result->second_confidence = act->out[1];

    for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
        if (act->out[j] > result->confidence) {
            result->second_cmd_id        = result->cmd_id;
            result->second_confidence    = result->confidence;
            result->cmd_id               = (uint32_t)j;
            result->confidence           = act->out[j];
        } else if (act->out[j] > result->second_confidence &&
                   (uint32_t)j != result->cmd_id) {
            result->second_cmd_id        = (uint32_t)j;
            result->second_confidence    = act->out[j];
        }
    }
}

// ── Public API ────────────────────────────────────────────────────────────────

void corvus_fcn_init(void) {
    if (g_initialized) return;

    // Initialize trainer
    g_trainer.learning_rate = 0.01f;
    g_trainer.epoch         = 0;
    g_trainer.samples_seen  = 0;
    g_trainer.last_loss     = 0.0f;
    g_trainer.accuracy      = 0.0f;

    // Try to load pre-trained weights from VFS
    if (!corvus_fcn_deserialize(NULL, 0)) {
        // Fall back to Xavier random initialization
        fcn_xavier_init(&g_params.w0[0][0], FCN_INPUT_DIM, FCN_HIDDEN1,
                        FCN_HIDDEN1 * FCN_INPUT_DIM);
        for (int j = 0; j < FCN_HIDDEN1; j++) g_params.b0[j] = 0.0f;

        fcn_xavier_init(&g_params.w1[0][0], FCN_HIDDEN1, FCN_HIDDEN2,
                        FCN_HIDDEN2 * FCN_HIDDEN1);
        for (int j = 0; j < FCN_HIDDEN2; j++) g_params.b1[j] = 0.0f;

        fcn_xavier_init(&g_params.w2[0][0], FCN_HIDDEN2, FCN_OUTPUT_DIM,
                        FCN_OUTPUT_DIM * FCN_HIDDEN2);
        for (int j = 0; j < FCN_OUTPUT_DIM; j++) g_params.b2[j] = 0.0f;
    }

    g_initialized = true;
}

void corvus_fcn_infer(const float* features, fcn_result_t* result) {
    if (!g_initialized) corvus_fcn_init();

    fcn_activations_t act;
    fcn_forward(features, &act);
    fcn_top2(&act, result);
}

// ── corvus_flow.c compatibility wrapper ──────────────────────────────────────
// corvus_flow.c calls: uint32_t corvus_fcn_infer(const float*, float*)
// We expose that signature here as a wrapper around the struct-based API.
uint32_t corvus_fcn_infer_simple(const float* features, float* out_confidence) {
    fcn_result_t result;
    corvus_fcn_infer(features, &result);
    if (out_confidence) *out_confidence = result.confidence;
    return result.cmd_id;
}

// ── Online SGD training step ──────────────────────────────────────────────────
// Mirrors Haskell_ML's trainNTimes / gradient descent loop.
// Called after each confirmed correct command — CORVUS adapts to Landon's voice.

void corvus_fcn_train_sample(const float* features, uint32_t label) {
    if (!g_initialized) corvus_fcn_init();
    if (label >= FCN_OUTPUT_DIM) return;

    // Forward pass — save activations for backprop
    fcn_activations_t act;
    fcn_forward(features, &act);

    // Cross-entropy loss: L = -log(p[label])
    float p_label = act.out[label];
    if (p_label < 1e-7f) p_label = 1e-7f;
    // Approximate -log(p) using: -log(p) ≈ (1-p)/p for p near 1, else use series
    float loss = 0.0f;
    // Simple: loss = 1.0 - p_label (hinge-like, good enough for online SGD)
    loss = 1.0f - p_label;
    g_trainer.last_loss = loss;

    // ── Backprop: output layer ────────────────────────────────────────────────
    // dL/d(logit_j) = p_j - 1{j==label}  (softmax + cross-entropy gradient)
    float d_out[FCN_OUTPUT_DIM];
    for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
        d_out[j] = act.out[j] - (uint32_t)j == label ? 1.0f : 0.0f;
    }

    // Gradient for w2, b2
    for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
        g_trainer.db2[j] = d_out[j];
        for (int i = 0; i < FCN_HIDDEN2; i++) {
            g_trainer.dw2[j][i] = d_out[j] * act.h1[i];
        }
    }

    // ── Backprop: hidden layer 2 ──────────────────────────────────────────────
    float d_h1[FCN_HIDDEN2];
    for (int i = 0; i < FCN_HIDDEN2; i++) {
        float grad = 0.0f;
        for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
            grad += d_out[j] * g_params.w2[j][i];
        }
        // ReLU derivative: 1 if h1[i] > 0, else 0
        d_h1[i] = (act.h1[i] > 0.0f) ? grad : 0.0f;
    }

    // Gradient for w1, b1
    for (int j = 0; j < FCN_HIDDEN2; j++) {
        g_trainer.db1[j] = d_h1[j];
        for (int i = 0; i < FCN_HIDDEN1; i++) {
            g_trainer.dw1[j][i] = d_h1[j] * act.h0[i];
        }
    }

    // ── Backprop: hidden layer 1 ──────────────────────────────────────────────
    float d_h0[FCN_HIDDEN1];
    for (int i = 0; i < FCN_HIDDEN1; i++) {
        float grad = 0.0f;
        for (int j = 0; j < FCN_HIDDEN2; j++) {
            grad += d_h1[j] * g_params.w1[j][i];
        }
        d_h0[i] = (act.h0[i] > 0.0f) ? grad : 0.0f;
    }

    // Gradient for w0, b0
    for (int j = 0; j < FCN_HIDDEN1; j++) {
        g_trainer.db0[j] = d_h0[j];
        for (int i = 0; i < FCN_INPUT_DIM; i++) {
            g_trainer.dw0[j][i] = d_h0[j] * features[i];
        }
    }

    // ── SGD weight update ─────────────────────────────────────────────────────
    float lr = g_trainer.learning_rate;

    for (int j = 0; j < FCN_HIDDEN1; j++) {
        g_params.b0[j] -= lr * g_trainer.db0[j];
        for (int i = 0; i < FCN_INPUT_DIM; i++) {
            g_params.w0[j][i] -= lr * g_trainer.dw0[j][i];
        }
    }
    for (int j = 0; j < FCN_HIDDEN2; j++) {
        g_params.b1[j] -= lr * g_trainer.db1[j];
        for (int i = 0; i < FCN_HIDDEN1; i++) {
            g_params.w1[j][i] -= lr * g_trainer.dw1[j][i];
        }
    }
    for (int j = 0; j < FCN_OUTPUT_DIM; j++) {
        g_params.b2[j] -= lr * g_trainer.db2[j];
        for (int i = 0; i < FCN_HIDDEN2; i++) {
            g_params.w2[j][i] -= lr * g_trainer.dw2[j][i];
        }
    }

    g_trainer.samples_seen++;
}

// ── Batch accuracy ────────────────────────────────────────────────────────────
// Mirrors Haskell_ML's classificationAccuracy function.
float corvus_fcn_accuracy(const float samples[][FCN_INPUT_DIM],
                           const uint32_t* labels, uint32_t count) {
    if (!g_initialized || count == 0) return 0.0f;
    uint32_t correct = 0;
    for (uint32_t s = 0; s < count; s++) {
        fcn_result_t result;
        corvus_fcn_infer(samples[s], &result);
        if (result.cmd_id == labels[s]) correct++;
    }
    g_trainer.accuracy = (float)correct / (float)count;
    return g_trainer.accuracy;
}

// ── Serialization ─────────────────────────────────────────────────────────────
void corvus_fcn_serialize(uint8_t* buf, uint32_t* out_len) {
    if (!buf || !out_len) return;
    uint32_t sz = sizeof(fcn_params_t);
    // Simple flat copy — params are POD
    const uint8_t* src = (const uint8_t*)&g_params;
    for (uint32_t i = 0; i < sz; i++) buf[i] = src[i];
    *out_len = sz;
}

bool corvus_fcn_deserialize(const uint8_t* buf, uint32_t len) {
    if (!buf) {
        // Try to load from VFS
        int32_t fd = vfs_open("/sys/corvus/fcn_weights.bin", 0);
        if (fd < 0) return false;
        uint32_t sz = sizeof(fcn_params_t);
        uint8_t* dst = (uint8_t*)&g_params;
        int32_t nread = vfs_read(fd, dst, sz);
        return (nread == (int32_t)sz);
    }
    if (len < sizeof(fcn_params_t)) return false;
    const uint8_t* src = buf;
    uint8_t* dst = (uint8_t*)&g_params;
    for (uint32_t i = 0; i < sizeof(fcn_params_t); i++) dst[i] = src[i];
    return true;
}

// ── Diagnostics ───────────────────────────────────────────────────────────────
void corvus_fcn_print_stats(void) {
    // Compute weight variance per layer (mirrors Haskell_ML's showPart)
    float var0 = 0.0f, var1 = 0.0f, var2 = 0.0f;
    uint32_t n0 = FCN_HIDDEN1 * FCN_INPUT_DIM;
    uint32_t n1 = FCN_HIDDEN2 * FCN_HIDDEN1;
    uint32_t n2 = FCN_OUTPUT_DIM * FCN_HIDDEN2;

    for (uint32_t i = 0; i < n0; i++) var0 += fcn_absf(g_params.w0[0][i]);
    for (uint32_t i = 0; i < n1; i++) var1 += fcn_absf(g_params.w1[0][i]);
    for (uint32_t i = 0; i < n2; i++) var2 += fcn_absf(g_params.w2[0][i]);

    // Output via terminal (no printf — bare metal)
    // Caller reads g_trainer for numeric stats
    (void)var0; (void)var1; (void)var2;
}

const fcn_trainer_t* corvus_fcn_get_trainer(void) { return &g_trainer; }
const fcn_params_t*  corvus_fcn_get_params(void)  { return &g_params;  }

// ── Command labels ────────────────────────────────────────────────────────────
// Maps FCN output index → human-readable command string
// Matches VCMD_* defines in corvus.h
const char* FCN_CMD_LABELS[FCN_OUTPUT_DIM] = {
    /* 00 */ "none",       /* 01 */ "hello",      /* 02 */ "yes",        /* 03 */ "no",
    /* 04 */ "stop",       /* 05 */ "go",          /* 06 */ "fire",       /* 07 */ "reload",
    /* 08 */ "jump",       /* 09 */ "crouch",      /* 10 */ "sprint",     /* 11 */ "walk",
    /* 12 */ "left",       /* 13 */ "right",       /* 14 */ "forward",    /* 15 */ "back",
    /* 16 */ "open",       /* 17 */ "close",       /* 18 */ "select",     /* 19 */ "cancel",
    /* 20 */ "volume_up",  /* 21 */ "volume_down", /* 22 */ "mute",       /* 23 */ "unmute",
    /* 24 */ "screenshot", /* 25 */ "record",      /* 26 */ "pause",      /* 27 */ "resume",
    /* 28 */ "next",       /* 29 */ "previous",    /* 30 */ "home",       /* 31 */ "back_nav",
    /* 32 */ "search",     /* 33 */ "help",        /* 34 */ "status",     /* 35 */ "settings",
    /* 36 */ "game_start", /* 37 */ "game_pause",  /* 38 */ "game_quit",  /* 39 */ "game_menu",
    /* 40 */ "vm_start",   /* 41 */ "vm_stop",     /* 42 */ "vm_pause",   /* 43 */ "vm_resume",
    /* 44 */ "admin",      /* 45 */ "calibrate",   /* 46 */ "patch",      /* 47 */ "shutdown",
};
