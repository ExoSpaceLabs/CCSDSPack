#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  bash test/package_tester/run_aarch64_validation.sh [--log <path>]

Run the complete native arm64 release validation from a CCSDSPack source
checkout. The runner records platform/source identity, requires a clean native
aarch64 checkout, rebuilds the DEB package from scratch, records package
metadata/hash, and invokes aarch64_validate.sh for installed-package tests.
EOF
}

log_path=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --log)
      [[ $# -ge 2 ]] || { echo "ERROR: --log requires a path" >&2; exit 2; }
      log_path="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "ERROR: unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"

for tool in git uname cmake cpack gcc g++ python3 dpkg dpkg-deb sha256sum find sed realpath tee; do
  command -v "${tool}" >/dev/null || {
    echo "ERROR: required command not found: ${tool}" >&2
    exit 3
  }
done

case "$(uname -m)" in
  aarch64|arm64) ;;
  *)
    echo "ERROR: native aarch64 required, detected $(uname -m)" >&2
    exit 4
    ;;
esac

cd "${repo_root}"

if ! git diff --quiet || ! git diff --cached --quiet; then
  echo "ERROR: working tree has tracked modifications; validate a clean candidate checkout" >&2
  exit 5
fi

if [[ -n "$(git ls-files --others --exclude-standard)" ]]; then
  echo "ERROR: working tree has untracked files; validate a clean candidate checkout" >&2
  git ls-files --others --exclude-standard >&2
  exit 5
fi

source_version_component() {
  local component="$1"
  sed -nE "s/^[[:space:]]*set\\(${component}[[:space:]]+\"?([0-9]+)\"?\\).*/\\1/p" "${repo_root}/CMakeLists.txt"
}

source_major="$(source_version_component MAJOR)"
source_minor="$(source_version_component MINOR)"
source_patch="$(source_version_component PATCH)"
if [[ -z "${source_major}" || -z "${source_minor}" || -z "${source_patch}" ]]; then
  echo "ERROR: could not determine CCSDSPack source version from CMakeLists.txt" >&2
  exit 6
fi
source_version="${source_major}.${source_minor}.${source_patch}"

if [[ -z "${log_path}" ]]; then
  log_path="${HOME}/ccsdspack-v${source_version}-aarch64-validation.log"
fi
mkdir -p "$(dirname "${log_path}")"
: > "${log_path}"
exec > >(tee "${log_path}") 2>&1

source_sha="$(git rev-parse HEAD)"
source_branch="$(git branch --show-current)"
board_model="$(tr -d '\0' </proc/device-tree/model 2>/dev/null || true)"
os_pretty="$(sed -n 's/^PRETTY_NAME=//p' /etc/os-release 2>/dev/null | tr -d '"')"

echo "=== CCSDSPack native arm64 validation ==="
echo "BOARD:${board_model:-unknown}"
echo "ARCH:$(uname -m)"
echo "OS:${os_pretty:-unknown}"
echo "KERNEL:$(uname -r)"
echo "GCC:$(gcc --version | head -n 1)"
echo "GXX:$(g++ --version | head -n 1)"
echo "CMAKE:$(cmake --version | head -n 1)"
echo "PYTHON:$(python3 --version)"
echo "CCSDSPACK_SOURCE_BRANCH:${source_branch:-detached}"
echo "CCSDSPACK_SOURCE_SHA:${source_sha}"
echo "CCSDSPACK_SOURCE_VERSION:${source_version}"
echo "VALIDATION_LOG:${log_path}"

echo
echo "Cleaning previous native build/package output..."
rm -rf "${repo_root}/build" "${repo_root}/packages"

echo
echo "Building native arm64 DEB package..."
"${repo_root}/package.sh" -p DEB

mapfile -t arm64_packages < <(
  find "${repo_root}/packages" -maxdepth 1 -type f \
    \( -name '*arm64*.deb' -o -name '*aarch64*.deb' \) \
    -print
)

if [[ ${#arm64_packages[@]} -ne 1 ]]; then
  echo "ERROR: expected exactly one generated arm64 DEB, found ${#arm64_packages[@]}" >&2
  printf '  %s\n' "${arm64_packages[@]:-}" >&2
  exit 7
fi

package_path="$(realpath "${arm64_packages[0]}")"
package_name="$(dpkg-deb -f "${package_path}" Package)"
package_version="$(dpkg-deb -f "${package_path}" Version)"
package_arch="$(dpkg-deb -f "${package_path}" Architecture)"
package_sha256="$(sha256sum "${package_path}" | sed 's/[[:space:]].*$//')"

[[ "${package_name}" == "ccsdspack" ]] || {
  echo "ERROR: package name is ${package_name}, expected ccsdspack" >&2
  exit 8
}
[[ "${package_version}" == "${source_version}" ]] || {
  echo "ERROR: package version is ${package_version}, expected ${source_version}" >&2
  exit 9
}
[[ "${package_arch}" == "arm64" ]] || {
  echo "ERROR: package architecture is ${package_arch}, expected arm64" >&2
  exit 10
}

echo
echo "Generated package identity:"
echo "CCSDSPACK_PACKAGE:${package_path}"
echo "CCSDSPACK_PACKAGE_NAME:${package_name}"
echo "CCSDSPACK_PACKAGE_VERSION:${package_version}"
echo "CCSDSPACK_PACKAGE_ARCH:${package_arch}"
echo "CCSDSPACK_PACKAGE_SHA256:${package_sha256}"

echo
echo "Running installed-package and acceptance validation..."
bash "${script_dir}/aarch64_validate.sh" "${package_path}"

echo
echo "=== CCSDSPack native arm64 validation summary ==="
echo "BOARD:${board_model:-unknown}"
echo "ARCH:$(uname -m)"
echo "CCSDSPACK_SOURCE_SHA:${source_sha}"
echo "CCSDSPACK_PACKAGE_VERSION:${package_version}"
echo "CCSDSPACK_PACKAGE_ARCH:${package_arch}"
echo "CCSDSPACK_PACKAGE_SHA256:${package_sha256}"
echo "VALIDATION_LOG:${log_path}"
echo "CCSDSPACK_AARCH64_RUNNER:PASS"
