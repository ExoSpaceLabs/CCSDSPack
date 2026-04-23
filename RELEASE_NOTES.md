# CCSDSPack v1.1.1

Maintenance release focusing on critical bug fixes and CMake integration improvements.

## Highlights
- Fixed a critical deserialization bug where primary headers were not correctly populated when using secondary headers.
- Modernized CMake package configuration for easier integration in downstream projects.
- Docker images now include a `:latest` tag for the most recent stable release.

## Bug Fixes
- **#42 (Deserialize issue):** Fixed `CCSDS::Packet::deserialize(data, headerType, headerSize)` skipping primary header processing.
- Fixed off-by-one errors in packet length validation during deserialization.
- Corrected `PusB` `eventId` type from `uint8_t` to `uint16_t` to prevent data truncation.
- Fixed `setApplicationData` return value handling in tests to resolve compiler warnings.

## Enhancements
- Added `CCSDSPackConfig.cmake.in` template for robust `find_package(CCSDSPack CONFIG)` support.
- Improved `build_terminal.sh` for reliable test execution regardless of current working directory.
- Expanded test suite with `testGroupEdgeCases.cpp` covering PUS B/C edge cases and large payloads.

## Distribution
- Docker image published to GHCR:
```bash
docker pull ghcr.io/exospacelabs/ccsdspack:latest
docker run --rm ghcr.io/exospacelabs/ccsdspack:latest /usr/bin/CCSDSPack_tester
```

___

For detailed changes, see `CHANGE_LOG.md`.
