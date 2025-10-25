sudo dpkg --add-architecture arm64
dpkg --print-foreign-architectures   # should show: arm64 (and i386 if you added it)

# 2) Backup current sources
sudo cp /etc/apt/sources.list /etc/apt/sources.list.bak.$(date +%s)

# 3) Write a clean dual-arch sources.list
sudo tee /etc/apt/sources.list >/dev/null <<'EOF'
# amd64 from main archive
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy-updates main restricted universe multiverse
deb [arch=amd64] http://archive.ubuntu.com/ubuntu jammy-backports main restricted universe multiverse
deb [arch=amd64] http://security.ubuntu.com/ubuntu jammy-security main restricted universe multiverse

# arm64 from ports (the place that actually serves non-amd64 reliably)
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-updates main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-backports main restricted universe multiverse
deb [arch=arm64] http://ports.ubuntu.com/ubuntu-ports jammy-security main restricted universe multiverse
EOF

# 4) Clean and refresh indexes (force IPv4 because some networks are drama)
sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*
sudo apt-get -o Acquire::ForceIPv4=true update

# 5) Install the cross toolchain and the arm64 runtimes shlibdeps wants
sudo apt-get install -y \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu qemu-user-static rsync \
  libc6:arm64 libgcc-s1:arm64 libstdc++6:arm64

# 6) Sanity check that the files exist
dpkg -L libc6:arm64        | grep -E 'ld-linux-aarch64\.so\.1|libc\.so\.6' || true
dpkg -L libgcc-s1:arm64    | grep libgcc_s.so.1 || true
dpkg -L libstdc++6:arm64   | grep 'libstdc\+\+\.so\.6' || true