#!/bin/bash
# defaults

package_type="DEB"
use_toolchain=0
toolchain=""
show_help=0

print_help() {
    cat <<EOF
Usage: $0 [OPTIONS]

Options:
  -p, --package-type TYPE  Set package type [DEB, RPM, TGZ]
  -t, --toolchain NAME     Set toolchain file for cross compilation.
  -h, --help               Show this help message
EOF
}

print_config() {
    echo "Configuration:"
    echo "  package_type = $package_type"
    echo "  toolchain    = $toolchain"
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
  cmake -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="${toolchain}" ..
else
  cmake -S . -DCMAKE_BUILD_TYPE=Release ..
fi

cmake --build . --config Release -- -j

# cpack might require super user privileges.
cpack -G "${package_type^^}" -C Release -V

# check for consistency:
#dpkg-deb --info ccsdspack-v1.0.0-Linux-x86_64.deb
#dpkg-deb --contents ccsdspack-v1.0.0-Linux-x86_64.deb
if [[ "${package_type^^}" == "DEB" ]]; then
  mv ccsdspack-v*.deb ../packages/.
elif  [[ "${package_type^^}" == "RPM" ]]; then
  mv ccsdspack-v*.rpm ../packages/.
else
  mv ccsdspack-v*.tar.gz ../packages/.
fi

# to install on the system (requires sudo)
# dpkg -i ccsdspack-v1.0.0-Linux-x86_64.deb

# to remove:
# dpkg --remove ccsdspack
