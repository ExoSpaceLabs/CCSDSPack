# CCSDSPack v1.1.0

Minor release of CCSDSPack, a lightweight C++ library for creating, parsing, and validating CCSDS Space Packets.
This release marks the transition from prototype to production-ready, with packaging, CI, and Docker support.

for previous releases checkout RELEASE_NOTES.md on tags
- v1.0.0

___

## Features
Cross build support for
- arm-none-eabi (tested on STM32H755Z-Q)
- aarch64-linux-gnu (tested on Raspberry PI 5)

pre-packaged static lib for arm baremetal and .deb for aarch64 libraries.
___

## Distribution

- Debian package (.deb) available in [Releases](https://github.com/ExoSpaceLabs/CCSDSPack/releases).
```bash
sudo dpkg -i ccsdspack-<version>-Linux-x86_64.deb
```

- Docker image published to GHCR:
```bash
docker pull ghcr.io/exospacelabs/ccsdspack:v1.1.0
docker run --rm ghcr.io/exospacelabs/ccsdspack:v1.1.0 /usr/bin/CCSDSPack_tester
```
___

## Development & CI

- Automated build and test matrix via GitHub Actions.
- `.deb` / `.tar.gz` packaging and artifact upload. 
- Automatic Docker image builds for tagged releases (published on GHCR). 
- UML diagrams and docs generation integrated (clang-uml + Doxygen configs included).

___

## Documentation

- Project docs available at: https://exospacelabs.github.io/
- See `EXAMPLES.md` for usage examples (including how to implement a custom abstract secondary header).

___

## Next Steps
- Expanded examples for real-world CCSDS telemetry/telecommand flows.
- Continuous improvements to documentation and diagrams.

___

This release provides a stable baseline for mission prototyping and educational use.
From here, future updates will add optional features and refinements without breaking existing APIs.

___
