#!/usr/bin/env bash
set -euo pipefail

# This helper intentionally prepares the Ubuntu 22.04 (Jammy) CI/package host.
# Do not silently rewrite a newer Ubuntu installation to Jammy repositories.
if [[ ! -r /etc/os-release ]]; then
  echo "ERROR: /etc/os-release is unavailable; cannot verify Ubuntu 22.04 host." >&2
  exit 2
fi

# shellcheck disable=SC1091
source /etc/os-release
if [[ "${ID:-}" != "ubuntu" || "${VERSION_ID:-}" != "22.04" ]]; then
  echo "ERROR: install-aarch-cross-build-deps.sh is supported only on Ubuntu 22.04 (Jammy)." >&2
  echo "Detected: ${PRETTY_NAME:-unknown}" >&2
  echo "For other distributions/releases, install the aarch64 cross toolchain and target runtimes using the native package manager." >&2
  exit 2
fi

sudo dpkg --add-architecture arm64
dpkg --print-foreign-architectures

# Keep amd64 on the main archive and arm64 on ports.ubuntu.com.
sudo cp /etc/apt/sources.list /etc/apt/sources.list.bak.$(date +%s)
sudo tee /etc/apt/sources.list >/dev/null <<'EOF'
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy-updates main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy-backports main restricted universe multiverse
deb [arch=amd64] http://security.ubuntu.com/ubuntu jammy-security main restricted universe multiverse

deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-updates main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-backports main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-security main restricted universe multiverse
EOF

sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*
sudo apt-get -o Acquire::ForceIPv4=true update

sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static rsync \
  libc6:arm64 libgcc-s1:arm64 libstdc++6:arm64

dpkg -L libc6:arm64     | grep -E 'ld-linux-aarch64\.so\.1|libc\.so\.6' || true
dpkg -L libgcc-s1:arm64 | grep libgcc_s.so.1 || true
dpkg -L libstdc++6:arm64 | grep 'libstdc\+\+\.so\.6' || true
