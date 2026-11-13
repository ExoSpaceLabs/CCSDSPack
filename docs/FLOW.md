<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Packet processing flow

CCSDSPack exposes three cooperating levels:

- `ccsds::Packet` represents one CCSDS Space Packet PDU;
- `ccsds::Manager` generates or consumes one Packet Identification/sequence stream;
- `ccsds::Validator` performs read-only object and sequence-stream checks.

## Construct one packet

1. Create a `Packet`.
2. Assign a version-0 `PrimaryHeader`.
3. Configure packet-level error control and data-field capacity as required.
4. Install an optional secondary header and application data.
5. Call `serialize()` and check the returned `ResultBuffer`.

`Packet::serialize()` performs checked C11 finalization from the **current** mutable header, secondary-header bytes and application data when automatic update is enabled. It does not trust a cached CRC or length after public mutable references/shared secondary headers can change. `Packet::update()` explicitly finalizes the same current state even when automatic updates are disabled; if enabled, it also runs a pending legacy secondary-header `update(DataField*)` callback. Inspection getters do not finalize hidden state.

`setUpdatePacketEnable(false)` selects **manual/frozen wire mode**: normal `serialize()` does not modify Packet Data Length or CRC, but **rejects** them when they no longer match the current data. A valid parsed packet therefore round-trips without implicit changes. Call `update()` explicitly to re-finalize a modified manual packet. `serializeSnapshot() const` instead produces a **fresh, non-mutating C11 finalized wire view** of the current packet for inspection/readback without executing secondary-header callbacks. This snapshot does not modify the stored Packet CRC/length. The optional callback is stateful and remains a legacy extension hook; it is not protocol authority.

## Generate a stream

1. Construct a complete Packet template.
2. Install it with `Manager::setPacketTemplate()`.
3. Set the per-packet content capacity with `setDataFieldSize()` when segmentation is required.
4. Submit the application payload with `setApplicationData()`.
5. Retrieve adjacent packets with `getPacketsBuffer()` or persist them with `write()`.

Manager emits `UNSEGMENTED` for a single packet or `FIRST_SEGMENT`, zero or more `CONTINUING_SEGMENT`, and `LAST_SEGMENT` for a segmented payload. Automatic sequence count advances per generated packet and wraps modulo 16384.

A Manager binds one complete Packet Identification:

```text
Packet Version Number + Packet Type + Secondary Header Flag + APID
```

## Parse one packet

1. Configure the Packet-level error-control mode expected by the wire stream.
2. Supply any secondary-header schema by preinstalling a header, using a typed parser, or using a runtime selector.
3. Call `deserializeBounded()`.
4. Use the returned byte count to locate the next packet.

The parser derives the exact packet boundary from Packet Data Length, rejects truncation, validates CRC16 when enabled, parses the supplied secondary-header schema, and commits state only after all parsing checks succeed.

## Validate packet state

```cpp
ccsds::Validator validator;
const auto report = validator.validate(packet);
```

Named checks provide packet, template, PUS, and sequence diagnostics. Validation does not mutate the Packet or secondary header.

## Parse a stream with Manager

A configured Manager uses its complete Packet template to parse adjacent packets transactionally. `load()` and `read()` reject malformed framing, truncation, CRC failures, secondary-header contract violations, and mixed Packet Identification values without partially appending a failed input batch.

For explicit sequence validation, one Validator instance should be used per sequence stream. `clear()` resets it for a new stream.

## Reassemble application data

`getApplicationDataBuffer()` concatenates stored application data in packet order and returns a checked `ResultBuffer`. `getPacketsBuffer() const`, `getPacketTemplate()` and indexed packet output use callback-free finalized snapshots: reading a copied packet must not invoke a mutating callback on a secondary header shared with the stored packet. Custom-header derived fields that require a callback must be prepared explicitly before read-style snapshot generation.

The optional Manager synchronization pattern is external stream framing and is not part of the CCSDS Space Packet PDU.

See [Examples](EXAMPLES.md) and [Structured validation](VALIDATION.md).
