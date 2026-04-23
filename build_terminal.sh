#!/bin/bash

# This script builds the project in the build directory and executes the Executable in the bin dir

# Exit on any error
set -e

# Ensure we are in the script's directory
cd "$(dirname "$0")"

# Create build directory if it doesn't exist
mkdir -p build

cd build
cmake ..
cmake --build .

# The binary output directory depends on the OS in CMakeLists.txt
# Linux/macOS: BINARY_OUTPUT_DIR is ${CMAKE_SOURCE_DIR}/bin
# Windows: BINARY_OUTPUT_DIR is ${CMAKE_BINARY_DIR}/bin

cd ..
if [ -f "./bin/CCSDSPack_tester" ]; then
    cd bin
    ./CCSDSPack_tester
elif [ -f "./build/bin/CCSDSPack_tester" ]; then
    cd build/bin
    ./CCSDSPack_tester
else
    echo "Error: CCSDSPack_tester not found!"
    exit 1
fi

