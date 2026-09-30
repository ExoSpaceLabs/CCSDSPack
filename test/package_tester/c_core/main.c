// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <stdio.h>
#include "ccsdspack/c/ccsdspack.h"

int main(void) {
    static const uint8_t bytes[CCSDS_PRIMARY_HEADER_SIZE] = {
        0x19U, 0x23U, 0xC0U, 0x2AU, 0x00U, 0x10U
    };
    ccsds_primary_header_t header;
    size_t packet_size = 0U;

    if (ccsds_primary_header_decode(bytes, sizeof(bytes), &header) != CCSDS_STATUS_OK) {
        return 1;
    }
    if (header.apid != 0x0123U || header.sequence_count != 0x002AU) {
        return 2;
    }
    if (ccsds_packet_declared_size(bytes, sizeof(bytes), &packet_size) != CCSDS_STATUS_OK) {
        return 3;
    }
    if (packet_size != 23U) {
        return 4;
    }

    {
        static const uint8_t expected[] = {
            0x1FU, 0x01U, 0x02U, 0x03U, 0x04U, 0xA0U, 0xB0U, 0xC0U
        };
        const ccsds_cuc_config_t config = {
            CCSDS_CUC_EPOCH_CCSDS_1958_TAI,
            CCSDS_CUC_PFIELD_EXPLICIT,
            4U,
            3U
        };
        const ccsds_cuc_time_t value = {
            UINT64_C(0x01020304), UINT64_C(0xA0B0C0)
        };
        uint8_t encoded[sizeof(expected)] = {0U};
        size_t written = 0U;

        if (ccsds_cuc_encode(&value, &config, encoded, sizeof(encoded), &written)
            != CCSDS_STATUS_OK) {
            return 5;
        }
        if (written != sizeof(expected)) {
            return 6;
        }
        for (size_t index = 0U; index < sizeof(expected); ++index) {
            if (encoded[index] != expected[index]) {
                return 7;
            }
        }
    }

    puts("CCSDSPACK_INSTALLED_C_CONSUMER:PASS");
    return 0;
}
