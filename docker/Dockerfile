FROM ubuntu:22.04

# Exact release tag to install, for example v2.0.0.
ARG CCSDSPACK_TAG

# Prevent interactive prompts during package install
ENV DEBIAN_FRONTEND=noninteractive

# Install required tools
RUN apt-get update && apt-get install -y \
    curl \
    dpkg \
    ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# Install the x86_64 package produced by the exact tag being published.
RUN test -n "${CCSDSPACK_TAG}" \
 && DEB_URL="https://github.com/ExoSpaceLabs/CCSDSPack/releases/download/${CCSDSPACK_TAG}/ccsdspack-${CCSDSPACK_TAG}-Linux-x86_64.deb" \
 && echo "Downloading ${DEB_URL}" \
 && curl -fL -o /tmp/ccsdspack.deb "${DEB_URL}" \
 && (dpkg -i /tmp/ccsdspack.deb || (apt-get update && apt-get -f install -y)) \
 && rm /tmp/ccsdspack.deb

# Verify the installed tagged package works.
RUN cd /usr/bin && ./CCSDSPack_tester

WORKDIR /usr/bin

CMD ["/bin/bash"]
