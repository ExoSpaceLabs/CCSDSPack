// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include "ccsdspack/c/segmentation.h"

ccsds_status_t ccsds_segmentation_packet_count(const size_t total_size,
                                               const size_t max_data_size,
                                               size_t *packet_count_out) {
    if (packet_count_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (total_size == 0U) {
        return CCSDS_STATUS_NO_DATA;
    }
    if (max_data_size == 0U) {
        return CCSDS_STATUS_INVALID_APPLICATION_DATA;
    }

    *packet_count_out = 1U + ((total_size - 1U) / max_data_size);
    return CCSDS_STATUS_OK;
}

uint16_t ccsds_sequence_next(const uint16_t sequence_count) {
    return (uint16_t)((sequence_count + 1U) & CCSDS_SEQUENCE_COUNT_MAX);
}

uint16_t ccsds_sequence_after_packets(const uint16_t start_sequence_count,
                                      const size_t packet_count,
                                      const int auto_sequence) {
    if (auto_sequence == 0) {
        return start_sequence_count;
    }
    return (uint16_t)(
        ((size_t)start_sequence_count
         + (packet_count & (size_t)CCSDS_SEQUENCE_COUNT_MAX))
        & (size_t)CCSDS_SEQUENCE_COUNT_MAX);
}

ccsds_status_t ccsds_segmentation_plan(const size_t total_size,
                                       const size_t max_data_size,
                                       const size_t packet_index,
                                       const uint16_t start_sequence_count,
                                       const int auto_sequence,
                                       ccsds_segment_plan_t *plan_out) {
    size_t packet_count;
    size_t offset;
    size_t remaining;
    ccsds_status_t status;
    ccsds_segment_plan_t plan;

    if (plan_out == NULL) {
        return CCSDS_STATUS_NULL_POINTER;
    }
    if (start_sequence_count > CCSDS_SEQUENCE_COUNT_MAX) {
        return CCSDS_STATUS_INVALID_HEADER_DATA;
    }

    status = ccsds_segmentation_packet_count(
        total_size, max_data_size, &packet_count);
    if (status != CCSDS_STATUS_OK) {
        return status;
    }
    if (packet_index >= packet_count) {
        return CCSDS_STATUS_INVALID_DATA;
    }

    offset = packet_index * max_data_size;
    remaining = total_size - offset;

    plan.packet_count = packet_count;
    plan.packet_index = packet_index;
    plan.offset = offset;
    plan.size = remaining < max_data_size ? remaining : max_data_size;

    if (packet_count == 1U) {
        plan.sequence_flags = CCSDS_SEQUENCE_UNSEGMENTED;
    } else if (packet_index == 0U) {
        plan.sequence_flags = CCSDS_SEQUENCE_FIRST;
    } else if (packet_index + 1U == packet_count) {
        plan.sequence_flags = CCSDS_SEQUENCE_LAST;
    } else {
        plan.sequence_flags = CCSDS_SEQUENCE_CONTINUING;
    }

    plan.sequence_count = auto_sequence != 0
        ? ccsds_sequence_after_packets(start_sequence_count, packet_index, 1)
        : start_sequence_count;

    *plan_out = plan;
    return CCSDS_STATUS_OK;
}
