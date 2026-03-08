// =============================================================================
// Deep Flow OS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// include/clouds.h — Cloud services definitions and structures
// =============================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Enum for various cloud service states
enum cloud_service_status_t {
    CLOUD_STATUS_DISCONNECTED = 0,
    CLOUD_STATUS_CONNECTING,
    CLOUD_STATUS_CONNECTED,
    CLOUD_STATUS_AUTHENTICATED,
    CLOUD_STATUS_ERROR
};

// Structure to hold cloud service configuration
typedef struct {
    char endpoint[256];         // URL of the cloud service endpoint
    char api_key[64];           // API key for authentication
    uint32_t timeout_ms;        // Connection timeout in milliseconds
    bool enable_telemetry;      // Flag to enable/disable telemetry
} cloud_config_t;

// Function prototypes for cloud operations

/**
 * @brief Initializes the cloud service module.
 * @param config Pointer to the cloud configuration structure.
 * @return True if initialization is successful, false otherwise.
 */
bool cloud_init(const cloud_config_t* config);

/**
 * @brief Connects to the configured cloud service.
 * @return The current status of the cloud connection.
 */
enum cloud_service_status_t cloud_connect(void);

/**
 * @brief Disconnects from the cloud service.
 */
void cloud_disconnect(void);

/**
 * @brief Sends data to the cloud service.
 * @param data Pointer to the data buffer to send.
 * @param size Size of the data buffer in bytes.
 * @return True if data was sent successfully, false otherwise.
 */
bool cloud_send_data(const uint8_t* data, uint32_t size);

/**
 * @brief Receives data from the cloud service.
 * @param buffer Pointer to the buffer to store received data.
 * @param max_size Maximum size of the buffer.
 * @return Number of bytes received, or 0 if no data or error.
 */
uint32_t cloud_receive_data(uint8_t* buffer, uint32_t max_size);

/**
 * @brief Gets the current status of the cloud service.
 * @return The current status of the cloud service.
 */
enum cloud_service_status_t cloud_get_status(void);
