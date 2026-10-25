// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_PACKET_VIEW_H
#define CCSDSPACK_C_PACKET_VIEW_H

#include <stddef.h>
#include <stdint.h>
#include "buffer.h"
#include "crc.h"
#include "error.h"
#include "primary_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Packet-level error-control policy used while inspecting a packet. */
typedef enum ccsds_packet_error_control {
    CCSDS_PACKET_ERROR_CONTROL_NONE = 0,
    CCSDS_PACKET_ERROR_CONTROL_CRC16 = 1
} ccsds_packet_error_control_t;

/** @brief CRC16 parameters for packet error-control validation. */
typedef struct ccsds_crc16_config {
    uint16_t polynomial;
    uint16_t initial_value;
    uint16_t final_xor_value;
} ccsds_crc16_config_t;

/** @brief Returns the CCSDSPack default CRC-16/CCITT-FALSE parameters. */
static inline ccsds_crc16_config_t ccsds_crc16_default_config(void) {
    const ccsds_crc16_config_t config = {
        CCSDS_CRC16_CCITT_FALSE_POLYNOMIAL,
        CCSDS_CRC16_CCITT_FALSE_INITIAL,
        CCSDS_CRC16_CCITT_FALSE_FINAL_XOR
    };
    return config;
}

/**
 * @brief Non-owning validated view over one CCSDS Space Packet.
 *
 * Every byte view aliases the caller-provided input buffer. The view does not
 * allocate, copy, retain ownership, or extend the input lifetime.
 *
 * packet covers the complete declared packet including the primary header and
 * optional packet error-control trailer.
 * body covers every byte after the six-octet primary header.
 * data_field follows CCSDSPack's established abstraction and excludes the
 * optional packet error-control trailer.
 * packet_error_control is empty for NONE or exactly two bytes for CRC16.
 */
typedef struct ccsds_packet_view {
    ccsds_primary_header_t primary_header;
    ccsds_buffer_view_t packet;
    ccsds_buffer_view_t body;
    ccsds_buffer_view_t data_field;
    ccsds_buffer_view_t packet_error_control;
    size_t consumed;
    uint16_t received_crc16;
} ccsds_packet_view_t;

/**
 * @brief Non-owning validated split-header/body packet view.
 *
 * Header and body may be discontiguous; all views alias caller-owned storage.
 * data_field excludes the optional CRC16 trailer. There is intentionally no
 * whole-packet pointer because the input spans need not be adjacent.
 */
typedef struct ccsds_packet_parts_view {
    ccsds_primary_header_t primary_header;
    ccsds_buffer_view_t header_bytes;
    ccsds_buffer_view_t body;
    ccsds_buffer_view_t data_field;
    ccsds_buffer_view_t packet_error_control;
    size_t consumed;
    uint16_t received_crc16;
} ccsds_packet_parts_view_t;

/** @brief Stable validation reasons for adapter diagnostics (not wire data). */
typedef enum ccsds_packet_parse_reason {
    CCSDS_PACKET_PARSE_OK = 0,
    CCSDS_PACKET_PARSE_HEADER_SIZE,
    CCSDS_PACKET_PARSE_NULL_INPUT,
    CCSDS_PACKET_PARSE_HEADER_INVALID,
    CCSDS_PACKET_PARSE_VERSION,
    CCSDS_PACKET_PARSE_BODY_TRUNCATED,
    CCSDS_PACKET_PARSE_BODY_SIZE_MISMATCH,
    CCSDS_PACKET_PARSE_CRC_TRAILER_TOO_SHORT,
    CCSDS_PACKET_PARSE_CRC_MISMATCH,
    CCSDS_PACKET_PARSE_IDLE_SECONDARY_HEADER,
    CCSDS_PACKET_PARSE_IDLE_DATA_REQUIRED,
    CCSDS_PACKET_PARSE_INVALID_POLICY
} ccsds_packet_parse_reason_t;

/**
 * @brief Checks separate six-byte header and exact body spans with no copying.
 *
 * body_size must equal the primary header's Packet Data Length + 1.
 * Output view is unchanged on failure. reason_out is optional and receives the
 * dominant failure reason or CCSDS_PACKET_PARSE_OK on success.
 */
ccsds_status_t ccsds_packet_parts_parse(
    const uint8_t *header_data, size_t header_size,
    const uint8_t *body_data, size_t body_size,
    ccsds_packet_error_control_t error_control,
    const ccsds_crc16_config_t *crc_config,
    ccsds_packet_parts_view_t *view_out,
    ccsds_packet_parse_reason_t *reason_out);

/** @brief Detailed diagnostic variant of the contiguous packet-view parser. */
ccsds_status_t ccsds_packet_view_parse_ex(
    const uint8_t *data, size_t size,
    ccsds_packet_error_control_t error_control,
    const ccsds_crc16_config_t *crc_config,
    ccsds_packet_view_t *view_out,
    ccsds_packet_parse_reason_t *reason_out);

/**
 * @brief Parses and validates one bounded packet without allocating or copying.
 *
 * Extra bytes after the first declared packet are permitted and reported only
 * through size - view->consumed, making this suitable for packet streams.
 *
 * When error_control is CRC16, crc_config may be NULL to select the default
 * CRC-16/CCITT-FALSE parameters.
 */
ccsds_status_t ccsds_packet_view_parse(const uint8_t *data,
                                       size_t size,
                                       ccsds_packet_error_control_t error_control,
                                       const ccsds_crc16_config_t *crc_config,
                                       ccsds_packet_view_t *view_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_PACKET_VIEW_H
