FROM ubuntu:22.04

# Prevent interactive prompts during package install
ENV DEBIAN_FRONTEND=noninteractive

# Install required tools
RUN apt-get update && apt-get install -y \
    wget \
    curl \
    dpkg \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# Fetch latest CCSDSPack release .deb from GitHub
RUN LATEST_DEB_URL=$(curl -s https://api.github.com/repos/ExoSpaceLabs/CCSDSPack/releases/latest \
    | grep browser_download_url \
    | grep '\Linux-x86_64.deb' \
    | cut -d '"' -f 4) \
 && echo "Downloading $LATEST_DEB_URL" \
 && curl -L -o /tmp/ccsdspack.deb "$LATEST_DEB_URL" \
 && dpkg -i /tmp/ccsdspack.deb || apt-get -f install -y \
 && rm /tmp/ccsdspack.deb

# Verify installation works
RUN cd /usr/bin && ./CCSDSPack_tester

WORKDIR /usr/bin

CMD ["/bin/bash"]
