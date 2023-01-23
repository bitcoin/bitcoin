# Release Process

1. Open PR to master that
   1. adds release notes to `doc/CHANGELOG.md` and
   2. if this is **not** a patch release, updates `_PKG_VERSION_{MAJOR,MINOR}` and `_LIB_VERSIONS_*` in `configure.ac`
2. After the PR is merged,
   * if this is **not** a patch release, create a release branch with name `MAJOR.MINOR`.
     Make sure that the branch contains the right commits.
     Create commit on the release branch that sets `_PKG_VERSION_IS_RELEASE` in `configure.ac` to `true`.
   * if this **is** a patch release, open a pull request with the bugfixes to the `MAJOR.MINOR` branch.
     Also include the release note commit bump `_PKG_VERSION_BUILD` and `_LIB_VERSIONS_*` in `configure.ac`.
4. Tag the commit with `git tag -s vMAJOR.MINOR.PATCH`.
5. Push branch and tag with `git push origin --tags`.
6. Create a new GitHub release with a link to the corresponding entry in `doc/CHANGELOG.md`.
