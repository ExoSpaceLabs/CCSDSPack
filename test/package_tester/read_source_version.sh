#!/usr/bin/env bash
# Copyright 2025-2026 ExoSpaceLabs
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version_file="${repo_root}/VERSION"
if [[ ! -f "$version_file" ]]; then
  echo "ERROR: missing CCSDSPack VERSION file: $version_file" >&2
  exit 6
fi
version="$(tr -d '[:space:]' < "$version_file")"
if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  echo "ERROR: invalid CCSDSPack VERSION '$version' (expected MAJOR.MINOR.PATCH)" >&2
  exit 6
fi
printf '%s\n' "$version"
