# CCSDSPack Docker image

This Dockerfile builds an Ubuntu 22.04 image and installs the x86_64 DEB from an
**exact CCSDSPack GitHub release tag**. It does not implicitly resolve the latest
release.

## Build

From the repository root:

```bash
docker build \
  --build-arg CCSDSPACK_TAG=v2.1.0 \
  -t ccsdspack:v2.1.0 \
  -f docker/Dockerfile docker
```

The release workflow uses the same mechanism and publishes both the versioned
GHCR tag and `latest` only from a Git release tag.

## Interactive shell

```bash
docker run -it --rm ccsdspack:v2.1.0 bash
```

## Run the installed tester

```bash
docker run --rm ccsdspack:v2.1.0 /usr/bin/CCSDSPack_tester
```
