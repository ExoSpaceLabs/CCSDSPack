// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/validation.h"
#include "ccsdspack/c/pus.h"

void ccsds_validation_report_reset(ccsds_validation_report_t *report) {
    size_t index;
    if (report == NULL) {
        return;
    }
    for (index = 0U; index < CCSDS_VALIDATION_REPORT_CAPACITY; ++index) {
        report->checks[index].code = 0U;
        report->checks[index].passed = 0U;
    }
    report->size = 0U;
}

ccsds_status_t ccsds_validation_report_set(ccsds_validation_report_t *report,
                                           const ccsds_validation_code_t code,
                                           const int passed) {
    size_t index;
    if (report == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (code >= CCSDS_VALIDATION_CODE_COUNT) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    for (index = 0U; index < report->size; ++index) {
        if (report->checks[index].code == code) {
            report->checks[index].passed =
                (uint8_t)(report->checks[index].passed != 0U && passed != 0);
            return CCSDS_STATUS_OK;
        }
    }

    if (report->size >= CCSDS_VALIDATION_REPORT_CAPACITY) {
        return CCSDS_STATUS_BUFFER_TOO_SMALL;
    }

    report->checks[report->size].code = code;
    report->checks[report->size].passed = (uint8_t)(passed != 0);
    ++report->size;
    return CCSDS_STATUS_OK;
}

int ccsds_validation_report_valid(const ccsds_validation_report_t *report) {
    size_t index;
    if (report == NULL) {
        return 0;
    }
    for (index = 0U; index < report->size; ++index) {
        if (report->checks[index].passed == 0U) {
            return 0;
        }
    }
    return 1;
}

int ccsds_validation_report_contains(const ccsds_validation_report_t *report,
                                     const ccsds_validation_code_t code) {
    size_t index;
    if (report == NULL || code >= CCSDS_VALIDATION_CODE_COUNT) {
        return 0;
    }
    for (index = 0U; index < report->size; ++index) {
        if (report->checks[index].code == code) {
            return 1;
        }
    }
    return 0;
}

int ccsds_validation_report_passed(const ccsds_validation_report_t *report,
                                   const ccsds_validation_code_t code) {
    size_t index;
    if (report == NULL || code >= CCSDS_VALIDATION_CODE_COUNT) {
        return 0;
    }
    for (index = 0U; index < report->size; ++index) {
        if (report->checks[index].code == code) {
            return report->checks[index].passed != 0U;
        }
    }
    return 0;
}

int ccsds_validation_report_failed(const ccsds_validation_report_t *report,
                                   const ccsds_validation_code_t code) {
    return ccsds_validation_report_contains(report, code)
        && !ccsds_validation_report_passed(report, code);
}

const char *ccsds_validation_code_name(const ccsds_validation_code_t code) {
    switch (code) {
        case CCSDS_VALIDATION_PRIMARY_HEADER: return "CCSDS primary header";
        case CCSDS_VALIDATION_PACKET_VERSION: return "CCSDS packet version";
        case CCSDS_VALIDATION_PACKET_DATA_LENGTH: return "Packet Data Length";
        case CCSDS_VALIDATION_CRC16: return "CRC16";
        case CCSDS_VALIDATION_SECONDARY_HEADER_PRESENCE: return "Secondary-header presence";
        case CCSDS_VALIDATION_SECONDARY_HEADER_DIRECTION: return "Secondary-header direction / Packet Type";
        case CCSDS_VALIDATION_SEQUENCE_FLAGS: return "Sequence flags";
        case CCSDS_VALIDATION_SEQUENCE_COUNT: return "Sequence count";
        case CCSDS_VALIDATION_PACKET_IDENTIFIER: return "Packet Identification";
        case CCSDS_VALIDATION_SEGMENTATION_CLASS: return "Segmentation class";
        case CCSDS_VALIDATION_TEMPLATE_PACKET_ERROR_CONTROL: return "Template packet error control";
        case CCSDS_VALIDATION_TEMPLATE_SECONDARY_HEADER: return "Template secondary-header contract";
        case CCSDS_VALIDATION_PUS_HEADER: return "PUS secondary header";
        case CCSDS_VALIDATION_PUS_REVISION: return "PUS revision";
        case CCSDS_VALIDATION_PUS_DIRECTION: return "PUS direction";
        case CCSDS_VALIDATION_PUS_PACKET_TYPE: return "PUS direction / Packet Type";
        case CCSDS_VALIDATION_PUS_TAILORING: return "PUS tailoring";
        case CCSDS_VALIDATION_PUS_SECONDARY_HEADER_SIZE: return "PUS secondary-header size";
        case CCSDS_VALIDATION_PUS_RESERVED_BITS: return "PUS reserved/version bits";
        case CCSDS_VALIDATION_PUS_SPARE_FIELDS: return "PUS spare fields";
        case CCSDS_VALIDATION_PUS_ACKNOWLEDGEMENT: return "PUS acknowledgement flags";
        case CCSDS_VALIDATION_PUS_SOURCE_ID: return "PUS source ID";
        case CCSDS_VALIDATION_PUS_DESTINATION_ID: return "PUS destination ID";
        case CCSDS_VALIDATION_PUS_PACKET_SUBCOUNTER: return "PUS-A packet subcounter";
        case CCSDS_VALIDATION_PUS_TIME_REFERENCE_STATUS: return "PUS-C time-reference status";
        case CCSDS_VALIDATION_PUS_TIMESTAMP: return "PUS CUC timestamp";
        default: return "Unknown validation check";
    }
}

void ccsds_sequence_validator_reset(ccsds_sequence_validator_t *validator) {
    if (validator != NULL) {
        validator->state = 0U;
    }
}

int ccsds_sequence_validator_initialized(const ccsds_sequence_validator_t *validator) {
    return validator != NULL
        && (validator->state & CCSDS_SEQUENCE_INITIALIZED_MASK) != 0U;
}

int ccsds_sequence_validator_segment_open(const ccsds_sequence_validator_t *validator) {
    return validator != NULL
        && (validator->state & CCSDS_SEQUENCE_OPEN_MASK) != 0U;
}

uint16_t ccsds_sequence_validator_expected_count(const ccsds_sequence_validator_t *validator) {
    return validator == NULL
        ? 0U
        : (uint16_t)(validator->state & CCSDS_SEQUENCE_COUNT_MASK);
}

int ccsds_sequence_flags_valid(const ccsds_sequence_validator_t *validator,
                               const uint8_t sequence_flags) {
    const int open = ccsds_sequence_validator_segment_open(validator);
    switch (sequence_flags) {
        case 3U:
        case 1U:
            return !open;
        case 0U:
        case 2U:
            return open;
        default:
            return 0;
    }
}

int ccsds_sequence_count_valid(const ccsds_sequence_validator_t *validator,
                               const uint16_t sequence_count) {
    if (validator == NULL || sequence_count > CCSDS_SEQUENCE_COUNT_MAX) {
        return 0;
    }
    return !ccsds_sequence_validator_initialized(validator)
        || sequence_count == ccsds_sequence_validator_expected_count(validator);
}

ccsds_status_t ccsds_sequence_validator_accept(ccsds_sequence_validator_t *validator,
                                               const ccsds_primary_header_t *header) {
    uint16_t state;
    uint16_t next;

    if (validator == NULL || header == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (ccsds_primary_header_validate(header) != CCSDS_STATUS_OK) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }

    next = (uint16_t)((header->sequence_count + 1U) & CCSDS_SEQUENCE_COUNT_MASK);
    state = (uint16_t)(CCSDS_SEQUENCE_INITIALIZED_MASK | next);
    if (header->sequence_flags == 1U || header->sequence_flags == 0U) {
        state = (uint16_t)(state | CCSDS_SEQUENCE_OPEN_MASK);
    }
    validator->state = state;
    return CCSDS_STATUS_OK;
}


static ccsds_status_t set_check(ccsds_validation_report_t *report,
                                const ccsds_validation_code_t code,
                                const int passed) {
    return ccsds_validation_report_set(report, code, passed);
}

static int direction_valid(const uint8_t direction) {
    return direction == CCSDS_PACKET_DIRECTION_TELEMETRY
        || direction == CCSDS_PACKET_DIRECTION_TELECOMMAND;
}

static uint8_t packet_type_for_direction(const uint8_t direction) {
    return direction == CCSDS_PACKET_DIRECTION_TELECOMMAND ? 1U : 0U;
}

ccsds_status_t ccsds_validate_packet_coherence(
    const ccsds_packet_coherence_input_t *input,
    const ccsds_sequence_validator_t *sequence,
    const int validate_sequence_count,
    ccsds_validation_report_t *report) {
    ccsds_status_t status;
    size_t packet_data_field_size;
    int primary_valid;
    int sequence_flags_valid;

    if (input == NULL || sequence == NULL || report == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    primary_valid =
        input->primary_header_state_valid != 0U
        && ccsds_primary_header_validate(&input->header) == CCSDS_STATUS_OK;
    status = set_check(report, CCSDS_VALIDATION_PRIMARY_HEADER, primary_valid);
    if (status != CCSDS_STATUS_OK || !primary_valid) {
        return status;
    }

    status = set_check(report, CCSDS_VALIDATION_PACKET_VERSION,
                       input->header.version_number == 0U);
    if (status != CCSDS_STATUS_OK) return status;

    packet_data_field_size =
        input->serialized_size >= CCSDS_PRIMARY_HEADER_SIZE
        ? input->serialized_size - CCSDS_PRIMARY_HEADER_SIZE
        : 0U;
    status = set_check(
        report,
        CCSDS_VALIDATION_PACKET_DATA_LENGTH,
        packet_data_field_size > 0U
        && (size_t)input->header.data_length == packet_data_field_size - 1U);
    if (status != CCSDS_STATUS_OK) return status;

    if (input->crc_checked != 0U) {
        status = set_check(report, CCSDS_VALIDATION_CRC16,
                           input->crc_valid != 0U);
        if (status != CCSDS_STATUS_OK) return status;
    }

    status = set_check(
        report,
        CCSDS_VALIDATION_SECONDARY_HEADER_PRESENCE,
        (input->header.secondary_header_flag != 0U)
          == (input->secondary_header_present != 0U));
    if (status != CCSDS_STATUS_OK) return status;

    if (input->secondary_header_present != 0U
        && input->secondary_direction != CCSDS_PACKET_DIRECTION_UNSPECIFIED) {
        status = set_check(
            report,
            CCSDS_VALIDATION_SECONDARY_HEADER_DIRECTION,
            direction_valid(input->secondary_direction)
            && input->header.type
                 == packet_type_for_direction(input->secondary_direction));
        if (status != CCSDS_STATUS_OK) return status;
    }

    sequence_flags_valid = ccsds_sequence_flags_valid(
        sequence, input->header.sequence_flags);
    status = set_check(report, CCSDS_VALIDATION_SEQUENCE_FLAGS,
                       sequence_flags_valid);
    if (status != CCSDS_STATUS_OK) return status;

    if (validate_sequence_count != 0) {
        status = set_check(
            report,
            CCSDS_VALIDATION_SEQUENCE_COUNT,
            ccsds_sequence_count_valid(sequence, input->header.sequence_count));
        if (status != CCSDS_STATUS_OK) return status;
    }

    return CCSDS_STATUS_OK;
}


ccsds_status_t ccsds_validate_pus_coherence(
    const ccsds_pus_coherence_input_t *input,
    ccsds_validation_report_t *report) {
    ccsds_status_t status;
    int revision_valid;
    int direction_is_valid;
    int size_valid;
    int reserved_valid = 0;

    if (input == NULL || report == NULL) return CCSDS_STATUS_NULL_POINTER;

    status = set_check(report, CCSDS_VALIDATION_PUS_HEADER, 1);
    if (status != CCSDS_STATUS_OK) return status;

    revision_valid = input->revision == 1U || input->revision == 2U;
    direction_is_valid = direction_valid(input->direction);

    status = set_check(report, CCSDS_VALIDATION_PUS_REVISION, revision_valid);
    if (status != CCSDS_STATUS_OK) return status;
    status = set_check(report, CCSDS_VALIDATION_PUS_DIRECTION, direction_is_valid);
    if (status != CCSDS_STATUS_OK) return status;
    status = set_check(
        report, CCSDS_VALIDATION_PUS_PACKET_TYPE,
        direction_is_valid
        && input->packet_type == packet_type_for_direction(input->direction));
    if (status != CCSDS_STATUS_OK) return status;
    status = set_check(report, CCSDS_VALIDATION_PUS_TAILORING,
                       input->tailoring_valid != 0U);
    if (status != CCSDS_STATUS_OK) return status;

    size_valid = input->serialized.data != NULL
        && input->serialized.size != 0U
        && input->serialized.size == input->expected_size;
    status = set_check(report, CCSDS_VALIDATION_PUS_SECONDARY_HEADER_SIZE,
                       size_valid);
    if (status != CCSDS_STATUS_OK) return status;

    if (size_valid) {
        if (input->revision == 1U
            && input->direction == CCSDS_PACKET_DIRECTION_TELECOMMAND) {
            reserved_valid = (input->serialized.data[0] & 0x80U) == 0U
                && ((input->serialized.data[0] >> 4U) & 0x07U) == 1U;
        } else if (input->revision == 1U
                   && input->direction == CCSDS_PACKET_DIRECTION_TELEMETRY) {
            reserved_valid = input->serialized.data[0] == 0x10U;
        } else if (input->revision == 2U) {
            reserved_valid = (input->serialized.data[0] >> 4U) == 2U;
        }
    }
    status = set_check(report, CCSDS_VALIDATION_PUS_RESERVED_BITS,
                       size_valid && reserved_valid);
    if (status != CCSDS_STATUS_OK) return status;
    status = set_check(
        report, CCSDS_VALIDATION_PUS_SPARE_FIELDS,
        size_valid && ccsds_pus_spare_is_zero(
            input->serialized.data, input->serialized.size, input->spare_octets));
    if (status != CCSDS_STATUS_OK) return status;

    if (input->direction == CCSDS_PACKET_DIRECTION_TELECOMMAND) {
        status = set_check(report, CCSDS_VALIDATION_PUS_ACKNOWLEDGEMENT,
                           input->acknowledgement_flags <= 0x0FU);
        if (status != CCSDS_STATUS_OK) return status;
        return set_check(
            report, CCSDS_VALIDATION_PUS_SOURCE_ID,
            ccsds_pus_identifier_fits(
                input->identifier_value, input->identifier_octets));
    }

    if (input->direction == CCSDS_PACKET_DIRECTION_TELEMETRY) {
        status = set_check(
            report, CCSDS_VALIDATION_PUS_DESTINATION_ID,
            ccsds_pus_identifier_fits(
                input->identifier_value, input->identifier_octets));
        if (status != CCSDS_STATUS_OK) return status;
        status = set_check(
            report, CCSDS_VALIDATION_PUS_TIMESTAMP,
            input->timestamp_present != 0U
                ? input->timestamp_valid != 0U
                : input->timestamp_zero_when_absent != 0U);
        if (status != CCSDS_STATUS_OK) return status;

        if (input->revision == 1U) {
            return set_check(
                report, CCSDS_VALIDATION_PUS_PACKET_SUBCOUNTER,
                input->packet_subcounter_present != 0U
                || input->packet_subcounter == 0U);
        }
        if (input->revision == 2U) {
            return set_check(
                report, CCSDS_VALIDATION_PUS_TIME_REFERENCE_STATUS,
                input->time_reference_status <= 0x0FU);
        }
    }

    return CCSDS_STATUS_OK;
}

ccsds_status_t ccsds_validate_template_coherence(
    const ccsds_primary_header_t *packet_header,
    const ccsds_template_coherence_input_t *template_input,
    ccsds_validation_report_t *report) {
    ccsds_status_t status;
    int template_header_valid;
    int packet_identifier_valid;
    int segmentation_class_valid = 0;

    if (packet_header == NULL || template_input == NULL || report == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }

    template_header_valid =
        template_input->primary_header_state_valid != 0U
        && ccsds_primary_header_validate(&template_input->header)
             == CCSDS_STATUS_OK;

    packet_identifier_valid =
        template_header_valid
        && template_input->header.version_number == packet_header->version_number
        && template_input->header.type == packet_header->type
        && template_input->header.secondary_header_flag
             == packet_header->secondary_header_flag
        && template_input->header.apid == packet_header->apid;
    status = set_check(report, CCSDS_VALIDATION_PACKET_IDENTIFIER,
                       packet_identifier_valid);
    if (status != CCSDS_STATUS_OK) return status;

    if (template_header_valid) {
        segmentation_class_valid =
            template_input->header.sequence_flags == 3U
            ? packet_header->sequence_flags == 3U
            : packet_header->sequence_flags != 3U;
    }
    status = set_check(report, CCSDS_VALIDATION_SEGMENTATION_CLASS,
                       segmentation_class_valid);
    if (status != CCSDS_STATUS_OK) return status;

    status = set_check(
        report,
        CCSDS_VALIDATION_TEMPLATE_PACKET_ERROR_CONTROL,
        template_input->packet_error_control_equal != 0U);
    if (status != CCSDS_STATUS_OK) return status;

    return set_check(
        report,
        CCSDS_VALIDATION_TEMPLATE_SECONDARY_HEADER,
        template_input->secondary_contract_equal != 0U);
}
