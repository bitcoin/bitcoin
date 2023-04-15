# Release Process

This document outlines the process for releasing versions of the form `$MAJOR.$MINOR.$PATCH`.

We distinguish between two types of releases: *regular* and *maintenance* releases.
Regular releases are releases of a new major or minor version as well as patches of the most recent release.
Maintenance releases, on the other hand, are required for patches of older releases.

You should coordinate with the other maintainers on the release date, if possible.
This date will be part of the release entry in [CHANGELOG.md](../CHANGELOG.md) and it should match the dates of the remaining steps in the release process (including the date of the tag and the GitHub release).
It is best if the maintainers are present during the release, so they can help ensure that the process is followed correctly and, in the case of a regular release, they are aware that they should not modify the master branch between merging the PR in step 1 and the PR in step 3.

This process also assumes that there will be no minor releases for old major releases.

## Regular release

1. Open a PR to the master branch with a commit (using message `"release: prepare for $MAJOR.$MINOR.$PATCH"`, for example) that
   * finalizes the release notes in [CHANGELOG.md](../CHANGELOG.md) (make sure to include an entry for `### ABI Compatibility`),
   * updates `_PKG_VERSION_*` and `_LIB_VERSION_*` and sets `_PKG_VERSION_IS_RELEASE` to `true` in `configure.ac`, and
   * updates `project(libsecp256k1 VERSION ...)` and `${PROJECT_NAME}_LIB_VERSION_*` in `CMakeLists.txt`.
2. After the PR is merged, tag the commit and push it:
   ```
   RELEASE_COMMIT=<merge commit of step 1>
   git tag -s v$MAJOR.$MINOR.$PATCH -m "libsecp256k1 $MAJOR.$MINOR.$PATCH" $RELEASE_COMMIT
   git push git@github.com:bitcoin-core/secp256k1.git v$MAJOR.$MINOR.$PATCH
   ```
3. Open a PR to the master branch with a commit (using message `"release cleanup: bump version after $MAJOR.$MINOR.$PATCH"`, for example) that
   * sets `_PKG_VERSION_IS_RELEASE` to `false` and increments `_PKG_VERSION_PATCH` and `_LIB_VERSION_REVISION` in `configure.ac`, and
   * increments the `$PATCH` component of `project(libsecp256k1 VERSION ...)` and `${PROJECT_NAME}_LIB_VERSION_REVISION` in `CMakeLists.txt`.

   If other maintainers are not present to approve the PR, it can be merged without ACKs.
4. Create a new GitHub release with a link to the corresponding entry in [CHANGELOG.md](../CHANGELOG.md).

## Maintenance release

Note that bugfixes only need to be backported to releases for which no compatible release without the bug exists.

1. If `$PATCH = 1`, create maintenance branch `$MAJOR.$MINOR`:
   ```
   git checkout -b $MAJOR.$MINOR v$MAJOR.$MINOR.0
   git push git@github.com:bitcoin-core/secp256k1.git $MAJOR.$MINOR
   ```
2. Open a pull request to the `$MAJOR.$MINOR` branch that
   * includes the bugfixes,
   * finalizes the release notes,
   * increments `_PKG_VERSION_PATCH` and `_LIB_VERSION_REVISION` in `configure.ac`
     and the `$PATCH` component of `project(libsecp256k1 VERSION ...)` and `${PROJECT_NAME}_LIB_VERSION_REVISION` in `CMakeLists.txt`
     (with commit message `"release: bump versions for $MAJOR.$MINOR.$PATCH"`, for example).
3. After the PRs are merged, update the release branch and tag the commit:
   ```
   git checkout $MAJOR.$MINOR && git pull
   git tag -s v$MAJOR.$MINOR.$PATCH -m "libsecp256k1 $MAJOR.$MINOR.$PATCH"
   ```
4. Push tag:
   ```
   git push git@github.com:bitcoin-core/secp256k1.git v$MAJOR.$MINOR.$PATCH
   ```
5. Create a new GitHub release with a link to the corresponding entry in [CHANGELOG.md](../CHANGELOG.md).
6. Open PR to the master branch that includes a commit (with commit message `"release notes: add $MAJOR.$MINOR.$PATCH"`, for example) that adds release notes to [CHANGELOG.md](../CHANGELOG.md).
