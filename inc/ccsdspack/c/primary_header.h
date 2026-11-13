// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_PRIMARY_HEADER_H
#define CCSDSPACK_C_PRIMARY_HEADER_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CCSDS_PRIMARY_HEADER_SIZE ((size_t)6U)
#define CCSDS_IDLE_APID           ((uint16_t)0x07FFU)
#define CCSDS_SEQUENCE_COUNT_MAX  ((uint16_t)0x3FFFU)

/**
 * @brief Plain C representation of the six-octet CCSDS Space Packet primary header.
 *
 * data_length stores the encoded CCSDS Packet Data Length value N-1.
 */
typedef struct ccsds_primary_header {
    uint8_t version_number;
    uint8_t type;
    uint8_t secondary_header_flag;
    uint16_t apid;
    uint8_t sequence_flags;
    uint16_t sequence_count;
    uint16_t data_length;
} ccsds_primary_header_t;

/** @brief Identifies the first out-of-range field without duplicating wire limits. */
typedef enum ccsds_primary_header_field {
    CCSDS_PRIMARY_HEADER_FIELD_NONE = 0,
    CCSDS_PRIMARY_HEADER_FIELD_VERSION,
    CCSDS_PRIMARY_HEADER_FIELD_TYPE,
    CCSDS_PRIMARY_HEADER_FIELD_SECONDARY_HEADER_FLAG,
    CCSDS_PRIMARY_HEADER_FIELD_APID,
    CCSDS_PRIMARY_HEADER_FIELD_SEQUENCE_FLAGS,
    CCSDS_PRIMARY_HEADER_FIELD_SEQUENCE_COUNT
} ccsds_primary_header_field_t;

/**
 * @brief Validates logical field widths and optionally reports the first
 *        offending field. No version-0 Space Packet profile policy is applied.
 * @note invalid_field_out is optional; set to FIELD_NONE for valid input
 *       and for a NULL header. No allocation or object ownership occurs.
 */
ccsds_status_t ccsds_primary_header_validate_ex(
    const ccsds_primary_header_t *header,
    ccsds_primary_header_field_t *invalid_field_out);

/** @brief Validates logical field widths without imposing version-0 policy. */
ccsds_status_t ccsds_primary_header_validate(const ccsds_primary_header_t *header);

/**
 * @brief Decodes the first six bytes from a caller-owned buffer.
 *
 * At least six bytes are required. The function does not allocate or retain input.
 */
ccsds_status_t ccsds_primary_header_decode(const uint8_t *data,
                                           size_t size,
                                           ccsds_primary_header_t *header_out);

/** @brief Encodes a validated primary header into exactly six caller-owned bytes. */
ccsds_status_t ccsds_primary_header_encode(const ccsds_primary_header_t *header,
                                           uint8_t *data_out,
                                           size_t capacity);

/** @brief Packs a validated primary header into the low 48 bits of a uint64_t. */
ccsds_status_t ccsds_primary_header_pack(const ccsds_primary_header_t *header,
                                         uint64_t *packed_out);

/** @brief Unpacks a low-48-bit primary-header representation. */
ccsds_status_t ccsds_primary_header_unpack(uint64_t packed,
                                           ccsds_primary_header_t *header_out);

/**
 * @brief Reads the version-0 primary header and returns the declared full packet size.
 *
 * The result is 6 + Packet Data Length + 1, matching CCSDS 133.0-B-2 semantics.
 */
ccsds_status_t ccsds_packet_declared_size(const uint8_t *data,
                                          size_t size,
                                          size_t *packet_size_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_PRIMARY_HEADER_H
