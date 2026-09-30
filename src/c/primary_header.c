// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/primary_header.h"

ccsds_status_t ccsds_primary_header_validate(const ccsds_primary_header_t *header) {
    if (header == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (header->version_number > 0x07U
        || header->type > 0x01U
        || header->secondary_header_flag > 0x01U
        || header->apid > CCSDS_IDLE_APID
        || header->sequence_flags > 0x03U
        || header->sequence_count > CCSDS_SEQUENCE_COUNT_MAX) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_primary_header_decode(const uint8_t *data,
                                           const size_t size,
                                           ccsds_primary_header_t *header_out) {
    uint16_t packet_id;
    uint16_t sequence;

    if (data == NULL || header_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (size < CCSDS_PRIMARY_HEADER_SIZE) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }

    packet_id = (uint16_t)(((uint16_t)data[0] << 8U) | (uint16_t)data[1]);
    sequence = (uint16_t)(((uint16_t)data[2] << 8U) | (uint16_t)data[3]);

    header_out->version_number = (uint8_t)((packet_id >> 13U) & 0x07U);
    header_out->type = (uint8_t)((packet_id >> 12U) & 0x01U);
    header_out->secondary_header_flag = (uint8_t)((packet_id >> 11U) & 0x01U);
    header_out->apid = (uint16_t)(packet_id & 0x07FFU);
    header_out->sequence_flags = (uint8_t)((sequence >> 14U) & 0x03U);
    header_out->sequence_count = (uint16_t)(sequence & CCSDS_SEQUENCE_COUNT_MAX);
    header_out->data_length = (uint16_t)(((uint16_t)data[4] << 8U) | (uint16_t)data[5]);

    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_primary_header_encode(const ccsds_primary_header_t *header,
                                           uint8_t *data_out,
                                           const size_t capacity) {
    uint16_t packet_id;
    uint16_t sequence;
    ccsds_status_t status;

    if (data_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (capacity < CCSDS_PRIMARY_HEADER_SIZE) {
        return CCSDS_STATUS_BUFFER_TOO_SMALL;
    }

    status = ccsds_primary_header_validate(header);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }

    packet_id = (uint16_t)(((uint16_t)header->version_number << 13U)
             | ((uint16_t)header->type << 12U)
             | ((uint16_t)header->secondary_header_flag << 11U)
             | header->apid);
    sequence = (uint16_t)(((uint16_t)header->sequence_flags << 14U)
            | header->sequence_count);

    data_out[0] = (uint8_t)(packet_id >> 8U);
    data_out[1] = (uint8_t)(packet_id & 0xFFU);
    data_out[2] = (uint8_t)(sequence >> 8U);
    data_out[3] = (uint8_t)(sequence & 0xFFU);
    data_out[4] = (uint8_t)(header->data_length >> 8U);
    data_out[5] = (uint8_t)(header->data_length & 0xFFU);

    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_primary_header_pack(const ccsds_primary_header_t *header,
                                         uint64_t *packed_out) {
    uint8_t bytes[CCSDS_PRIMARY_HEADER_SIZE];
    ccsds_status_t status;

    if (packed_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_primary_header_encode(header, bytes, sizeof(bytes));
    if (status != CCSDS_STATUS_OK) {
        return status;
    }

    *packed_out = ((uint64_t)bytes[0] << 40U)
                | ((uint64_t)bytes[1] << 32U)
                | ((uint64_t)bytes[2] << 24U)
                | ((uint64_t)bytes[3] << 16U)
                | ((uint64_t)bytes[4] << 8U)
                | (uint64_t)bytes[5];
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_primary_header_unpack(const uint64_t packed,
                                           ccsds_primary_header_t *header_out) {
    uint8_t bytes[CCSDS_PRIMARY_HEADER_SIZE];

    if (header_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (packed > UINT64_C(0xFFFFFFFFFFFF)) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }

    bytes[0] = (uint8_t)(packed >> 40U);
    bytes[1] = (uint8_t)(packed >> 32U);
    bytes[2] = (uint8_t)(packed >> 24U);
    bytes[3] = (uint8_t)(packed >> 16U);
    bytes[4] = (uint8_t)(packed >> 8U);
    bytes[5] = (uint8_t)packed;

    return ccsds_primary_header_decode(bytes, sizeof(bytes), header_out);
}

ccsds_status_t ccsds_packet_declared_size(const uint8_t *data,
                                          const size_t size,
                                          size_t *packet_size_out) {
    ccsds_primary_header_t header;
    ccsds_status_t status;

    if (packet_size_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_primary_header_decode(data, size, &header);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }
    if (header.version_number != 0U) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }

    *packet_size_out = CCSDS_PRIMARY_HEADER_SIZE
                     + (size_t)header.data_length
                     + (size_t)1U;
    return CCSDS_STATUS_OK;
}
