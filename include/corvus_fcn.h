// =============================================================================
// Instinct OS v1.2 — Dedicated to Landon Pankuch
// =============================================================================
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// include/corvus_fcn.h — CORVUS Fully-Connected Neural Network (FCN)
//
// Architecture ported from Haskell_ML by David Banas (capn-freako):
//   https://github.com/capn-freako/Haskell_ML
//
// The Haskell_ML library implements typed FCNs using ConCat automatic
// differentiation. The layer structure V n --+ V m represents a fully
// connected layer mapping n inputs to m outputs with learned weights
// and biases. We port this architecture to bare-metal C:
//
//   Haskell:  (V 5 --+ V CMD_COUNT) :*: (V 16 --+ V 5) :*: (V INPUT_DIM --+ V 16)
//   C:        input[INPUT_DIM] -> hidden1[16] -> hidden2[5] -> output[CMD_COUNT]
//
// This FCN runs inside CORVUS at ring 0 with zero dependencies.
// It classifies phoneme feature vectors from the dysarthria engine
// into voice command IDs — enabling CORVUS to understand Landon's
// speech even when slurred, partial, or dysarthric.
//
// Training: weights are pre-trained offline (Python/Haskell) and
// embedded as static float arrays. Online fine-tuning via SGD is
// supported for adaptive personalization to Landon's voice over time.
// =============================================================================
#ifndef INSTINCT_CORVUS_FCN_H
#define INSTINCT_CORVUS_FCN_H

#include <stdint.h>
#include <stdbool.h>

// ── Network dimensions ────────────────────────────────────────────────────────
// Mirrors Haskell_ML's typed layer sizes, adapted for voice command classification.
// Input: 32 phoneme/MFCC-style features from the dysarthria engine
// Hidden 1: 16 neurons (V 16 in Haskell notation)
// Hidden 2: 8 neurons  (V 8)
// Output: number of CORVUS command classes
#define FCN_INPUT_DIM   32    // phoneme feature vector length
#define FCN_HIDDEN1     16    // layer 1 width  (V INPUT_DIM --+ V 16)
#define FCN_HIDDEN2     8     // layer 2 width  (V 16 --+ V 8)
#define FCN_OUTPUT_DIM  48    // command classes (V 8 --+ V CMD_COUNT)

// ── Weight/bias storage ───────────────────────────────────────────────────────
// Stored in the same layout as Haskell_ML's HasLayers typeclass:
//   getWeights returns [[weight_matrix_per_layer]]
//   getBiases  returns [[bias_vector_per_layer]]
// Layer 0: weights[FCN_HIDDEN1][FCN_INPUT_DIM], biases[FCN_HIDDEN1]
// Layer 1: weights[FCN_HIDDEN2][FCN_HIDDEN1],   biases[FCN_HIDDEN2]
// Layer 2: weights[FCN_OUTPUT_DIM][FCN_HIDDEN2], biases[FCN_OUTPUT_DIM]
typedef struct {
    float w0[FCN_HIDDEN1][FCN_INPUT_DIM];   // input → hidden1 weights
    float b0[FCN_HIDDEN1];                  // hidden1 biases
    float w1[FCN_HIDDEN2][FCN_HIDDEN1];     // hidden1 → hidden2 weights
    float b1[FCN_HIDDEN2];                  // hidden2 biases
    float w2[FCN_OUTPUT_DIM][FCN_HIDDEN2];  // hidden2 → output weights
    float b2[FCN_OUTPUT_DIM];               // output biases
} fcn_params_t;

// ── Activation buffers (forward pass scratch space) ───────────────────────────
typedef struct {
    float h0[FCN_HIDDEN1];      // post-activation hidden layer 1
    float h1[FCN_HIDDEN2];      // post-activation hidden layer 2
    float out[FCN_OUTPUT_DIM];  // softmax output (probability distribution)
} fcn_activations_t;

// ── Training state (for online SGD fine-tuning) ───────────────────────────────
// Mirrors Haskell_ML's trainNTimes / gradient descent loop.
// Allows CORVUS to adapt to Landon's voice over time without recompiling.
typedef struct {
    float    learning_rate;      // η — step size (e.g. 0.01)
    uint32_t epoch;              // current training epoch
    uint32_t samples_seen;       // total samples processed
    float    last_loss;          // cross-entropy loss from last batch
    float    accuracy;           // classification accuracy 0.0–1.0
    // Gradient accumulators (same shape as fcn_params_t)
    float    dw0[FCN_HIDDEN1][FCN_INPUT_DIM];
    float    db0[FCN_HIDDEN1];
    float    dw1[FCN_HIDDEN2][FCN_HIDDEN1];
    float    db1[FCN_HIDDEN2];
    float    dw2[FCN_OUTPUT_DIM][FCN_HIDDEN2];
    float    db2[FCN_OUTPUT_DIM];
} fcn_trainer_t;

// ── Inference result ──────────────────────────────────────────────────────────
typedef struct {
    uint32_t cmd_id;            // predicted CORVUS command ID
    float    confidence;        // softmax probability of top class (0.0–1.0)
    uint32_t second_cmd_id;     // second-best prediction (for ambiguity handling)
    float    second_confidence;
} fcn_result_t;

// ── CORVUS command class labels ───────────────────────────────────────────────
// Maps FCN output index → human-readable command string
// Matches the VCMD_* and GCMD_* defines in corvus.h and game_engine.h
extern const char* FCN_CMD_LABELS[FCN_OUTPUT_DIM];

// ── Public API ────────────────────────────────────────────────────────────────

// Initialize the FCN with pre-trained weights embedded in corvus_fcn.c
void         corvus_fcn_init(void);

// Forward pass: classify a feature vector into a command
// features: float[FCN_INPUT_DIM] — phoneme features from dysarthria engine
// result:   filled with top-1 and top-2 predictions
void         corvus_fcn_infer(const float* features, fcn_result_t* result);

// Online training: update weights given a correct label (supervised fine-tuning)
// features: float[FCN_INPUT_DIM]
// label:    correct command index (0..FCN_OUTPUT_DIM-1)
void         corvus_fcn_train_sample(const float* features, uint32_t label);

// Batch accuracy (mirrors Haskell_ML's classificationAccuracy function)
// samples:  array of feature vectors
// labels:   array of correct command indices
// count:    number of samples
// returns:  accuracy in [0.0, 1.0]
float        corvus_fcn_accuracy(const float samples[][FCN_INPUT_DIM],
                                  const uint32_t* labels, uint32_t count);

// Save/load weights to/from a flat binary blob (for persistence across boots)
void         corvus_fcn_serialize(uint8_t* buf, uint32_t* out_len);
bool         corvus_fcn_deserialize(const uint8_t* buf, uint32_t len);

// Diagnostic: print weight variance per layer (mirrors Haskell_ML's showPart)
void         corvus_fcn_print_stats(void);

// Get current trainer state
const fcn_trainer_t* corvus_fcn_get_trainer(void);

// Get raw parameters (for inspection or export)
const fcn_params_t*  corvus_fcn_get_params(void);

#endif // INSTINCT_CORVUS_FCN_H
