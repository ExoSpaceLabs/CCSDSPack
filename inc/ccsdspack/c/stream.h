// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_STREAM_H
#define CCSDSPACK_C_STREAM_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"
#include "packet_view.h"

#define CCSDS_PACKET_STREAM_SYNC_SIZE ((size_t)4U)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Non-owning cursor over a bounded stream of CCSDS Space Packets.
 *
 * The cursor never allocates or copies. Optional four-octet synchronization
 * framing is treated as transport framing outside the Space Packet PDU.
 */
typedef struct ccsds_packet_stream {
    const uint8_t *data;
    size_t size;
    size_t offset;
    uint32_t sync_pattern;
    ccsds_packet_error_control_t error_control;
    ccsds_crc16_config_t crc_config;
    uint8_t sync_enabled;
} ccsds_packet_stream_t;

/** @brief Returns the stream-frame prefix size for the selected sync policy. */
size_t ccsds_packet_stream_prefix_size(int sync_enabled);

/**
 * @brief Writes the optional four-octet big-endian synchronization prefix.
 *
 * When sync is disabled, zero bytes are written and output may be NULL.
 */
ccsds_status_t ccsds_packet_stream_write_prefix(int sync_enabled,
                                                uint32_t sync_pattern,
                                                uint8_t *output,
                                                size_t capacity,
                                                size_t *written_out);

/**
 * @brief Initializes a packet-stream cursor over caller-owned storage.
 *
 * crc_config may be NULL to select the default CRC-16/CCITT-FALSE parameters.
 */
ccsds_status_t ccsds_packet_stream_init(ccsds_packet_stream_t *stream,
                                        const uint8_t *data,
                                        size_t size,
                                        ccsds_packet_error_control_t error_control,
                                        const ccsds_crc16_config_t *crc_config,
                                        int sync_enabled,
                                        uint32_t sync_pattern);

/** @brief Returns the number of unconsumed bytes in the stream. */
size_t ccsds_packet_stream_remaining(const ccsds_packet_stream_t *stream);

/**
 * @brief Parses the next framed packet and advances the cursor transactionally.
 *
 * The returned packet view aliases the original stream buffer. CCSDS_STATUS_NO_DATA
 * is returned when the cursor is exactly at end-of-stream. On any failure the
 * cursor offset is unchanged.
 *
 * frame_consumed_out receives packet bytes plus the optional four-byte sync word.
 */
ccsds_status_t ccsds_packet_stream_next(ccsds_packet_stream_t *stream,
                                        ccsds_packet_view_t *view_out,
                                        size_t *frame_consumed_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_STREAM_H
