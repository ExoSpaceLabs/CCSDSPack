#!/bin/bash

# This script builds the project in the build directory and executes the Executable in the bin dir

cd build
cmake ..
make

cd ../bin
./CCSDSPack_tester

