
<div style="text-align: center;">
    <img alt="ccsds_pack_logo" src="docs/imgs/Logo.png" width="400" />
</div>

# CCSDSPack - [ExoSpaceLabs](https://github.com/ExoSpaceLabs)
**CCSDSPack** is a lightweight, high-performance C++ library for handling 
[CCSDS (Consultative Committee for Space Data Systems)](https://public.ccsds.org/) packets.
It provides utilities to create, parse, and manage CCSDS-compliant telemetry (TM) and 
telecommand (TC) packets with a modern C++ design and customizable serialization.


## Table of Contents
- [Status](##Status)
- [Features](#Features)
- [Documentation](#Documentation)
- [Install](#Install)
    - [Linux](#Linux)
    - [Windows](#Windows)
- [Examples](#Examples)

---
## Status
The following tables show the current overall build and regression test status of the library.

| Linux | Windows |
|-------|---------|
 | ![Build Status](https://img.shields.io/github/actions/workflow/status/Inczert/CCSDSPack/linux.yml?branch=main)| ![Build Status](https://img.shields.io/github/actions/workflow/status/Inczert/CCSDSPack/windows.yml?branch=main) | 

Specific distribution build and regression status are shown below

| OS      | Distribution  |  status |
|---------|---------------|--------------|
| Linux   | ubuntu-22.04  | ![Ubuntu 22.04](https://github.com/Inczert/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-22-04)     |
|         | ubuntu-24.04  | ![Ubuntu 24.04](https://github.com/Inczert/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-24-04)    |
|         | ubuntu-latest | ![Ubuntu Latest](https://github.com/Inczert/CCSDSPack/actions/workflows/linux.yml/badge.svg?job=ubuntu-latest)      |
| Windows | 2019          | ![Windows 10](https://github.com/Inczert/CCSDSPack/actions/workflows/windows.yml/badge.svg?job=windows-2019)     |
|         | latest        | ![Windows Latest](https://github.com/Inczert/CCSDSPack/actions/workflows/windows.yml/badge.svg?job=windows-latest)      |
---

## Features

- 📦 Support for both **TM** and **TC** CCSDS packets using integrated PUS standards
- ⚙️ Custom serialization with support for `std::vector`, `std::shared_ptr`, and user-defined types
- 🔁 End-to-end encoding / decoding and validation
- 🧪 Easy to test and integrate
- 🛠️ Built-in extensibility with clear abstraction
-  User-friendly installation and usage within your code.
- Optimised for fast execution.
---
## Documentation
Full API documentation is available and hosted here:  [CCSDSPack Documentation](https://exospacelabs.github.io/CCSDSPack/html/)


C++ Library for CCSDS Space Packet manipulation. i.e. generation, extraction, analysis and more

![ccsds packet image](docs/imgs/Packet_management.png)

### Manager
Features Provided by the CCSDS::Manager class(Assuming the Packet identifier data is known):

* Generate CCSDS Packets with desired data (Segmented and Unsegmented).
* Validate Packets coherence / against template (packet with set identifier).
* Update Error Control and data specific parameters (CRC16, Counters, Flags Length...).
* Include / remove / change Sync Pattern.
* Read / Write a binary file and extract CCSDS Packets.

### The CCSDS Packet protocol
The CCSDS packet is described by:

![ccsds packet image](docs/imgs/ccsdsPacket.png)


### PUS TC (PUS-A) inclusion
This section shows how a PUS-A Packet can be included in the CCSDS packet

[insert image of a PUS-A packet]()

### PUS TM (PUS-B) inclusion
This section shows how a PUS-B Packet can be inluded in the CCSDS packet

[insert image of a PUS-B packet]()

###  PUS-C inclusion
A more flexible and variable sized standard.

[insert image of a PUS-C packet]()

### Other Documents
Please check out the documentation on [ccsds documentation](https://public.ccsds.org/Publications/default.aspx). Reccomended documents are within the Blue books.

Also take a look of the following documents:

* [CCSDS 133.0-B-2](https://public.ccsds.org/Pubs/133x0b2e2.pdf) - Space Packet Protocol
* [CCSDS 133.1-B-3](https://public.ccsds.org/Pubs/133x1b3e1.pdf) - Encapsulation Packet Protocol
* [CCSDS 524.1-B-1](https://public.ccsds.org/Pubs/524x1b1.pdf) - Mission Operations--MAL Space Packet Transport Binding and Binary Encoding

---

## Install
1) Source  - use the cmake and make commands to compile the whole project and install it.
2) Package - Install using prebuilt .deb package from [release link to be implemented](). Build and package library using github actions and let github host them.
3) Docker  - Docker image available github hosted container repo [link to repo usage readme]() to be built using actions.

### Source
CMake flags:

The following flags can be provided to cmake when building the project to enable disable build of 
specific provided features. such as tester, which may or may not be of interest. 

| Cmake Flag (default value)  |  Description                                                 |
|-----------------------------|--------------------------------------------------------------|
| -DBUILD_TESTER=ON           | build tester, that performs regression tests of the library. | 
| -DENABLE_ENCODER=ON         | build encoder executable that encodes a file using ccsds packets |
| -DENABLE_DECODER=ON         | build decoder executable that decodes a binary file containing ccsdspackets |

see example usage during cmake setup.

---

#### Linux

Install dependencies (GCC ≥ 8.5.0, CMake ≥ 3.20,  C++17 or newer)
```bash

sudo apt update
sudo apt install -y cmake make g++ 
```
Clone Repo:
```bash

git clone https://github.com/ExoSpaceLabs/CCSDSPack.git
cd CCSDSPack
```

set up cmake and build
```bash

cd build && cmake .. && make
#cmake .. -DBUILD_TESTER=OFF
```
Assuming build has been successful the tester can be run from the build directory. So if
you are still within the build dir just run the following commands:
```bash

cd ../bin && ./CCSDSPack_tester 
```
The library is advised to be installed using the following commands under /usr/local
```bash

make install
```
Note: this might require privileged `sudo` command as per permission restrictions.

---
#### Windows
tested on Windows-2019 and Windows-latest (see git hub [Actions](https://github.com/ExoSpaceLabs/CCSDSPack/actions/workflows/windows.yml))

Install dependencies:

```shell

choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System' -y
choco install ninja -y
choco install mingw -y
```

Configure CMake and build using MinGW (Assuming the repo has been cloned)
```shell

cd build
cmake -G "MinGW Makefiles" ..
cmake --build . -- 
```
The tester can already be executed using the following commands:
```shell

cd build/bin && ./CCSDSPack_tester.exe
```
Note: Cmake will deploy test files to bin directory, if the tester is not executed from the
bin directory some tests will fail.

---

### Package (TBD)
[the releases shall be built and deployed by github actions]()

download your release from [releases link to be implemented]() and install it using dpkg. 

```bash

curl -LO hhtps://github.com/ExoSpaceLabs/CCSDSPack/ ... ccsdspack-<version>_<architecture>.deb
sudo dpkg -i ccsdspack-<version>_<architecture>.deb

```

### Docker (TBD)
[docker image still to be defined and deployed by github container repo ]()

```bash

docker build <link to repo>
```

## Examples

### Encoder:

Encode a specific file into the application data of CCSDS packets and save streamed packets 
data to a desired file.

requirements: 
* a file to encode
* configuration file which holds the template packet data and settings

```bash

ccsds_encoder -i <file_to_encode> -o <encoded_binaryFile> -t <config_file>
```
### Decoder:

Decode a previously encoded binary file holding serialized CCSDS packets, extract application 
data and recreate orifinal file. 

requirements:
* an encoded binary file 
* configuration file used to encode the packets holding template packet data and settings

```bash

ccsds_decoder -i <encoded_binaryFile> -o <decoded_file> -t <config_file>
```

Note: If the decoded file is named with the original file extension it will be usable as the original file,

for configuration file and other info see [link to readme to be inserted]().

### C++
The following examples show how the high level C++ APIs can be used in a project

Note: Assume a Big endian logic for data processing.
1) This example shows how this library can be used to generate a ccsds packet or stream of packets using CCSDSPack

```c++
#include "CCSDSPack.h"

int main(){

  std::vector<uint8_t> inputBytes; // assume data is present
  // make a packet and set header to be used as template  
  CCSDS::Packet templatePacket;
  if(const auto exp = templatePacket.setPrimaryHeader(0xF7FF4FFFFFFF); !exp.has_value()){
    std::cerr << exp.error().message() << std::endl;
    return exp.error().code();
  }

  // set template packet in manager
  CCSDS::Manager manager(templatePacket);
  manager.setDatFieldSize(1024); // sets max datafield size
  
  // load data
  if (const auto exp = manager.setApplicationData(inputBytes); !exp.has_value()) {
    std::cerr <<  exp.error().message() << std::endl;
    return exp.error().code();
  }
  std::vector<CCSDS::Packet> packets = manager.getPackets();
  // to manipulate as required.
  
  return 0;
}
```
Where the 'inputBytes' are a set of bytes that are to be set as application into the packets. This is performed by 
first setting a template packet in the manager. Which then will be used as reference for all packets generated. and the 
application data is then set using the setApplicationData manager member method. This generates CCSDS packets in  the 
manager. It can be retrieved by using the getPackets method. and further manipulation can be performed if required.

2) Assuming you already have a CCSDS packet stream and want to extract the data from it.

```c++
#include "CCSDSPack.h"

int main(){

  std::vector<CCSDS::Packet> packets; // assume data is present

  // set template packet in manager
  CCSDS::Manager;
  manager.setDatFieldSize(1024); // sets max datafield size
  
  // load data
  if (const auto exp = manager.load(packets); !exp.has_value()) {
    std::cerr <<  exp.error().message() << std::endl;
    return exp.error().code();
  }
  // get the data buffer of the packets.
  std::vector<uint8_t> data = manager.getApplicationDataBuffer();
  return 0;
}
```
Assuming we have a vector of CCSDS packets prepared ad hoc. These packets then can be loaded by the manager.
If required the manager allows the packets to be validated for coherence, or if the template is set against it.
In the example above the application data is retrieved from all packets into a single stream of data.

for more examples see [link to readme for more examples to be inserted]()

