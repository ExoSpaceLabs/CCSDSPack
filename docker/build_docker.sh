#!/usr/bin/env bash
set -euo pipefail

usage() {
  echo "Usage: bash docker/build_docker.sh <release-tag> (e.g. v2.1.0)" >&2
}

if [[ $# -ne 1 || ! "$1" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  usage
  exit 2
fi

tag="$1"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

docker build \
  --build-arg "CCSDSPACK_TAG=${tag}" \
  -t "ccsdspack:${tag}" \
  -f "${script_dir}/Dockerfile" "${script_dir}"
docker run --rm "ccsdspack:${tag}" /usr/bin/CCSDSPack_tester
