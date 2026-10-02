// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

extern char __HeapBase;
extern char __HeapLimit;

void *_sbrk(ptrdiff_t increment) {
  static char *current = &__HeapBase;
  char *previous = current;

  if (increment >= 0) {
    if ((uintptr_t)current + (uintptr_t)increment > (uintptr_t)&__HeapLimit) {
      errno = ENOMEM;
      return (void *)-1;
    }
  } else {
    if ((uintptr_t)current < (uintptr_t)&__HeapBase + (uintptr_t)(-increment)) {
      errno = ENOMEM;
      return (void *)-1;
    }
  }

  current += increment;
  return previous;
}
