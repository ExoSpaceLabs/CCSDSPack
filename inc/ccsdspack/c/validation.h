// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_VALIDATION_H
#define CCSDSPACK_C_VALIDATION_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"
#include "primary_header.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCSDS_VALIDATION_REPORT_CAPACITY ((size_t)32U)
#define CCSDS_SEQUENCE_COUNT_MASK        ((uint16_t)0x3FFFU)
#define CCSDS_SEQUENCE_OPEN_MASK         ((uint16_t)0x4000U)
#define CCSDS_SEQUENCE_INITIALIZED_MASK  ((uint16_t)0x8000U)

typedef uint8_t ccsds_validation_code_t;

enum {
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
};

typedef struct ccsds_validation_check {
    ccsds_validation_code_t code;
    uint8_t passed;
} ccsds_validation_check_t;

/**
 * @brief Fixed-capacity, allocation-free validation report.
 *
 * Re-setting an existing code combines the new result using logical AND,
 * preserving the established v2 report behavior.
 */
typedef struct ccsds_validation_report {
    ccsds_validation_check_t checks[CCSDS_VALIDATION_REPORT_CAPACITY];
    size_t size;
} ccsds_validation_report_t;

/** @brief Stateful CCSDS sequence/segmentation validator. */
typedef struct ccsds_sequence_validator {
    uint16_t state;
} ccsds_sequence_validator_t;

enum {
    CCSDS_PACKET_DIRECTION_UNSPECIFIED = 0U,
    CCSDS_PACKET_DIRECTION_TELEMETRY = 1U,
    CCSDS_PACKET_DIRECTION_TELECOMMAND = 2U
};

/**
 * @brief Plain generic packet-coherence facts consumed by the C validator.
 *
 * primary_header_state_valid carries ownership-layer invalid state that cannot
 * be reconstructed from retained field values alone. secondary_direction uses
 * CCSDS_PACKET_DIRECTION_* constants. crc_checked controls whether the CRC16
 * validation code is included in the report.
 */
typedef struct ccsds_packet_coherence_input {
    ccsds_primary_header_t header;
    size_t serialized_size;
    uint8_t primary_header_state_valid;
    uint8_t secondary_header_present;
    uint8_t secondary_direction;
    uint8_t crc_checked;
    uint8_t crc_valid;
} ccsds_packet_coherence_input_t;

/** @brief Template-comparison facts whose object-specific equality is supplied by the adapter. */
typedef struct ccsds_template_coherence_input {
    ccsds_primary_header_t header;
    uint8_t primary_header_state_valid;
    uint8_t packet_error_control_equal;
    uint8_t secondary_contract_equal;
} ccsds_template_coherence_input_t;

void ccsds_validation_report_reset(ccsds_validation_report_t *report);
ccsds_status_t ccsds_validation_report_set(ccsds_validation_report_t *report,
                                           ccsds_validation_code_t code,
                                           int passed);
int ccsds_validation_report_valid(const ccsds_validation_report_t *report);
int ccsds_validation_report_contains(const ccsds_validation_report_t *report,
                                     ccsds_validation_code_t code);
int ccsds_validation_report_passed(const ccsds_validation_report_t *report,
                                   ccsds_validation_code_t code);
const char *ccsds_validation_code_name(ccsds_validation_code_t code);

void ccsds_sequence_validator_reset(ccsds_sequence_validator_t *validator);
int ccsds_sequence_validator_initialized(const ccsds_sequence_validator_t *validator);
int ccsds_sequence_validator_segment_open(const ccsds_sequence_validator_t *validator);
uint16_t ccsds_sequence_validator_expected_count(const ccsds_sequence_validator_t *validator);

/**
 * @brief Evaluates sequence flags against the current segmentation state.
 *
 * CCSDS sequence flag values are 00 continuing, 01 first, 10 last, 11 unsegmented.
 */
int ccsds_sequence_flags_valid(const ccsds_sequence_validator_t *validator,
                               uint8_t sequence_flags);

/** @brief Evaluates a sequence count against the current expected value. */
int ccsds_sequence_count_valid(const ccsds_sequence_validator_t *validator,
                               uint16_t sequence_count);

/**
 * @brief Accepts one validated primary header and advances sequence state.
 *
 * The next expected count wraps modulo 16384. FIRST/CONTINUING leave the
 * segmented-sequence state open; LAST/UNSEGMENTED leave it closed.
 */
ccsds_status_t ccsds_sequence_validator_accept(ccsds_sequence_validator_t *validator,
                                               const ccsds_primary_header_t *header);

/**
 * @brief Populates generic packet-coherence checks into a fixed C report.
 *
 * Sequence state is inspected but never advanced. This lets higher-level
 * validation append PUS/template checks before deciding whether to accept the
 * packet into the sequence stream.
 */
ccsds_status_t ccsds_validate_packet_coherence(
    const ccsds_packet_coherence_input_t *input,
    const ccsds_sequence_validator_t *sequence,
    int validate_sequence_count,
    ccsds_validation_report_t *report);

/** @brief Populates generic template-comparison checks into a fixed C report. */
ccsds_status_t ccsds_validate_template_coherence(
    const ccsds_primary_header_t *packet_header,
    const ccsds_template_coherence_input_t *template_input,
    ccsds_validation_report_t *report);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_VALIDATION_H
