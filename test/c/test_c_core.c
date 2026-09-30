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

    {
        static const uint8_t expected_be16[] = {0xABU, 0xCDU};
        static const uint8_t expected_le16[] = {0xCDU, 0xABU};
        static const uint8_t expected_be48[] = {
            0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U
        };
        static const uint8_t expected_le48[] = {
            0x06U, 0x05U, 0x04U, 0x03U, 0x02U, 0x01U
        };
        uint8_t bytes[8] = {0U};

        if (!ccsds_byte_order_is_valid(CCSDS_BYTE_ORDER_BIG)
            || !ccsds_byte_order_is_valid(CCSDS_BYTE_ORDER_LITTLE)
            || ccsds_byte_order_is_valid((ccsds_byte_order_t)2)) {
            fprintf(stderr, "bytes: byte-order validation mismatch\n");
            failed = 1;
        }

        ccsds_store_u16(bytes, 0xABCDU, CCSDS_BYTE_ORDER_BIG);
        if (memcmp(bytes, expected_be16, sizeof(expected_be16)) != 0
            || ccsds_load_u16(bytes, CCSDS_BYTE_ORDER_BIG) != 0xABCDU) {
            fprintf(stderr, "bytes: explicit big-endian 16-bit mismatch\n");
            failed = 1;
        }

        ccsds_store_u16(bytes, 0xABCDU, CCSDS_BYTE_ORDER_LITTLE);
        if (memcmp(bytes, expected_le16, sizeof(expected_le16)) != 0
            || ccsds_load_u16(bytes, CCSDS_BYTE_ORDER_LITTLE) != 0xABCDU) {
            fprintf(stderr, "bytes: explicit little-endian 16-bit mismatch\n");
            failed = 1;
        }

        ccsds_store_uint(bytes, 6U, UINT64_C(0x010203040506),
                         CCSDS_BYTE_ORDER_BIG);
        if (memcmp(bytes, expected_be48, sizeof(expected_be48)) != 0
            || ccsds_load_uint(bytes, 6U, CCSDS_BYTE_ORDER_BIG)
               != UINT64_C(0x010203040506)) {
            fprintf(stderr, "bytes: explicit big-endian generic mismatch\n");
            failed = 1;
        }

        ccsds_store_uint(bytes, 6U, UINT64_C(0x010203040506),
                         CCSDS_BYTE_ORDER_LITTLE);
        if (memcmp(bytes, expected_le48, sizeof(expected_le48)) != 0
            || ccsds_load_uint(bytes, 6U, CCSDS_BYTE_ORDER_LITTLE)
               != UINT64_C(0x010203040506)) {
            fprintf(stderr, "bytes: explicit little-endian generic mismatch\n");
            failed = 1;
        }

        ccsds_store_be16(bytes, 0xABCDU);
        if (ccsds_load_be16(bytes) != 0xABCDU
            || memcmp(bytes, expected_be16, sizeof(expected_be16)) != 0) {
            fprintf(stderr, "bytes: BE compatibility wrapper mismatch\n");
            failed = 1;
        }

        ccsds_store_le16(bytes, 0xABCDU);
        if (ccsds_load_le16(bytes) != 0xABCDU
            || memcmp(bytes, expected_le16, sizeof(expected_le16)) != 0) {
            fprintf(stderr, "bytes: LE compatibility wrapper mismatch\n");
            failed = 1;
        }
    }

    {
        static const uint8_t explicit_expected[] = {
            0x1FU, 0x01U, 0x02U, 0x03U, 0x04U, 0xA0U, 0xB0U, 0xC0U
        };
        static const uint8_t implicit_expected[] = {0x12U, 0x34U, 0x80U};
        const ccsds_cuc_config_t explicit_config = {
            CCSDS_CUC_EPOCH_CCSDS_1958_TAI,
            CCSDS_CUC_PFIELD_EXPLICIT,
            4U,
            3U
        };
        const ccsds_cuc_config_t implicit_config = {
            CCSDS_CUC_EPOCH_AGENCY_DEFINED,
            CCSDS_CUC_PFIELD_IMPLICIT,
            2U,
            1U
        };
        const ccsds_cuc_time_t explicit_time = {
            UINT64_C(0x01020304), UINT64_C(0xA0B0C0)
        };
        const ccsds_cuc_time_t implicit_time = {
            UINT64_C(0x1234), UINT64_C(0x80)
        };
        ccsds_cuc_time_t decoded_time = {0U, 0U};
        uint8_t encoded[8] = {0U};
        size_t written = 0U;

        failed |= expect_status("cuc-explicit-encode",
            ccsds_cuc_encode(&explicit_time, &explicit_config,
                             encoded, sizeof(encoded), &written),
            CCSDS_STATUS_OK);
        if (written != sizeof(explicit_expected)
            || memcmp(encoded, explicit_expected, sizeof(explicit_expected)) != 0) {
            fprintf(stderr, "cuc-explicit-encode: wire vector mismatch\n");
            failed = 1;
        }

        failed |= expect_status("cuc-explicit-decode",
            ccsds_cuc_decode(explicit_expected, sizeof(explicit_expected),
                             &explicit_config, &decoded_time),
            CCSDS_STATUS_OK);
        if (decoded_time.coarse != explicit_time.coarse
            || decoded_time.fine != explicit_time.fine) {
            fprintf(stderr, "cuc-explicit-decode: counter mismatch\n");
            failed = 1;
        }

        memset(encoded, 0, sizeof(encoded));
        written = 0U;
        failed |= expect_status("cuc-implicit-encode",
            ccsds_cuc_encode(&implicit_time, &implicit_config,
                             encoded, sizeof(encoded), &written),
            CCSDS_STATUS_OK);
        if (written != sizeof(implicit_expected)
            || memcmp(encoded, implicit_expected, sizeof(implicit_expected)) != 0) {
            fprintf(stderr, "cuc-implicit-encode: wire vector mismatch\n");
            failed = 1;
        }

        {
            ccsds_cuc_config_t invalid = explicit_config;
            invalid.coarse_octets = 0U;
            failed |= expect_status("cuc-invalid-coarse",
                ccsds_cuc_validate(&invalid),
                CCSDS_STATUS_CUC_INVALID_COARSE_WIDTH);
        }

        {
            const ccsds_cuc_config_t one_byte = {
                CCSDS_CUC_EPOCH_CCSDS_1958_TAI,
                CCSDS_CUC_PFIELD_EXPLICIT,
                1U,
                1U
            };
            const ccsds_cuc_time_t overflow = {UINT64_C(0x100), 0U};
            failed |= expect_status("cuc-coarse-overflow",
                ccsds_cuc_encode(&overflow, &one_byte,
                                 encoded, sizeof(encoded), &written),
                CCSDS_STATUS_CUC_COARSE_OVERFLOW);
            {
                const uint8_t bad_pfield[] = {0x20U, 0x01U, 0x02U};
                failed |= expect_status("cuc-pfield-mismatch",
                    ccsds_cuc_decode(bad_pfield, sizeof(bad_pfield),
                                     &one_byte, &decoded_time),
                    CCSDS_STATUS_CUC_PFIELD_MISMATCH);
            }
        }
    }

    {
        uint8_t packet[13] = {
            0x01U, 0x23U, 0xC0U, 0x2AU, 0x00U, 0x04U,
            0xAAU, 0xBBU, 0xCCU, 0x00U, 0x00U,
            0xDEU, 0xADU
        };
        ccsds_packet_view_t view;
        uint16_t packet_crc = 0U;

        failed |= expect_status("packet-view-crc-build",
            ccsds_crc16_ccitt_false(packet, 9U, &packet_crc),
            CCSDS_STATUS_OK);
        ccsds_store_be16(packet + 9U, packet_crc);

        failed |= expect_status("packet-view-crc",
            ccsds_packet_view_parse(packet, sizeof(packet),
                                    CCSDS_PACKET_ERROR_CONTROL_CRC16,
                                    NULL, &view),
            CCSDS_STATUS_OK);
        if (view.consumed != 11U
            || view.packet.data != packet
            || view.packet.size != 11U
            || view.body.data != packet + 6U
            || view.body.size != 5U
            || view.data_field.data != packet + 6U
            || view.data_field.size != 3U
            || view.packet_error_control.data != packet + 9U
            || view.packet_error_control.size != 2U
            || view.received_crc16 != packet_crc
            || view.primary_header.apid != 0x0123U
            || view.primary_header.sequence_count != 0x002AU) {
            fprintf(stderr, "packet-view-crc: zero-copy view mismatch\n");
            failed = 1;
        }

        packet[7] ^= 0x01U;
        failed |= expect_status("packet-view-crc-mismatch",
            ccsds_packet_view_parse(packet, 11U,
                                    CCSDS_PACKET_ERROR_CONTROL_CRC16,
                                    NULL, &view),
            CCSDS_STATUS_INVALID_CHECKSUM);
        packet[7] ^= 0x01U;

        failed |= expect_status("packet-view-truncated",
            ccsds_packet_view_parse(packet, 10U,
                                    CCSDS_PACKET_ERROR_CONTROL_CRC16,
                                    NULL, &view),
            CCSDS_STATUS_INVALID_DATA);

        {
            uint8_t no_pec[10] = {
                0x01U, 0x23U, 0xC0U, 0x2AU, 0x00U, 0x03U,
                0x10U, 0x20U, 0x30U, 0x40U
            };
            failed |= expect_status("packet-view-no-pec",
                ccsds_packet_view_parse(no_pec, sizeof(no_pec),
                                        CCSDS_PACKET_ERROR_CONTROL_NONE,
                                        NULL, &view),
                CCSDS_STATUS_OK);
            if (view.consumed != sizeof(no_pec)
                || view.data_field.data != no_pec + 6U
                || view.data_field.size != 4U
                || view.packet_error_control.data != NULL
                || view.packet_error_control.size != 0U) {
                fprintf(stderr, "packet-view-no-pec: view mismatch\n");
                failed = 1;
            }
        }
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
