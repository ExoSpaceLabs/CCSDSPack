# -----------------------------
# Packaging with CPack
# -----------------------------

message(STATUS "INFO: SYSTEM          : ${CMAKE_SYSTEM_NAME}")
message(STATUS "INFO: SYSTEM PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

# Basic metadata
set(CPACK_PACKAGE_NAME "ccsdspack")
set(CPACK_PACKAGE_VENDOR "ExoSpaceLabs")
set(CPACK_PACKAGE_CONTACT "Robert Incze")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Robert Incze")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CCSDS Packet Manager library and tools")
set(CPACK_PACKAGE_VERSION       ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

set(ARCH "${CMAKE_SYSTEM_PROCESSOR}")
# Ensure the DEB architecture is correct
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    set(ARCH "arm64")
endif()

# Generators per platform
if(WIN32)
    # Simple, reliable: zip the installed tree (DLL + EXEs + headers + libs)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    # .tgz by default; you can add DragNDrop later if you bundle apps
    set(CPACK_GENERATOR "TGZ")
else()
    # Linux: produce .deb, .packages, and .tar.gz
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    # Auto-detect shared library dependencies for Debian/Ubuntu
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()

# Install prefix recorded inside packages (default is /usr/local)
# You can override at install time; leaving default is fine:
# set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# define output filename
set(CPACK_PACKAGE_FILE_NAME "ccsdspack${VER}-${CMAKE_SYSTEM_NAME}-${ARCH}")

include(CPack)
