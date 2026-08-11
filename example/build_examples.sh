#!/usr/bin/env bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repository_dir="$(cd "${script_dir}/.." && pwd)"
selection="${1:-all}"
install_prefix="${2:-${CCSDSPACK_PREFIX:-}}"
build_root="${CCSDSPACK_EXAMPLE_BUILD_DIR:-${repository_dir}/build/examples}"

case "${selection}" in
  all)
    source_dir="${script_dir}"
    ;;
  basic_packet|custom_secondary_header|pus_c_telecommand|pus_c_telemetry|raw_buffer_packet|raw_buffer_manager)
    source_dir="${script_dir}/${selection}"
    ;;
  *)
    echo "Usage: $0 [all|basic_packet|custom_secondary_header|pus_c_telecommand|pus_c_telemetry|raw_buffer_packet|raw_buffer_manager] [install-prefix]" >&2
    exit 2
    ;;
esac

cmake_args=(-S "${source_dir}" -B "${build_root}/${selection}" -DCMAKE_BUILD_TYPE=Release)
if [[ -n "${install_prefix}" ]]; then
  cmake_args+=("-DCMAKE_PREFIX_PATH=${install_prefix}")
  export PATH="${install_prefix}/bin:${PATH}"
  export LD_LIBRARY_PATH="${install_prefix}/lib:${install_prefix}/lib64:${LD_LIBRARY_PATH:-}"
  export DYLD_LIBRARY_PATH="${install_prefix}/lib:${install_prefix}/lib64:${DYLD_LIBRARY_PATH:-}"
fi

cmake "${cmake_args[@]}"
cmake --build "${build_root}/${selection}" --config Release --parallel
ctest --test-dir "${build_root}/${selection}" -C Release --output-on-failure
