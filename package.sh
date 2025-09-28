#!/bin/bash
# defaults

package_type="DEB"
use_toolchain=0
toolchain=""
show_help=0
mcu_flags=""

print_help() {
    cat <<EOF
Usage: $0 [OPTIONS]

Options:
  -p, --package-type TYPE  Set package type [DEB, RPM, TGZ, MCU]
  -t, --toolchain NAME     Set toolchain file for cross compilation.
  -m, --mcu-flags "FLAGS"  Set flags when MCU type is selected.
  -h, --help               Show this help message

Examples:
  To build deb package using system architecture (e.g. x85_64):
  ./package.sh -p DEB

  Cross-Compile deb package for aarch64 (e.g. Raspberry Pi):
  ./package.sh -p DEB -t cmake/toolchains/aarch64-linux-gnu.cmake

  Cross-Compile for STM32 Cortex-M7 processor:
  ./package.sh -t cmake/toolchains/arm-none-eabi.cmake -p MCU -m "-fno-exceptions -fno-rtti -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard"
EOF
}

print_config() {
    echo "Configuration:"
    echo "  package_type = $package_type"
    echo "  toolchain    = $toolchain"
    echo "  mcu-flags    = $mcu_flags"
}

# argument loop
while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--package-type)
            package_type="$2"
            shift 2
            ;;
        -t|--toolchain)
            toolchain="$2"
            use_toolchain=1
            shift 2
            ;;
        -h|--help)
            show_help=1
            shift
            ;;
        -m|--mcu-flags)
            mcu_flags="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Try '$0 --help' for usage."
            exit 1
            ;;
    esac
done

# print help or config
if [[ $show_help -eq 1 ]]; then
  print_help
  exit 0
fi

print_config

rm -rf build/*
cd build

if [[ use_toolchain -eq 1 ]]; then
  if [[ "${package_type^^}" == "MCU" ]]; then
    if [[ "${mcu_flags^^}" == "" ]]; then
      cmake -S . -DCMAKE_BUILD_TYPE=Release -DCCSDSPACK_BUILD_MCU=ON -DCMAKE_TOOLCHAIN_FILE="${toolchain}" ..
    else
      cmake -S . -DCMAKE_BUILD_TYPE=Release -DCCSDSPACK_BUILD_MCU=ON -DMCU_FLAGS="${mcu_flags}" -DCMAKE_TOOLCHAIN_FILE="${toolchain}" ..
    fi
  else
    cmake -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="${toolchain}" ..
  fi
else
  cmake -S . -DCMAKE_BUILD_TYPE=Release ..
fi

cmake --build . --config Release -- -j

# cpack might require super user privileges.
if [[ "${package_type^^}" == "MCU" ]]; then
  cpack -G "TGZ" -C Release -V
else
  cpack -G "${package_type^^}" -C Release -V
fi

if [[ "${package_type^^}" == "DEB" ]]; then
  mv ccsdspack-v*.deb ../packages/.
elif  [[ "${package_type^^}" == "RPM" ]]; then
  mv ccsdspack-v*.rpm ../packages/.
else
  mv ccsdspack-v*.tar.gz ../packages/.
fi

# check for consistency:
#dpkg-deb --info ccsdspack-v1.0.0-Linux-x86_64.deb
#dpkg-deb --contents ccsdspack-v1.0.0-Linux-x86_64.deb

# to install on the system (requires sudo)
# dpkg -i ccsdspack-v1.0.0-Linux-x86_64.deb

# to remove:
# dpkg --remove ccsdspack
