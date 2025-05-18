# Release process

This document outlines the process for releasing versions of the form `$MAJOR.$MINOR.$PATCH`.

We distinguish between two types of releases: *regular* and *maintenance* releases.
Regular releases are releases of a new major or minor version as well as patches of the most recent release.
Maintenance releases, on the other hand, are required for patches of older releases.

You should coordinate with the other maintainers on the release date, if possible.
This date will be part of the release entry in [CHANGELOG.md](../CHANGELOG.md) and it should match the dates of the remaining steps in the release process (including the date of the tag and the GitHub release).
It is best if the maintainers are present during the release, so they can help ensure that the process is followed correctly and, in the case of a regular release, they are aware that they should not modify the master branch between merging the PR in step 1 and the PR in step 3.

This process also assumes that there will be no minor releases for old major releases.

We aim to cut a regular release every 3-4 months, approximately twice as frequent as major Tortoisecoin Core releases. Every second release should be published one month before the feature freeze of the next major Tortoisecoin Core release, allowing sufficient time to update the library in Core.

## Sanity checks
Perform these checks when reviewing the release PR (see below):

1. Ensure `make distcheck` doesn't fail.
   ```shell
   ./autogen.sh && ./configure --enable-dev-mode && make distcheck
   ```
2. Check installation with autotools:
   ```shell
   dir=$(mktemp -d)
   ./autogen.sh && ./configure --prefix=$dir && make clean && make install && ls -RlAh $dir
   gcc -o ecdsa examples/ecdsa.c $(PKG_CONFIG_PATH=$dir/lib/pkgconfig pkg-config --cflags --libs libsecp256k1) -Wl,-rpath,"$dir/lib" && ./ecdsa
   ```
3. Check installation with CMake:
   ```shell
   dir=$(mktemp -d)
   build=$(mktemp -d)
   cmake -B $build -DCMAKE_INSTALL_PREFIX=$dir && cmake --build $build && cmake --install $build && ls -RlAh $dir
   gcc -o ecdsa examples/ecdsa.c -I $dir/include -L $dir/lib*/ -l secp256k1 -Wl,-rpath,"$dir/lib",-rpath,"$dir/lib64" && ./ecdsa
   ```
4. Use the [`check-abi.sh`](/tools/check-abi.sh) tool to verify that there are no unexpected ABI incompatibilities and that the version number and the release notes accurately reflect all potential ABI changes. To run this tool, the `abi-dumper` and `abi-compliance-checker` packages are required.
   ```shell
   tools/check-abi.sh
   ```

## Regular release

1. Open a PR to the master branch with a commit (using message `"release: prepare for $MAJOR.$MINOR.$PATCH"`, for example) that
   * finalizes the release notes in [CHANGELOG.md](../CHANGELOG.md) by
       * adding a section for the release (make sure that the version number is a link to a diff between the previous and new version),
       * removing the `[Unreleased]` section header,
       * ensuring that the release notes are not missing entries (check the `needs-changelog` label on github), and
       * including an entry for `### ABI Compatibility` if it doesn't exist,
   * sets `_PKG_VERSION_IS_RELEASE` to `true` in `configure.ac`, and,
   * if this is not a patch release,
       * updates `_PKG_VERSION_*` and `_LIB_VERSION_*`  in `configure.ac`, and
       * updates `project(libsecp256k1 VERSION ...)` and `${PROJECT_NAME}_LIB_VERSION_*` in `CMakeLists.txt`.
2. Perform the [sanity checks](#sanity-checks) on the PR branch.
3. After the PR is merged, tag the commit, and push the tag:
   ```
   RELEASE_COMMIT=<merge commit of step 1>
   git tag -s v$MAJOR.$MINOR.$PATCH -m "libsecp256k1 $MAJOR.$MINOR.$PATCH" $RELEASE_COMMIT
   git push git@github.com:tortoisecoin-core/secp256k1.git v$MAJOR.$MINOR.$PATCH
   ```
4. Open a PR to the master branch with a commit (using message `"release cleanup: bump version after $MAJOR.$MINOR.$PATCH"`, for example) that
   * sets `_PKG_VERSION_IS_RELEASE` to `false` and increments `_PKG_VERSION_PATCH` and `_LIB_VERSION_REVISION` in `configure.ac`,
   * increments the `$PATCH` component of `project(libsecp256k1 VERSION ...)` and `${PROJECT_NAME}_LIB_VERSION_REVISION` in `CMakeLists.txt`, and
   * adds an `[Unreleased]` section header to the [CHANGELOG.md](../CHANGELOG.md).

   If other maintainers are not present to approve the PR, it can be merged without ACKs.
5. Create a new GitHub release with a link to the corresponding entry in [CHANGELOG.md](../CHANGELOG.md).
6. Send an announcement email to the tortoisecoin-dev mailing list.

## Maintenance release

Note that bug fixes need to be backported only to releases for which no compatible release without the bug exists.

1. If there's no maintenance branch `$MAJOR.$MINOR`, create one:
   ```
   git checkout -b $MAJOR.$MINOR v$MAJOR.$MINOR.$((PATCH - 1))
   git push git@github.com:tortoisecoin-core/secp256k1.git $MAJOR.$MINOR
   ```
2. Open a pull request to the `$MAJOR.$MINOR` branch that
   * includes the bug fixes,
   * finalizes the release notes similar to a regular release,
   * increments `_PKG_VERSION_PATCH` and `_LIB_VERSION_REVISION` in `configure.ac`
     and the `$PATCH` component of `project(libsecp256k1 VERSION ...)` and `${PROJECT_NAME}_LIB_VERSION_REVISION` in `CMakeLists.txt`
     (with commit message `"release: bump versions for $MAJOR.$MINOR.$PATCH"`, for example).
3. Perform the [sanity checks](#sanity-checks) on the PR branch.
4. After the PRs are merged, update the release branch, tag the commit, and push the tag:
   ```
   git checkout $MAJOR.$MINOR && git pull
   git tag -s v$MAJOR.$MINOR.$PATCH -m "libsecp256k1 $MAJOR.$MINOR.$PATCH"
   git push git@github.com:tortoisecoin-core/secp256k1.git v$MAJOR.$MINOR.$PATCH
   ```
6. Create a new GitHub release with a link to the corresponding entry in [CHANGELOG.md](../CHANGELOG.md).
7. Send an announcement email to the tortoisecoin-dev mailing list.
8. Open PR to the master branch that includes a commit (with commit message `"release notes: add $MAJOR.$MINOR.$PATCH"`, for example) that adds release notes to [CHANGELOG.md](../CHANGELOG.md).
