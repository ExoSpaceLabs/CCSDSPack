// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/stream_receiver.h"
#include "ccsdspack/c/bytes.h"
#include <string.h>

static void receiver_drop(ccsds_stream_receiver_t *rx, const size_t count) {
    const size_t remainder = rx->buffered - count;
    if (remainder != 0U) {
        memmove(rx->storage, rx->storage + count, remainder);
    }
    rx->buffered = remainder;
    rx->dropped_bytes += count;
}

static int receiver_prefix_matches(const ccsds_stream_receiver_t *rx,
                                   const size_t offset) {
    return rx->buffered - offset >= 4U
        && (uint32_t)ccsds_load_be_uint(rx->storage + offset, 4U)
            == rx->sync_pattern;
}

/*
 * A rejected candidate may already contain a later, valid sync and packet.
 * Search starting one byte after the rejected prefix to guarantee progress.
 * If no complete marker exists, retain the final three bytes to support a
 * marker spanning two independently delivered input chunks.
 */
static void receiver_recover(ccsds_stream_receiver_t *rx) {
    size_t offset;
    for (offset = 1U; offset + 4U <= rx->buffered; ++offset) {
        if (receiver_prefix_matches(rx, offset)) {
            receiver_drop(rx, offset);
            rx->phase = CCSDS_STREAM_RX_READ_HEADER;
            rx->required_frame_size = 0U;
            return;
        }
    }
    if (rx->buffered > 3U) receiver_drop(rx, rx->buffered - 3U);
    rx->phase = CCSDS_STREAM_RX_SEEK_SYNC;
    rx->required_frame_size = 0U;
}

static void receiver_release_delivered(ccsds_stream_receiver_t *rx) {
    if (rx->delivered_frame_size == 0U) return;
    /* Retain any bytes which were already buffered during resynchronization. */
    {
        const size_t remainder = rx->buffered - rx->delivered_frame_size;
        if (remainder != 0U) {
            memmove(rx->storage, rx->storage + rx->delivered_frame_size, remainder);
        }
        rx->buffered = remainder;
    }
    rx->delivered_frame_size = 0U;
    rx->phase = CCSDS_STREAM_RX_SEEK_SYNC;
    rx->required_frame_size = 0U;
}

/* Scan buffered data until a complete packet is found or input is needed. */
static int receiver_drain(ccsds_stream_receiver_t *rx,
                          ccsds_packet_view_t *packet_out) {
    for (;;) {
        if (rx->phase == CCSDS_STREAM_RX_SEEK_SYNC) {
            while (rx->buffered >= 4U && !receiver_prefix_matches(rx, 0U)) {
                receiver_drop(rx, 1U);
            }
            if (rx->buffered < 4U) return 0;
            rx->phase = CCSDS_STREAM_RX_READ_HEADER;
        }
        if (rx->phase == CCSDS_STREAM_RX_READ_HEADER) {
            size_t declared = 0U;
            ccsds_status_t status;
            if (rx->buffered < 4U + CCSDS_PRIMARY_HEADER_SIZE) return 0;

            status = ccsds_packet_declared_size(
                rx->storage + 4U, CCSDS_PRIMARY_HEADER_SIZE, &declared);
            if (status != CCSDS_STATUS_OK
                || declared < CCSDS_PRIMARY_HEADER_SIZE + 1U
                || declared > rx->capacity - 4U
                || (rx->error_control == CCSDS_PACKET_ERROR_CONTROL_CRC16
                    && declared < CCSDS_PRIMARY_HEADER_SIZE + 2U)) {
                ++rx->rejected_headers;
                receiver_recover(rx);
                continue;
            }
            rx->required_frame_size = declared + 4U;
            rx->phase = CCSDS_STREAM_RX_READ_BODY;
        }
        if (rx->phase == CCSDS_STREAM_RX_READ_BODY) {
            ccsds_packet_view_t view;
            ccsds_status_t status;
            if (rx->buffered < rx->required_frame_size) return 0;

            status = ccsds_packet_view_parse(
                rx->storage + 4U, rx->required_frame_size - 4U,
                rx->error_control, &rx->crc_config, &view);
            if (status != CCSDS_STATUS_OK) {
                ++rx->rejected_packets;
                receiver_recover(rx);
                continue;
            }
            ++rx->accepted_packets;
            rx->delivered_frame_size = rx->required_frame_size;
            *packet_out = view;
            return 1;
        }
    }
}

ccsds_status_t ccsds_stream_receiver_init(ccsds_stream_receiver_t *receiver,
                                          uint8_t *storage,
                                          const size_t capacity,
                                          const uint32_t sync_pattern,
                                          const ccsds_packet_error_control_t error_control,
                                          const ccsds_crc16_config_t *crc_config) {
    ccsds_stream_receiver_t init = {0};
    if (receiver == NULL || storage == NULL) return CCSDS_STATUS_NULL_POINTER;
    if (capacity < 4U + CCSDS_PRIMARY_HEADER_SIZE
                      + (error_control == CCSDS_PACKET_ERROR_CONTROL_CRC16 ? 2U : 1U)
        || (error_control != CCSDS_PACKET_ERROR_CONTROL_NONE
            && error_control != CCSDS_PACKET_ERROR_CONTROL_CRC16)) {
        return CCSDS_STATUS_INVALID_DATA;
    }
    init.storage = storage;
    init.capacity = capacity;
    init.sync_pattern = sync_pattern;
    init.error_control = error_control;
    init.crc_config =
        crc_config != NULL ? *crc_config : ccsds_crc16_default_config();
    init.phase = CCSDS_STREAM_RX_SEEK_SYNC;
    *receiver = init;
    return CCSDS_STATUS_OK;
}

void ccsds_stream_receiver_reset(ccsds_stream_receiver_t *receiver) {
    if (receiver == NULL) return;
    receiver->buffered = 0U;
    receiver->required_frame_size = 0U;
    receiver->delivered_frame_size = 0U;
    receiver->phase = CCSDS_STREAM_RX_SEEK_SYNC;
}

ccsds_status_t ccsds_stream_receiver_feed(ccsds_stream_receiver_t *receiver,
                                          const uint8_t *input,
                                          const size_t input_size,
                                          size_t *consumed_out,
                                          ccsds_packet_view_t *packet_out,
                                          int *ready_out) {
    size_t index;
    if (receiver == NULL || consumed_out == NULL
        || packet_out == NULL || ready_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (input_size != 0U && input == NULL) return CCSDS_STATUS_NULL_POINTER;
    if (receiver->storage == NULL || receiver->capacity < 11U
        || receiver->buffered > receiver->capacity) return CCSDS_STATUS_INVALID_DATA;

    *consumed_out = 0U;
    *ready_out = 0;
    receiver_release_delivered(receiver);
    if (receiver_drain(receiver, packet_out)) {
        *ready_out = 1;
        return CCSDS_STATUS_OK;
    }

    for (index = 0U; index < input_size; ++index) {
        /* Capacity was checked when the candidate's header was decoded. */
        if (receiver->buffered >= receiver->capacity) {
            return CCSDS_STATUS_BUFFER_TOO_SMALL;
        }
        receiver->storage[receiver->buffered++] = input[index];
        *consumed_out = index + 1U;
        if (receiver_drain(receiver, packet_out)) {
            *ready_out = 1;
            return CCSDS_STATUS_OK;
        }
    }
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_stream_receiver_resync(ccsds_stream_receiver_t *receiver) {
    if (receiver == NULL) return CCSDS_STATUS_NULL_POINTER;
    if (receiver->storage == NULL || receiver->capacity < 11U
        || receiver->buffered > receiver->capacity) return CCSDS_STATUS_INVALID_DATA;

    receiver_release_delivered(receiver);
    if (receiver->phase == CCSDS_STREAM_RX_READ_HEADER
        || receiver->phase == CCSDS_STREAM_RX_READ_BODY) {
        ++receiver->rejected_packets;
        receiver_recover(receiver);
    }
    return CCSDS_STATUS_OK;
}
