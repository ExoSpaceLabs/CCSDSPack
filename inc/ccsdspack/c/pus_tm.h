// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_PUS_TM_H
#define CCSDSPACK_C_PUS_TM_H

#include <stddef.h>
#include <stdint.h>
#include "cuc.h"
#include "error.h"
#include "pus.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief ECSS-E-70-41A TM wire-layout tailoring. */
typedef struct ccsds_pus_a_tm_tailoring {
    uint8_t destination_id_octets;
    uint8_t packet_subcounter_present;
    uint8_t timestamp_present;
    ccsds_cuc_config_t cuc;
    uint8_t secondary_header_spare_octets;
} ccsds_pus_a_tm_tailoring_t;

/** @brief ECSS-E-ST-70-41C TM wire-layout tailoring. */
typedef struct ccsds_pus_c_tm_tailoring {
    uint8_t timestamp_present;
    ccsds_cuc_config_t cuc;
    uint8_t secondary_header_spare_octets;
} ccsds_pus_c_tm_tailoring_t;

/** @brief PUS-A TM secondary-header values. */
typedef struct ccsds_pus_a_tm_fields {
    uint8_t service_type;
    uint8_t service_subtype;
    uint8_t packet_subcounter;
    uint32_t destination_id;
    ccsds_cuc_time_t timestamp;
} ccsds_pus_a_tm_fields_t;

/** @brief PUS-C TM secondary-header values. */
typedef struct ccsds_pus_c_tm_fields {
    uint8_t time_reference_status;
    uint8_t service_type;
    uint8_t service_subtype;
    uint16_t message_type_counter;
    uint32_t destination_id;
    ccsds_cuc_time_t timestamp;
} ccsds_pus_c_tm_fields_t;

/** @brief Validates PUS-A TM tailoring. */
ccsds_status_t ccsds_pus_a_tm_validate_tailoring(
    const ccsds_pus_a_tm_tailoring_t *tailoring);

/** @brief Validates PUS-C TM tailoring. */
ccsds_status_t ccsds_pus_c_tm_validate_tailoring(
    const ccsds_pus_c_tm_tailoring_t *tailoring);

/** @brief Returns exact encoded size for valid PUS-A TM tailoring, otherwise zero. */
size_t ccsds_pus_a_tm_encoded_size(const ccsds_pus_a_tm_tailoring_t *tailoring);

/** @brief Returns exact encoded size for valid PUS-C TM tailoring, otherwise zero. */
size_t ccsds_pus_c_tm_encoded_size(const ccsds_pus_c_tm_tailoring_t *tailoring);

/** @brief Encodes one PUS-A TM secondary header into caller-owned storage. */
ccsds_status_t ccsds_pus_a_tm_encode(
    const ccsds_pus_a_tm_fields_t *fields,
    const ccsds_pus_a_tm_tailoring_t *tailoring,
    uint8_t *output,
    size_t capacity,
    size_t *written_out);

/** @brief Decodes one complete PUS-A TM secondary header. */
ccsds_status_t ccsds_pus_a_tm_decode(
    const uint8_t *data,
    size_t size,
    const ccsds_pus_a_tm_tailoring_t *tailoring,
    ccsds_pus_a_tm_fields_t *fields_out);

/** @brief Encodes one PUS-C TM secondary header into caller-owned storage. */
ccsds_status_t ccsds_pus_c_tm_encode(
    const ccsds_pus_c_tm_fields_t *fields,
    const ccsds_pus_c_tm_tailoring_t *tailoring,
    uint8_t *output,
    size_t capacity,
    size_t *written_out);

/** @brief Decodes one complete PUS-C TM secondary header. */
ccsds_status_t ccsds_pus_c_tm_decode(
    const uint8_t *data,
    size_t size,
    const ccsds_pus_c_tm_tailoring_t *tailoring,
    ccsds_pus_c_tm_fields_t *fields_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_PUS_TM_H
