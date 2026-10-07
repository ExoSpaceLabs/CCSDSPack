#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  gcc-arm-none-eabi \
  binutils-arm-none-eabi \
  libnewlib-arm-none-eabi \
  libstdc++-arm-none-eabi-newlib
