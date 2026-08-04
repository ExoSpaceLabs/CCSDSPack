<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Packet processing flow

[Documentation index](README.md) | [Examples](EXAMPLES.md) | [v1.2 profile](CCSDS_133_0_B_2_PROFILE.md)

CCSDSPack exposes two main levels:

- `CCSDS::Packet` represents exactly one CCSDS Space Packet PDU;
- `CCSDS::Manager` generates or consumes a stream whose packets share one complete Packet Identification value.

## Construct one packet

1. Create a `Packet`.
2. Assign a version-0 `PrimaryHeader`.
3. Configure the Packet Data Field capacity and packet-error-control mode.
4. Add an optional secondary header and application data.
5. Call `serialize()` and check the returned `ResultBuffer`.

`serialize()` finalizes Packet Data Length, the stored sequence count, and the optional CCSDSPack CRC16 trailer. It returns the exact validation/finalization error instead of an empty byte vector. `update()` provides the same checked finalization without producing bytes. Inspection getters do not perform hidden finalization.

## Generate a packet stream

1. Create a valid packet template.
2. Construct a `Manager` from the template or call `setPacketTemplate()`.
3. Set the per-packet data capacity with `setDataFieldSize()`.
4. Call `setApplicationData()` with the complete payload.
5. Retrieve adjacent packet bytes with `getPacketsBuffer()` and check its `ResultBuffer`, or persist them with `write()`.

The Manager assigns `UNSEGMENTED` for one generated packet or `FIRST_SEGMENT`, `CONTINUING_SEGMENT`, and `LAST_SEGMENT` for a segmented sequence. Automatic Packet Sequence Count advances once per generated packet and wraps modulo 16384.

A Manager binds one complete Packet Identification value:

```text
Packet Version Number + Packet Type + Secondary Header Flag + APID
```

Applications handling multiple identifiers use separate Manager instances or independent Packet objects.

## Parse one packet

1. Configure the receiving `Packet` with the expected packet-error-control mode.
2. Call `deserializeBounded()` on a buffer beginning with a packet.
3. Use the returned byte count to locate the next packet or preserve unrelated trailing bytes.

The parser derives the exact boundary from Packet Data Length, rejects truncation, validates the configured CRC16 trailer when enabled, and commits decoded state only after validation succeeds.

## Parse a packet stream

A configured Manager can call `load()` or `read()` to parse adjacent packets transactionally. The operation rejects malformed framing, truncated packets, CRC failures, and mixed Packet Identification values without partially appending the failed input.

The optional Manager synchronization pattern is project-specific stream framing. It is not part of the CCSDS Space Packet PDU.

## Reassemble application data

After packets are loaded, call `getApplicationDataBuffer()` and check the returned `ResultBuffer`. The Manager concatenates application data in stored packet order.

See [Examples](EXAMPLES.md) for complete source snippets.
