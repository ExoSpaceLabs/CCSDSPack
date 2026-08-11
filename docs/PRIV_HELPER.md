# Maintainer branch workflow

This note records the repository workflow used for CCSDSPack v2 development. It is a maintainer aid, not part of the public library API.

## Branch roles

- `main`: released/approved line; release tags are created from approved `main` commits;
- `develop`: integrated development line;
- `v2.0.0-dev`: v2.0.0 staging/integration branch until the release is promoted;
- `feature/<name>`: short-lived implementation branches.

## Feature flow

For v2.0.0 work:

```text
feature/<name> -> v2.0.0-dev -> develop -> main -> tag v2.0.0
```

Features are reviewed through pull requests and are not merged directly from a feature branch into `develop` while the v2 staging branch is active.

## Commit messages

Library/documentation commits use Conventional Commit form with the project scope and a real issue reference when applicable:

```text
feat(CCSDSPack): #123 description
docs(CCSDSPack): #123 description
fix(CCSDSPack): #123 description
```

CI-only changes use the `CI` scope where appropriate.

## Release control

Before each promotion:

1. reconcile the relevant issue acceptance criteria;
2. run/verify required hosted checks;
3. keep protocol/compliance claims aligned with the tested implementation;
4. merge through a reviewed pull request;
5. tag only the approved `main` commit after the final release gates pass.

Automatic UML generation is not a release gate. The manual workflow can be used for ad-hoc architecture inspection when useful.
