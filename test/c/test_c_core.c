// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <string.h>
#include "ccsdspack/c/ccsdspack.h"

static int expect_status(const char *name,
                         const ccsds_status_t actual,
                         const ccsds_status_t expected) {
    if (actual == expected) {
        return 0;
    }
    fprintf(stderr, "%s: expected %s, got %s\n",
            name, ccsds_status_name(expected), ccsds_status_name(actual));
    return 1;
}

int main(void) {
    static const uint8_t crc_vector[] = {
        (uint8_t)'1', (uint8_t)'2', (uint8_t)'3',
        (uint8_t)'4', (uint8_t)'5', (uint8_t)'6',
        (uint8_t)'7', (uint8_t)'8', (uint8_t)'9'
    };
    static const uint8_t expected_header[CCSDS_PRIMARY_HEADER_SIZE] = {
        0x19U, 0x23U, 0xC0U, 0x2AU, 0x00U, 0x10U
    };

    ccsds_primary_header_t header = {
        0U, 1U, 1U, 0x0123U, 3U, 0x002AU, 0x0010U
    };
    ccsds_primary_header_t decoded;
    uint8_t encoded[CCSDS_PRIMARY_HEADER_SIZE] = {0U};
    uint16_t crc = 0U;
    uint64_t packed = 0U;
    size_t declared_size = 0U;
    int failed = 0;

    failed |= expect_status("crc",
        ccsds_crc16_ccitt_false(crc_vector, sizeof(crc_vector), &crc),
        CCSDS_STATUS_OK);
    if (crc != 0x29B1U) {
        fprintf(stderr, "crc: expected 0x29B1, got 0x%04X\n", (unsigned)crc);
        failed = 1;
    }

    failed |= expect_status("encode",
        ccsds_primary_header_encode(&header, encoded, sizeof(encoded)),
        CCSDS_STATUS_OK);
    if (memcmp(encoded, expected_header, sizeof(encoded)) != 0) {
        fprintf(stderr, "encode: wire vector mismatch\n");
        failed = 1;
    }

    failed |= expect_status("decode",
        ccsds_primary_header_decode(encoded, sizeof(encoded), &decoded),
        CCSDS_STATUS_OK);
    if (decoded.version_number != header.version_number
        || decoded.type != header.type
        || decoded.secondary_header_flag != header.secondary_header_flag
        || decoded.apid != header.apid
        || decoded.sequence_flags != header.sequence_flags
        || decoded.sequence_count != header.sequence_count
        || decoded.data_length != header.data_length) {
        fprintf(stderr, "decode: field mismatch\n");
        failed = 1;
    }

    failed |= expect_status("pack",
        ccsds_primary_header_pack(&header, &packed),
        CCSDS_STATUS_OK);
    if (packed != UINT64_C(0x1923C02A0010)) {
        fprintf(stderr, "pack: expected 0x1923C02A0010\n");
        failed = 1;
    }

    memset(&decoded, 0, sizeof(decoded));
    failed |= expect_status("unpack",
        ccsds_primary_header_unpack(packed, &decoded),
        CCSDS_STATUS_OK);
    if (decoded.apid != 0x0123U || decoded.sequence_count != 0x002AU) {
        fprintf(stderr, "unpack: field mismatch\n");
        failed = 1;
    }

    failed |= expect_status("declared-size",
        ccsds_packet_declared_size(encoded, sizeof(encoded), &declared_size),
        CCSDS_STATUS_OK);
    if (declared_size != 23U) {
        fprintf(stderr, "declared-size: expected 23, got %zu\n", declared_size);
        failed = 1;
    }

    header.apid = 0x0800U;
    failed |= expect_status("invalid-apid",
        ccsds_primary_header_validate(&header),
        CCSDS_STATUS_INVALID_HEADER_DATA);
    header.apid = 0x0123U;

    failed |= expect_status("small-output",
        ccsds_primary_header_encode(&header, encoded, 5U),
        CCSDS_STATUS_BUFFER_TOO_SMALL);

    encoded[0] = (uint8_t)(encoded[0] | 0x20U);
    failed |= expect_status("non-v0-framing",
        ccsds_packet_declared_size(encoded, sizeof(encoded), &declared_size),
        CCSDS_STATUS_INVALID_HEADER_DATA);

    failed |= expect_status("empty-crc",
        ccsds_crc16_ccitt_false(NULL, 0U, &crc),
        CCSDS_STATUS_OK);
    if (crc != 0xFFFFU) {
        fprintf(stderr, "empty-crc: expected initial value 0xFFFF\n");
        failed = 1;
    }

    if (failed != 0) {
        return 1;
    }

    puts("CCSDSPACK_C_CORE_TEST:PASS");
    return 0;
}
