# Examples

This page shows practical, copy‑paste examples for using **CCSDSPack** in C++.
All snippets assume C++17 or newer and big‑endian wire format, as per CCSDS.

[../](README.md) - CCSDSPack Documentation

---

## Table of Contents
- [1) Bytes Packets (with segmentation) Save to file](#1-bytes-packets-with-segmentation-save-to-file)
- [2) Read packets from file Reassemble bytes](#2-read-packets-from-file-reassemble-bytes)
- [3) End to end round trip (sanity test)](#3-end-to-end-round-trip-sanity-test)
- [4) From a packet vector: serialize and de‑serialize](#4-from-a-packet-vector-serialize-de-serialize)
- [5) Using a config file for template & settings](#5-using-a-config-file-for-template--settings)
- [6) Validating packets (CLI helper)](#6-validating-packets-cli-helper)
- [7) Error‑first pattern (no exceptions)](#7-error-first-pattern-no-exceptions)
- [8)Using a custom Secondary header](#8-Using-a-custom-secondary-header)

---


> Headers:
> ```c++
> #include "CCSDSPack.h"
> #include <iostream>
> #include <vector>
> ```

## 1) Bytes Packets (with segmentation) save to file

Create a template packet, segment an input buffer into CCSDS packets, and save them as a binary container.

```c++
#include "CCSDSPack.h"
#include <iostream>
#include <vector>

int main() {
  // 1) Define a template primary header (example value)
  CCSDS::Packet tpl;
  if (const auto r = tpl.setPrimaryHeader(0xF7FF4FFFFFFF); !r.has_value()) {
    std::cerr << r.error().message() << "\n";
    return r.error().code();
  }

  // 2) Application bytes (e.g., file contents you loaded elsewhere)
  std::vector<std::uint8_t> app(5000, 0xAB);

  // 3) Segment & build packets
  CCSDS::Manager m(tpl);
  m.setDatFieldSize(1024); // max application data field per packet

  if (const auto r = m.setApplicationData(app); !r.has_value()) {
    std::cerr << r.error().message() << "\n";
    return r.error().code();
  }

  // 4) (Optional) inspect in-memory packets
  auto packets = m.getPackets();
  std::cout << "Generated " << packets.size() << " packets\n";

  // 5) Persist them via Manager's binary writer
  if (const auto r = m.writePacketsBinary("packets.bin"); !r.has_value()) {
    std::cerr << r.error().message() << "\n";
    return r.error().code();
  }

  return 0;
}
```
## 2) Read packets from file Reassemble bytes
   Load serialized CCSDS packets from disk and reconstruct the original application data.

```c++
#include "CCSDSPack.h"
#include <iostream>
#include <vector>

int main() {
CCSDS::Manager m;
m.setDatFieldSize(1024); // keep consistent with your system/template

// Load the packet container created earlier
if (const auto r = m.readPacketsBinary("packets.bin"); !r.has_value()) {
std::cerr << r.error().message() << "\n";
return r.error().code();
}

// Reassemble application bytes
std::vector<std::uint8_t> app = m.getApplicationDataBuffer();
std::cout << "Recovered " << app.size() << " bytes\n";
return 0;
}
```
## 3) End to end round trip (sanity test)
   A minimal test you can drop into your project to validate encoding/decoding with file I/O.

```c++

#include "CCSDSPack.h"
#include <cassert>
#include <iostream>
#include <vector>

int main() {
// Source data
std::vector<std::uint8_t> src(8192);
for (size_t i = 0; i < src.size(); ++i) src[i] = static_cast<uint8_t>(i & 0xFF);

// Template header
CCSDS::Packet tpl;
if (const auto r = tpl.setPrimaryHeader(0xF7FF4FFFFFFF); !r.has_value())
return r.error().code();

// Encode to packets
CCSDS::Manager enc(tpl);
enc.setDatFieldSize(1024);
if (const auto r = enc.setApplicationData(src); !r.has_value())
return r.error().code();

// Save packets to disk
if (const auto r = enc.writePacketsBinary("packets.bin"); !r.has_value())
return r.error().code();

// Decode back from disk
CCSDS::Manager dec;
dec.setDatFieldSize(1024);
if (const auto r = dec.readPacketsBinary("packets.bin"); !r.has_value())
return r.error().code();

auto dst = dec.getApplicationDataBuffer();
assert(src == dst && "Round-trip mismatch");
std::cout << "Round-trip OK: " << dst.size() << " bytes\n";
return 0;
}
```
## 4) From a packet vector: serialize de serialize
   When you already have std::vector of CCSDS::Packet, serialize to disk and load back.

```c++
#include "CCSDSPack.h"
#include <iostream>
#include <vector>

int main() {
// Pretend you already built some packets
std::vector<CCSDS::Packet> packets = /* ... */;

// Write vector to disk via Manager
{
CCSDS::Manager m;
if (const auto r = m.load(packets); !r.has_value()) {
std::cerr << r.error().message() << "\n";
return r.error().code();
}
if (const auto r = m.writePacketsBinary("packets.bin"); !r.has_value()) {
std::cerr << r.error().message() << "\n";
return r.error().code();
}
}

// Later: read back and recover the vector
CCSDS::Manager m2;
if (const auto r = m2.readPacketsBinary("packets.bin"); !r.has_value()) {
std::cerr << r.error().message() << "\n";
return r.error().code();
}
auto roundtripped = m2.getPackets();
std::cout << "Loaded " << roundtripped.size() << " packets from file\n";
return 0;
}
```
## 5) Using a config file for template & settings
   Leverage a configuration file to control data_field_size, sync pattern, validation, and packet header fields without recompiling.

```c++

#include "CCSDSPack.h"
#include <iostream>

int main() {
Config cfg;
const std::string path = "templatePacket.cfg";

if (const auto r = cfg.load(path); !r.has_value()) {
std::cerr << "[Error " << r.error().code() << "]: " << r.error().message() << "\n";
return r.error().code();
}

// Example: fetch data_field_size (int) with error checking
uint16_t dataFieldSize = 0;
if (cfg.isKey("data_field_size")) {
if (const auto r = cfg.get<int>("data_field_size"); r.has_value()) {
dataFieldSize = static_cast<uint16_t>(r.value());
} else {
std::cerr << r.error().message() << "\n";
return r.error().code();
}
}

// Build packets using the template + size from config
CCSDS::Packet tpl;
// (Optional) You can also derive header bits from your config if you store them there
if (const auto r = tpl.setPrimaryHeader(0xF7FF4FFFFFFF); !r.has_value())
return r.error().code();

CCSDS::Manager m(tpl);
if (dataFieldSize) m.setDatFieldSize(dataFieldSize);

// Set your application data and proceed as in Example #1 ...
return 0;
}
```
See `CONFIG.md` for the exact config syntax and the full list of supported keys.

## 6) Validating packets (CLI helper)
   While you can check coherence in your app flow, the quickest way to validate a packet container is via the provided CLI:

```bash

# Validate with a template (verbose report + packet print)
ccsds_validator -i ./packets.bin -c ./template.cfg -v -p
```
If a config is provided, loaded packets are validated against the template packet defined there.
Use this together with the round‑trip example to make CI‑friendly checks.

## 7) Error first pattern (no exceptions)
   All high‑level operations return a result. Always check has_value() and handle error().

```c++

auto res = manager.setApplicationData(buffer);
if (!res.has_value()) {
std::cerr << "[CCSDSPack] " << res.error().code() << ": " << res.error().message() << "\n";
return res.error().code();
}
```

## 8) Using a custom secondary header
Custom secondary headers can be defined and used where necessary. Custom secondary header definition:
```c++

class CustomSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
public:
  CustomSecondaryHeader() {variableLength = true;};

  /**
   * @brief Constructs a CustomSecondaryHeader object with all fields explicitly set.
   */
  explicit CustomSecondaryHeader(const std::vector<uint8_t>& data) : m_data(data) {
    variableLength= true;
  };

  [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<uint8_t> &data) override {m_data = data; return true;};

  [[nodiscard]] std::uint16_t getSize() const override { return m_data.size(); }
  [[nodiscard]] std::string getType() const override { return m_type; }

  [[nodiscard]] std::vector<uint8_t> serialize() const override {return m_data;};
  void update(CCSDS::DataField* dataField) override {m_dataLength = m_data.size();}
  CCSDS::ResultBool loadFromConfig(const Config &config) override{return true;};

private:
  std::vector<uint8_t> m_data{};
  uint16_t m_dataLength = 0;
  const std::string m_type = "CustomSecondaryHeader";
};
```
Where:
- The custom secondary header class extends from `CCSDS::SecondaryHeaderAbstract` class.
- All `virtual` member functions to be defined (all which are defined in the example above).
- In case of secondary header length is variable set `variableLength= true;` in the constructor /s.

It is also recommended to view PUS service classes in the `PusServices.h` header and source file.

The defined custom class must then be registered and set by each packet where used. This can be achieved as follows:
```c++

  CCSDS::Packet packet;
  packet.RegisterSecondaryHeader<CustomSecondaryHeader>();
  
  if (const auto res = templatePacket.setDataFieldHeader(data, "CustomSecondaryHeader"); !res.has_value()) {
    std::cerr << "Error: "<< res.error().message() << ". CODE: " << res.error().code() << std::endl;
    return res.error().code();
  }
  
  // alternatively using the setDataFieldHeader overload 
  CustomSecondaryHeader myHeader{};
  auto ptr = std::make_shared<CustomSecondaryHeader>(myHeader);
  templatePacket.setDataFieldHeader(ptr);
  
```

At this point the custom secondary header is part of the selected packet. If the secondary header is registered on a 
packet used as a template packet for the `CCSDS::Manager`, than no successive registrations are required. Otherwise, 
the registration is required to be performed on each individual packet where necessary.

---

---




Notes:
* Replace the example primary‑header constant with values suitable for your mission (APID, etc.).
* Keep setDatFieldSize() consistent across encode/decode paths to avoid mismatches.
*  The binary read/write helpers (writePacketsBinary, readPacketsBinary) make it easy to persist and reload packet sets.

Additionally, library usage can be referred from the `test/src` source files.