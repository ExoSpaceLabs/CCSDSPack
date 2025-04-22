
<div style="text-align: center;">
    <img alt="ccsds_pack_logo" src="docs/imgs/Logo.png" width="400" />
</div>

# CCSDSPack
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
| ![Build Status](https://img.shields.io/github/actions/workflow/status/Inczert/CCSDSPack/windows.yml?branch=main) | ![Build Status](https://img.shields.io/github/actions/workflow/status/Inczert/CCSDSPack/linux.yml?branch=main) | 

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


NOTE: These Images are to be replaced as the source is not attendible and not clear.
### PUS TC (PUS-A) inclusion
This section shows how a PUS-A Packet can be included in the CCSDS packet

![PUS TC packet image](docs/imgs/PUS_TC.png)


### PUS TM (PUS-B) inclusion
This section shows how a PUS-B Packet can be inluded in the CCSDS packet

![PUS TM packet image](docs/imgs/PUS_TM.png)


###  PUS-C inclusion
A more flexible and variable sized standard.

### Other Documents
Please check out the documentation on [ccsds documentation](https://public.ccsds.org/Publications/default.aspx). Reccomended documents are within the Blue books.

Also take a look of the following documents:

* [CCSDS 133.0-B-2](https://public.ccsds.org/Pubs/133x0b2e2.pdf) - Space Packet Protocol
* [CCSDS 133.1-B-3](https://public.ccsds.org/Pubs/133x1b3e1.pdf) - Encapsulation Packet Protocol
* [CCSDS 524.1-B-1](https://public.ccsds.org/Pubs/524x1b1.pdf) - Mission Operations--MAL Space Packet Transport Binding and Binary Encoding

---

## Install
1) Source - use the cmake and make commands to compile the whole project and install it.
2) TODO Deb / RPM    - Use the precompiled rpm installers and linux commands to extract and install.
3) TODO Docker - Installation within a Docker container, use the provided bash script under the "docker" directory.

CMake flags:
- -DBUILD_TESTER=ON (default) build tester, set to OFF to skip tester build.

see example usage during cmake setup.
### Linux

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
### Windows
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

## Examples
1) This example shows how this library can be used to generate a ccsds packet or stream of packets using CCSDSPack
* Assume a Big endian logic for data processing.

TBD
```c++
#include "CCSDSPack.h"

int main(){
  //Todo

  return 0;
}
```
Where "explanation" of what does it do.

2) Assuming you already have a CCSDS packet stream and want to extract the data from it

TBD
```c++
#include "CCSDSPack.h"

int main(){
 //Todo
  return 0;
}
```
Where "explanation" of what does it do.

continue with a link to different README for more examples.
