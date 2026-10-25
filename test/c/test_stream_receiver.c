// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ccsdspack/c/ccsdspack.h"

#define CHECK(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "stream receiver check failed at line %d: %s\\n", __LINE__, #expr); \
        return 1; \
    } \
} while (0)

static const uint8_t sync_bytes[] = {0x1AU, 0xCFU, 0xFCU, 0x1DU};

static size_t make_frame(uint8_t *out, const uint16_t seq,
                         const uint8_t *payload, const size_t payload_size,
                         const int with_crc) {
    size_t body_size = payload_size + (with_crc ? 2U : 0U);
    uint16_t crc = 0U;
    memcpy(out, sync_bytes, 4U);
    out[4] = 0x00U;
    out[5] = 0x2AU;
    out[6] = (uint8_t)(0xC0U | ((seq >> 8U) & 0x3FU));
    out[7] = (uint8_t)seq;
    out[8] = (uint8_t)((body_size - 1U) >> 8U);
    out[9] = (uint8_t)(body_size - 1U);
    memcpy(out + 10U, payload, payload_size);
    if (with_crc) {
        if (ccsds_crc16_ccitt_false(out + 4U, 6U + payload_size, &crc)
            != CCSDS_STATUS_OK) return 0U;
        ccsds_store_be16(out + 10U + payload_size, crc);
    }
    return 10U + body_size;
}

static int test_partial_and_multiple(void) {
    uint8_t storage[128] = {0U};
    uint8_t wire[100] = {0U};
    const uint8_t a[] = {0xAAU, 0xBBU};
    const uint8_t b[] = {0xCCU};
    const uint8_t noise[] = {0xEEU, 0xDAU, 0x22U};
    size_t size, frame1, used, consumed;
    ccsds_stream_receiver_t rx;
    ccsds_packet_view_t view;
    int ready = -1;

    CHECK(ccsds_stream_receiver_init(&rx, storage, sizeof(storage),
          0x1ACFFC1DU, CCSDS_PACKET_ERROR_CONTROL_NONE, NULL) == CCSDS_STATUS_OK);
    memcpy(wire, noise, sizeof(noise));
    frame1 = make_frame(wire + sizeof(noise), 1U, a, sizeof(a), 0);
    size = sizeof(noise) + frame1;
    size += make_frame(wire + size, 2U, b, sizeof(b), 0);

    CHECK(ccsds_stream_receiver_feed(&rx, wire, 5U, &consumed, &view, &ready)
          == CCSDS_STATUS_OK);
    CHECK(!ready && consumed == 5U);
    CHECK(ccsds_stream_receiver_feed(&rx, wire + 5U, size - 5U,
                                     &used, &view, &ready) == CCSDS_STATUS_OK);
    CHECK(ready && used == sizeof(noise) + frame1 - 5U);
    CHECK(view.primary_header.sequence_count == 1U);
    CHECK(view.data_field.size == sizeof(a)
          && memcmp(view.data_field.data, a, sizeof(a)) == 0);
    CHECK(view.packet.data == storage + 4U);
    CHECK(ccsds_stream_receiver_feed(&rx, wire + 5U + used,
                                     size - 5U - used, &consumed, &view, &ready)
          == CCSDS_STATUS_OK);
    CHECK(ready && consumed == size - 5U - used);
    CHECK(view.primary_header.sequence_count == 2U
          && view.data_field.size == sizeof(b));
    CHECK(rx.accepted_packets == 2U && rx.dropped_bytes >= sizeof(noise));
    CHECK(ccsds_stream_receiver_feed(&rx, NULL, 0U, &consumed, &view, &ready)
          == CCSDS_STATUS_OK && !ready && consumed == 0U);
    return 0;
}

static int test_all_chunk_boundaries(void) {
    uint8_t frame[40] = {0U};
    const uint8_t payload[] = {0x42U, 0x43U, 0x44U};
    const size_t size = make_frame(frame, 9U, payload, sizeof(payload), 0);
    size_t split;
    for (split = 1U; split < size; ++split) {
        uint8_t storage[64] = {0U};
        ccsds_stream_receiver_t rx;
        ccsds_packet_view_t view;
        size_t consumed = 0U;
        int ready = 0;
        CHECK(ccsds_stream_receiver_init(&rx, storage, sizeof(storage),
              0x1ACFFC1DU, CCSDS_PACKET_ERROR_CONTROL_NONE, NULL) == CCSDS_STATUS_OK);
        CHECK(ccsds_stream_receiver_feed(&rx, frame, split, &consumed, &view, &ready)
              == CCSDS_STATUS_OK && consumed == split && !ready);
        CHECK(ccsds_stream_receiver_feed(&rx, frame + split, size - split,
                                        &consumed, &view, &ready) == CCSDS_STATUS_OK);
        CHECK(ready && consumed == size - split
              && view.primary_header.sequence_count == 9U);
    }
    return 0;
}

static int test_payload_marker_not_preempted(void) {
    uint8_t storage[128] = {0U};
    uint8_t frame[96] = {0U};
    /* Entire marker + plausible valid short packet embedded in application data. */
    const uint8_t payload[] = {
        0x1AU, 0xCFU, 0xFCU, 0x1DU,
        0x00U, 0x2AU, 0xC0U, 0x63U, 0x00U, 0x00U, 0xBEU,
        0xFAU
    };
    size_t consumed = 0U;
    int ready = 0;
    ccsds_stream_receiver_t rx;
    ccsds_packet_view_t view;
    const size_t size = make_frame(frame, 17U, payload, sizeof(payload), 0);
    CHECK(ccsds_stream_receiver_init(&rx, storage, sizeof(storage),
          0x1ACFFC1DU, CCSDS_PACKET_ERROR_CONTROL_NONE, NULL) == CCSDS_STATUS_OK);
    CHECK(ccsds_stream_receiver_feed(&rx, frame, size, &consumed, &view, &ready)
          == CCSDS_STATUS_OK);
    CHECK(ready && consumed == size && view.primary_header.sequence_count == 17U
          && view.data_field.size == sizeof(payload)
          && memcmp(view.data_field.data, payload, sizeof(payload)) == 0
          && rx.rejected_packets == 0U);
    return 0;
}

static int test_reject_and_recover(void) {
    uint8_t storage[80] = {0U};
    uint8_t wire[120] = {0U};
    const uint8_t payload[] = {0xA5U, 0x5AU};
    ccsds_stream_receiver_t rx;
    ccsds_packet_view_t view;
    size_t consumed = 0U, size = 0U;
    int ready = 0;
    CHECK(ccsds_stream_receiver_init(&rx, storage, sizeof(storage),
          0x1ACFFC1DU, CCSDS_PACKET_ERROR_CONTROL_CRC16, NULL) == CCSDS_STATUS_OK);
    size = make_frame(wire, 1U, payload, sizeof(payload), 1);
    CHECK(size != 0U);
    wire[size - 1U] ^= 0x80U; /* Force a CRC failure. */
    size += make_frame(wire + size, 2U, payload, sizeof(payload), 1);
    CHECK(ccsds_stream_receiver_feed(&rx, wire, size, &consumed, &view, &ready)
          == CCSDS_STATUS_OK);
    CHECK(ready && consumed == size && rx.rejected_packets == 1U
          && view.primary_header.sequence_count == 2U);

    ccsds_stream_receiver_reset(&rx);
    /* Oversized declared packet is rejected at the header boundary. */
    memcpy(wire, sync_bytes, 4U);
    wire[4] = 0U; wire[5] = 0x2AU; wire[6] = 0xC0U;
    wire[7] = 3U; wire[8] = 0xFFU; wire[9] = 0xFFU;
    size = 10U + make_frame(wire + 10U, 4U, payload, sizeof(payload), 1);
    CHECK(ccsds_stream_receiver_feed(&rx, wire, size, &consumed, &view, &ready)
          == CCSDS_STATUS_OK);
    CHECK(ready && consumed == size && rx.rejected_headers == 1U
          && view.primary_header.sequence_count == 4U);
    return 0;
}

static int test_timeout_resync(void) {
    uint8_t storage[96] = {0U};
    uint8_t wire[96] = {0U};
    const uint8_t payload[] = {0x56U, 0x78U};
    ccsds_stream_receiver_t rx;
    ccsds_packet_view_t view;
    size_t consumed = 0U, size;
    int ready = 0;
    CHECK(ccsds_stream_receiver_init(&rx, storage, sizeof(storage),
          0x1ACFFC1DU, CCSDS_PACKET_ERROR_CONTROL_NONE, NULL) == CCSDS_STATUS_OK);
    memcpy(wire, sync_bytes, 4U);
    /* Incomplete candidate claims 30 data-field bytes. */
    wire[4] = 0U; wire[5] = 0x2AU; wire[6] = 0xC0U;
    wire[7] = 1U; wire[8] = 0U; wire[9] = 29U;
    wire[10] = 0xB1U; wire[11] = 0xB2U;
    size = 12U + make_frame(wire + 12U, 33U, payload, sizeof(payload), 0);
    CHECK(ccsds_stream_receiver_feed(&rx, wire, size, &consumed, &view, &ready)
          == CCSDS_STATUS_OK && consumed == size && !ready);
    CHECK(ccsds_stream_receiver_resync(&rx) == CCSDS_STATUS_OK);
    CHECK(ccsds_stream_receiver_feed(&rx, NULL, 0U, &consumed, &view, &ready)
          == CCSDS_STATUS_OK && consumed == 0U && ready);
    CHECK(view.primary_header.sequence_count == 33U && rx.rejected_packets == 1U);
    return 0;
}

static int test_errors_and_minimum_buffer(void) {
    uint8_t storage[11] = {0U};
    uint8_t frame[20] = {0U};
    const uint8_t payload[] = {0x01U};
    ccsds_stream_receiver_t rx;
    ccsds_packet_view_t view;
    size_t consumed = 777U, size = make_frame(frame, 7U, payload, 1U, 0);
    int ready = -1;
    CHECK(size == sizeof(storage));
    CHECK(ccsds_stream_receiver_init(&rx, storage, 10U, 0x1ACFFC1DU,
          CCSDS_PACKET_ERROR_CONTROL_NONE, NULL) == CCSDS_STATUS_INVALID_DATA);
    CHECK(ccsds_stream_receiver_init(&rx, storage, sizeof(storage), 0x1ACFFC1DU,
          CCSDS_PACKET_ERROR_CONTROL_NONE, NULL) == CCSDS_STATUS_OK);
    CHECK(ccsds_stream_receiver_feed(&rx, NULL, 1U, &consumed, &view, &ready)
          == CCSDS_STATUS_NULL_POINTER);
    CHECK(consumed == 777U && ready == -1);
    CHECK(ccsds_stream_receiver_feed(&rx, frame, size, &consumed, &view, &ready)
          == CCSDS_STATUS_OK && ready && consumed == size);
    CHECK(view.consumed == 7U && view.data_field.size == 1U);
    ccsds_stream_receiver_reset(&rx);
    CHECK(rx.phase == CCSDS_STREAM_RX_SEEK_SYNC && rx.accepted_packets == 1U);
    return 0;
}

int main(void) {
    if (test_partial_and_multiple() != 0
        || test_all_chunk_boundaries() != 0
        || test_payload_marker_not_preempted() != 0
        || test_reject_and_recover() != 0
        || test_timeout_resync() != 0
        || test_errors_and_minimum_buffer() != 0) return 1;
    puts("CCSDSPACK_C_STREAM_RECEIVER:PASS");
    return 0;
}
