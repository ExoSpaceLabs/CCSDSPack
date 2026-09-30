// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_PACKET_ENCODE_H
#define CCSDSPACK_C_PACKET_ENCODE_H

#include <stddef.h>
#include <stdint.h>
#include "buffer.h"
#include "error.h"
#include "packet_view.h"
#include "primary_header.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Finalizes one CCSDS Space Packet header and optional packet CRC.
 *
 * data_field excludes packet error-control bytes. The caller retains ownership
 * of all inputs. On success, header receives the finalized Packet Data Length
 * and sequence count, crc_out receives the packet CRC (or zero when disabled),
 * and serialized_size_out receives the complete packet size.
 *
 * On error, header and output values are left unchanged.
 */
ccsds_status_t ccsds_packet_finalize(ccsds_primary_header_t *header,
                                     uint16_t sequence_count,
                                     ccsds_buffer_view_t data_field,
                                     ccsds_packet_error_control_t error_control,
                                     const ccsds_crc16_config_t *crc_config,
                                     uint16_t *crc_out,
                                     size_t *serialized_size_out);

/**
 * @brief Encodes an already-finalized packet into caller-owned storage.
 *
 * The function validates Packet Data Length against data_field plus the selected
 * packet error-control trailer. No allocation occurs. The data-field view may
 * overlap the destination buffer, allowing safe in-place assembly.
 */
ccsds_status_t ccsds_packet_encode(const ccsds_primary_header_t *header,
                                   ccsds_buffer_view_t data_field,
                                   ccsds_packet_error_control_t error_control,
                                   uint16_t crc16,
                                   uint8_t *output,
                                   size_t capacity,
                                   size_t *written_out);

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_PACKET_ENCODE_H
