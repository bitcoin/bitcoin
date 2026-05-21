### Build System

- The undocumented `BITCOIN_GENBUILD_NO_GIT` environment variable is no longer
  required and has been removed. The build system now automatically detects
  when it is being built from a source archive or as a subproject of a git-aware
  parent project and skips git metadata fetching.

  Users who need to disable git execution explicitly can still do so by
  configuring with `-DCMAKE_DISABLE_FIND_PACKAGE_Git=ON`.
