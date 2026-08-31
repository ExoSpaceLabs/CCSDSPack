# Structured validation evidence matrix

## Purpose

This matrix records how the public `ccsds::ValidationCode` surface is exercised for v2.0.0. It distinguishes direct malformed fixtures, template/sequence-state failures, and presence/classification checks. A check is not required to have an artificial failing object when its semantics are intentionally positive-only.

## Matrix

| ValidationCode | Release evidence |
|---|---|
| `PrimaryHeader` | direct invalid Header state in `testGroupEvidence.cpp` |
| `PacketVersion` | direct non-zero Space Packet version in `testGroupEvidence.cpp` |
| `PacketDataLength` | incoherent encoded length in `testGroupValidator.cpp` and bounded parser negatives |
| `Crc16` | application-data corruption after finalization in `testGroupValidator.cpp`; parser CRC corruption tests |
| `SecondaryHeaderPresence` | direct flag/object mismatch in `testGroupEvidence.cpp` |
| `SecondaryHeaderDirection` | primary Packet Type corrupted against concrete PUS direction in `testGroupValidator.cpp` |
| `SequenceFlags` | continuation without open sequence and unsegmented-before-close cases in `testGroupValidator.cpp` |
| `SequenceCount` | discontinuous sequence count in `testGroupValidator.cpp` |
| `PacketIdentifier` | template APID/Packet Identification mismatch in `testGroupValidator.cpp` |
| `SegmentationClass` | segmented packet against unsegmented template in `testGroupValidator.cpp` |
| `TemplatePacketErrorControl` | template/packet PEC mismatch in `testGroupValidator.cpp` |
| `TemplateSecondaryHeader` | template/PUS tailoring mismatch in `testGroupValidator.cpp` |
| `PusHeader` | positive classification check emitted only after an installed header reports `isPusHeader()==true`; not a meaningful negative state by design |
| `PusRevision` | invalid concrete PUS revision fixture in `testGroupEvidence.cpp` |
| `PusDirection` | invalid concrete PUS direction fixture in `testGroupEvidence.cpp` |
| `PusPacketType` | invalid direction fixture in `testGroupEvidence.cpp` and primary Packet Type corruption in `testGroupValidator.cpp` |
| `PusTailoring` | invalid revision/direction fixtures and invalid PUS-A identifier-width tailoring in `testGroupEvidence.cpp` |
| `PusSecondaryHeaderSize` | invalid TC field state producing non-serializable/size-incoherent PUS header in `testGroupValidator.cpp` |
| `PusReservedBits` | malformed PUS-C first octet in `testGroupValidator.cpp`; parser reserved/version negatives |
| `PusSpareFields` | non-zero trailing spare evidence fixture in `testGroupValidator.cpp` and direct parser rejection |
| `PusAcknowledgement` | acknowledgement value wider than four bits in `testGroupValidator.cpp`; full valid `0x0..0xF` matrix in `testGroupEvidence.cpp` |
| `PusSourceId` | source identifier exceeding configured width in `testGroupValidator.cpp` |
| `PusDestinationId` | destination identifier exceeding PUS-A tailored width in `testGroupEvidence.cpp` |
| `PusPacketSubcounter` | non-zero PUS-A TM subcounter while field is disabled in `testGroupValidator.cpp` |
| `PusTimeReferenceStatus` | PUS-C TM status wider than four bits in `testGroupEvidence.cpp` |
| `PusTimestamp` | CUC timestamp exceeding configured width in `testGroupValidator.cpp` |

`testGroupEvidence.cpp` also enumerates all 26 public codes, verifies every code has a non-unknown symbolic name, and checks that `ValidationReport::Capacity` can contain the complete public set.

## Parser and Validator separation

Some malformed wire encodings are rejected before a `Packet` object can validly contain them. Those cases are covered by parser tests rather than reconstructed as impossible in-memory states solely to make Validator fail. Conversely, Validator-only template and sequence checks operate on already parsed/constructed packets and therefore do not require a malformed byte vector.

Where both layers apply, release evidence includes both direct parser rejection and the corresponding structured Validator check. The bounded fuzz/sanitizer gates add memory-safety and undefined-behavior evidence across the same parsing surfaces but do not replace the semantic fixtures in this matrix.
