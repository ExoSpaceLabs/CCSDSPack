// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/packet_view.h"
#include "ccsdspack/c/bytes.h"

ccsds_status_t ccsds_packet_view_parse(const uint8_t *data,
                                       const size_t size,
                                       const ccsds_packet_error_control_t error_control,
                                       const ccsds_crc16_config_t *crc_config,
                                       ccsds_packet_view_t *view_out) {
    ccsds_primary_header_t header;
    ccsds_crc16_config_t config;
    size_t packet_size;
    size_t body_size;
    size_t pec_size = 0U;
    uint16_t calculated_crc = 0U;
    ccsds_status_t status;

    if (data == NULL || view_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (error_control != CCSDS_PACKET_ERROR_CONTROL_NONE
        && error_control != CCSDS_PACKET_ERROR_CONTROL_CRC16) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    status = ccsds_primary_header_decode(data, size, &header);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }
    if (header.version_number != 0U) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }

    packet_size = CCSDS_PRIMARY_HEADER_SIZE
                + (size_t)header.data_length
                + (size_t)1U;
    if (size < packet_size) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    body_size = packet_size - CCSDS_PRIMARY_HEADER_SIZE;
    if (error_control == CCSDS_PACKET_ERROR_CONTROL_CRC16) {
        if (body_size < 2U) {
            return CCSDS_STATUS_INVALID_DATA;
        }
        pec_size = 2U;
        config = crc_config != NULL ? *crc_config : ccsds_crc16_default_config();

        status = ccsds_crc16_compute(
            data,
            packet_size - pec_size,
            config.polynomial,
            config.initial_value,
            config.final_xor_value,
            &calculated_crc);
        if (status != CCSDS_STATUS_OK) {
            return status;
        }

        view_out->received_crc16 = ccsds_load_be16(data + packet_size - pec_size);
        if (calculated_crc != view_out->received_crc16) {
            return CCSDS_STATUS_INVALID_CHECKSUM;
        }
    } else {
        view_out->received_crc16 = 0U;
    }

    view_out->primary_header = header;
    view_out->packet.data = data;
    view_out->packet.size = packet_size;
    view_out->body.data = data + CCSDS_PRIMARY_HEADER_SIZE;
    view_out->body.size = body_size;
    view_out->data_field.data = data + CCSDS_PRIMARY_HEADER_SIZE;
    view_out->data_field.size = body_size - pec_size;
    view_out->packet_error_control.data =
        pec_size == 0U ? NULL : data + packet_size - pec_size;
    view_out->packet_error_control.size = pec_size;
    view_out->consumed = packet_size;

    if (header.apid == CCSDS_IDLE_APID) {
        if (header.secondary_header_flag != 0U) {
            return CCSDS_STATUS_INVALID_HEADER_DATA;
        }
        if (view_out->data_field.size == 0U) {
            return CCSDS_STATUS_INVALID_DATA;
        }
    }

    return CCSDS_STATUS_OK;
}
