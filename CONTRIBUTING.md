# Contributing to CCSDSPack

## Branch model

- `main` is the approved release line. Release tags are created only from qualified `main` commits.
- `develop` is the integration and qualification line for ongoing work, CI, and required native/physical hardware validation.
- Short-lived `feature/`, `fix/`, `perf/`, `test/`, `docs/`, and `ci/` branches target `develop`.
- Release promotion is performed through a reviewed `develop -> main` pull request only after required tests and hardware campaigns pass on `develop`.

Do not maintain long-lived release-staging branches unless a future release explicitly requires one.

## Change control

Protocol and standards-facing changes must preserve the documented CCSDS/ECSS claim boundary and update the corresponding evidence when behavior changes.

Before merging implementation changes:

1. keep independent fixed-vector and negative conformance tests green;
2. keep Linux, Windows, Doxygen, ASan, UBSan, and bounded fuzz gates green;
3. preserve installed C and C++ consumer compatibility unless a versioned breaking change is intentional;
4. update documentation when public API, configuration, package, or compliance behavior changes.

Round-trip tests alone are not sufficient evidence for standards-defined wire behavior.

## Commit messages

Use Conventional Commit style with the project scope where practical:

```text
feat(CCSDSPack): description
fix(CCSDSPack): description
perf(CCSDSPack): description
docs(CCSDSPack): description
ci(CCSDSPack): description
test(CCSDSPack): description
```

## Release control

**All qualification belongs on `develop`.** `main` is the approved release line, not a staging or hardware-test branch.

A release is promoted and tagged only after:

1. freeze the implementation candidate on `develop` and run its hosted CI/conformance gates;
2. run required native and physical target campaigns against that exact implementation SHA on `develop` and record source, package and executable hashes;
3. update and reconcile the release acceptance list, compliance evidence, changelog and release notes on `develop`; verify evidence-only changes do not modify the qualified source, public headers, build scripts or hardware harness;
4. review and promote the fully qualified `develop` revision to `main`;
5. pass final `main` CI on the exact commit being tagged;
6. create the release tag from the approved `main` commit and verify GitHub Release artifacts, package identity/hashes and GHCR images.

Do not merge unqualified changes into `main` merely because hosted CI is green. If a release-candidate implementation or hardware harness changes after physical tests, reassess whether the tests must be rerun.
