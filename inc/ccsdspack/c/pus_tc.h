// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_PUS_TC_H
#define CCSDSPACK_C_PUS_TC_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Common telecommand secondary-header fields. */
typedef struct ccsds_pus_tc_fields {
    uint8_t acknowledgement_flags;
    uint8_t service_type;
    uint8_t service_subtype;
    uint32_t source_id;
} ccsds_pus_tc_fields_t;

/** @brief ECSS-E-70-41A TC wire-layout tailoring. */
typedef struct ccsds_pus_a_tc_tailoring {
    uint8_t source_id_octets;
    uint8_t secondary_header_spare_octets;
} ccsds_pus_a_tc_tailoring_t;

/** @brief ECSS-E-ST-70-41C TC wire-layout tailoring. */
typedef struct ccsds_pus_c_tc_tailoring {
    uint8_t secondary_header_spare_octets;
} ccsds_pus_c_tc_tailoring_t;

/** @brief Returns non-zero for the supported PUS-A identifier widths 0, 1, 2, or 4. */
int ccsds_pus_identifier_width_is_valid(uint8_t octets);

/** @brief Validates PUS-A TC tailoring. */
ccsds_status_t ccsds_pus_a_tc_validate_tailoring(
    const ccsds_pus_a_tc_tailoring_t *tailoring);

/** @brief Returns the exact encoded size for valid PUS-A TC tailoring, otherwise zero. */
size_t ccsds_pus_a_tc_encoded_size(const ccsds_pus_a_tc_tailoring_t *tailoring);

/** @brief Returns the exact encoded size for PUS-C TC tailoring. */
size_t ccsds_pus_c_tc_encoded_size(const ccsds_pus_c_tc_tailoring_t *tailoring);

/** @brief Encodes one PUS-A TC secondary header into caller-owned storage. */
ccsds_status_t ccsds_pus_a_tc_encode(
    const ccsds_pus_tc_fields_t *fields,
    const ccsds_pus_a_tc_tailoring_t *tailoring,
    uint8_t *output,
    size_t capacity,
    size_t *written_out);

/** @brief Decodes one complete PUS-A TC secondary header. */
ccsds_status_t ccsds_pus_a_tc_decode(
    const uint8_t *data,
    size_t size,
    const ccsds_pus_a_tc_tailoring_t *tailoring,
    ccsds_pus_tc_fields_t *fields_out);

/** @brief Encodes one PUS-C TC secondary header into caller-owned storage. */
ccsds_status_t ccsds_pus_c_tc_encode(
    const ccsds_pus_tc_fields_t *fields,
    const ccsds_pus_c_tc_tailoring_t *tailoring,
    uint8_t *output,
    size_t capacity,
    size_t *written_out);

/** @brief Decodes one complete PUS-C TC secondary header. */
ccsds_status_t ccsds_pus_c_tc_decode(
    const uint8_t *data,
    size_t size,
    const ccsds_pus_c_tc_tailoring_t *tailoring,
    ccsds_pus_tc_fields_t *fields_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_PUS_TC_H
