// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/pus_tm.h"
#include "ccsdspack/c/bytes.h"

static int cuc_is_empty(const ccsds_cuc_config_t *config) {
    return config->epoch == CCSDS_CUC_EPOCH_UNSPECIFIED
        && config->pfield_mode == CCSDS_CUC_PFIELD_IMPLICIT
        && config->coarse_octets == 0U
        && config->fine_octets == 0U;
}

static ccsds_status_t validate_time_tailoring(const uint8_t timestamp_present,
                                              const ccsds_cuc_config_t *config) {
    if (timestamp_present > 1U) {
        return CCSDS_STATUS_PUS_INVALID_BOOLEAN;
    }
    if (timestamp_present != 0U) {
        return ccsds_cuc_validate(config);
    }
    if (!cuc_is_empty(config)) {
        return CCSDS_STATUS_PUS_DISABLED_TIME_CONFIG;
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

ccsds_status_t ccsds_pus_a_tm_validate_tailoring(
    const ccsds_pus_a_tm_tailoring_t *tailoring) {
    ccsds_status_t status;

    if (tailoring == NULL) return CCSDS_STATUS_NULL_POINTER;
    if (!ccsds_pus_identifier_width_is_valid(tailoring->destination_id_octets)) {
        return CCSDS_STATUS_PUS_INVALID_IDENTIFIER_WIDTH;
    }
    if (tailoring->packet_subcounter_present > 1U) {
        return CCSDS_STATUS_PUS_INVALID_BOOLEAN;
    }
    status = validate_time_tailoring(tailoring->timestamp_present, &tailoring->cuc);
    return status;
}

ccsds_status_t ccsds_pus_c_tm_validate_tailoring(
    const ccsds_pus_c_tm_tailoring_t *tailoring) {
    if (tailoring == NULL) return CCSDS_STATUS_NULL_POINTER;
    return validate_time_tailoring(tailoring->timestamp_present, &tailoring->cuc);
}

size_t ccsds_pus_a_tm_encoded_size(const ccsds_pus_a_tm_tailoring_t *tailoring) {
    if (ccsds_pus_a_tm_validate_tailoring(tailoring) != CCSDS_STATUS_OK) {
        return 0U;
    }
    return (size_t)3U
         + (tailoring->packet_subcounter_present != 0U ? 1U : 0U)
         + (size_t)tailoring->destination_id_octets
         + (tailoring->timestamp_present != 0U
              ? ccsds_cuc_encoded_size(&tailoring->cuc) : 0U)
         + (size_t)tailoring->secondary_header_spare_octets;
}

size_t ccsds_pus_c_tm_encoded_size(const ccsds_pus_c_tm_tailoring_t *tailoring) {
    if (ccsds_pus_c_tm_validate_tailoring(tailoring) != CCSDS_STATUS_OK) {
        return 0U;
    }
    return (size_t)7U
         + (tailoring->timestamp_present != 0U
              ? ccsds_cuc_encoded_size(&tailoring->cuc) : 0U)
         + (size_t)tailoring->secondary_header_spare_octets;
}

ccsds_status_t ccsds_pus_a_tm_encode(
    const ccsds_pus_a_tm_fields_t *fields,
    const ccsds_pus_a_tm_tailoring_t *tailoring,
    uint8_t *output,
    const size_t capacity,
    size_t *written_out) {
    size_t offset = 0U;
    size_t required;
    size_t cuc_written = 0U;
    ccsds_status_t status;

    if (fields == NULL || tailoring == NULL || written_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_pus_a_tm_validate_tailoring(tailoring);
    if (status != CCSDS_STATUS_OK) return status;
    if (!ccsds_pus_identifier_fits(fields->destination_id,
                                   tailoring->destination_id_octets)) {
        return CCSDS_STATUS_PUS_IDENTIFIER_OVERFLOW;
    }

    required = ccsds_pus_a_tm_encoded_size(tailoring);
    if (capacity < required) return CCSDS_STATUS_BUFFER_TOO_SMALL;
    if (output == NULL) return CCSDS_STATUS_NULL_POINTER;

    output[offset++] = 0x10U;
    output[offset++] = fields->service_type;
    output[offset++] = fields->service_subtype;

    if (tailoring->packet_subcounter_present != 0U) {
        output[offset++] = fields->packet_subcounter;
    }

    ccsds_store_uint(output + offset, tailoring->destination_id_octets,
                     fields->destination_id, CCSDS_BYTE_ORDER_BIG);
    offset += (size_t)tailoring->destination_id_octets;

    if (tailoring->timestamp_present != 0U) {
        status = ccsds_cuc_encode(
            &fields->timestamp,
            &tailoring->cuc,
            output + offset,
            capacity - offset,
            &cuc_written);
        if (status != CCSDS_STATUS_OK) return status;
        offset += cuc_written;
    }

    write_spare(output, offset, tailoring->secondary_header_spare_octets);
    offset += (size_t)tailoring->secondary_header_spare_octets;

    *written_out = offset;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_pus_a_tm_decode(
    const uint8_t *data,
    const size_t size,
    const ccsds_pus_a_tm_tailoring_t *tailoring,
    ccsds_pus_a_tm_fields_t *fields_out) {
    size_t offset = 0U;
    size_t timestamp_size = 0U;
    size_t expected;
    ccsds_status_t status;

    if (tailoring == NULL || fields_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_pus_a_tm_validate_tailoring(tailoring);
    if (status != CCSDS_STATUS_OK) return status;
    expected = ccsds_pus_a_tm_encoded_size(tailoring);
    if (size != expected) return CCSDS_STATUS_PUS_SIZE_MISMATCH;
    if (data == NULL) return CCSDS_STATUS_NULL_POINTER;
    if (data[0] != 0x10U) return CCSDS_STATUS_PUS_INVALID_VERSION;
    if (!ccsds_pus_spare_is_zero(
            data, size, tailoring->secondary_header_spare_octets)) {
        return CCSDS_STATUS_PUS_NONZERO_SPARE;
    }

    fields_out->service_type = data[1];
    fields_out->service_subtype = data[2];
    offset = 3U;

    if (tailoring->packet_subcounter_present != 0U) {
        fields_out->packet_subcounter = data[offset++];
    } else {
        fields_out->packet_subcounter = 0U;
    }

    fields_out->destination_id = (uint32_t)ccsds_load_uint(
        data + offset, tailoring->destination_id_octets, CCSDS_BYTE_ORDER_BIG);
    offset += (size_t)tailoring->destination_id_octets;

    if (tailoring->timestamp_present != 0U) {
        timestamp_size = ccsds_cuc_encoded_size(&tailoring->cuc);
        status = ccsds_cuc_decode(
            data + offset, timestamp_size, &tailoring->cuc, &fields_out->timestamp);
        if (status != CCSDS_STATUS_OK) return status;
    } else {
        fields_out->timestamp.coarse = UINT64_C(0);
        fields_out->timestamp.fine = UINT64_C(0);
    }

    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_pus_c_tm_encode(
    const ccsds_pus_c_tm_fields_t *fields,
    const ccsds_pus_c_tm_tailoring_t *tailoring,
    uint8_t *output,
    const size_t capacity,
    size_t *written_out) {
    size_t offset = 0U;
    size_t required;
    size_t cuc_written = 0U;
    ccsds_status_t status;

    if (fields == NULL || tailoring == NULL || written_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_pus_c_tm_validate_tailoring(tailoring);
    if (status != CCSDS_STATUS_OK) return status;
    if (fields->time_reference_status > 0x0FU) {
        return CCSDS_STATUS_PUS_INVALID_TIME_REFERENCE_STATUS;
    }
    if (!ccsds_pus_identifier_fits(fields->destination_id, 2U)) {
        return CCSDS_STATUS_PUS_IDENTIFIER_OVERFLOW;
    }

    required = ccsds_pus_c_tm_encoded_size(tailoring);
    if (capacity < required) return CCSDS_STATUS_BUFFER_TOO_SMALL;
    if (output == NULL) return CCSDS_STATUS_NULL_POINTER;

    output[offset++] = (uint8_t)(0x20U | fields->time_reference_status);
    output[offset++] = fields->service_type;
    output[offset++] = fields->service_subtype;
    ccsds_store_be16(output + offset, fields->message_type_counter);
    offset += 2U;
    ccsds_store_be16(output + offset, (uint16_t)fields->destination_id);
    offset += 2U;

    if (tailoring->timestamp_present != 0U) {
        status = ccsds_cuc_encode(
            &fields->timestamp,
            &tailoring->cuc,
            output + offset,
            capacity - offset,
            &cuc_written);
        if (status != CCSDS_STATUS_OK) return status;
        offset += cuc_written;
    }

    write_spare(output, offset, tailoring->secondary_header_spare_octets);
    offset += (size_t)tailoring->secondary_header_spare_octets;

    *written_out = offset;
    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_pus_c_tm_decode(
    const uint8_t *data,
    const size_t size,
    const ccsds_pus_c_tm_tailoring_t *tailoring,
    ccsds_pus_c_tm_fields_t *fields_out) {
    size_t timestamp_size = 0U;
    size_t expected;
    ccsds_status_t status;

    if (tailoring == NULL || fields_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    status = ccsds_pus_c_tm_validate_tailoring(tailoring);
    if (status != CCSDS_STATUS_OK) return status;
    expected = ccsds_pus_c_tm_encoded_size(tailoring);
    if (size != expected) return CCSDS_STATUS_PUS_SIZE_MISMATCH;
    if (data == NULL) return CCSDS_STATUS_NULL_POINTER;
    if ((data[0] & 0xF0U) != 0x20U) return CCSDS_STATUS_PUS_INVALID_VERSION;
    if (!ccsds_pus_spare_is_zero(
            data, size, tailoring->secondary_header_spare_octets)) {
        return CCSDS_STATUS_PUS_NONZERO_SPARE;
    }

    fields_out->time_reference_status = (uint8_t)(data[0] & 0x0FU);
    fields_out->service_type = data[1];
    fields_out->service_subtype = data[2];
    fields_out->message_type_counter = ccsds_load_be16(data + 3U);
    fields_out->destination_id = (uint32_t)ccsds_load_be16(data + 5U);

    if (tailoring->timestamp_present != 0U) {
        timestamp_size = ccsds_cuc_encoded_size(&tailoring->cuc);
        status = ccsds_cuc_decode(
            data + 7U, timestamp_size, &tailoring->cuc, &fields_out->timestamp);
        if (status != CCSDS_STATUS_OK) return status;
    } else {
        fields_out->timestamp.coarse = UINT64_C(0);
        fields_out->timestamp.fine = UINT64_C(0);
    }

    return CCSDS_STATUS_OK;
}
