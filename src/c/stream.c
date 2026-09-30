// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/stream.h"
#include "ccsdspack/c/bytes.h"

ccsds_status_t ccsds_packet_stream_init(ccsds_packet_stream_t *stream,
                                        const uint8_t *data,
                                        const size_t size,
                                        const ccsds_packet_error_control_t error_control,
                                        const ccsds_crc16_config_t *crc_config,
                                        const int sync_enabled,
                                        const uint32_t sync_pattern) {
    if (stream == NULL || data == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (size == 0U) {
        return CCSDS_STATUS_NO_DATA;
    }
    if (error_control != CCSDS_PACKET_ERROR_CONTROL_NONE
        && error_control != CCSDS_PACKET_ERROR_CONTROL_CRC16) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    stream->data = data;
    stream->size = size;
    stream->offset = 0U;
    stream->sync_pattern = sync_pattern;
    stream->error_control = error_control;
    stream->crc_config =
        crc_config != NULL ? *crc_config : ccsds_crc16_default_config();
    stream->sync_enabled = (uint8_t)(sync_enabled != 0);
    return CCSDS_STATUS_OK;
}

size_t ccsds_packet_stream_remaining(const ccsds_packet_stream_t *stream) {
    if (stream == NULL || stream->offset > stream->size) {
        return 0U;
    }
    return stream->size - stream->offset;
}

ccsds_status_t ccsds_packet_stream_next(ccsds_packet_stream_t *stream,
                                        ccsds_packet_view_t *view_out,
                                        size_t *frame_consumed_out) {
    size_t packet_offset;
    size_t frame_size;
    ccsds_packet_view_t view;
    ccsds_status_t status;

    if (stream == NULL || view_out == NULL || frame_consumed_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (stream->data == NULL || stream->offset > stream->size) {
        return CCSDS_STATUS_INVALID_DATA;
    }
    if (stream->offset == stream->size) {
        return CCSDS_STATUS_NO_DATA;
    }

    packet_offset = stream->offset;
    if (stream->sync_enabled != 0U) {
        if (stream->size - packet_offset < 4U) {
            return CCSDS_STATUS_INVALID_DATA;
        }
        if ((uint32_t)ccsds_load_be_uint(stream->data + packet_offset, 4U) != stream->sync_pattern) {
            return CCSDS_STATUS_INVALID_DATA;
        }
        packet_offset += 4U;
    }

    status = ccsds_packet_view_parse(
        stream->data + packet_offset,
        stream->size - packet_offset,
        stream->error_control,
        &stream->crc_config,
        &view);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }

    frame_size = (packet_offset - stream->offset) + view.consumed;
    stream->offset += frame_size;
    *view_out = view;
    *frame_consumed_out = frame_size;
    return CCSDS_STATUS_OK;
}
