// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_VALIDATION_H
#define CCSDSPACK_C_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CCSDS_VALIDATION_REPORT_CAPACITY ((size_t)32U)

/**
 * @brief Stable structured validation codes shared by the C and C++ APIs.
 *
 * Numeric values intentionally match the established CCSDSPack v2
 * ccsds::ValidationCode ABI.
 */
typedef enum ccsds_validation_code {
    CCSDS_VALIDATION_PRIMARY_HEADER = 0,
    CCSDS_VALIDATION_PACKET_VERSION = 1,
    CCSDS_VALIDATION_PACKET_DATA_LENGTH = 2,
    CCSDS_VALIDATION_CRC16 = 3,
    CCSDS_VALIDATION_SECONDARY_HEADER_PRESENCE = 4,
    CCSDS_VALIDATION_SECONDARY_HEADER_DIRECTION = 5,
    CCSDS_VALIDATION_SEQUENCE_FLAGS = 6,
    CCSDS_VALIDATION_SEQUENCE_COUNT = 7,
    CCSDS_VALIDATION_PACKET_IDENTIFIER = 8,
    CCSDS_VALIDATION_SEGMENTATION_CLASS = 9,
    CCSDS_VALIDATION_TEMPLATE_PACKET_ERROR_CONTROL = 10,
    CCSDS_VALIDATION_TEMPLATE_SECONDARY_HEADER = 11,
    CCSDS_VALIDATION_PUS_HEADER = 12,
    CCSDS_VALIDATION_PUS_REVISION = 13,
    CCSDS_VALIDATION_PUS_DIRECTION = 14,
    CCSDS_VALIDATION_PUS_PACKET_TYPE = 15,
    CCSDS_VALIDATION_PUS_TAILORING = 16,
    CCSDS_VALIDATION_PUS_SECONDARY_HEADER_SIZE = 17,
    CCSDS_VALIDATION_PUS_RESERVED_BITS = 18,
    CCSDS_VALIDATION_PUS_SPARE_FIELDS = 19,
    CCSDS_VALIDATION_PUS_ACKNOWLEDGEMENT = 20,
    CCSDS_VALIDATION_PUS_SOURCE_ID = 21,
    CCSDS_VALIDATION_PUS_DESTINATION_ID = 22,
    CCSDS_VALIDATION_PUS_PACKET_SUBCOUNTER = 23,
    CCSDS_VALIDATION_PUS_TIME_REFERENCE_STATUS = 24,
    CCSDS_VALIDATION_PUS_TIMESTAMP = 25,
    CCSDS_VALIDATION_CODE_COUNT = 26
} ccsds_validation_code_t;

/** @brief One named pass/fail validation result. */
typedef struct ccsds_validation_check {
    ccsds_validation_code_t code;
    uint8_t passed;
} ccsds_validation_check_t;

/** @brief Fixed-capacity, allocation-free structured validation report. */
typedef struct ccsds_validation_report {
    ccsds_validation_check_t checks[CCSDS_VALIDATION_REPORT_CAPACITY];
    size_t size;
} ccsds_validation_report_t;

/** @brief Stable human-readable check name. */
const char *ccsds_validation_code_name(ccsds_validation_code_t code);

/** @brief Clears a report to an empty valid state. */
void ccsds_validation_report_clear(ccsds_validation_report_t *report);

/**
 * @brief Adds or combines a check.
 *
 * Repeated writes to the same code preserve failure: PASS followed by FAIL,
 * or FAIL followed by PASS, remains failed. Returns zero only for invalid
 * arguments or a full report when inserting a new code.
 */
int ccsds_validation_report_set(ccsds_validation_report_t *report,
                                ccsds_validation_code_t code,
                                int passed);

/** @brief Returns non-zero when every recorded check passed. */
int ccsds_validation_report_valid(const ccsds_validation_report_t *report);

/** @brief Returns non-zero when the report contains the requested code. */
int ccsds_validation_report_contains(const ccsds_validation_report_t *report,
                                     ccsds_validation_code_t code);

/** @brief Returns non-zero only when the requested code exists and passed. */
int ccsds_validation_report_passed(const ccsds_validation_report_t *report,
                                   ccsds_validation_code_t code);

/** @brief Returns non-zero only when the requested code exists and failed. */
int ccsds_validation_report_failed(const ccsds_validation_report_t *report,
                                   ccsds_validation_code_t code);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_VALIDATION_H
