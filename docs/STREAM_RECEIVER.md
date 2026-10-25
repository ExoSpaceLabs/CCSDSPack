# Incremental C11 stream receiver (v2.2 development)

[Documentation index](README.md) | [C11 API](C_API.md) | [Framing and Packet PDU profile](CCSDS_133_0_B_2_PROFILE.md)

The new `ccsds_stream_receiver_t` accepts arbitrary-length transport chunks and
emits validated, non-owning Space Packet views from caller-provided scratch
storage. Unlike `ccsds_packet_stream_t`, which walks an existing contiguous
buffer, this receiver maintains state across UART/DMA/serial read boundaries.

## Scope and contract

- Supports CCSDSPack's optional **four-byte big-endian external sync word**
  (`0x1ACFFC1D` by convention). In this API a sync prefix is mandatory; the
  existing unframed `ccsds_packet_stream_t` remains available.
- States: `SEEK_SYNC`, `READ_HEADER`, `READ_BODY`. Searches for exact sync,
  checks the primary header's version and declared length, enforces the caller's
  maximum packet size and validates the complete packet with
  `ccsds_packet_view_parse()` (including CRC16 when selected).
- Never accepts a sync word inside a **currently valid/incomplete packet
  body** as evidence of a new frame. This avoids destroying legitimate data
  containing sync-like bytes. On rejected candidates, rescans their buffered
  suffix for overlapping/new sync markers.
- For an incomplete frame with a *plausible but wrong* length, no receiver can
  reliably tell intentional packet-body bytes from a new sync word without
  further information. The caller should enforce a framing deadline and invoke
  `ccsds_stream_receiver_resync()` if it expires. This explicitly abandons the
  candidate and rescans buffered data; **there is no built-in wall-clock**.
- Packet-level CRC is optional, like in CCSDSPack generally. Without CRC,
  corrupt payloads might look valid; sync recovery cannot guarantee they are
  detected. No PUS-specific parsing, transfer-frame, CFDP or network routing
  is implied.
- No allocation, filesystem, exceptions, locks or RTOS requirements.
  The caller provides a writable buffer of at least `4 + maximum_packet_size`
  bytes (minimum 11). The receiver rejects larger declared packets. Views
  returned by `feed()` point inside that buffer and become invalid on the
  **next** `feed()`, `resync()` or `reset()`.

## Example: fragmented UART input

```c
#include <stdint.h>
#include <ccsdspack/c/ccsdspack.h>

static uint8_t packet_storage[4U + 2048U];
static ccsds_stream_receiver_t receiver;

int setup_receiver(void) {
    return ccsds_stream_receiver_init(
        &receiver, packet_storage, sizeof(packet_storage),
        0x1ACFFC1DU, CCSDS_PACKET_ERROR_CONTROL_CRC16, NULL
    ) == CCSDS_STATUS_OK;
}

int on_uart_data(const uint8_t *chunk, size_t count) {
    size_t offset = 0U;
    while (offset < count) {
        size_t used = 0U;
        int ready = 0;
        ccsds_packet_view_t packet;
        ccsds_status_t status = ccsds_stream_receiver_feed(
            &receiver, chunk + offset, count - offset,
            &used, &packet, &ready);
        if (status != CCSDS_STATUS_OK) return -1;
        offset += used;
        if (ready) {
            /* Process/copy packet here before the next feed() call. */
            handle_packet(&packet);
        }
    }
    return 0;
}
```

`handle_packet()` is an application-owned function. It can inspect the
validated Space Packet view or pass its data field to the mission-specific
PUS codec; do not retain borrowed pointers after the next feed.

`ccsds_stream_receiver_feed()` returns at most one packet per invocation.
`consumed_out` precisely identifies the consumed prefix of the supplied
chunk; remaining input must be resubmitted. Zero-length input with NULL is
valid and can drain a complete candidate recovered by `resync()`. The output
packet is untouched unless `ready_out == 1`.

## Recovery and diagnostics

The receiver exposes counters for accepted packets, rejected headers,
rejected complete packets and discarded bytes. Counters are monotonic until
overflow (no automatic reset); `reset()` clears only buffered state.
Malformed headers exceeding configured storage are rejected **before** their
body is buffered, so an attacker cannot cause unbounded allocation.

Regression tests exercise every partial-input boundary, multiple packets in
one chunk, noise before a sync word, valid payloads containing an entire
sync-like framed packet, CRC mismatch followed by recovery, oversized header
rejection, external timeout/resync recovery and minimum-capacity behavior.

## Integration plan

This is the C11 streaming primitive for [issue #38](https://github.com/ExoSpaceLabs/CCSDSPack/issues/38).
The existing `ccsds_packet_stream_t` ABI and C++ Manager remain unchanged.
Follow-on work may wrap this receiver for DMA rings or stream transports, but
transport-dependent interrupts, buffers, clocks and deadlines belong in the
device-specific integration.
