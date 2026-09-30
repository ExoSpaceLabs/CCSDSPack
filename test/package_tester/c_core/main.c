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

    puts("CCSDSPACK_INSTALLED_C_CONSUMER:PASS");
    return 0;
}
