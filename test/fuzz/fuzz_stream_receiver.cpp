// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <ccsdspack/c/ccsdspack.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size) {
    if (size == 0U) return 0;

    std::array<std::uint8_t, 516U> storage{};
    ccsds_stream_receiver_t rx{};
    const auto mode = (data[0] & 1U) != 0U
        ? CCSDS_PACKET_ERROR_CONTROL_CRC16
        : CCSDS_PACKET_ERROR_CONTROL_NONE;
    if (ccsds_stream_receiver_init(&rx, storage.data(), storage.size(),
            0x1ACFFC1DU, mode, nullptr) != CCSDS_STATUS_OK) std::abort();

    const std::uint8_t prefix[] = {0x1AU, 0xCFU, 0xFCU, 0x1DU};
    std::size_t used = 0U;
    ccsds_packet_view_t packet{};
    int ready = 0;
    if (ccsds_stream_receiver_feed(&rx, prefix, sizeof(prefix),
            &used, &packet, &ready) != CCSDS_STATUS_OK
        || used != sizeof(prefix) || ready) std::abort();

    std::size_t pos = 1U;
    std::size_t attempts = 0U;
    while (pos < size && attempts++ < 2U * size + 10U) {
        const std::size_t available = size - pos;
        const std::size_t chunk_size = available < 17U ? available : 17U;
        used = 0U;
        ready = 0;
        if (ccsds_stream_receiver_feed(&rx, data + pos, chunk_size,
                &used, &packet, &ready) != CCSDS_STATUS_OK) std::abort();
        if (used > chunk_size) std::abort();
        pos += used;
        if (ready) {
            if (packet.packet.data != storage.data() + 4U
                || packet.consumed > storage.size() - 4U
                || packet.primary_header.version_number != 0U) std::abort();
        } else if (used == 0U) {
            std::abort();
        }
        if (pos % 37U == 0U) {
            if (ccsds_stream_receiver_resync(&rx) != CCSDS_STATUS_OK) std::abort();
        }
    }
    if (pos < size) std::abort();
    return 0;
}
