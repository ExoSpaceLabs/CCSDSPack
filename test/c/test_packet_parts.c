// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "ccsdspack/c/ccsdspack.h"

#define REQUIRE(condition) do { if (!(condition)) { \
  fprintf(stderr, "packet parts test failed at line %d: %s\n", __LINE__, #condition); \
  return 1; \
} } while (0)

static int test_crc_parts_and_contiguous(void) {
    uint8_t header[6] = {0x01U, 0x23U, 0xC0U, 0x2AU, 0x00U, 0x04U};
    uint8_t body[5] = {0xAAU, 0xBBU, 0xCCU, 0U, 0U};
    uint8_t contiguous[13] = {0U};
    ccsds_packet_parts_view_t parts;
    ccsds_packet_view_t complete;
    ccsds_packet_parse_reason_t reason = CCSDS_PACKET_PARSE_HEADER_INVALID;
    uint16_t crc = CCSDS_CRC16_CCITT_FALSE_INITIAL;

    REQUIRE(ccsds_crc16_update(&crc, header, sizeof(header),
                               CCSDS_CRC16_CCITT_FALSE_POLYNOMIAL) == CCSDS_STATUS_OK);
    REQUIRE(ccsds_crc16_update(&crc, body, 3U,
                               CCSDS_CRC16_CCITT_FALSE_POLYNOMIAL) == CCSDS_STATUS_OK);
    ccsds_store_be16(body + 3U, crc);
    memcpy(contiguous, header, sizeof(header));
    memcpy(contiguous + sizeof(header), body, sizeof(body));
    contiguous[11U] = 0xDEU;
    contiguous[12U] = 0xADU;

    REQUIRE(ccsds_packet_parts_parse(
        header, sizeof(header), body, sizeof(body),
        CCSDS_PACKET_ERROR_CONTROL_CRC16, NULL,
        &parts, &reason) == CCSDS_STATUS_OK);
    REQUIRE(reason == CCSDS_PACKET_PARSE_OK);
    REQUIRE(parts.header_bytes.data == header
        && parts.body.data == body
        && parts.data_field.data == body
        && parts.data_field.size == 3U
        && parts.packet_error_control.data == body + 3U
        && parts.packet_error_control.size == 2U
        && parts.received_crc16 == crc
        && parts.consumed == 11U);

    REQUIRE(ccsds_packet_view_parse_ex(
        contiguous, sizeof(contiguous), CCSDS_PACKET_ERROR_CONTROL_CRC16,
        NULL, &complete, &reason) == CCSDS_STATUS_OK);
    REQUIRE(complete.packet.data == contiguous
        && complete.consumed == 11U
        && complete.data_field.size == parts.data_field.size
        && complete.received_crc16 == parts.received_crc16
        && complete.primary_header.sequence_count == parts.primary_header.sequence_count);
    /* Legacy API still delegates to the same policy, accepting stream suffix. */
    REQUIRE(ccsds_packet_view_parse(
        contiguous, sizeof(contiguous), CCSDS_PACKET_ERROR_CONTROL_CRC16,
        NULL, &complete) == CCSDS_STATUS_OK);

    /* Custom polynomial/init/final-XOR, on two physically separate spans. */
    {
        const ccsds_crc16_config_t custom = {0x8005U, 0x1234U, 0x55AAU};
        uint16_t custom_crc = custom.initial_value;
        REQUIRE(ccsds_crc16_update(&custom_crc, header, sizeof(header),
                                   custom.polynomial) == CCSDS_STATUS_OK);
        REQUIRE(ccsds_crc16_update(&custom_crc, body, 3U,
                                   custom.polynomial) == CCSDS_STATUS_OK);
        ccsds_store_be16(body + 3U, (uint16_t)(custom_crc ^ custom.final_xor_value));
        REQUIRE(ccsds_packet_parts_parse(
            header, sizeof(header), body, sizeof(body),
            CCSDS_PACKET_ERROR_CONTROL_CRC16, &custom, &parts, &reason)
                == CCSDS_STATUS_OK);
    }
    return 0;
}

static int test_failures_and_transactions(void) {
    uint8_t h[6] = {0x01U, 0x23U, 0xC0U, 0x2AU, 0x00U, 0x02U};
    uint8_t b[4] = {0xA1U, 0xB2U, 0xC3U, 0xD4U};
    ccsds_packet_parts_view_t view;
    ccsds_packet_parts_view_t unchanged;
    ccsds_packet_parse_reason_t reason = CCSDS_PACKET_PARSE_OK;
    ccsds_status_t status;
    memset(&view, 0xA5, sizeof(view));
    unchanged = view;

#define BAD(code, why, hsize, bsize, mode) do { \
  reason = CCSDS_PACKET_PARSE_OK; \
  status = ccsds_packet_parts_parse(h, hsize, b, bsize, mode, NULL, &view, &reason); \
  REQUIRE(status == code && reason == why); \
  REQUIRE(memcmp(&view, &unchanged, sizeof(view)) == 0); \
} while (0)

    BAD(CCSDS_STATUS_INVALID_HEADER_DATA, CCSDS_PACKET_PARSE_HEADER_SIZE,
        5U, 3U, CCSDS_PACKET_ERROR_CONTROL_NONE);
    BAD(CCSDS_STATUS_INVALID_DATA, CCSDS_PACKET_PARSE_BODY_SIZE_MISMATCH,
        6U, 4U, CCSDS_PACKET_ERROR_CONTROL_NONE);
    BAD(CCSDS_STATUS_INVALID_DATA, CCSDS_PACKET_PARSE_BODY_SIZE_MISMATCH,
        6U, 2U, CCSDS_PACKET_ERROR_CONTROL_NONE);
    h[0] = 0x21U; /* Version 1 */
    BAD(CCSDS_STATUS_INVALID_HEADER_DATA, CCSDS_PACKET_PARSE_VERSION,
        6U, 3U, CCSDS_PACKET_ERROR_CONTROL_NONE);
    h[0] = 0x01U;
    h[4] = 0x00U; h[5] = 0x00U; /* 1 byte body */
    BAD(CCSDS_STATUS_INVALID_DATA, CCSDS_PACKET_PARSE_CRC_TRAILER_TOO_SHORT,
        6U, 1U, CCSDS_PACKET_ERROR_CONTROL_CRC16);
    h[5] = 0x02U;
    BAD(CCSDS_STATUS_INVALID_CHECKSUM, CCSDS_PACKET_PARSE_CRC_MISMATCH,
        6U, 3U, CCSDS_PACKET_ERROR_CONTROL_CRC16);
    h[0] = 0x0FU; h[1] = 0xFFU; /* Idle APID + secondary-header flag */
    BAD(CCSDS_STATUS_INVALID_HEADER_DATA, CCSDS_PACKET_PARSE_IDLE_SECONDARY_HEADER,
        6U, 3U, CCSDS_PACKET_ERROR_CONTROL_NONE);
    h[0] = 0x07U;
    h[5] = 0x01U; /* Two CRC bytes, zero user data */
    BAD(CCSDS_STATUS_INVALID_DATA, CCSDS_PACKET_PARSE_IDLE_DATA_REQUIRED,
        6U, 2U, CCSDS_PACKET_ERROR_CONTROL_CRC16);
    h[0] = 0x01U; h[1] = 0x23U; h[5] = 0x02U;
    reason = CCSDS_PACKET_PARSE_OK;
    REQUIRE(ccsds_packet_parts_parse(
        h, sizeof(h), NULL, 3U, CCSDS_PACKET_ERROR_CONTROL_NONE,
        NULL, &view, &reason) == CCSDS_STATUS_NULL_POINTER);
    REQUIRE(reason == CCSDS_PACKET_PARSE_NULL_INPUT);
    REQUIRE(memcmp(&view, &unchanged, sizeof(view)) == 0);
    return 0;
#undef BAD
}

static int test_no_crc_and_contiguous_error_parity(void) {
    uint8_t packet[10] = {0x01U, 0x23U, 0xC0U, 0x01U, 0x00U, 0x03U,
                          0x12U, 0x34U, 0x56U, 0x78U};
    ccsds_packet_view_t contiguous;
    ccsds_packet_parts_view_t parts;
    ccsds_packet_parse_reason_t reason = CCSDS_PACKET_PARSE_OK;
    REQUIRE(ccsds_packet_parts_parse(
        packet, 6U, packet + 6U, 4U, CCSDS_PACKET_ERROR_CONTROL_NONE,
        NULL, &parts, &reason) == CCSDS_STATUS_OK);
    REQUIRE(parts.received_crc16 == 0U && parts.data_field.size == 4U);
    REQUIRE(ccsds_packet_view_parse_ex(
        packet, 9U, CCSDS_PACKET_ERROR_CONTROL_NONE,
        NULL, &contiguous, &reason) == CCSDS_STATUS_INVALID_DATA
        && reason == CCSDS_PACKET_PARSE_BODY_TRUNCATED);
    packet[0] = 0x21U;
    REQUIRE(ccsds_packet_view_parse_ex(
        packet, sizeof(packet), CCSDS_PACKET_ERROR_CONTROL_NONE,
        NULL, &contiguous, &reason) == CCSDS_STATUS_INVALID_HEADER_DATA
        && reason == CCSDS_PACKET_PARSE_VERSION);
    packet[0] = 0x01U;
    REQUIRE(ccsds_packet_view_parse_ex(
        NULL, 5U, CCSDS_PACKET_ERROR_CONTROL_NONE,
        NULL, &contiguous, &reason) == CCSDS_STATUS_INVALID_HEADER_DATA
        && reason == CCSDS_PACKET_PARSE_HEADER_SIZE);
    REQUIRE(ccsds_packet_view_parse_ex(
        NULL, sizeof(packet), CCSDS_PACKET_ERROR_CONTROL_NONE,
        NULL, &contiguous, &reason) == CCSDS_STATUS_NULL_POINTER
        && reason == CCSDS_PACKET_PARSE_NULL_INPUT);
    return 0;
}

int main(void) {
    if (test_crc_parts_and_contiguous() != 0
        || test_failures_and_transactions() != 0
        || test_no_crc_and_contiguous_error_parity() != 0) return 1;
    puts("CCSDSPACK_C_PACKET_PARTS:PASS");
    return 0;
}
