#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  bash test/performance/run_matched_v2_0_vs_v2_1.sh [--baseline <ref>] [--candidate <ref>] [--output-dir <dir>]

Build v2.0 and v2.1 back-to-back with the same host toolchain and compile the
same protocol benchmark source against both public C++ APIs.

Defaults:
  baseline: 4e198ae4c7f730737d78c1ea2f71ec3ce42ca7eb
  candidate: HEAD
  output-dir: build/matched-performance
EOF
}

baseline_ref="4e198ae4c7f730737d78c1ea2f71ec3ce42ca7eb"
candidate_ref="HEAD"
output_dir="build/matched-performance"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --baseline) baseline_ref="$2"; shift 2 ;;
    --candidate) candidate_ref="$2"; shift 2 ;;
    --output-dir) output_dir="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "ERROR: unknown argument: $1" >&2; usage; exit 2 ;;
  esac
done

for tool in git cmake g++ sha256sum uname; do
  command -v "$tool" >/dev/null || {
    echo "ERROR: required command not found: $tool" >&2
    exit 3
  }
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "${script_dir}/../.." && pwd)"
benchmark_source="${repo_root}/test/performance/protocol_benchmark.cpp"
[[ -f "${benchmark_source}" ]] || {
  echo "ERROR: benchmark source not found: ${benchmark_source}" >&2
  exit 4
}

cd "${repo_root}"
repo_root="$(pwd)"
output_dir="$(realpath -m "${output_dir}")"
rm -rf "${output_dir}"
mkdir -p "${output_dir}"

if ! git rev-parse --verify "${baseline_ref}^{commit}" >/dev/null 2>&1; then
  echo "Fetching baseline ref ${baseline_ref}..."
  git fetch --no-tags --depth=1 origin "${baseline_ref}"
fi

baseline_sha="$(git rev-parse "${baseline_ref}^{commit}")"
candidate_sha="$(git rev-parse "${candidate_ref}^{commit}")"

tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/ccsdspack-matched-perf.XXXXXX")"
baseline_tree="${tmp_root}/v2.0"
candidate_tree="${tmp_root}/v2.1"

cleanup() {
  git -C "${repo_root}" worktree remove --force "${baseline_tree}" >/dev/null 2>&1 || true
  git -C "${repo_root}" worktree remove --force "${candidate_tree}" >/dev/null 2>&1 || true
  rm -rf "${tmp_root}"
}
trap cleanup EXIT

git worktree add --detach "${baseline_tree}" "${baseline_sha}" >/dev/null
git worktree add --detach "${candidate_tree}" "${candidate_sha}" >/dev/null

build_revision() {
  local tree="$1"
  local label="$2"
  local build_dir="${tree}/build-matched"

  cmake -S "${tree}" -B "${build_dir}"     -DCMAKE_BUILD_TYPE=Release     -DCCSDSPACK_BUILD_BENCHMARKS=OFF
  cmake --build "${build_dir}" -- -j

  local lib_dir="${tree}/lib"
  local c_lib_dir="${build_dir}/lib"
  local exe="${output_dir}/protocol-benchmark-${label}"

  g++ -std=c++17 -O3 -DNDEBUG     "${benchmark_source}"     -I"${tree}/inc"     -L"${lib_dir}"     -Wl,-rpath,"${lib_dir}"     -lccsdspack     -o "${exe}"

  LD_LIBRARY_PATH="${lib_dir}:${c_lib_dir}:${LD_LIBRARY_PATH:-}"     "${exe}" "${label}"     | tee "${output_dir}/protocol-${label}.csv"
}

{
  echo "CCSDSPack matched v2.0-v2.1 performance run"
  echo "host_arch=$(uname -m)"
  echo "kernel=$(uname -r)"
  echo "compiler=$(g++ --version | head -n 1)"
  echo "cmake=$(cmake --version | head -n 1)"
  echo "baseline_sha=${baseline_sha}"
  echo "candidate_sha=${candidate_sha}"
  echo "benchmark_sha256=$(sha256sum "${benchmark_source}" | awk '{print $1}')"
} | tee "${output_dir}/metadata.txt"

build_revision "${baseline_tree}" "v2.0"
build_revision "${candidate_tree}" "v2.1"

python3 - "${output_dir}" <<'PY'
import csv
import pathlib
import sys

root = pathlib.Path(sys.argv[1])

def rows(path):
    with path.open(newline="") as f:
        reader = csv.DictReader(line for line in f if not line.startswith("CCSDSPACK_"))
        return {(r["operation"], r["variant"]): r for r in reader}

base = rows(root / "protocol-v2.0.csv")
cand = rows(root / "protocol-v2.1.csv")
if set(base) != set(cand):
    missing_base = sorted(set(cand) - set(base))
    missing_cand = sorted(set(base) - set(cand))
    raise SystemExit(f"benchmark row mismatch baseline_missing={missing_base} candidate_missing={missing_cand}")

with (root / "protocol-comparison.csv").open("w", newline="") as f:
    fieldnames = [
        "operation", "variant",
        "v2_0_ns_per_op", "v2_1_ns_per_op", "speedup",
        "v2_0_allocations_per_op", "v2_1_allocations_per_op",
        "v2_0_allocated_bytes_per_op", "v2_1_allocated_bytes_per_op",
    ]
    writer = csv.DictWriter(f, fieldnames=fieldnames)
    writer.writeheader()
    for key in sorted(base):
        b, c = base[key], cand[key]
        bns = float(b["ns_per_op"])
        cns = float(c["ns_per_op"])
        writer.writerow({
            "operation": key[0],
            "variant": key[1],
            "v2_0_ns_per_op": f"{bns:.2f}",
            "v2_1_ns_per_op": f"{cns:.2f}",
            "speedup": f"{(bns / cns):.3f}" if cns else "inf",
            "v2_0_allocations_per_op": b["allocations_per_op"],
            "v2_1_allocations_per_op": c["allocations_per_op"],
            "v2_0_allocated_bytes_per_op": b["allocated_bytes_per_op"],
            "v2_1_allocated_bytes_per_op": c["allocated_bytes_per_op"],
        })

print((root / "protocol-comparison.csv").read_text(), end="")
PY

echo "CCSDSPACK_MATCHED_PROTOCOL_BENCHMARK:PASS"
echo "Results: ${output_dir}"
