# CCSDSPack Docker Image

This Dockerfile builds an Ubuntu 22.04 image with CCSDSPack installed from the latest GitHub release.

## Build

```bash

docker build -t ccsdspack:latest .
```
Interactive shell:

```bash

docker run -it --rm ccsdspack:latest bash
```

Run the built-in tester:
```bash

docker run --rm ccsdspack:latest /usr/bin/CCSDSPack_tester
```