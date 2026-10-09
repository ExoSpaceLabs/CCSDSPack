# Contributing to CCSDSPack

## Branch model

- `main` is the approved release line. Release tags are created only from qualified `main` commits.
- `develop` is the integration line for ongoing work.
- Short-lived `feature/`, `fix/`, `perf/`, `test/`, `docs/`, and `ci/` branches target `develop`.
- Release promotion is performed through a reviewed `develop -> main` pull request.

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

A release is tagged only after:

1. the release acceptance list is reconciled;
2. the candidate is promoted to `main`;
3. final `main` CI is green on the exact commit to be tagged;
4. required native/physical validation evidence is recorded;
5. release notes and compliance documents match the tested implementation.

Tag publication must then be verified for GitHub Release assets, package identity/hashes, and GHCR images.
