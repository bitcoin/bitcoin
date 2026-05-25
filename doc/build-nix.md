# What is Nix

[Nix](https://nix.dev/) is a declarative package manager that simplifies
dependency management. It is used by the [NixOS](https://nixos.org/), a Linux
distribution, but Nix itself can be installed on other Linux distributions,
macOS, and Windows (with WSL).

Several things make Nix interesting. It's a lazy functional programming
language, which is used to write package recipes. Builds specified via package
recipes are not performed until they are actually needed. The Nix project
maintains a collection of package recipes called
[Nixpkgs](https://github.com/nixos/nixpkgs).

There is a centralized cache on the filesystem, called the Nix store, where
build artifacts are located. Being a deterministic system, the same package
recipe always results in the same build artifact (matching the same hash). So
the store helps you save time by avoiding rebuilding the same packages and
also optimizes storage. You can also share binary artifacts between machines.

Nix also supports ad-hoc build environments. You can easily create environments
supporting different library versions or compilers. This is very useful for
development and is the focus of this guide.

# Installation

[Download](https://nixos.org/download/) the Nix package manager and follow the
[manual](https://nix.dev/manual/nix/latest/) for how to install it. Note that
you will likely need to restart your shell at the end of the process. If done
correctly, `nix --version` and `nix-shell --version` should return the version
string.

# Building packages

Here is a simple `shell.nix` file to help you get started, which specifies the
mandatory dependencies:

```nix
with import <nixpkgs> {};

mkShell {
  packages = [
    cmake
    llvmPackages_latest.clang
    llvmPackages_latest.lld
    llvmPackages_latest.llvm
    sqlite
    capnproto
    boost
    pkg-config
    libevent
    python3
  ];

  shellHook = ''
    export CC=clang
    export CXX=clang++
  '';
}
```

The first time you run `nix-shell --pure`, the Nix expression specified in
`shell.nix` will be evaluated and packages will be installed. Once done, you'll
find yourself in an environment with the packages listed above and without
access to your host packages (hence the `--pure` flag). On subsequent runs,
`nix-shell --pure` will immediately drop you into this environment because the
packages will be in your local Nix store.

To check that you're inside the Nix shell, the environment variable
`IN_NIX_SHELL` can be used.

This should allow you to build the project using the LLVM/Clang toolchain and
run tests with coverage, as specified in [Developer Notes](developer-notes.md).
Please continue following instructions listed there.
