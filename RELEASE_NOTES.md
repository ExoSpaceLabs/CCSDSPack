# CCSDSPack v1.0.0

The first stable release of CCSDSPack, a lightweight C++ library for creating, parsing, and validating CCSDS Space Packets.
This release marks the transition from prototype to production-ready, with packaging, CI, and Docker support.

___

## Features

- Full support for CCSDS Primary Header encoding/decoding.
- Abstract Secondary Header interface, allowing users to define mission-specific extensions.
- Ready-to-use command-line tools:
    - `ccsds_encoder`
    - `ccsds_decoder` 
    - `ccsds_validator` 
    - `CCSDSPack_tester` (built-in test harness).
- CMake integration for easy inclusion in external projects.

___

## Distribution

- Debian package (.deb) available in [Releases](https://github.com/ExoSpaceLabs/CCSDSPack/releases).
```bash
sudo dpkg -i ccsdspack-<version>-Linux-x86_64.deb
```

- Docker image published to GHCR:
```bash
docker pull ghcr.io/exospacelabs/ccsdspack:v1.0.0
docker run --rm ghcr.io/exospacelabs/ccsdspack:v1.0.0 /usr/bin/CCSDSPack_tester
```
___

## Development & CI

- Automated build and test matrix via GitHub Actions.
- `.deb` packaging and artifact upload. 
- Automatic Docker image builds for tagged releases (published on GHCR). 
- UML diagrams and docs generation integrated (clang-uml + Doxygen configs included).

___

## Documentation

- Project docs available at: https://exospacelabs.github.io/
- See `EXAMPLES.md` for usage examples (including how to implement a custom abstract secondary header).

___

## Next Steps
- Expanded examples for real-world CCSDS telemetry/telecommand flows.
- Additional packaging targets (RPM, Homebrew, etc.). 
- Continuous improvements to documentation and diagrams.

___

This release provides a stable baseline for mission prototyping and educational use.
From here, future updates will add optional features and refinements without breaking existing APIs.

___