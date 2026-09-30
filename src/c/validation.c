// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/validation.h"

static int code_is_valid(const ccsds_validation_code_t code) {
    return (int)code >= (int)CCSDS_VALIDATION_PRIMARY_HEADER
        && (int)code < (int)CCSDS_VALIDATION_CODE_COUNT;
}

const char *ccsds_validation_code_name(const ccsds_validation_code_t code) {
    switch (code) {
        case CCSDS_VALIDATION_PRIMARY_HEADER: return "CCSDS primary header";
        case CCSDS_VALIDATION_PACKET_VERSION: return "CCSDS packet version";
        case CCSDS_VALIDATION_PACKET_DATA_LENGTH: return "Packet Data Length";
        case CCSDS_VALIDATION_CRC16: return "CRC16";
        case CCSDS_VALIDATION_SECONDARY_HEADER_PRESENCE:
            return "Secondary-header presence";
        case CCSDS_VALIDATION_SECONDARY_HEADER_DIRECTION:
            return "Secondary-header direction / Packet Type";
        case CCSDS_VALIDATION_SEQUENCE_FLAGS: return "Sequence flags";
        case CCSDS_VALIDATION_SEQUENCE_COUNT: return "Sequence count";
        case CCSDS_VALIDATION_PACKET_IDENTIFIER: return "Packet Identification";
        case CCSDS_VALIDATION_SEGMENTATION_CLASS: return "Segmentation class";
        case CCSDS_VALIDATION_TEMPLATE_PACKET_ERROR_CONTROL:
            return "Template packet error control";
        case CCSDS_VALIDATION_TEMPLATE_SECONDARY_HEADER:
            return "Template secondary-header contract";
        case CCSDS_VALIDATION_PUS_HEADER: return "PUS secondary header";
        case CCSDS_VALIDATION_PUS_REVISION: return "PUS revision";
        case CCSDS_VALIDATION_PUS_DIRECTION: return "PUS direction";
        case CCSDS_VALIDATION_PUS_PACKET_TYPE: return "PUS direction / Packet Type";
        case CCSDS_VALIDATION_PUS_TAILORING: return "PUS tailoring";
        case CCSDS_VALIDATION_PUS_SECONDARY_HEADER_SIZE:
            return "PUS secondary-header size";
        case CCSDS_VALIDATION_PUS_RESERVED_BITS: return "PUS reserved/version bits";
        case CCSDS_VALIDATION_PUS_SPARE_FIELDS: return "PUS spare fields";
        case CCSDS_VALIDATION_PUS_ACKNOWLEDGEMENT: return "PUS acknowledgement flags";
        case CCSDS_VALIDATION_PUS_SOURCE_ID: return "PUS source ID";
        case CCSDS_VALIDATION_PUS_DESTINATION_ID: return "PUS destination ID";
        case CCSDS_VALIDATION_PUS_PACKET_SUBCOUNTER: return "PUS-A packet subcounter";
        case CCSDS_VALIDATION_PUS_TIME_REFERENCE_STATUS:
            return "PUS-C time-reference status";
        case CCSDS_VALIDATION_PUS_TIMESTAMP: return "PUS CUC timestamp";
        case CCSDS_VALIDATION_CODE_COUNT: break;
    }
    return "Unknown validation check";
}

void ccsds_validation_report_clear(ccsds_validation_report_t *report) {
    if (report == NULL) return;
    report->size = 0U;
}

int ccsds_validation_report_set(ccsds_validation_report_t *report,
                                const ccsds_validation_code_t code,
                                const int passed) {
    size_t index;

    if (report == NULL || !code_is_valid(code)) return 0;

    for (index = 0U; index < report->size; ++index) {
        if (report->checks[index].code == code) {
            report->checks[index].passed =
                (uint8_t)(report->checks[index].passed != 0U && passed != 0);
            return 1;
        }
    }

    if (report->size >= CCSDS_VALIDATION_REPORT_CAPACITY) return 0;
    report->checks[report->size].code = code;
    report->checks[report->size].passed = (uint8_t)(passed != 0);
    ++report->size;
    return 1;
}

int ccsds_validation_report_valid(const ccsds_validation_report_t *report) {
    size_t index;

    if (report == NULL) return 0;
    for (index = 0U; index < report->size; ++index) {
        if (report->checks[index].passed == 0U) return 0;
    }
    return 1;
}

int ccsds_validation_report_contains(const ccsds_validation_report_t *report,
                                     const ccsds_validation_code_t code) {
    size_t index;

    if (report == NULL || !code_is_valid(code)) return 0;
    for (index = 0U; index < report->size; ++index) {
        if (report->checks[index].code == code) return 1;
    }
    return 0;
}

int ccsds_validation_report_passed(const ccsds_validation_report_t *report,
                                   const ccsds_validation_code_t code) {
    size_t index;

    if (report == NULL || !code_is_valid(code)) return 0;
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
