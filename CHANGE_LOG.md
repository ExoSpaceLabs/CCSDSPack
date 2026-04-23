# CCSDSPack v1.0.0
***Date: 18/08/2025*** 
- The first stable release of the library see RELEASE_NOTES.md ON TAG v1.0.0

# CCSDSPack v1.1.1 (Includes previous 1.1.0 changes)
***Date: 25/10/2025*** 
- Baremetal static lib build support.
- includes toolchain for arm and instructions for build.
- include scripts to install cross-build dependencies.

# CCSDSPack v1.1.1
***Date: 23/04/2026***
- Fixed #42: Primary header now correctly deserialized when using secondary header types.
- Fixed off-by-one errors in deserialization length checks.
- Fixed PusB eventId truncation (uint8_t -> uint16_t).
- Standardized CMake package configuration using template file.
- Added latest tag support for Docker images.
- Expanded test suite for edge cases.