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

## Automated versioning and publication

`VERSION` is the single source of truth for CMake project, library, package and release versions. It contains one semantic version without a `v` prefix (for example `2.1.0`). Do not edit hardcoded version constants in CMake. To prepare a new release on `develop`, update `VERSION`, the matching `CHANGELOG.md` section and `RELEASE_NOTES.md` heading, and complete the required physical/native qualification.

After a reviewed `develop -> main` pull request merges, Linux, Windows, Generate Doxygen Docs and Robustness workflows must all pass on the **same current main SHA**. `.github/workflows/tag-release.yml` then creates the immutable annotated `vX.Y.Z` tag only if it does not already exist, and dispatches the existing `linux.yml` workflow **on that tag** to publish packages, the GitHub Release and GHCR images. Ordinary pushes to `develop`, main merges without a version change, failed CI, stale main commits and previously tagged versions do not make new tags.

GitHub suppresses ordinary push-triggered workflows for tags created with `GITHUB_TOKEN`, hence the explicit `workflow_dispatch`. Release artifacts and GHCR publication must still be inspected for success. The tagger refuses to move an existing version tag. If publication fails after tagging, rerun the tagged Linux workflow via GitHub Actions rather than retagging. GitHub Actions permissions must allow the tagger to write repository contents and dispatch workflows; branch protections and release approval are additional controls.

**Release acceptance remains a human decision.** Do not promote unqualified source/build/harness changes merely to trigger this automation. The new version-file parsing alters CMake's version derivation and must be validated by hosted CI; reassess hardware qualification if its outputs or source identity materially change.

## Release control

**All qualification belongs on `develop`.** `main` is the approved release line, not a staging or hardware-test branch.

A release is promoted and tagged only after:

1. freeze the implementation candidate on `develop` and run its hosted CI/conformance gates;
2. run required native and physical target campaigns against that exact implementation SHA on `develop` and record source, package and executable hashes;
3. update and reconcile the release acceptance list, compliance evidence, changelog and release notes on `develop`; verify evidence-only changes do not modify the qualified source, public headers, build scripts or hardware harness;
4. review and promote the fully qualified `develop` revision to `main`;
5. pass final `main` CI on the exact commit being tagged;
6. allow the successful main CI workflows to create the `VERSION` tag and dispatch tagged packaging; verify GitHub Release artifacts, package identity/hashes and GHCR images.

Do not merge unqualified changes into `main` merely because hosted CI is green. If a release-candidate implementation or hardware harness changes after physical tests, reassess whether the tests must be rerun.
