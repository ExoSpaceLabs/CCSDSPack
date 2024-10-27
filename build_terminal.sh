#!/bin/bash

# This script builds the poject in the build directory and executes the Executible in the bin dir

cd build
cmake ..
make

cd ../bin
./CCSDSPack_tester

