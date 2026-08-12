#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  bash test/package_tester/aarch64_validate.sh <ccsdspack-arm64.deb>

Run this script from a CCSDSPack source checkout on an aarch64/arm64 Linux
system. It installs the package, runs the installed regression and CLI suites,
builds the external installed-package consumer, and executes once the same
board-independent hardware acceptance core used by STM32 validation.
EOF
}

if [[ $# -ne 1 ]]; then usage; exit 2; fi
case "$(uname -m)" in aarch64|arm64) ;; *) echo "ERROR: native arm64 required" >&2; exit 3 ;; esac
for tool in sudo dpkg dpkg-deb cmake python3 ctest g++ realpath mktemp cp; do
  command -v "${tool}" >/dev/null || { echo "ERROR: required command not found: ${tool}" >&2; exit 4; }
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
package_path="$(realpath -m "$1")"
[[ -f "${package_path}" ]] || { echo "ERROR: package not found: ${package_path}" >&2; exit 5; }

package_arch="$(dpkg-deb -f "${package_path}" Architecture)"
[[ "${package_arch}" == "arm64" ]] || { echo "ERROR: package architecture is ${package_arch}, expected arm64" >&2; exit 6; }
package_version="$(dpkg-deb -f "${package_path}" Version)"
[[ "${package_version}" == "2.0.0" ]] || { echo "ERROR: package version is ${package_version}, expected 2.0.0" >&2; exit 7; }
package_name="$(dpkg-deb -f "${package_path}" Package)"
sudo dpkg -i "${package_path}"

mapfile -t installed_files < <(dpkg -L "${package_name}")
find_installed() {
  local pattern="$1" path
  for path in "${installed_files[@]}"; do
    if [[ "${path}" =~ ${pattern} ]]; then printf '%s\n' "${path}"; return 0; fi
  done
  return 1
}

tester="$(find_installed '/CCSDSPack_tester$')" || exit 8
encoder="$(find_installed '/ccsds_encoder$')" || exit 9
decoder="$(find_installed '/ccsds_decoder$')" || exit 10
validator="$(find_installed '/ccsds_validator$')" || exit 11
cmake_config="$(find_installed '/cmake/CCSDSPack/CCSDSPackConfig.cmake$')" || exit 12
test_resources="$(find_installed '/test_resources$')" || exit 13
library_file="$(find_installed '/libccsdspack\.so(\.2(\.0\.0)?)?$' || true)"
bin_dir="$(dirname "${tester}")"
cmake_dir="$(dirname "${cmake_config}")"
if [[ -n "${library_file}" ]]; then export LD_LIBRARY_PATH="$(dirname "${library_file}"):${LD_LIBRARY_PATH:-}"; fi

validation_work_dir="$(mktemp -d "${TMPDIR:-/tmp}/ccsdspack-aarch64-validation.XXXXXX")"
trap 'rm -rf "${validation_work_dir}"' EXIT
cp -R "${test_resources}" "${validation_work_dir}/test_resources"

(
  cd "${validation_work_dir}"
  "${tester}"
)
python3 "${repo_root}/test/cli_integration.py" --bin-dir "${bin_dir}" --resources "${repo_root}/test/test_resources"

consumer_build="${repo_root}/build/aarch64-package-validation"
rm -rf "${consumer_build}"
cmake -S "${repo_root}/test/package_tester/shared_lib" -B "${consumer_build}" -DCMAKE_BUILD_TYPE=Release -DCCSDSPack_DIR="${cmake_dir}"
cmake --build "${consumer_build}" -- -j"$(nproc)"
ctest --test-dir "${consumer_build}" --output-on-failure -R '^installed_shared_library_consumer$'

# Execute the common acceptance body exactly once and retain its explicit marker.
"${consumer_build}/ccsdspack_hardware_acceptance"

echo "CCSDSPACK_AARCH64_TEST:PASS"
