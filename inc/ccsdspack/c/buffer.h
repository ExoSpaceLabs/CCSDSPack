// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#ifndef CCSDSPACK_C_BUFFER_H
#define CCSDSPACK_C_BUFFER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Non-owning immutable byte view. */
typedef struct ccsds_buffer_view {
    const uint8_t *data;
    size_t size;
} ccsds_buffer_view_t;

/** @brief Caller-owned writable byte buffer. */
typedef struct ccsds_buffer {
    uint8_t *data;
    size_t capacity;
    size_t size;
} ccsds_buffer_t;

#ifdef __cplusplus
}
#endif

#endif // CCSDSPACK_C_BUFFER_H
