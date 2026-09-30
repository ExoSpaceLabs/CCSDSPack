# CCSDSPack performance baseline

This document records repeatable performance and ownership evidence for the v2.1 C-core
transition. It is not a promise that hosted-runner wall-clock timings remain numerically
stable between machines or toolchains.

The acceptance rules are architectural:

- low-level C packet inspection performs zero heap allocations;
- returned packet/data-field/PEC spans alias the caller-owned input;
- no mandatory packet or payload byte copy is performed by the C view;
- wire and error behavior remains covered by the regression/conformance suite.

Timing measurements are evidence used to detect trends and guide migration work. They are
not hard CI thresholds.

## Harness

Build manually with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCCSDSPACK_BUILD_BENCHMARKS=ON
cmake --build build --target CCSDSPack_packet_benchmark -- -j
./build/bin/CCSDSPack_packet_benchmark
```

The harness compares two public paths:

- `c_zero_copy`: `ccsds_packet_view_parse()` over caller-owned bytes.
- `cpp_owning_raw`: `ccsds::buffer::deserializeBounded(Packet&, uint8_t*, size)`.

The C++ path is intentionally measured as an owning `Packet` parse. In v2.1 the raw-buffer
adapter is pointer-native and no longer materializes the complete input in a bridge vector.
The owning `Packet` still copies state it must retain. The C path validates framing and returns
non-owning views, so the two paths deliberately provide different ownership semantics.

Allocation accounting overrides the benchmark executable's global `operator new` /
`operator new[]` while the measured operation runs. Setup, packet construction and warmup
are outside the measured region.

`raw_adapter_bridge_copy_bytes_per_op` records only the transport-adapter bridge copy:

- C view: 0 bytes because returned spans alias the original buffer.
- v2.1 C++ raw adapter: 0 bytes because bounded parsing is pointer-native.

Owned C++ state still copies bytes it must retain; those costs are visible in allocation and
timing measurements rather than being mislabeled as an adapter bridge copy.

## Initial hosted baseline

Recorded from GitHub Actions Linux run **36698707271**, Ubuntu `ubuntu-latest`
(**Ubuntu 24.04.5 LTS**) on 2026-09-30. The uploaded performance artifact was
`ccsdspack-performance-4982985fcab923fea040ae2518d6dad3c524c011`.

### Packet error control disabled

| Packet bytes | C zero-copy ns/op | C allocs/op | C allocated bytes/op | C++ owning ns/op | C++ allocs/op | C++ allocated bytes/op | Minimum C++ input copy | C++/C time ratio |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 11.64 | 0 | 0 | 279.25 | 9 | 492 | 64 | 23.99x |
| 256 | 11.54 | 0 | 0 | 271.30 | 9 | 1,452 | 256 | 23.51x |
| 1,024 | 11.36 | 0 | 0 | 319.93 | 9 | 5,292 | 1,024 | 28.16x |
| 8,192 | 11.50 | 0 | 0 | 4,334.91 | 9 | 41,132 | 8,192 | 376.95x |
| 32,768 | 11.38 | 0 | 0 | 15,492.77 | 9 | 164,012 | 32,768 | 1,361.40x |

The C inspection time is approximately size-independent here because no packet-wide operation
is requested: the parser reads the fixed primary header, validates the declared bounds and
returns spans. The existing C++ path must materialize/own packet content, so its cost grows
with packet size.

### CRC16 enabled

| Packet bytes | C zero-copy ns/op | C allocs/op | C allocated bytes/op | C++ owning ns/op | C++ allocs/op | C++ allocated bytes/op | Minimum C++ input copy | C++/C time ratio |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 489.53 | 0 | 0 | 743.00 | 11 | 554 | 64 | 1.52x |
| 256 | 2,033.77 | 0 | 0 | 2,328.28 | 11 | 1,706 | 256 | 1.14x |
| 1,024 | 8,321.29 | 0 | 0 | 8,606.53 | 11 | 6,314 | 1,024 | 1.03x |
| 8,192 | 66,402.12 | 0 | 0 | 71,406.91 | 11 | 49,322 | 8,192 | 1.08x |
| 32,768 | 264,158.92 | 0 | 0 | 284,915.82 | 11 | 196,778 | 32,768 | 1.08x |

CRC16 necessarily scans the packet, so CRC computation dominates both paths at larger sizes.
The C implementation still preserves the architectural advantage: zero heap allocation and
no packet/payload materialization.

## How this baseline is used

For each subsequent C-core migration slice:

1. Preserve byte-for-byte wire equivalence and existing error behavior.
2. Keep low-level C codecs caller-buffer based and allocation-free.
3. Extend this harness when a migrated operation has a meaningful before/after comparison.
4. Record hosted timing trends without turning machine-noise into a release gate.
5. Record MCU text/data/bss changes for release candidates and hardware validation builds.

The next planned measurements are PUS TC/TM encode/decode after their wire codecs move into
the C core.


## Final matched v2.0 main versus v2.1 candidate

A final comparison was run with **both implementations built back-to-back on the same
Ubuntu 24.04 GitHub Actions runner**, using GCC 13.3.0, Release builds, identical benchmark
source, packet vectors, iteration counts, and allocation instrumentation.

Compared commits:

- v2.0 `main`: `4e198ae4c7f730737d78c1ea2f71ec3ce42ca7eb`
- v2.1 candidate after PR #154: `e87c292d86aaf21dad5a15480d7c52825fc648db`

Matched hosted comparison run: **36779670450**.

### C++ owning raw parse

Packet error control disabled:

| Packet bytes | v2.0 main ns/op | v2.1 ns/op | Speedup | Allocations/op main -> v2.1 | Allocated bytes/op main -> v2.1 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 257.66 | 160.70 | 1.60x | 9 -> 3 | 492 -> 242 |
| 256 | 272.94 | 169.79 | 1.61x | 9 -> 3 | 1,452 -> 434 |
| 1,024 | 326.42 | 189.53 | 1.72x | 9 -> 3 | 5,292 -> 1,202 |
| 8,192 | 15,060.76 | 3,143.02 | 4.79x | 9 -> 3 | 41,132 -> 8,370 |
| 32,768 | 70,982.53 | 23,059.30 | 3.08x | 9 -> 3 | 164,012 -> 32,946 |

CRC16 enabled:

| Packet bytes | v2.0 main ns/op | v2.1 ns/op | Speedup | Allocations/op main -> v2.1 | Allocated bytes/op main -> v2.1 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 64 | 765.17 | 643.95 | 1.19x | 11 -> 3 | 554 -> 240 |
| 256 | 2,285.69 | 2,153.79 | 1.06x | 11 -> 3 | 1,706 -> 432 |
| 1,024 | 8,318.59 | 8,145.17 | 1.02x | 11 -> 3 | 6,314 -> 1,200 |
| 8,192 | 76,273.82 | 66,979.96 | 1.14x | 11 -> 3 | 49,322 -> 8,368 |
| 32,768 | 302,727.44 | 267,361.07 | 1.13x | 11 -> 3 | 196,778 -> 32,944 |

The no-CRC owning parse is 1.6x to 4.8x faster in this matched run, with allocations reduced
from nine to three. When CRC16 is active, packet-wide checksum scanning dominates runtime,
but v2.1 still reduces allocations from eleven to three and allocated bytes by roughly
57–83% across the measured packet sizes.

### C++ packet serialization

The same run also compares the public vector-returning `Packet::serialize()` API.

For already-finalized packets, v2.1 needs one allocation for the final returned packet
buffer; v2.0 main needs three. Examples:

| Mode | Packet bytes | PEC | v2.0 main ns/op | v2.1 ns/op | Allocations/op main -> v2.1 | Allocated bytes/op main -> v2.1 |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| cached | 64 | none | 94.39 | 65.82 | 3 -> 1 | 128 -> 64 |
| cached | 1,024 | none | 104.32 | 82.77 | 3 -> 1 | 2,048 -> 1,024 |
| cached | 32,768 | none | 1,965.77 | 1,197.28 | 3 -> 1 | 65,536 -> 32,768 |
| cached | 64 | CRC16 | 97.90 | 64.91 | 3 -> 1 | 126 -> 64 |
| cached | 32,768 | CRC16 | 11,865.03 | 11,875.56 | 3 -> 1 | 65,534 -> 32,768 |

For forced re-finalization before every serialization, v2.1 removes the aggregate
DataField temporaries and keeps the public vector-returning API to one allocation:

| Packet bytes | PEC | v2.0 main ns/op | v2.1 ns/op | Allocations/op main -> v2.1 | Allocated bytes/op main -> v2.1 |
| ---: | --- | ---: | ---: | ---: | ---: |
| 64 | none | 144.75 | 103.49 | 4 -> 1 | 186 -> 64 |
| 1,024 | none | 164.10 | 130.87 | 4 -> 1 | 3,066 -> 1,024 |
| 8,192 | none | 533.65 | 368.83 | 4 -> 1 | 24,570 -> 8,192 |
| 32,768 | none | 3,077.04 | 1,247.67 | 4 -> 1 | 98,298 -> 32,768 |
| 64 | CRC16 | 660.76 | 607.04 | 6 -> 1 | 250 -> 64 |
| 32,768 | CRC16 | 279,024.05 | 266,674.66 | 6 -> 1 | 131,066 -> 32,768 |

CRC16 again dominates large packets, so the main serialization gain there is elimination of
heap churn and intermediate buffers rather than a large wall-clock ratio.

### Cortex-M7 linked footprint

The same v2.0 public MCU compile probe was compiled against both implementations with:

```text
-Os -ffunction-sections -fdata-sections
-fno-exceptions -fno-rtti
-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard
```

The probe was relocatably linked against each static library. Without section garbage
collection the retained text was 46,751 bytes for v2.0 and 55,714 bytes for v2.1.

With `--gc-sections` rooted at the probe entry point, which better approximates the final
embedded link:

| Variant | text | data | bss | total |
| --- | ---: | ---: | ---: | ---: |
| v2.0 main | 36,238 | 0 | 0 | 36,238 |
| v2.1 | 40,166 | 0 | 0 | 40,166 |

The matched probe therefore retains **3,928 additional text bytes (+10.8%)** in v2.1.
This is tracked as an explicit embedded cost of the broader C-core implementation rather
than hidden behind archive-size comparisons. Physical STM32 validation remains required
before the v2.1 release is tagged.

MCU comparison with garbage collection was recorded in run **36779957111**.

## v2.1 interpretation

The v2.1 transition meets the primary performance goals:

- pure-C packet inspection remains allocation-free and zero-copy;
- the existing C++ raw parse path is materially faster than v2.0 main;
- C++ parse allocations fall from 9/11 to 3;
- vector-returning serialization falls to one allocation and removes aggregate DataField
  temporaries;
- CRC-dominated workloads preserve or improve throughput while substantially reducing heap
  traffic.

The tradeoff measured so far is Cortex-M7 code size: the identical garbage-collected public
consumer retains about 3.9 KiB more text than v2.0. That footprint is small in absolute terms
but is retained as a release metric and should be checked against the physical STM32 build.
