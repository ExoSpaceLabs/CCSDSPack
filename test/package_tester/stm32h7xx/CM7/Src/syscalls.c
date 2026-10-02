// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

extern char __HeapBase;
extern char __HeapLimit;

static char *g_heap_current = &__HeapBase;
static size_t g_heap_peak_bytes = 0u;

void *_sbrk(ptrdiff_t increment) {
  char *previous = g_heap_current;

  if (increment >= 0) {
    if ((uintptr_t)g_heap_current + (uintptr_t)increment > (uintptr_t)&__HeapLimit) {
      errno = ENOMEM;
      return (void *)-1;
    }
  } else {
    if ((uintptr_t)g_heap_current < (uintptr_t)&__HeapBase + (uintptr_t)(-increment)) {
      errno = ENOMEM;
      return (void *)-1;
    }
  }

  g_heap_current += increment;

  const size_t used = (size_t)(g_heap_current - &__HeapBase);
  if (used > g_heap_peak_bytes) {
    g_heap_peak_bytes = used;
  }

  return previous;
}

size_t ccsdspack_h755_heap_capacity_bytes(void) {
  return (size_t)(&__HeapLimit - &__HeapBase);
}

size_t ccsdspack_h755_heap_peak_bytes(void) {
  return g_heap_peak_bytes;
}
