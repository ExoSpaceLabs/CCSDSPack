<!--
Copyright 2025-2026 ExoSpaceLabs
SPDX-License-Identifier: Apache-2.0
-->

# Examples

[Documentation index](README.md) | [Configuration](CONFIG.md) | [Space Packet profile](CCSDS_133_0_B_2_PROFILE.md)

These examples use the v2 C++17 API. They create CCSDS Space Packets with Packet Version Number `000`. The default packet-error-control profile is the project-specific CRC16 trailer; configure `PacketErrorControlMode::None` explicitly for CRC-free packets.

## Build integration

After installing CCSDSPack:

```cmake
find_package(CCSDSPack CONFIG REQUIRED)

add_executable(example main.cpp)
target_link_libraries(example PRIVATE ccsdspack::CCSDSPack)
target_compile_features(example PRIVATE cxx_std_17)
```

Four complete installed-package consumers are maintained under [`example/`](../example/README.md).
Each has its own `CMakeLists.txt` and uses `find_package(CCSDSPack 2.0 CONFIG REQUIRED)`.
After installing the library, build every example or one target directory:

```bash
./example/build_examples.sh all /path/to/install/prefix
./example/build_examples.sh custom_secondary_header /path/to/install/prefix
```

## Create and parse one packet

```cpp
#include <CCSDSPack.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  ccsds::Packet packet;
  packet.setDataFieldSize(1024);

  const auto headerResult = packet.setPrimaryHeader(ccsds::PrimaryHeader{
    0,                       // Packet Version Number: must be 000
    0,                       // Packet Type: telemetry
    0,                       // no secondary header
    0x123,                   // APID
    ccsds::UNSEGMENTED,
    0,                       // Packet Sequence Count
    0                        // calculated by serialize()
  });
  if (!headerResult) {
    std::cerr << headerResult.error().message() << '\n';
    return headerResult.error().code();
  }

  const auto dataResult = packet.setApplicationData({0x10, 0x20, 0x30});
  if (!dataResult) {
    std::cerr << dataResult.error().message() << '\n';
    return dataResult.error().code();
  }

  const auto wireResult = packet.serialize();
  if (!wireResult) {
    std::cerr << wireResult.error().message() << '\n';
    return wireResult.error().code();
  }
  const auto &wire = wireResult.value();

  ccsds::Packet decoded; // CRC16 is also the receiving default
  const auto consumedResult = decoded.deserializeBounded(wire);
  if (!consumedResult) {
    std::cerr << consumedResult.error().message() << '\n';
    return consumedResult.error().code();
  }

  std::cout << "Consumed " << consumedResult.value()
            << " bytes, APID=" << decoded.getPrimaryHeader().getAPID()
            << ", application bytes=" << decoded.getApplicationDataBytes().size()
            << '\n';
  return 0;
}
```

`serialize()` returns `ResultBuffer` and reports the exact finalization error. `update()` returns `ResultBool` when finalization without bytes is required. Getters inspect the stored state and do not perform hidden finalization.

## Segment a payload with Manager

One Manager represents one complete Packet Identification value and one Packet Sequence Count stream.

```cpp
#include <CCSDSPack.h>

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
  ccsds::Packet packetTemplate;
  packetTemplate.setDataFieldSize(1024);

  const auto headerResult = packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
    0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
  });
  if (!headerResult) return headerResult.error().code();

  ccsds::Manager manager;
  const auto templateResult = manager.setPacketTemplate(packetTemplate);
  if (!templateResult) return templateResult.error().code();

  manager.setDataFieldSize(1024);

  std::vector<std::uint8_t> payload(5000, 0xAB);
  const auto dataResult = manager.setApplicationData(payload);
  if (!dataResult) {
    std::cerr << dataResult.error().message() << '\n';
    return dataResult.error().code();
  }

  const auto streamResult = manager.getPacketsBuffer();
  if (!streamResult) return streamResult.error().code();
  const auto &stream = streamResult.value();
  std::cout << "Generated " << manager.getTotalPackets()
            << " packets in " << stream.size() << " bytes\n";

  const auto writeResult = manager.write("packets.bin");
  if (!writeResult) return writeResult.error().code();
  return 0;
}
```

A one-packet result uses `UNSEGMENTED`. A multi-packet result uses `FIRST_SEGMENT`, zero or more `CONTINUING_SEGMENT` packets, and `LAST_SEGMENT`. Automatic sequence counting advances once per packet and wraps modulo 16384.

## Read a stream and reassemble application data

The receiving Manager must use the same Packet Identification and packet-error-control profile as the stream.

```cpp
#include <CCSDSPack.h>

#include <iostream>

int main() {
  ccsds::Packet packetTemplate;
  packetTemplate.setDataFieldSize(1024);

  const auto headerResult = packetTemplate.setPrimaryHeader(ccsds::PrimaryHeader{
    0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0
  });
  if (!headerResult) return headerResult.error().code();

  ccsds::Manager manager(packetTemplate);
  manager.setDataFieldSize(1024);

  const auto readResult = manager.read("packets.bin");
  if (!readResult) {
    std::cerr << readResult.error().message() << '\n';
    return readResult.error().code();
  }

  const auto payloadResult = manager.getApplicationDataBuffer();
  if (!payloadResult) return payloadResult.error().code();

  std::cout << "Recovered " << payloadResult.value().size() << " bytes\n";
  return 0;
}
```

`read()` and `load()` parse transactionally. A malformed packet, CRC failure, truncation, or mixed Packet Identification does not partially append the failed input.

## Parse concatenated packets manually

Use `deserializeBounded()` when the application owns stream iteration or must preserve trailing bytes.

```cpp
#include <CCSDSPack.h>

#include <cstddef>
#include <cstdint>
#include <vector>

ccsds::ResultBool parseStream(const std::vector<std::uint8_t>& stream) {
  std::size_t offset = 0;

  while (offset < stream.size()) {
    std::vector<std::uint8_t> remaining(stream.begin() + offset, stream.end());

    ccsds::Packet packet;
    const auto consumedResult = packet.deserializeBounded(remaining);
    if (!consumedResult) return consumedResult.error();

    offset += consumedResult.value();
    // Process packet.getPrimaryHeader() and packet.getApplicationDataBytes().
  }

  return true;
}
```

For large or zero-copy streams, wrap this policy around the application's own buffering layer. The current v2 parsing API accepts vectors.

## Create CRC-free packets

The sender and receiver must both select `None`. The mode is never inferred from packet bytes.

```cpp
ccsds::Packet sender;
sender.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
sender.setDataFieldSize(1024);
sender.setPrimaryHeader({0, 0, 0, 0x123, ccsds::UNSEGMENTED, 0, 0});
sender.setApplicationData({0x01, 0x02});
const auto wireResult = sender.serialize();
if (!wireResult) return wireResult.error().code();
const auto &wire = wireResult.value();

ccsds::Packet receiver;
receiver.setPacketErrorControlMode(ccsds::PacketErrorControlMode::None);
const auto consumed = receiver.deserializeBounded(wire);
if (!consumed) return consumed.error().code();
```

## Add an opaque secondary header

For mission-specific opaque bytes that do not use a registered typed header, use
the `BufferHeader` path through `setSecondaryHeader()`.

```cpp
ccsds::Packet packet;
packet.setDataFieldSize(1024);
packet.setPrimaryHeader({0, 0, 1, 0x123, ccsds::UNSEGMENTED, 0, 0});

const auto secondaryResult = packet.setSecondaryHeader(
  std::vector<std::uint8_t>{0xAA, 0x55}
);
if (!secondaryResult) return secondaryResult.error().code();

const auto dataResult = packet.setApplicationData({0x10, 0x20});
if (!dataResult) return dataResult.error().code();

const auto wireResult = packet.serialize();
if (!wireResult) return wireResult.error().code();
const auto &wire = wireResult.value();
```

When parsing an opaque secondary header, provide its byte count:

```cpp
ccsds::Packet decoded;
const auto consumed = decoded.deserializeBounded(wire, 2U);
if (!consumed) return consumed.error().code();
```

## Use a configuration file

A minimal template configuration is:

```ini
mission_profile:string=generic
ccsds_packet_error_control:string=crc16

ccsds_version_number:int=0
ccsds_type:bool=false
ccsds_secondary_header_flag:bool=false
ccsds_APID:int=0x123
ccsds_segmented:bool=false

data_field_size:int=1024

define_secondary_header:bool=false
```

Load it directly into a Manager:

```cpp
ccsds::Manager manager;
const auto templateResult = manager.loadTemplateConfigFile("template.cfg");
if (!templateResult) return templateResult.error().code();

const auto dataResult = manager.setApplicationData({0x10, 0x20, 0x30});
if (!dataResult) return dataResult.error().code();

const auto writeResult = manager.write("packets.bin");
if (!writeResult) return writeResult.error().code();
```

See [CONFIG.md](CONFIG.md) for all keys, PUS/CUC profiles, and Idle Packet
constraints. Complete CLI-ready configurations are in
[`example/config`](../example/config).

## Command-line equivalent

```bash
ccsds_encoder -i payload.bin -o packets.bin -c template.cfg
ccsds_validator -i packets.bin -c template.cfg --verbose
ccsds_decoder -i packets.bin -o recovered.bin -c template.cfg
```

Use `--packet-error-control none` consistently on all three tools for a CRC-free stream. See [CLI.md](CLI.md) for complete options and trailing-byte handling.
