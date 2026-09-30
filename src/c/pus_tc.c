// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/pus_tc.h"
#include "ccsdspack/c/bytes.h"

static ccsds_status_t validate_fields(const ccsds_pus_tc_fields_t *fields,
                                      const uint8_t source_id_octets) {
    if (fields == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (fields->acknowledgement_flags > 0x0FU) {
        return CCSDS_STATUS_PUS_INVALID_ACK_FLAGS;
    }
    if (!ccsds_pus_identifier_fits(fields->source_id, source_id_octets)) {
        return CCSDS_STATUS_PUS_IDENTIFIER_OVERFLOW;
    }
    return CCSDS_STATUS_OK;
}

static void write_spare(uint8_t *output, const size_t offset,
                        const uint8_t spare_octets) {
    uint8_t index;
    for (index = 0U; index < spare_octets; ++index) {
        output[offset + (size_t)index] = 0U;
    }
}

ccsds_status_t ccsds_pus_a_tc_validate_tailoring(
    const ccsds_pus_a_tc_tailoring_t *tailoring) {
    if (tailoring == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (!ccsds_pus_identifier_width_is_valid(tailoring->source_id_octets)) {
        return CCSDS_STATUS_PUS_INVALID_IDENTIFIER_WIDTH;
    }
    return CCSDS_STATUS_OK;
}

size_t ccsds_pus_a_tc_encoded_size(const ccsds_pus_a_tc_tailoring_t *tailoring) {
    if (ccsds_pus_a_tc_validate_tailoring(tailoring) != CCSDS_STATUS_OK) {
        return 0U;
    }
    return (size_t)3U
         + (size_t)tailoring->source_id_octets
         + (size_t)tailoring->secondary_header_spare_octets;
}

size_t ccsds_pus_c_tc_encoded_size(const ccsds_pus_c_tc_tailoring_t *tailoring) {
    if (tailoring == NULL) {
        return 0U;
    }
    return (size_t)5U + (size_t)tailoring->secondary_header_spare_octets;
}

ccsds_status_t ccsds_pus_a_tc_encode(
    const ccsds_pus_tc_fields_t *fields,
    const ccsds_pus_a_tc_tailoring_t *tailoring,
    uint8_t *output,
    const size_t capacity,
    size_t *written_out) {
    size_t offset = 0U;
    size_t required;
    ccsds_status_t status;

    if (tailoring == NULL || written_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    status = ccsds_pus_a_tc_validate_tailoring(tailoring);
    if (status != CCSDS_STATUS_OK) return status;
    status = validate_fields(fields, tailoring->source_id_octets);
    if (status != CCSDS_STATUS_OK) return status;

    required = ccsds_pus_a_tc_encoded_size(tailoring);
    if (capacity < required) return CCSDS_STATUS_BUFFER_TOO_SMALL;
    if (output == NULL) return CCSDS_STATUS_NULL_POINTER;

    output[offset++] = (uint8_t)(0x10U | fields->acknowledgement_flags);
    output[offset++] = fields->service_type;
    output[offset++] = fields->service_subtype;
    ccsds_store_uint(output + offset, tailoring->source_id_octets,
                     fields->source_id, CCSDS_BYTE_ORDER_BIG);
    offset += (size_t)tailoring->source_id_octets;
    write_spare(output, offset, tailoring->secondary_header_spare_octets);
    offset += (size_t)tailoring->secondary_header_spare_octets;

    *written_out = offset;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_pus_a_tc_decode(
    const uint8_t *data,
    const size_t size,
    const ccsds_pus_a_tc_tailoring_t *tailoring,
    ccsds_pus_tc_fields_t *fields_out) {
    ccsds_status_t status;
    size_t expected;

    if (tailoring == NULL || fields_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    status = ccsds_pus_a_tc_validate_tailoring(tailoring);
    if (status != CCSDS_STATUS_OK) return status;

    expected = ccsds_pus_a_tc_encoded_size(tailoring);
    if (size != expected) return CCSDS_STATUS_PUS_SIZE_MISMATCH;
    if (data == NULL) return CCSDS_STATUS_NULL_POINTER;
    if ((data[0] & 0xF0U) != 0x10U) return CCSDS_STATUS_PUS_INVALID_VERSION;
    if (!ccsds_pus_spare_is_zero(data, size, tailoring->secondary_header_spare_octets)) {
        return CCSDS_STATUS_PUS_NONZERO_SPARE;
    }

    fields_out->acknowledgement_flags = (uint8_t)(data[0] & 0x0FU);
    fields_out->service_type = data[1];
    fields_out->service_subtype = data[2];
    fields_out->source_id = (uint32_t)ccsds_load_uint(
        data + 3U, tailoring->source_id_octets, CCSDS_BYTE_ORDER_BIG);
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_pus_c_tc_encode(
    const ccsds_pus_tc_fields_t *fields,
    const ccsds_pus_c_tc_tailoring_t *tailoring,
    uint8_t *output,
    const size_t capacity,
    size_t *written_out) {
    size_t offset = 0U;
    size_t required;
    ccsds_status_t status;

    if (tailoring == NULL || written_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    status = validate_fields(fields, 2U);
    if (status != CCSDS_STATUS_OK) return status;

    required = ccsds_pus_c_tc_encoded_size(tailoring);
    if (capacity < required) return CCSDS_STATUS_BUFFER_TOO_SMALL;
    if (output == NULL) return CCSDS_STATUS_NULL_POINTER;

    output[offset++] = (uint8_t)(0x20U | fields->acknowledgement_flags);
    output[offset++] = fields->service_type;
    output[offset++] = fields->service_subtype;
    ccsds_store_uint(output + offset, 2U, fields->source_id, CCSDS_BYTE_ORDER_BIG);
    offset += 2U;
    write_spare(output, offset, tailoring->secondary_header_spare_octets);
    offset += (size_t)tailoring->secondary_header_spare_octets;

    *written_out = offset;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_pus_c_tc_decode(
    const uint8_t *data,
    const size_t size,
    const ccsds_pus_c_tc_tailoring_t *tailoring,
    ccsds_pus_tc_fields_t *fields_out) {
    size_t expected;

    if (tailoring == NULL || fields_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    expected = ccsds_pus_c_tc_encoded_size(tailoring);
    if (size != expected) return CCSDS_STATUS_PUS_SIZE_MISMATCH;
    if (data == NULL) return CCSDS_STATUS_NULL_POINTER;
    if ((data[0] & 0xF0U) != 0x20U) return CCSDS_STATUS_PUS_INVALID_VERSION;
    if (!ccsds_pus_spare_is_zero(data, size, tailoring->secondary_header_spare_octets)) {
        return CCSDS_STATUS_PUS_NONZERO_SPARE;
    }

    fields_out->acknowledgement_flags = (uint8_t)(data[0] & 0x0FU);
    fields_out->service_type = data[1];
    fields_out->service_subtype = data[2];
    fields_out->source_id = (uint32_t)ccsds_load_uint(
        data + 3U, 2U, CCSDS_BYTE_ORDER_BIG);
    return CCSDS_STATUS_OK;
}
