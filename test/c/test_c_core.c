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

    {
        static const uint8_t expected_a[] = {0x1AU, 0x11U, 0x02U, 0x55U, 0x00U};
        static const uint8_t expected_c[] = {0x2FU, 0x11U, 0x01U, 0x12U, 0x34U};
        const ccsds_pus_a_tc_tailoring_t tailoring_a = {1U, 1U};
        const ccsds_pus_c_tc_tailoring_t tailoring_c = {0U};
        const ccsds_pus_tc_fields_t fields_a = {0x0AU, 17U, 2U, UINT32_C(0x55)};
        const ccsds_pus_tc_fields_t fields_c = {0x0FU, 17U, 1U, UINT32_C(0x1234)};
        ccsds_pus_tc_fields_t decoded_tc = {0U, 0U, 0U, 0U};
        uint8_t pus_bytes[8] = {0U};
        size_t pus_written = 0U;

        failed |= expect_status("pus-a-tc-encode",
            ccsds_pus_a_tc_encode(&fields_a, &tailoring_a,
                                  pus_bytes, sizeof(pus_bytes), &pus_written),
            CCSDS_STATUS_OK);
        if (pus_written != sizeof(expected_a)
            || memcmp(pus_bytes, expected_a, sizeof(expected_a)) != 0) {
            fprintf(stderr, "pus-a-tc-encode: wire vector mismatch\n");
            failed = 1;
        }

        failed |= expect_status("pus-a-tc-decode",
            ccsds_pus_a_tc_decode(expected_a, sizeof(expected_a),
                                  &tailoring_a, &decoded_tc),
            CCSDS_STATUS_OK);
        if (decoded_tc.acknowledgement_flags != fields_a.acknowledgement_flags
            || decoded_tc.service_type != fields_a.service_type
            || decoded_tc.service_subtype != fields_a.service_subtype
            || decoded_tc.source_id != fields_a.source_id) {
            fprintf(stderr, "pus-a-tc-decode: field mismatch\n");
            failed = 1;
        }

        memset(pus_bytes, 0, sizeof(pus_bytes));
        pus_written = 0U;
        failed |= expect_status("pus-c-tc-encode",
            ccsds_pus_c_tc_encode(&fields_c, &tailoring_c,
                                  pus_bytes, sizeof(pus_bytes), &pus_written),
            CCSDS_STATUS_OK);
        if (pus_written != sizeof(expected_c)
            || memcmp(pus_bytes, expected_c, sizeof(expected_c)) != 0) {
            fprintf(stderr, "pus-c-tc-encode: wire vector mismatch\n");
            failed = 1;
        }

        failed |= expect_status("pus-c-tc-decode",
            ccsds_pus_c_tc_decode(expected_c, sizeof(expected_c),
                                  &tailoring_c, &decoded_tc),
            CCSDS_STATUS_OK);
        if (decoded_tc.acknowledgement_flags != fields_c.acknowledgement_flags
            || decoded_tc.service_type != fields_c.service_type
            || decoded_tc.service_subtype != fields_c.service_subtype
            || decoded_tc.source_id != fields_c.source_id) {
            fprintf(stderr, "pus-c-tc-decode: field mismatch\n");
            failed = 1;
        }

        {
            ccsds_pus_a_tc_tailoring_t invalid_width = {3U, 0U};
            failed |= expect_status("pus-a-tc-invalid-width",
                ccsds_pus_a_tc_validate_tailoring(&invalid_width),
                CCSDS_STATUS_PUS_INVALID_IDENTIFIER_WIDTH);
        }

        {
            const ccsds_pus_a_tc_tailoring_t one_octet = {1U, 0U};
            ccsds_pus_tc_fields_t overflow = fields_a;
            overflow.source_id = UINT32_C(0x100);
            failed |= expect_status("pus-a-tc-source-overflow",
                ccsds_pus_a_tc_encode(&overflow, &one_octet,
                                      pus_bytes, sizeof(pus_bytes), &pus_written),
                CCSDS_STATUS_PUS_IDENTIFIER_OVERFLOW);
        }

        {
            uint8_t invalid_version[sizeof(expected_a)];
            memcpy(invalid_version, expected_a, sizeof(expected_a));
            invalid_version[0] = 0x2AU;
            failed |= expect_status("pus-a-tc-invalid-version",
                ccsds_pus_a_tc_decode(invalid_version, sizeof(invalid_version),
                                      &tailoring_a, &decoded_tc),
                CCSDS_STATUS_PUS_INVALID_VERSION);
        }

        {
            uint8_t nonzero_spare[sizeof(expected_a)];
            memcpy(nonzero_spare, expected_a, sizeof(expected_a));
            nonzero_spare[sizeof(nonzero_spare) - 1U] = 1U;
            failed |= expect_status("pus-a-tc-nonzero-spare",
                ccsds_pus_a_tc_decode(nonzero_spare, sizeof(nonzero_spare),
                                      &tailoring_a, &decoded_tc),
                CCSDS_STATUS_PUS_NONZERO_SPARE);
        }

        {
            ccsds_pus_tc_fields_t invalid_ack = fields_c;
            invalid_ack.acknowledgement_flags = 0x10U;
            failed |= expect_status("pus-c-tc-invalid-ack",
                ccsds_pus_c_tc_encode(&invalid_ack, &tailoring_c,
                                      pus_bytes, sizeof(pus_bytes), &pus_written),
                CCSDS_STATUS_PUS_INVALID_ACK_FLAGS);
        }
    }

    {
        static const uint8_t expected_a_tm[] = {
            0x10U, 0x03U, 0x19U, 0x44U, 0x7EU,
            0x01U, 0x02U, 0x03U, 0x04U, 0x00U
        };
        static const uint8_t expected_c_tm[] = {
            0x23U, 0x03U, 0x19U, 0x12U, 0x34U, 0xABU, 0xCDU
        };
        static const uint8_t expected_c_tm_time[] = {
            0x25U, 0x05U, 0x01U, 0x00U, 0x07U, 0x01U, 0x02U,
            0x11U, 0x22U, 0x33U, 0x44U, 0x00U
        };
        const ccsds_cuc_config_t implicit_4_0 = {
            CCSDS_CUC_EPOCH_CCSDS_1958_TAI,
            CCSDS_CUC_PFIELD_IMPLICIT,
            4U,
            0U
        };
        const ccsds_pus_a_tm_tailoring_t tailoring_a_tm = {
            1U, 1U, 1U, implicit_4_0, 1U
        };
        const ccsds_pus_c_tm_tailoring_t tailoring_c_tm = {
            0U,
            {CCSDS_CUC_EPOCH_UNSPECIFIED, CCSDS_CUC_PFIELD_IMPLICIT, 0U, 0U},
            0U
        };
        const ccsds_pus_c_tm_tailoring_t tailoring_c_tm_time = {
            1U, implicit_4_0, 1U
        };
        const ccsds_pus_a_tm_fields_t fields_a_tm = {
            3U, 25U, 0x44U, UINT32_C(0x7E),
            {UINT64_C(0x01020304), UINT64_C(0)}
        };
        const ccsds_pus_c_tm_fields_t fields_c_tm = {
            3U, 3U, 25U, 0x1234U, UINT32_C(0xABCD),
            {UINT64_C(0), UINT64_C(0)}
        };
        const ccsds_pus_c_tm_fields_t fields_c_tm_time = {
            5U, 5U, 1U, 7U, UINT32_C(0x0102),
            {UINT64_C(0x11223344), UINT64_C(0)}
        };
        ccsds_pus_a_tm_fields_t decoded_a_tm = {0};
        ccsds_pus_c_tm_fields_t decoded_c_tm = {0};
        uint8_t tm_bytes[16] = {0U};
        size_t tm_written = 0U;

        failed |= expect_status("pus-a-tm-encode",
            ccsds_pus_a_tm_encode(&fields_a_tm, &tailoring_a_tm,
                                  tm_bytes, sizeof(tm_bytes), &tm_written),
            CCSDS_STATUS_OK);
        if (tm_written != sizeof(expected_a_tm)
            || memcmp(tm_bytes, expected_a_tm, sizeof(expected_a_tm)) != 0) {
            fprintf(stderr, "pus-a-tm-encode: wire vector mismatch\n");
            failed = 1;
        }

        failed |= expect_status("pus-a-tm-decode",
            ccsds_pus_a_tm_decode(expected_a_tm, sizeof(expected_a_tm),
                                  &tailoring_a_tm, &decoded_a_tm),
            CCSDS_STATUS_OK);
        if (decoded_a_tm.service_type != fields_a_tm.service_type
            || decoded_a_tm.service_subtype != fields_a_tm.service_subtype
            || decoded_a_tm.packet_subcounter != fields_a_tm.packet_subcounter
            || decoded_a_tm.destination_id != fields_a_tm.destination_id
            || decoded_a_tm.timestamp.coarse != fields_a_tm.timestamp.coarse
            || decoded_a_tm.timestamp.fine != fields_a_tm.timestamp.fine) {
            fprintf(stderr, "pus-a-tm-decode: field mismatch\n");
            failed = 1;
        }

        memset(tm_bytes, 0, sizeof(tm_bytes));
        tm_written = 0U;
        failed |= expect_status("pus-c-tm-encode",
            ccsds_pus_c_tm_encode(&fields_c_tm, &tailoring_c_tm,
                                  tm_bytes, sizeof(tm_bytes), &tm_written),
            CCSDS_STATUS_OK);
        if (tm_written != sizeof(expected_c_tm)
            || memcmp(tm_bytes, expected_c_tm, sizeof(expected_c_tm)) != 0) {
            fprintf(stderr, "pus-c-tm-encode: wire vector mismatch\n");
            failed = 1;
        }

        failed |= expect_status("pus-c-tm-decode",
            ccsds_pus_c_tm_decode(expected_c_tm, sizeof(expected_c_tm),
                                  &tailoring_c_tm, &decoded_c_tm),
            CCSDS_STATUS_OK);
        if (decoded_c_tm.time_reference_status != fields_c_tm.time_reference_status
            || decoded_c_tm.service_type != fields_c_tm.service_type
            || decoded_c_tm.service_subtype != fields_c_tm.service_subtype
            || decoded_c_tm.message_type_counter != fields_c_tm.message_type_counter
            || decoded_c_tm.destination_id != fields_c_tm.destination_id) {
            fprintf(stderr, "pus-c-tm-decode: field mismatch\n");
            failed = 1;
        }

        memset(tm_bytes, 0, sizeof(tm_bytes));
        tm_written = 0U;
        failed |= expect_status("pus-c-tm-time-encode",
            ccsds_pus_c_tm_encode(&fields_c_tm_time, &tailoring_c_tm_time,
                                  tm_bytes, sizeof(tm_bytes), &tm_written),
            CCSDS_STATUS_OK);
        if (tm_written != sizeof(expected_c_tm_time)
            || memcmp(tm_bytes, expected_c_tm_time, sizeof(expected_c_tm_time)) != 0) {
            fprintf(stderr, "pus-c-tm-time-encode: wire vector mismatch\n");
            failed = 1;
        }

        failed |= expect_status("pus-c-tm-time-decode",
            ccsds_pus_c_tm_decode(expected_c_tm_time, sizeof(expected_c_tm_time),
                                  &tailoring_c_tm_time, &decoded_c_tm),
            CCSDS_STATUS_OK);
        if (decoded_c_tm.timestamp.coarse != fields_c_tm_time.timestamp.coarse
            || decoded_c_tm.timestamp.fine != fields_c_tm_time.timestamp.fine) {
            fprintf(stderr, "pus-c-tm-time-decode: timestamp mismatch\n");
            failed = 1;
        }

        {
            ccsds_pus_a_tm_tailoring_t invalid_width = tailoring_a_tm;
            invalid_width.destination_id_octets = 3U;
            failed |= expect_status("pus-a-tm-invalid-width",
                ccsds_pus_a_tm_validate_tailoring(&invalid_width),
                CCSDS_STATUS_PUS_INVALID_IDENTIFIER_WIDTH);
        }

        {
            ccsds_pus_c_tm_tailoring_t disabled_with_cuc = tailoring_c_tm;
            disabled_with_cuc.cuc = implicit_4_0;
            failed |= expect_status("pus-c-tm-disabled-time-config",
                ccsds_pus_c_tm_validate_tailoring(&disabled_with_cuc),
                CCSDS_STATUS_PUS_DISABLED_TIME_CONFIG);
        }

        {
            ccsds_pus_c_tm_fields_t invalid_status = fields_c_tm;
            invalid_status.time_reference_status = 0x10U;
            failed |= expect_status("pus-c-tm-invalid-time-reference",
                ccsds_pus_c_tm_encode(&invalid_status, &tailoring_c_tm,
                                      tm_bytes, sizeof(tm_bytes), &tm_written),
                CCSDS_STATUS_PUS_INVALID_TIME_REFERENCE_STATUS);
        }

        {
            ccsds_pus_a_tm_fields_t overflow = fields_a_tm;
            overflow.destination_id = UINT32_C(0x100);
            failed |= expect_status("pus-a-tm-destination-overflow",
                ccsds_pus_a_tm_encode(&overflow, &tailoring_a_tm,
                                      tm_bytes, sizeof(tm_bytes), &tm_written),
                CCSDS_STATUS_PUS_IDENTIFIER_OVERFLOW);
        }

        {
            uint8_t nonzero_spare[sizeof(expected_c_tm_time)];
            memcpy(nonzero_spare, expected_c_tm_time, sizeof(nonzero_spare));
            nonzero_spare[sizeof(nonzero_spare) - 1U] = 1U;
            failed |= expect_status("pus-c-tm-nonzero-spare",
                ccsds_pus_c_tm_decode(nonzero_spare, sizeof(nonzero_spare),
                                      &tailoring_c_tm_time, &decoded_c_tm),
                CCSDS_STATUS_PUS_NONZERO_SPARE);
        }

        {
            ccsds_pus_a_tm_fields_t time_overflow = fields_a_tm;
            time_overflow.timestamp.coarse = UINT64_C(0x100000000);
            failed |= expect_status("pus-a-tm-time-overflow",
                ccsds_pus_a_tm_encode(&time_overflow, &tailoring_a_tm,
                                      tm_bytes, sizeof(tm_bytes), &tm_written),
                CCSDS_STATUS_CUC_COARSE_OVERFLOW);
        }
    }

    {
        ccsds_validation_report_t report;
        ccsds_sequence_validator_t sequence = {0U};
        ccsds_primary_header_t sequence_header = {
            0U, 0U, 0U, 1U, 3U, 5U, 0U
        };
        ccsds_validation_code_t code;

        ccsds_validation_report_reset(&report);
        failed |= expect_status("validation-report-set-pass",
            ccsds_validation_report_set(&report,
                                        CCSDS_VALIDATION_PRIMARY_HEADER, 1),
            CCSDS_STATUS_OK);
        failed |= expect_status("validation-report-set-fail",
            ccsds_validation_report_set(&report,
                                        CCSDS_VALIDATION_PRIMARY_HEADER, 0),
            CCSDS_STATUS_OK);
        if (report.size != 1U
            || ccsds_validation_report_valid(&report)
            || !ccsds_validation_report_contains(
                 &report, CCSDS_VALIDATION_PRIMARY_HEADER)
            || ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_PRIMARY_HEADER)) {
            fprintf(stderr, "validation-report: AND/lookup semantics mismatch\n");
            failed = 1;
        }

        for (code = 0U; code < CCSDS_VALIDATION_CODE_COUNT; ++code) {
            const char *name = ccsds_validation_code_name(code);
            if (name == NULL || strcmp(name, "Unknown validation check") == 0) {
                fprintf(stderr, "validation-code-name: missing code %u\n",
                        (unsigned)code);
                failed = 1;
            }
        }

        if (!ccsds_sequence_flags_valid(&sequence, 3U)
            || !ccsds_sequence_flags_valid(&sequence, 1U)
            || ccsds_sequence_flags_valid(&sequence, 0U)
            || ccsds_sequence_flags_valid(&sequence, 2U)
            || !ccsds_sequence_count_valid(&sequence, 5U)) {
            fprintf(stderr, "sequence-initial: state mismatch\n");
            failed = 1;
        }

        failed |= expect_status("sequence-accept-unsegmented",
            ccsds_sequence_validator_accept(&sequence, &sequence_header),
            CCSDS_STATUS_OK);
        if (!ccsds_sequence_validator_initialized(&sequence)
            || ccsds_sequence_validator_segment_open(&sequence)
            || ccsds_sequence_validator_expected_count(&sequence) != 6U
            || !ccsds_sequence_count_valid(&sequence, 6U)
            || ccsds_sequence_count_valid(&sequence, 7U)) {
            fprintf(stderr, "sequence-unsegmented: state mismatch\n");
            failed = 1;
        }

        ccsds_sequence_validator_reset(&sequence);
        sequence_header.sequence_flags = 1U;
        sequence_header.sequence_count = 10U;
        failed |= expect_status("sequence-accept-first",
            ccsds_sequence_validator_accept(&sequence, &sequence_header),
            CCSDS_STATUS_OK);
        if (!ccsds_sequence_validator_segment_open(&sequence)
            || !ccsds_sequence_flags_valid(&sequence, 0U)
            || !ccsds_sequence_flags_valid(&sequence, 2U)
            || ccsds_sequence_flags_valid(&sequence, 3U)
            || !ccsds_sequence_count_valid(&sequence, 11U)) {
            fprintf(stderr, "sequence-first: state mismatch\n");
            failed = 1;
        }

        sequence_header.sequence_flags = 2U;
        sequence_header.sequence_count = 11U;
        failed |= expect_status("sequence-accept-last",
            ccsds_sequence_validator_accept(&sequence, &sequence_header),
            CCSDS_STATUS_OK);
        if (ccsds_sequence_validator_segment_open(&sequence)
            || ccsds_sequence_validator_expected_count(&sequence) != 12U) {
            fprintf(stderr, "sequence-last: state mismatch\n");
            failed = 1;
        }

        ccsds_sequence_validator_reset(&sequence);
        sequence_header.sequence_flags = 3U;
        sequence_header.sequence_count = CCSDS_SEQUENCE_COUNT_MAX;
        failed |= expect_status("sequence-rollover",
            ccsds_sequence_validator_accept(&sequence, &sequence_header),
            CCSDS_STATUS_OK);
        if (ccsds_sequence_validator_expected_count(&sequence) != 0U
            || !ccsds_sequence_count_valid(&sequence, 0U)) {
            fprintf(stderr, "sequence-rollover: expected zero\n");
            failed = 1;
        }
    }

    {
        size_t packet_count = 0U;
        ccsds_segment_plan_t plan;

        failed |= expect_status("segment-count-single",
            ccsds_segmentation_packet_count(10U, 16U, &packet_count),
            CCSDS_STATUS_OK);
        if (packet_count != 1U) {
            fprintf(stderr, "segment-count-single: expected 1\n");
            failed = 1;
        }

        failed |= expect_status("segment-plan-single",
            ccsds_segmentation_plan(10U, 16U, 0U, 7U, 1, &plan),
            CCSDS_STATUS_OK);
        if (plan.packet_count != 1U
            || plan.offset != 0U
            || plan.size != 10U
            || plan.sequence_flags != CCSDS_SEQUENCE_UNSEGMENTED
            || plan.sequence_count != 7U) {
            fprintf(stderr, "segment-plan-single: plan mismatch\n");
            failed = 1;
        }

        failed |= expect_status("segment-count-multi",
            ccsds_segmentation_packet_count(40U, 16U, &packet_count),
            CCSDS_STATUS_OK);
        if (packet_count != 3U) {
            fprintf(stderr, "segment-count-multi: expected 3\n");
            failed = 1;
        }

        failed |= expect_status("segment-plan-first",
            ccsds_segmentation_plan(40U, 16U, 0U, 0x3FFFU, 1, &plan),
            CCSDS_STATUS_OK);
        if (plan.offset != 0U || plan.size != 16U
            || plan.sequence_flags != CCSDS_SEQUENCE_FIRST
            || plan.sequence_count != 0x3FFFU) {
            fprintf(stderr, "segment-plan-first: plan mismatch\n");
            failed = 1;
        }

        failed |= expect_status("segment-plan-middle",
            ccsds_segmentation_plan(40U, 16U, 1U, 0x3FFFU, 1, &plan),
            CCSDS_STATUS_OK);
        if (plan.offset != 16U || plan.size != 16U
            || plan.sequence_flags != CCSDS_SEQUENCE_CONTINUING
            || plan.sequence_count != 0U) {
            fprintf(stderr, "segment-plan-middle: plan mismatch\n");
            failed = 1;
        }

        failed |= expect_status("segment-plan-last",
            ccsds_segmentation_plan(40U, 16U, 2U, 0x3FFFU, 1, &plan),
            CCSDS_STATUS_OK);
        if (plan.offset != 32U || plan.size != 8U
            || plan.sequence_flags != CCSDS_SEQUENCE_LAST
            || plan.sequence_count != 1U) {
            fprintf(stderr, "segment-plan-last: plan mismatch\n");
            failed = 1;
        }

        failed |= expect_status("segment-plan-manual-sequence",
            ccsds_segmentation_plan(40U, 16U, 2U, 123U, 0, &plan),
            CCSDS_STATUS_OK);
        if (plan.sequence_count != 123U
            || ccsds_sequence_after_packets(123U, 3U, 0) != 123U) {
            fprintf(stderr, "segment-plan-manual-sequence: count mismatch\n");
            failed = 1;
        }

        if (ccsds_sequence_after_packets(0x3FFFU, 3U, 1) != 2U
            || ccsds_sequence_next(0x3FFFU) != 0U) {
            fprintf(stderr, "segment-sequence-rollover: count mismatch\n");
            failed = 1;
        }

        failed |= expect_status("segment-empty",
            ccsds_segmentation_packet_count(0U, 16U, &packet_count),
            CCSDS_STATUS_NO_DATA);
        failed |= expect_status("segment-zero-capacity",
            ccsds_segmentation_packet_count(10U, 0U, &packet_count),
            CCSDS_STATUS_INVALID_APPLICATION_DATA);
        failed |= expect_status("segment-index-range",
            ccsds_segmentation_plan(40U, 16U, 3U, 0U, 1, &plan),
            CCSDS_STATUS_INVALID_DATA);
    }

    {
        static const uint8_t first_data[] = {0x10U, 0x11U};
        static const uint8_t middle_data[] = {0x20U, 0x21U};
        static const uint8_t last_data[] = {0x30U};
        static const uint8_t expected[] = {0x10U, 0x11U, 0x20U, 0x21U, 0x30U};
        ccsds_reassembly_state_t state;
        ccsds_primary_header_t segment = {
            0U, 0U, 0U, 42U, CCSDS_SEQUENCE_FIRST, 100U, 0U
        };
        uint8_t output[sizeof(expected) + 1U] = {0U};
        size_t written_now = 0U;
        int complete = 0;

        ccsds_reassembly_reset(&state, 1);
        failed |= expect_status("reassembly-first",
            ccsds_reassembly_accept(
                &state, &segment,
                (ccsds_buffer_view_t){first_data, sizeof(first_data)},
                output, sizeof(output), &written_now, &complete),
            CCSDS_STATUS_OK);
        if (written_now != sizeof(first_data) || complete != 0 || state.written != 2U) {
            fprintf(stderr, "reassembly-first: state mismatch\n");
            failed = 1;
        }

        segment.sequence_flags = CCSDS_SEQUENCE_CONTINUING;
        segment.sequence_count = 101U;
        failed |= expect_status("reassembly-continuing",
            ccsds_reassembly_accept(
                &state, &segment,
                (ccsds_buffer_view_t){middle_data, sizeof(middle_data)},
                output, sizeof(output), &written_now, &complete),
            CCSDS_STATUS_OK);

        segment.sequence_flags = CCSDS_SEQUENCE_LAST;
        segment.sequence_count = 102U;
        failed |= expect_status("reassembly-last",
            ccsds_reassembly_accept(
                &state, &segment,
                (ccsds_buffer_view_t){last_data, sizeof(last_data)},
                output, sizeof(output), &written_now, &complete),
            CCSDS_STATUS_OK);
        if (complete == 0 || state.written != sizeof(expected)
            || memcmp(output, expected, sizeof(expected)) != 0) {
            fprintf(stderr, "reassembly-last: output mismatch\n");
            failed = 1;
        }

        {
            const size_t before = state.written;
            const uint8_t before_output = output[0];
            segment.sequence_flags = CCSDS_SEQUENCE_CONTINUING;
            segment.sequence_count = 105U;
            failed |= expect_status("reassembly-invalid-sequence",
                ccsds_reassembly_accept(
                    &state, &segment,
                    (ccsds_buffer_view_t){last_data, sizeof(last_data)},
                    output, sizeof(output), &written_now, &complete),
                CCSDS_STATUS_VALIDATION_FAILURE);
            if (state.written != before || output[0] != before_output) {
                fprintf(stderr, "reassembly-invalid-sequence: state was modified\n");
                failed = 1;
            }
        }

        {
            ccsds_reassembly_state_t unchecked;
            uint8_t unchecked_output[2] = {0U};
            ccsds_reassembly_reset(&unchecked, 0);
            segment.sequence_flags = CCSDS_SEQUENCE_CONTINUING;
            segment.sequence_count = 999U;
            failed |= expect_status("reassembly-validation-disabled",
                ccsds_reassembly_accept(
                    &unchecked, &segment,
                    (ccsds_buffer_view_t){first_data, sizeof(first_data)},
                    unchecked_output, sizeof(unchecked_output),
                    &written_now, &complete),
                CCSDS_STATUS_OK);
            if (memcmp(unchecked_output, first_data, sizeof(first_data)) != 0) {
                fprintf(stderr, "reassembly-validation-disabled: copy mismatch\n");
                failed = 1;
            }
        }

        {
            ccsds_reassembly_state_t small;
            uint8_t small_output[1] = {0xEEU};
            ccsds_reassembly_reset(&small, 0);
            segment.sequence_flags = CCSDS_SEQUENCE_UNSEGMENTED;
            failed |= expect_status("reassembly-small-output",
                ccsds_reassembly_accept(
                    &small, &segment,
                    (ccsds_buffer_view_t){first_data, sizeof(first_data)},
                    small_output, sizeof(small_output),
                    &written_now, &complete),
                CCSDS_STATUS_BUFFER_TOO_SMALL);
            if (small.written != 0U || small_output[0] != 0xEEU) {
                fprintf(stderr, "reassembly-small-output: transaction mismatch\n");
                failed = 1;
            }
        }
    }

    {
        static const uint8_t stream_bytes[] = {
            0x00U, 0x01U, 0xC0U, 0x01U, 0x00U, 0x01U, 0xAAU, 0xBBU,
            0x00U, 0x01U, 0xC0U, 0x02U, 0x00U, 0x00U, 0xCCU
        };
        static const uint8_t framed_bytes[] = {
            0x1AU, 0xCFU, 0xFCU, 0x1DU,
            0x00U, 0x01U, 0xC0U, 0x01U, 0x00U, 0x01U, 0xAAU, 0xBBU,
            0x1AU, 0xCFU, 0xFCU, 0x1DU,
            0x00U, 0x01U, 0xC0U, 0x02U, 0x00U, 0x00U, 0xCCU
        };
        ccsds_packet_stream_t stream;
        ccsds_packet_view_t view;
        size_t frame_consumed = 0U;

        failed |= expect_status("stream-init",
            ccsds_packet_stream_init(
                &stream, stream_bytes, sizeof(stream_bytes),
                CCSDS_PACKET_ERROR_CONTROL_NONE, NULL,
                0, 0x1ACFFC1DU),
            CCSDS_STATUS_OK);
        if (ccsds_packet_stream_remaining(&stream) != sizeof(stream_bytes)) {
            fprintf(stderr, "stream-init: remaining mismatch\n");
            failed = 1;
        }

        failed |= expect_status("stream-next-first",
            ccsds_packet_stream_next(&stream, &view, &frame_consumed),
            CCSDS_STATUS_OK);
        if (frame_consumed != 8U || view.consumed != 8U
            || view.primary_header.sequence_count != 1U
            || view.data_field.size != 2U
            || view.data_field.data[0] != 0xAAU
            || ccsds_packet_stream_remaining(&stream) != 7U) {
            fprintf(stderr, "stream-next-first: view mismatch\n");
            failed = 1;
        }

        failed |= expect_status("stream-next-second",
            ccsds_packet_stream_next(&stream, &view, &frame_consumed),
            CCSDS_STATUS_OK);
        if (frame_consumed != 7U || view.consumed != 7U
            || view.primary_header.sequence_count != 2U
            || view.data_field.size != 1U
            || view.data_field.data[0] != 0xCCU
            || ccsds_packet_stream_remaining(&stream) != 0U) {
            fprintf(stderr, "stream-next-second: view mismatch\n");
            failed = 1;
        }
        failed |= expect_status("stream-end",
            ccsds_packet_stream_next(&stream, &view, &frame_consumed),
            CCSDS_STATUS_NO_DATA);

        failed |= expect_status("stream-sync-init",
            ccsds_packet_stream_init(
                &stream, framed_bytes, sizeof(framed_bytes),
                CCSDS_PACKET_ERROR_CONTROL_NONE, NULL,
                1, 0x1ACFFC1DU),
            CCSDS_STATUS_OK);
        failed |= expect_status("stream-sync-first",
            ccsds_packet_stream_next(&stream, &view, &frame_consumed),
            CCSDS_STATUS_OK);
        if (frame_consumed != 12U || view.packet.data != framed_bytes + 4U) {
            fprintf(stderr, "stream-sync-first: framing mismatch\n");
            failed = 1;
        }
        failed |= expect_status("stream-sync-second",
            ccsds_packet_stream_next(&stream, &view, &frame_consumed),
            CCSDS_STATUS_OK);
        if (frame_consumed != 11U || ccsds_packet_stream_remaining(&stream) != 0U) {
            fprintf(stderr, "stream-sync-second: framing mismatch\n");
            failed = 1;
        }

        {
            uint8_t bad_sync[sizeof(framed_bytes)];
            size_t before;
            memcpy(bad_sync, framed_bytes, sizeof(framed_bytes));
            bad_sync[0] ^= 0x01U;
            failed |= expect_status("stream-bad-sync-init",
                ccsds_packet_stream_init(
                    &stream, bad_sync, sizeof(bad_sync),
                    CCSDS_PACKET_ERROR_CONTROL_NONE, NULL,
                    1, 0x1ACFFC1DU),
                CCSDS_STATUS_OK);
            before = stream.offset;
            failed |= expect_status("stream-bad-sync",
                ccsds_packet_stream_next(&stream, &view, &frame_consumed),
                CCSDS_STATUS_INVALID_DATA);
            if (stream.offset != before) {
                fprintf(stderr, "stream-bad-sync: cursor advanced on failure\n");
                failed = 1;
            }
        }

        {
            const size_t truncated_size = sizeof(stream_bytes) - 1U;
            size_t before;
            failed |= expect_status("stream-truncated-init",
                ccsds_packet_stream_init(
                    &stream, stream_bytes, truncated_size,
                    CCSDS_PACKET_ERROR_CONTROL_NONE, NULL,
                    0, 0U),
                CCSDS_STATUS_OK);
            failed |= expect_status("stream-truncated-first",
                ccsds_packet_stream_next(&stream, &view, &frame_consumed),
                CCSDS_STATUS_OK);
            before = stream.offset;
            failed |= expect_status("stream-truncated-second",
                ccsds_packet_stream_next(&stream, &view, &frame_consumed),
                CCSDS_STATUS_INVALID_DATA);
            if (stream.offset != before) {
                fprintf(stderr, "stream-truncated-second: cursor advanced on failure\n");
                failed = 1;
            }
        }
    }

    {
        ccsds_sequence_validator_t sequence = {0U};
        ccsds_validation_report_t report;
        ccsds_packet_coherence_input_t input = {
            {0U, 1U, 1U, 42U, CCSDS_SEQUENCE_UNSEGMENTED, 7U, 2U},
            9U,
            1U,
            1U,
            CCSDS_PACKET_DIRECTION_TELECOMMAND,
            1U,
            1U
        };
        ccsds_template_coherence_input_t template_input = {
            {0U, 1U, 1U, 42U, CCSDS_SEQUENCE_UNSEGMENTED, 0U, 0U},
            1U,
            1U,
            1U
        };

        ccsds_sequence_validator_reset(&sequence);
        ccsds_validation_report_reset(&report);
        failed |= expect_status("coherence-valid",
            ccsds_validate_packet_coherence(&input, &sequence, 1, &report),
            CCSDS_STATUS_OK);
        if (!ccsds_validation_report_valid(&report)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_PRIMARY_HEADER)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_PACKET_VERSION)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_PACKET_DATA_LENGTH)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_CRC16)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_SECONDARY_HEADER_PRESENCE)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_SECONDARY_HEADER_DIRECTION)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_SEQUENCE_FLAGS)
            || !ccsds_validation_report_passed(
                 &report, CCSDS_VALIDATION_SEQUENCE_COUNT)) {
            fprintf(stderr, "coherence-valid: expected all generic checks to pass\n");
            failed = 1;
        }

        input.header.version_number = 1U;
        ccsds_validation_report_reset(&report);
        failed |= expect_status("coherence-version",
            ccsds_validate_packet_coherence(&input, &sequence, 1, &report),
            CCSDS_STATUS_OK);
        if (!ccsds_validation_report_failed(
                &report, CCSDS_VALIDATION_PACKET_VERSION)) {
            fprintf(stderr, "coherence-version: version check did not fail\n");
            failed = 1;
        }
        input.header.version_number = 0U;

        input.secondary_direction = CCSDS_PACKET_DIRECTION_TELEMETRY;
        ccsds_validation_report_reset(&report);
        failed |= expect_status("coherence-direction",
            ccsds_validate_packet_coherence(&input, &sequence, 1, &report),
            CCSDS_STATUS_OK);
        if (!ccsds_validation_report_failed(
                &report, CCSDS_VALIDATION_SECONDARY_HEADER_DIRECTION)) {
            fprintf(stderr, "coherence-direction: direction check did not fail\n");
            failed = 1;
        }
        input.secondary_direction = CCSDS_PACKET_DIRECTION_TELECOMMAND;

        {
            ccsds_primary_header_t accepted = input.header;
            accepted.sequence_count = 7U;
            failed |= expect_status("coherence-sequence-seed",
                ccsds_sequence_validator_accept(&sequence, &accepted),
                CCSDS_STATUS_OK);
            input.header.sequence_count = 9U;
            ccsds_validation_report_reset(&report);
            failed |= expect_status("coherence-sequence-count",
                ccsds_validate_packet_coherence(&input, &sequence, 1, &report),
                CCSDS_STATUS_OK);
            if (!ccsds_validation_report_failed(
                    &report, CCSDS_VALIDATION_SEQUENCE_COUNT)) {
                fprintf(stderr, "coherence-sequence-count: discontinuity not reported\n");
                failed = 1;
            }
            input.header.sequence_count = 8U;
        }

        ccsds_validation_report_reset(&report);
        failed |= expect_status("template-valid",
            ccsds_validate_template_coherence(
                &input.header, &template_input, &report),
            CCSDS_STATUS_OK);
        if (!ccsds_validation_report_valid(&report)) {
            fprintf(stderr, "template-valid: expected all template checks to pass\n");
            failed = 1;
        }

        template_input.header.sequence_flags = CCSDS_SEQUENCE_FIRST;
        ccsds_validation_report_reset(&report);
        failed |= expect_status("template-segmentation",
            ccsds_validate_template_coherence(
                &input.header, &template_input, &report),
            CCSDS_STATUS_OK);
        if (!ccsds_validation_report_failed(
                &report, CCSDS_VALIDATION_SEGMENTATION_CLASS)) {
            fprintf(stderr, "template-segmentation: class mismatch not reported\n");
            failed = 1;
        }

        template_input.header.sequence_flags = CCSDS_SEQUENCE_UNSEGMENTED;
        template_input.packet_error_control_equal = 0U;
        template_input.secondary_contract_equal = 0U;
        ccsds_validation_report_reset(&report);
        failed |= expect_status("template-contracts",
            ccsds_validate_template_coherence(
                &input.header, &template_input, &report),
            CCSDS_STATUS_OK);
        if (!ccsds_validation_report_failed(
                &report, CCSDS_VALIDATION_TEMPLATE_PACKET_ERROR_CONTROL)
            || !ccsds_validation_report_failed(
                 &report, CCSDS_VALIDATION_TEMPLATE_SECONDARY_HEADER)) {
            fprintf(stderr, "template-contracts: adapter equality mismatch not reported\n");
            failed = 1;
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
