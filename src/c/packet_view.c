// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/packet_view.h"
#include "ccsdspack/c/bytes.h"

/*
 * The sole C11 wire-policy implementation for contiguous and split inputs.
 * exact_body==0 permits trailing stream bytes; exact_body!=0 requires equality.
 * On error, all published views remain untouched.
 */
static ccsds_status_t parse_parts(const uint8_t *header_data,
                                 const size_t header_size,
                                 const uint8_t *body_data,
                                 const size_t body_size,
                                 const ccsds_packet_error_control_t error_control,
                                 const ccsds_crc16_config_t *crc_config,
                                 const int exact_body,
                                 ccsds_packet_parts_view_t *view_out,
                                 ccsds_packet_parse_reason_t *reason_out) {
    ccsds_packet_parts_view_t view = {0};
    ccsds_crc16_config_t config;
    ccsds_status_t status;
    ccsds_packet_parse_reason_t reason = CCSDS_PACKET_PARSE_OK;
    size_t required_body;
    size_t crc_size = 0U;
    uint16_t crc_state;
    uint16_t received_crc = 0U;

    if (header_size != CCSDS_PRIMARY_HEADER_SIZE) {
        status = CCSDS_STATUS_INVALID_HEADER_DATA;
        reason = CCSDS_PACKET_PARSE_HEADER_SIZE;
        goto failure;
    }
    if (header_data == NULL || view_out == NULL) {
        status = CCSDS_STATUS_NULL_POINTER;
        reason = CCSDS_PACKET_PARSE_NULL_INPUT;
        goto failure;
    }
    if (error_control != CCSDS_PACKET_ERROR_CONTROL_NONE
        && error_control != CCSDS_PACKET_ERROR_CONTROL_CRC16) {
        status = CCSDS_STATUS_INVALID_DATA;
        reason = CCSDS_PACKET_PARSE_INVALID_POLICY;
        goto failure;
    }
    status = ccsds_primary_header_decode(header_data, header_size,
                                         &view.primary_header);
    if (status != CCSDS_STATUS_OK) {
        reason = CCSDS_PACKET_PARSE_HEADER_INVALID;
        goto failure;
    }
    if (view.primary_header.version_number != 0U) {
        status = CCSDS_STATUS_INVALID_HEADER_DATA;
        reason = CCSDS_PACKET_PARSE_VERSION;
        goto failure;
    }
    required_body = (size_t)view.primary_header.data_length + 1U;
    if ((exact_body != 0 && body_size != required_body)
        || (exact_body == 0 && body_size < required_body)) {
        status = CCSDS_STATUS_INVALID_DATA;
        reason = exact_body != 0
            ? CCSDS_PACKET_PARSE_BODY_SIZE_MISMATCH
            : CCSDS_PACKET_PARSE_BODY_TRUNCATED;
        goto failure;
    }
    if (body_data == NULL) {
        status = CCSDS_STATUS_NULL_POINTER;
        reason = CCSDS_PACKET_PARSE_NULL_INPUT;
        goto failure;
    }
    crc_size = error_control == CCSDS_PACKET_ERROR_CONTROL_CRC16 ? 2U : 0U;
    if (required_body < crc_size) {
        status = CCSDS_STATUS_INVALID_DATA;
        reason = CCSDS_PACKET_PARSE_CRC_TRAILER_TOO_SHORT;
        goto failure;
    }
    /* Idle policy is applied before checking CRC, matching existing C++ error
       precedence for malformed idle packets. */
    if (view.primary_header.apid == CCSDS_IDLE_APID) {
        if (view.primary_header.secondary_header_flag != 0U) {
            status = CCSDS_STATUS_INVALID_HEADER_DATA;
            reason = CCSDS_PACKET_PARSE_IDLE_SECONDARY_HEADER;
            goto failure;
        }
        if (required_body == crc_size) {
            status = CCSDS_STATUS_INVALID_DATA;
            reason = CCSDS_PACKET_PARSE_IDLE_DATA_REQUIRED;
            goto failure;
        }
    }
    if (crc_size != 0U) {
        config = crc_config != NULL ? *crc_config : ccsds_crc16_default_config();
        crc_state = config.initial_value;
        status = ccsds_crc16_update(&crc_state, header_data,
                                    CCSDS_PRIMARY_HEADER_SIZE,
                                    config.polynomial);
        if (status != CCSDS_STATUS_OK) goto failure;
        status = ccsds_crc16_update(&crc_state, body_data,
                                    required_body - crc_size,
                                    config.polynomial);
        if (status != CCSDS_STATUS_OK) goto failure;
        received_crc = ccsds_load_be16(body_data + required_body - crc_size);
        if ((uint16_t)(crc_state ^ config.final_xor_value) != received_crc) {
            status = CCSDS_STATUS_INVALID_CHECKSUM;
            reason = CCSDS_PACKET_PARSE_CRC_MISMATCH;
            goto failure;
        }
    }

    view.header_bytes.data = header_data;
    view.header_bytes.size = CCSDS_PRIMARY_HEADER_SIZE;
    view.body.data = body_data;
    view.body.size = required_body;
    view.data_field.data = body_data;
    view.data_field.size = required_body - crc_size;
    view.packet_error_control.data = crc_size == 0U
        ? NULL : body_data + required_body - crc_size;
    view.packet_error_control.size = crc_size;
    view.received_crc16 = received_crc;
    view.consumed = CCSDS_PRIMARY_HEADER_SIZE + required_body;
    *view_out = view;
    if (reason_out != NULL) *reason_out = CCSDS_PACKET_PARSE_OK;
    return CCSDS_STATUS_OK;

failure:
    if (reason_out != NULL) *reason_out = reason;
    return status;
}

ccsds_status_t ccsds_packet_parts_parse(
    const uint8_t *header_data, const size_t header_size,
    const uint8_t *body_data, const size_t body_size,
    const ccsds_packet_error_control_t error_control,
    const ccsds_crc16_config_t *crc_config,
    ccsds_packet_parts_view_t *view_out,
    ccsds_packet_parse_reason_t *reason_out) {
    return parse_parts(header_data, header_size, body_data, body_size,
                       error_control, crc_config, 1, view_out, reason_out);
}

ccsds_status_t ccsds_packet_view_parse_ex(
    const uint8_t *data, const size_t size,
    const ccsds_packet_error_control_t error_control,
    const ccsds_crc16_config_t *crc_config,
    ccsds_packet_view_t *view_out,
    ccsds_packet_parse_reason_t *reason_out) {
    ccsds_packet_parts_view_t parts = {0};
    ccsds_packet_view_t view = {0};
    ccsds_status_t status;
    ccsds_packet_parse_reason_t reason = CCSDS_PACKET_PARSE_OK;

    if (size < CCSDS_PRIMARY_HEADER_SIZE) {
        status = CCSDS_STATUS_INVALID_HEADER_DATA;
        reason = CCSDS_PACKET_PARSE_HEADER_SIZE;
        goto failure;
    }
    if (data == NULL || view_out == NULL) {
        status = CCSDS_STATUS_NULL_POINTER;
        reason = CCSDS_PACKET_PARSE_NULL_INPUT;
        goto failure;
    }
    status = parse_parts(data, CCSDS_PRIMARY_HEADER_SIZE,
                         data + CCSDS_PRIMARY_HEADER_SIZE,
                         size - CCSDS_PRIMARY_HEADER_SIZE,
                         error_control, crc_config, 0, &parts, &reason);
    if (status != CCSDS_STATUS_OK) goto failure;

    view.primary_header = parts.primary_header;
    view.packet.data = data;
    view.packet.size = parts.consumed;
    view.body = parts.body;
    view.data_field = parts.data_field;
    view.packet_error_control = parts.packet_error_control;
    view.consumed = parts.consumed;
    view.received_crc16 = parts.received_crc16;
    *view_out = view;
    if (reason_out != NULL) *reason_out = CCSDS_PACKET_PARSE_OK;
    return CCSDS_STATUS_OK;

failure:
    if (reason_out != NULL) *reason_out = reason;
    return status;
}

ccsds_status_t ccsds_packet_view_parse(
    const uint8_t *data, const size_t size,
    const ccsds_packet_error_control_t error_control,
    const ccsds_crc16_config_t *crc_config,
    ccsds_packet_view_t *view_out) {
    return ccsds_packet_view_parse_ex(data, size, error_control, crc_config,
                                      view_out, NULL);
}
