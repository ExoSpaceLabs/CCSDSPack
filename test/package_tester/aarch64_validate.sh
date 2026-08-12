#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  bash test/package_tester/aarch64_validate.sh <ccsdspack-arm64.deb>

Run this script from a CCSDSPack source checkout on an aarch64/arm64 Linux
system, such as a 64-bit Raspberry Pi OS installation. It installs the package,
runs the installed regression tester and CLI integration suite, builds the
external installed-package consumer, and executes the same board-independent
hardware acceptance core used by the STM32 validation image.

Run the script as a normal user. Package installation still requires elevation,
so the script invokes sudo for dpkg -i and runs the tests unprivileged.
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

for tool in sudo dpkg dpkg-deb cmake python3 ctest g++ realpath mktemp cp; do
  command -v "${tool}" >/dev/null || {
    echo "ERROR: required command not found: ${tool}" >&2
    exit 4
  }
done

if [[ ${EUID} -eq 0 ]]; then
  echo "WARNING: launch this script as a normal user; it elevates only dpkg -i." >&2
fi

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
if [[ "${package_version}" != "2.0.0" ]]; then
  echo "ERROR: package version is ${package_version}, expected 2.0.0" >&2
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
test_resources="$(find_installed '/test_resources$')" || {
  echo "ERROR: installed CCSDSPack_tester resources not found" >&2
  exit 13
}
library_file="$(find_installed '/libccsdspack\.so(\.2(\.0\.0)?)?$' || true)"

bin_dir="$(dirname "${tester}")"
cmake_dir="$(dirname "${cmake_config}")"
if [[ -n "${library_file}" ]]; then
  export LD_LIBRARY_PATH="$(dirname "${library_file}"):${LD_LIBRARY_PATH:-}"
fi

for executable in "${tester}" "${encoder}" "${decoder}" "${validator}"; do
  if [[ ! -x "${executable}" ]]; then
    echo "ERROR: installed executable is not runnable: ${executable}" >&2
    exit 14
  fi
done

if [[ ! -d "${test_resources}" ]]; then
  echo "ERROR: installed tester resource path is not a directory: ${test_resources}" >&2
  exit 15
fi

validation_work_dir="$(mktemp -d "${TMPDIR:-/tmp}/ccsdspack-aarch64-validation.XXXXXX")"
trap 'rm -rf "${validation_work_dir}"' EXIT

cp -R "${test_resources}" "${validation_work_dir}/test_resources"

echo "Running installed native regression tester"
(
  cd "${validation_work_dir}"
  "${tester}"
)

echo "Running installed encoder/decoder/validator integration suite"
python3 "${repo_root}/test/cli_integration.py" \
  --bin-dir "${bin_dir}" \
  --resources "${repo_root}/test/test_resources"

echo "Building external consumer against installed CCSDSPack 2.0.0"
consumer_build="${repo_root}/build/aarch64-package-validation"
rm -rf "${consumer_build}"
cmake \
  -S "${repo_root}/test/package_tester/shared_lib" \
  -B "${consumer_build}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCCSDSPack_DIR="${cmake_dir}"
cmake --build "${consumer_build}" -- -j"$(nproc)"
ctest --test-dir "${consumer_build}" --output-on-failure

echo "Running shared hardware acceptance suite natively on arm64"
hardware_build="${repo_root}/build/aarch64-hardware-acceptance"
rm -rf "${hardware_build}"
cmake \
  -S "${repo_root}/test/package_tester/hardware" \
  -B "${hardware_build}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCCSDSPack_DIR="${cmake_dir}"
cmake --build "${hardware_build}" -- -j"$(nproc)"
ctest --test-dir "${hardware_build}" --output-on-failure

"${hardware_build}/ccsdspack_hardware_acceptance"

echo "CCSDSPACK_AARCH64_TEST:PASS"
