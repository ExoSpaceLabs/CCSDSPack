#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  ./test/package_tester/aarch64_validate.sh <ccsdspack-arm64.deb>

Run this script from a CCSDSPack source checkout on an aarch64/arm64 Linux
system, such as a 64-bit Raspberry Pi OS installation. It installs the package,
runs the installed regression tester and CLI integration suite, then builds and
runs an external CMake consumer against the installed package metadata.
EOF
}

if [[ $# -ne 1 ]]; then
  usage
  exit 2
fi

case "$(uname -m)" in
  aarch64|arm64)
    ;;
  *)
    echo "ERROR: this validation must run natively on aarch64/arm64; got $(uname -m)" >&2
    exit 3
    ;;
esac

for tool in sudo dpkg dpkg-deb cmake python3 ctest g++ realpath; do
  command -v "${tool}" >/dev/null || {
    echo "ERROR: required command not found: ${tool}" >&2
    exit 4
  }
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
package_path="$(realpath -m "$1")"

if [[ ! -f "${package_path}" ]]; then
  echo "ERROR: package not found: ${package_path}" >&2
  exit 5
fi

package_arch="$(dpkg-deb -f "${package_path}" Architecture)"
if [[ "${package_arch}" != "arm64" ]]; then
  echo "ERROR: package architecture is ${package_arch}, expected arm64" >&2
  exit 6
fi

package_version="$(dpkg-deb -f "${package_path}" Version)"
if [[ "${package_version}" != "1.2.0" ]]; then
  echo "ERROR: package version is ${package_version}, expected 1.2.0" >&2
  exit 7
fi

package_name="$(dpkg-deb -f "${package_path}" Package)"
echo "Installing ${package_name} ${package_version} (${package_arch})"
sudo dpkg -i "${package_path}"

mapfile -t installed_files < <(dpkg -L "${package_name}")

find_installed() {
  local pattern="$1"
  local path
  for path in "${installed_files[@]}"; do
    if [[ "${path}" =~ ${pattern} ]]; then
      printf '%s\n' "${path}"
      return 0
    fi
  done
  return 1
}

tester="$(find_installed '/CCSDSPack_tester$')" || {
  echo "ERROR: installed CCSDSPack_tester not found" >&2
  exit 8
}
encoder="$(find_installed '/ccsds_encoder$')" || {
  echo "ERROR: installed ccsds_encoder not found" >&2
  exit 9
}
decoder="$(find_installed '/ccsds_decoder$')" || {
  echo "ERROR: installed ccsds_decoder not found" >&2
  exit 10
}
validator="$(find_installed '/ccsds_validator$')" || {
  echo "ERROR: installed ccsds_validator not found" >&2
  exit 11
}
cmake_config="$(find_installed '/cmake/CCSDSPack/CCSDSPackConfig.cmake$')" || {
  echo "ERROR: installed CCSDSPackConfig.cmake not found" >&2
  exit 12
}
library_file="$(find_installed '/libccsdspack\.so(\.1(\.2\.0)?)?$' || true)"

bin_dir="$(dirname "${tester}")"
cmake_dir="$(dirname "${cmake_config}")"
if [[ -n "${library_file}" ]]; then
  export LD_LIBRARY_PATH="$(dirname "${library_file}"):${LD_LIBRARY_PATH:-}"
fi

for executable in "${tester}" "${encoder}" "${decoder}" "${validator}"; do
  if [[ ! -x "${executable}" ]]; then
    echo "ERROR: installed executable is not runnable: ${executable}" >&2
    exit 13
  fi
done

echo "Running installed native regression tester"
(
  cd "${bin_dir}"
  "${tester}"
)

echo "Running installed encoder/decoder/validator integration suite"
python3 "${repo_root}/test/cli_integration.py" \
  --bin-dir "${bin_dir}" \
  --resources "${repo_root}/test/test_resources"

echo "Building external consumer against installed CCSDSPack 1.2.0"
consumer_build="${repo_root}/build/aarch64-package-validation"
rm -rf "${consumer_build}"
cmake \
  -S "${repo_root}/test/package_tester/shared_lib" \
  -B "${consumer_build}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCCSDSPack_DIR="${cmake_dir}"
cmake --build "${consumer_build}" -- -j"$(nproc)"
ctest --test-dir "${consumer_build}" --output-on-failure

echo "CCSDSPACK_AARCH64_TEST:PASS"
