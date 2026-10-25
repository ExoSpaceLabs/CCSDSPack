// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_STREAM_RECEIVER_H
#define CCSDSPACK_C_STREAM_RECEIVER_H

#include <stddef.h>
#include <stdint.h>
#include "packet_view.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Incremental receiver for streams carrying the existing four-byte external
 * synchronization prefix. The prefix is not part of the CCSDS Space Packet PDU.
 *
 * No memory is allocated: storage must contain at least the four sync bytes
 * plus the largest complete packet the caller intends to accept (minimum 11).
 * Large declared packet sizes are rejected before buffering their bodies.
 *
 * A returned packet view aliases storage + 4 and remains valid until the next
 * feed(), resync() or reset() call. No pointer is retained to input chunks.
 *
 * A valid packet payload may itself contain the sync word. Never preempt a
 * current frame just because a candidate sync appears within its body.
 * On invalid frames, replay the already-buffered suffix to search for another
 * sync, including across input-chunk boundaries. Call resync() on an externally
 * detected timeout of a truncated/incomplete candidate.
 */
typedef enum ccsds_stream_receiver_phase {
    CCSDS_STREAM_RX_SEEK_SYNC = 0,
    CCSDS_STREAM_RX_READ_HEADER = 1,
    CCSDS_STREAM_RX_READ_BODY = 2
} ccsds_stream_receiver_phase_t;

typedef struct ccsds_stream_receiver {
    uint8_t *storage;
    size_t capacity;
    size_t buffered;
    size_t required_frame_size;
    size_t delivered_frame_size;
    uint32_t sync_pattern;
    ccsds_packet_error_control_t error_control;
    ccsds_crc16_config_t crc_config;
    ccsds_stream_receiver_phase_t phase;
    size_t dropped_bytes;
    size_t rejected_headers;
    size_t rejected_packets;
    size_t accepted_packets;
} ccsds_stream_receiver_t;

/*
 * Initializes one receiver. Caller storage is writable and remains owned by
 * the caller. NULL crc_config selects CRC-16/CCITT-FALSE defaults.
 * CRC error control NONE is supported but cannot detect corrupt payloads.
 */
ccsds_status_t ccsds_stream_receiver_init(ccsds_stream_receiver_t *receiver,
                                          uint8_t *storage,
                                          size_t capacity,
                                          uint32_t sync_pattern,
                                          ccsds_packet_error_control_t error_control,
                                          const ccsds_crc16_config_t *crc_config);

/* Drops buffered bytes without resetting cumulative counters or configuration. */
void ccsds_stream_receiver_reset(ccsds_stream_receiver_t *receiver);

/*
 * Consumes up to input_size bytes, returning at most one complete packet.
 * A successful partial feed reports ready_out=0; no packet is lost.
 * When ready_out=1, consumed_out is the number of input bytes consumed
 * through that packet. Pass any remaining input to the next feed() call.
 *
 * Input may be NULL only when input_size==0. feed(NULL,0) can drain a packet
 * already buffered following an explicit resync().
 * packet_out is unchanged unless ready_out=1.
 * On invalid API arguments, no input is consumed.
 */
ccsds_status_t ccsds_stream_receiver_feed(ccsds_stream_receiver_t *receiver,
                                          const uint8_t *input,
                                          size_t input_size,
                                          size_t *consumed_out,
                                          ccsds_packet_view_t *packet_out,
                                          int *ready_out);

/*
 * Abandons an incomplete current frame (e.g. on caller-enforced timeout) and
 * searches already-buffered bytes for the next candidate sync, preserving
 * overlapping sync prefixes. A subsequent feed(NULL,0) can emit a recovered
 * complete packet. Never call while using a previously returned packet view.
 */
ccsds_status_t ccsds_stream_receiver_resync(ccsds_stream_receiver_t *receiver);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_STREAM_RECEIVER_H
