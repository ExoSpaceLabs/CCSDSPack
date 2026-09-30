// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/validation.h"

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
