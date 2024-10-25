# CCSDSPack
C++ Library for CCSDS Space Packet manipulation. i.e. generation, extraction, analisys and more

## What is a CCSDS Packet Description
Describe a typical packet and its components graphs and such.
```Include links for references```

## Install
1) Source - use the cmake and make commands to compile the whole project and install it.
2) RPM    - Use the precompiled rpm installers and linux commands to extract and install.
3) Docker - Installation within a Docker container, use the provided bash script under the "docker" directory.

## Examples
1) This example shows how this library can be used to generate a ccsds packet or stream of packets using CCSDSPack

TBD
```
#include "CCSDSPack.h"

int main(){
  CCSDSManager ccsdsManager;
  ccsdsManager.ID = {0xFF ... };

  ccsdsManager.data({0x00,0xff ....});
  // auto data ccsdsManager.packets();             // all ccsds packets appendedd.
  auto data ccsdsManager.packet(int packetNumber); // single ccsds packet specified
  return 0;
}
```
Where "explanation" of what does it do.

2) Assuming you already have a CCSDS packet stream and want to extract the data from it

TBD
```
#include "CCSDSPack.h"

int main(){
  CCSDSManager ccsdsManager;
  ccsdsManager.ID = {0xFF ... };

  auto data = ccsdsManager.data();

  return 0;
}
```
Where "explanation" of what does it do.
