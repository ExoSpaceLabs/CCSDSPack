cmake_minimum_required(VERSION 3.13)
project(ccsdspack_hello LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the package installed by your .deb (usually under /usr) or your custom prefix.
# If it's under a non-standard prefix, pass -DCMAKE_PREFIX_PATH=<prefix> when configuring.
find_package(CCSDSPack CONFIG REQUIRED)

add_executable(hello_ccsds src/main.cpp)
target_link_libraries(hello_ccsds PRIVATE ccsdspack::CCSDSPack)

# (Optional) On Linux, make the installed lib discoverable when running from the build tree.
# If you installed to /usr this isn't needed; if you installed to /usr/local, you may prefer:
if(UNIX AND NOT APPLE)
    set_target_properties(hello_ccsds PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
    )
endif()
