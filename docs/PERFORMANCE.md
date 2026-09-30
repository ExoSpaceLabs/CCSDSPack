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

The C++ path is intentionally measured as it exists today. Its raw-buffer adapter first
materializes the complete input in a `std::vector`, and the owning parser performs further
internal allocations/copies. The C path only validates framing and returns non-owning views,
so these two paths do not provide identical ownership semantics. This comparison establishes
the cost of the existing ownership boundary and gives later wrapper migration a baseline.

Allocation accounting overrides the benchmark executable's global `operator new` /
`operator new[]` while the measured operation runs. Setup, packet construction and warmup
are outside the measured region.

`mandatory_input_copy_bytes_per_op` is deliberately conservative:

- C view: 0 bytes because returned spans alias the original buffer.
- current C++ raw adapter: at least the complete packet size because the public adapter
  constructs a vector from the raw input before parsing.

It is not an attempt to count every internal byte copy.

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
