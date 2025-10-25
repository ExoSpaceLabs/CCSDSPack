sudo dpkg --add-architecture arm64
sudo dpkg --print-foreign-architectures  # should show: arm64
# 2) Backup current sources
sudo apt-get update && sudo apt-get install -y \
            gcc-arm-none-eabi binutils-arm-none-eabi \
            libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib