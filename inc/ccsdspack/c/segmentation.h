// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_SEGMENTATION_H
#define CCSDSPACK_C_SEGMENTATION_H

#include <stddef.h>
#include <stdint.h>
#include "error.h"
#include "primary_header.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ccsds_sequence_flag_t;

enum {
    CCSDS_SEQUENCE_CONTINUING = 0U,
    CCSDS_SEQUENCE_FIRST = 1U,
    CCSDS_SEQUENCE_LAST = 2U,
    CCSDS_SEQUENCE_UNSEGMENTED = 3U
};

/**
 * @brief One allocation-free segmentation decision for a caller-owned payload.
 */
typedef struct ccsds_segment_plan {
    size_t packet_count;
    size_t packet_index;
    size_t offset;
    size_t size;
    ccsds_sequence_flag_t sequence_flags;
    uint16_t sequence_count;
} ccsds_segment_plan_t;

/** @brief Returns the number of packets required to carry total_size bytes. */
ccsds_status_t ccsds_segmentation_packet_count(size_t total_size,
                                               size_t max_data_size,
                                               size_t *packet_count_out);

/**
 * @brief Computes one packet's payload slice, flags, and sequence count.
 *
 * start_sequence_count must fit the CCSDS 14-bit sequence-count field.
 * When auto_sequence is zero, every planned packet uses start_sequence_count.
 */
ccsds_status_t ccsds_segmentation_plan(size_t total_size,
                                       size_t max_data_size,
                                       size_t packet_index,
                                       uint16_t start_sequence_count,
                                       int auto_sequence,
                                       ccsds_segment_plan_t *plan_out);

/** @brief Returns count+1 modulo the CCSDS 14-bit sequence-count range. */
uint16_t ccsds_sequence_next(uint16_t sequence_count);

/**
 * @brief Returns the sequence state after emitting packet_count packets.
 *
 * When auto_sequence is zero the start value is returned unchanged.
 */
uint16_t ccsds_sequence_after_packets(uint16_t start_sequence_count,
                                      size_t packet_count,
                                      int auto_sequence);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_SEGMENTATION_H
