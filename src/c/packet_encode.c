// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/packet_encode.h"
#include "ccsdspack/c/bytes.h"
#include "ccsdspack/c/crc.h"
#include <string.h>

static ccsds_status_t packet_error_control_size(
    const ccsds_packet_error_control_t error_control,
    size_t *size_out) {
    if (size_out == NULL) return CCSDS_STATUS_NULL_POINTER;
    if (error_control == CCSDS_PACKET_ERROR_CONTROL_NONE) {
        *size_out = 0U;
        return CCSDS_STATUS_OK;
    }
    if (error_control == CCSDS_PACKET_ERROR_CONTROL_CRC16) {
        *size_out = 2U;
        return CCSDS_STATUS_OK;
    }
    return CCSDS_STATUS_INVALID_DATA;
}

static ccsds_status_t validate_packet_data_field(
    const ccsds_primary_header_t *header,
    const ccsds_packet_data_parts_t data_field,
    const size_t pec_size,
    size_t *packet_data_field_size_out) {
    size_t data_field_size;
    size_t packet_data_field_size;

    if (header == NULL || packet_data_field_size_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if ((data_field.first.data == NULL && data_field.first.size != 0U)
        || (data_field.second.data == NULL && data_field.second.size != 0U)) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (data_field.first.size > (size_t)UINT16_MAX + (size_t)1U
        || data_field.second.size > (size_t)UINT16_MAX + (size_t)1U
        || data_field.second.size
             > (size_t)UINT16_MAX + (size_t)1U - data_field.first.size) {
        return CCSDS_STATUS_INVALID_DATA;
    }
    data_field_size = data_field.first.size + data_field.second.size;
    if (data_field_size > (size_t)UINT16_MAX + (size_t)1U - pec_size) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    packet_data_field_size = data_field_size + pec_size;
    if (packet_data_field_size == 0U
        || packet_data_field_size > (size_t)UINT16_MAX + (size_t)1U) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    if (header->version_number != 0U) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }
    if (header->apid == CCSDS_IDLE_APID) {
        if (header->secondary_header_flag != 0U) {
            return CCSDS_STATUS_INVALID_HEADER_DATA;
        }
        if (data_field_size == 0U) {
            return CCSDS_STATUS_INVALID_DATA;
        }
    }

    *packet_data_field_size_out = packet_data_field_size;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_packet_finalize_parts(
    ccsds_primary_header_t *header,
    const uint16_t sequence_count,
    const ccsds_packet_data_parts_t data_field,
    const ccsds_packet_error_control_t error_control,
    const ccsds_crc16_config_t *crc_config,
    uint16_t *crc_out,
    size_t *serialized_size_out) {
    ccsds_primary_header_t staged;
    ccsds_crc16_config_t config;
    uint8_t header_bytes[CCSDS_PRIMARY_HEADER_SIZE];
    size_t pec_size;
    size_t packet_data_field_size;
    uint16_t crc = 0U;
    uint16_t crc_state;
    ccsds_status_t status;

    if (header == NULL || crc_out == NULL || serialized_size_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (sequence_count > CCSDS_SEQUENCE_COUNT_MAX) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }

    status = packet_error_control_size(error_control, &pec_size);
    if (status != CCSDS_STATUS_OK) return status;

    staged = *header;
    staged.sequence_count = sequence_count;

    status = validate_packet_data_field(
        &staged, data_field, pec_size, &packet_data_field_size);
    if (status != CCSDS_STATUS_OK) return status;

    staged.data_length = (uint16_t)(packet_data_field_size - 1U);
    status = ccsds_primary_header_validate(&staged);
    if (status != CCSDS_STATUS_OK) return status;

    if (error_control == CCSDS_PACKET_ERROR_CONTROL_CRC16) {
        config = crc_config != NULL ? *crc_config : ccsds_crc16_default_config();

        status = ccsds_primary_header_encode(
            &staged, header_bytes, sizeof(header_bytes));
        if (status != CCSDS_STATUS_OK) return status;

        crc_state = config.initial_value;
        status = ccsds_crc16_update(
            &crc_state, header_bytes, sizeof(header_bytes), config.polynomial);
        if (status != CCSDS_STATUS_OK) return status;
        status = ccsds_crc16_update(
            &crc_state, data_field.first.data, data_field.first.size, config.polynomial);
        if (status != CCSDS_STATUS_OK) return status;
        status = ccsds_crc16_update(
            &crc_state, data_field.second.data, data_field.second.size, config.polynomial);
        if (status != CCSDS_STATUS_OK) return status;
        crc = (uint16_t)(crc_state ^ config.final_xor_value);
    }

    *header = staged;
    *crc_out = crc;
    *serialized_size_out = CCSDS_PRIMARY_HEADER_SIZE + packet_data_field_size;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_packet_encode_parts(
    const ccsds_primary_header_t *header,
    const ccsds_packet_data_parts_t data_field,
    const ccsds_packet_error_control_t error_control,
    const uint16_t crc16,
    uint8_t *output,
    const size_t capacity,
    size_t *written_out) {
    size_t pec_size;
    size_t packet_data_field_size;
    size_t required;
    ccsds_status_t status;

    if (header == NULL || written_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = packet_error_control_size(error_control, &pec_size);
    if (status != CCSDS_STATUS_OK) return status;
    status = validate_packet_data_field(
        header, data_field, pec_size, &packet_data_field_size);
    if (status != CCSDS_STATUS_OK) return status;
    status = ccsds_primary_header_validate(header);
    if (status != CCSDS_STATUS_OK) return status;

    if ((size_t)header->data_length + 1U != packet_data_field_size) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    required = CCSDS_PRIMARY_HEADER_SIZE + packet_data_field_size;
    if (capacity < required) return CCSDS_STATUS_BUFFER_TOO_SMALL;
    if (output == NULL) return CCSDS_STATUS_NULL_POINTER;

    if (data_field.first.size != 0U) {
        memmove(output + CCSDS_PRIMARY_HEADER_SIZE,
                data_field.first.data,
                data_field.first.size);
    }
    if (data_field.second.size != 0U) {
        memmove(output + CCSDS_PRIMARY_HEADER_SIZE + data_field.first.size,
                data_field.second.data,
                data_field.second.size);
    }

    status = ccsds_primary_header_encode(header, output, capacity);
    if (status != CCSDS_STATUS_OK) return status;

    if (pec_size != 0U) {
        ccsds_store_be16(
            output + CCSDS_PRIMARY_HEADER_SIZE
              + data_field.first.size + data_field.second.size,
            crc16);
    }

    *written_out = required;
    return CCSDS_STATUS_OK;
}


ccsds_status_t ccsds_packet_finalize(ccsds_primary_header_t *header,
                                     const uint16_t sequence_count,
                                     const ccsds_buffer_view_t data_field,
                                     const ccsds_packet_error_control_t error_control,
                                     const ccsds_crc16_config_t *crc_config,
                                     uint16_t *crc_out,
                                     size_t *serialized_size_out) {
    const ccsds_packet_data_parts_t parts = {
        data_field,
        {NULL, 0U}
    };
    return ccsds_packet_finalize_parts(
        header, sequence_count, parts, error_control, crc_config,
        crc_out, serialized_size_out);
}

ccsds_status_t ccsds_packet_encode(const ccsds_primary_header_t *header,
                                   const ccsds_buffer_view_t data_field,
                                   const ccsds_packet_error_control_t error_control,
                                   const uint16_t crc16,
                                   uint8_t *output,
                                   const size_t capacity,
                                   size_t *written_out) {
    const ccsds_packet_data_parts_t parts = {
        data_field,
        {NULL, 0U}
    };
    return ccsds_packet_encode_parts(
        header, parts, error_control, crc16, output, capacity, written_out);
}
