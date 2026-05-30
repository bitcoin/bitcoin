BUILD BITCOIN CORE DEPENDS ON WINDOWS WITH MSYS2
===========================

This document explains how to build the Bitcoin Core dependencies in the
`depends/` tree natively on Windows using [MSYS2](https://www.msys2.org/), and
then configure and build `bitcoind` and `bitcoin-qt` against them.

This is the third "option which may work but is not extensively tested" listed
in [build-windows.md](build-windows.md). The officially-supported Windows paths
remain (a) cross-compiling from Linux/WSL with Mingw-w64 and (b) the native
MSVC + vcpkg flow in [build-windows-msvc.md](build-windows-msvc.md). The MSYS2
flow documented here uses the **same `depends/` makefiles** as the Linux
cross-compile — nothing in `depends/` needs to be modified.

Scope: only the dependencies needed to build `bitcoind` and `bitcoin-qt`
(Boost, libevent, SQLite, Qt + qrencode, and ZeroMQ). The IPC/multiprocess
packages (Cap'n Proto) are disabled by default on `mingw32` hosts, which is what
we use.


Why this works (and the one gotcha)
------------------

The `depends` build is "builder and host agnostic": the *builder* tools (the
`gcc`/`tar`/`sha256sum` that run on your machine) are decoupled from the *host*
toolchain (the compiler that produces the Windows binaries). See
[depends/description.md](../depends/description.md).

MSYS2 ships a complete native Mingw-w64 toolchain whose driver binaries are
named with the standard `x86_64-w64-mingw32-` prefix (e.g.
`x86_64-w64-mingw32-gcc`). That is exactly the `HOST` triplet the depends system
already supports for the Linux cross-compile (`depends/hosts/mingw32.mk`).

**The gotcha:** you cannot just run `make` and let it auto-detect the host. In a
native MSYS2 shell `./config.guess` reports `x86_64-pc-msys` (MSYS shell) or
`x86_64-pc-mingw64` (MINGW64/UCRT64 shell). The depends `Makefile` only
recognizes the substring `mingw32` as a Windows OS
([depends/Makefile](../depends/Makefile) line ~100), so an auto-detected native
build has no matching makefile and will fail. There are **two** auto-detected
values to override, and you must set both:

- **`HOST`** — the target you build *for*. Pass `HOST=x86_64-w64-mingw32`
  (the same triplet for both the UCRT64 and MINGW64 shells — see
  [Choose an environment](#1-choose-an-environment)). Without it, depends has no
  matching `hosts/*.mk`.
- **`BUILD`** — the machine you build *on*. `config.guess` reports
  `x86_64-pc-mingw64`, from which depends derives `build_os=mingw64` and tries to
  `include builders/mingw64.mk` ([depends/Makefile](../depends/Makefile) line
  ~117), which does not exist. The build-side GNU tools MSYS2 provides
  (`sha256sum`, `curl`, `tar` under `/usr/bin`) match the Linux builder profile,
  so pass `BUILD=x86_64-pc-linux-gnu` to load `builders/linux.mk`. Without this
  you get errors like `builders/mingw64.mk: No such file or directory` and, as
  fallout, empty commands such as `echo -n "..." |  | cut ...` (because
  `build_SHA256SUM` was never set).

With both set, the build runs as a "cross-compile" where the build machine just
happens to be the same Windows box.

`depends/hosts/mingw32.mk` first looks for a `*-gcc-posix` compiler (a
Debian-only naming convention); MSYS2 does not use that suffix, so the lookup
falls through to `depends/hosts/default.mk`, which uses the plain prefixed
`x86_64-w64-mingw32-gcc`. That is the compiler MSYS2 provides, so no makefile
changes are required.


1. Choose an environment
---------------

MSYS2 has several environments (see <https://www.msys2.org/docs/environments/>).
What matters here is which C runtime your produced binaries link against:

| MSYS2 shell | C runtime | depends `HOST` to pass  |
| ----------- | --------- | -------------- |
| **UCRT64**  | UCRT      | `x86_64-w64-mingw32`    |
| MINGW64     | MSVCRT    | `x86_64-w64-mingw32`    |

UCRT64 is the MSYS2-recommended environment, and MINGW64 is being deprecated
upstream (announced for 2026-03-15). Prefer **UCRT64**.

**Use `HOST=x86_64-w64-mingw32` for both** — do *not* use the
`x86_64-w64-mingw32ucrt` triplet. That `*ucrt` triplet is a Debian/Ubuntu
naming convention (the `g++-mingw-w64-ucrt64` package installs binaries literally
named `x86_64-w64-mingw32ucrt-g++`). MSYS2 does **not** do this: both its UCRT64
and MINGW64 toolchains use the plain `x86_64-w64-mingw32-` binary prefix, and the
C runtime is selected by *which install tree you run from* (`/ucrt64` vs
`/mingw64`), not by the triplet string. Passing `*ucrt` to depends makes it look
for `x86_64-w64-mingw32ucrt-g++`, which does not exist, and CMake fails with
"Could not find the compiler specified in the environment variable CXX:
x86_64-w64-mingw32ucrt-g++". So: launch the **UCRT64** shell (for UCRT binaries)
but still pass `HOST=x86_64-w64-mingw32`.

Do all of the steps below from the chosen environment's shell (launch
"MSYS2 UCRT64" from the Start Menu, *not* the plain "MSYS2 MSYS" shell — the
plain MSYS shell does not have the Mingw-w64 toolchain on its `PATH`). Make sure
`$MSYSTEM` actually matches the shell you opened (it should print `UCRT64`); a
mismatched `MSYSTEM` with a different tree on `PATH` causes confusing failures.


2. Install the toolchain and build tools
------------------------------

For UCRT64:

```bash
pacman -Syu                 # update; may ask you to reopen the shell, then run again
pacman -S --needed \
    base-devel \
    git \
    mingw-w64-ucrt-x86_64-toolchain \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-pkgconf \
    mingw-w64-ucrt-x86_64-python \
    bison \
    zip \
    mingw-w64-ucrt-x86_64-nsis
```

For MINGW64, substitute the `ucrt-x86_64` package prefix with `x86_64`
(e.g. `mingw-w64-x86_64-toolchain`).

Notes:
- `base-devel` provides `make`, `patch`, `curl`, `tar`, `sha256sum`, etc. —
  the builder-side tools the depends Makefiles invoke.
- `bison`, `ninja`, `pkgconf`, and `python` are only needed if you build the GUI
  (Qt). If you will pass `NO_QT=1`, you can omit them.
- The depends system invokes `curl` to download sources; `base-devel` includes
  it.

Confirm the host compiler is on the `PATH`:

```bash
x86_64-w64-mingw32-gcc --version
```


### Create prefixed binutils (required)

MSYS2 ships its **compilers** under the `x86_64-w64-mingw32-` prefix
(`x86_64-w64-mingw32-gcc`, `-g++`) but its **binutils** only *unprefixed*
(`ar`, `nm`, `ranlib`, `strip`, `objcopy`, `objdump`). The depends system — and
the generated `toolchain.cmake` it later hands to the Bitcoin Core build — expect
the prefixed names (`x86_64-w64-mingw32-ar`, etc.). Without them, configuring a
package fails with an empty archiver command, e.g.:

```
which: no x86_64-w64-mingw32-ar in (...)
...
    "" qc CMakeFiles/cmTC_xxxxx.dir/objects.a ...
    /bin/sh: line 1: : command not found
  The C++ compiler ... is not able to compile a simple test program.
```

Create the prefixed names once (they are the same UCRT64 tools). Copies are used
rather than symlinks because MSYS2 symlinks to `.exe` are unreliable:

```bash
BIN=/ucrt64/bin         # use /mingw64/bin in the MINGW64 shell
for t in ar nm ranlib strip objcopy objdump dlltool windres; do
    [ -e "$BIN/x86_64-w64-mingw32-$t.exe" ] || cp "$BIN/$t.exe" "$BIN/x86_64-w64-mingw32-$t.exe"
done
```

Verify:

```bash
for t in ar nm ranlib strip; do command -v x86_64-w64-mingw32-$t; done
```


3. Build the dependencies
---------------

From the repository root, in the UCRT64 shell (same command for MINGW64):

```bash
make -C depends BUILD=x86_64-pc-linux-gnu HOST=x86_64-w64-mingw32 MSYS_STAGING=1 -j"$(nproc)"
```

Three flags are required on every invocation:

- `BUILD=x86_64-pc-linux-gnu` — selects the GNU builder profile; see
  [Why this works](#why-this-works-and-the-one-gotcha) above.
- `HOST=x86_64-w64-mingw32` — the Windows target. The UCRT vs MSVCRT runtime is
  decided by which shell you launched, not by the triplet, so this value is
  correct in both UCRT64 and MINGW64.
- `MSYS_STAGING=1` — fixes a native-Windows staging-path problem. CMake's
  `DESTDIR` install on Windows drops the leading drive letter from the install
  prefix (a package configured with prefix `/c/.../<host>` installs to
  `<staging>/Users/...` instead of the `<staging>/c/Users/...` that depends'
  postprocess/cache steps expect), which otherwise fails with errors such as
  `rm: cannot remove 'include/ev*.h': No such file or directory`. With
  `MSYS_STAGING=1`, depends relocates each package's staged files back to the
  expected location after install. It is a no-op (and harmless) on non-Windows
  builds. Autotools/b2 packages are unaffected either way; this matters for the
  CMake-based packages (libevent, zeromq, qrencode, expat, freetype).

To build without the GUI (smaller, fewer dependencies):

```bash
make -C depends BUILD=x86_64-pc-linux-gnu HOST=x86_64-w64-mingw32 MSYS_STAGING=1 NO_QT=1 -j"$(nproc)"
```

Helpful options (full list in [depends/README.md](../depends/README.md#dependency-options)):
- `NO_QT=1`     skip Qt + qrencode (no `bitcoin-qt`)
- `NO_ZMQ=1`    skip ZeroMQ
- `NO_WALLET=1` skip SQLite (no wallet)
- `LOG=1`       per-package log files, printed on failure (very useful for triage)

`NO_IPC` defaults to `1` on `mingw32` hosts, so Cap'n Proto / libmultiprocess
are skipped automatically — no action needed for the `bitcoind` + `bitcoin-qt`
goal.

On success the build prints the path to a generated toolchain file, e.g.:

```
depends/x86_64-w64-mingw32/toolchain.cmake
```


4. Configure and build Bitcoin Core against the depends output
---------------------------------

Point CMake at the generated toolchain file (adjust the triplet to match what
you built):

```bash
export MSYS_STAGING=1
cmake -B build --toolchain depends/x86_64-w64-mingw32/toolchain.cmake
cmake --build build -j"$(nproc)"
```

The resulting `bitcoind.exe` and `bitcoin-qt.exe` will be under `build/bin/`.

If you intend to build the NSIS `.exe` installer in step 5, export
`MSYS_STAGING=1` **before** the `cmake -B build` configure above (the same opt-in
variable used by the depends build). It gates an MSYS2-only fix-up of the
generated NSIS script; see [Optional: the NSIS `.exe`
installer](#optional-the-nsis-exe-installer). It only affects installer
generation, so it is harmless to set it always and not needed if you only want
the `.zip`


5. (Optional) Produce a `bitcoin-31.0-win64-unsigned.zip` archive
---------------------------------

The official release archives (e.g. `bitcoin-31.0-win64-unsigned.zip`) are
produced by the deterministic [Guix build](../contrib/guix/README.md), not by
this MSYS2 flow. The steps below reproduce a **functionally equivalent**
distribution `.zip` from your local MSYS2 build. The result is *not* bit-for-bit
reproducible and *not* code-signed — the `-unsigned` suffix reflects that the
release process signs these archives in a later, separate step. This is useful
for packaging your own build to copy to another machine; for an official,
verifiable release use Guix instead.

This stage uses `zip`, which is not part of `base-devel`; install it once:

```bash
pacman -S --needed zip
```

Run from the repository root, after a successful step 4. Both the name prefix and
the version are parsed straight from `CMakeLists.txt` so they stay correct across
version bumps (and forks) — no need to edit anything by hand. The prefix is the
lowercased first word of `CLIENT_NAME` (`"Bitcoin Core"` → `bitcoin`). The version
uses `MAJOR.MINOR` and appends `.BUILD` only when the build component is non-zero,
matching the release-archive convention (`31.0`, but e.g. `31.1.2` for a point
release):

```bash
name=$(sed -n 's/^set(CLIENT_NAME "\([^"]*\)").*/\1/p' CMakeLists.txt)
prefix=$(echo "$name" | awk '{print tolower($1)}')         # "Bitcoin Core" -> bitcoin
maj=$(sed -n 's/^set(CLIENT_VERSION_MAJOR \([0-9]\+\))/\1/p' CMakeLists.txt)
min=$(sed -n 's/^set(CLIENT_VERSION_MINOR \([0-9]\+\))/\1/p' CMakeLists.txt)
bld=$(sed -n 's/^set(CLIENT_VERSION_BUILD \([0-9]\+\))/\1/p' CMakeLists.txt)
VERSION="${maj}.${min}"; [ "$bld" != 0 ] && VERSION="${VERSION}.${bld}"
DISTNAME="${prefix}-${VERSION}"
echo "Packaging ${DISTNAME}"            # sanity-check: prints "Packaging bitcoin-31.0"

# 1. Install the build into a clean, named staging tree (mirrors the release
#    layout: binaries under bin/, data under share/). --strip drops debug info
#    so the binaries are release-sized.
rm -rf dist
cmake --install build --strip --prefix "$(pwd)/dist/${DISTNAME}"

# 2. Add the same supplemental files the release archives ship with.
cp doc/README_windows.txt         "dist/${DISTNAME}/readme.txt"
cp share/examples/${prefix}.conf  "dist/${DISTNAME}/"
mkdir -p "dist/${DISTNAME}/share"
cp -r share/rpcauth               "dist/${DISTNAME}/share/"

# 3. Zip it up. -X omits extra file attributes; sorting the file list keeps the
#    archive order stable across runs.
( cd dist && find "${DISTNAME}" -not -name '*.dbg' | sort | zip -X@ "../${DISTNAME}-win64-unsigned.zip" )
```

This produces `bitcoin-31.0-win64-unsigned.zip` in the repository root, with the
contents laid out under a top-level `bitcoin-31.0/` directory just like the
official archive.

### Optional: the NSIS `.exe` installer

The release process also produces a `bitcoin-31.0-win64-setup-unsigned.exe`
installer via the CMake `deploy` target, which requires NSIS (`makensis`). If you
want it, install NSIS and rerun the target:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-nsis   # use the x86_64 prefix in MINGW64
```

```bash
cmake --build build -t deploy
# Produces build/bitcoin-win64-setup.exe; rename to match the release convention:
mv build/${prefix}-win64-setup.exe "${prefix}-${VERSION}-win64-setup-unsigned.exe"
```

If `makensis` is genuinely not installed, the `deploy` target prints `Error: NSIS
not found` and does nothing — the `.zip` above does not need it.

**Run this from a true UCRT64 shell.** The `-U` reconfigure triggers a CMake
*generate* step, which runs a smoke test of the native Qt host tool
(`depends/x86_64-w64-mingw32/native/bin/uic.exe -h`) for the GUI's AUTOUIC. That
tool is UCRT-linked, so it needs the UCRT64 runtime DLLs ahead of any other
toolchain on `PATH`. In a mismatched shell (e.g. `MSYSTEM=MINGW64` with
`/mingw64/bin` before `/ucrt64/bin` on `PATH`) it loads the wrong, ABI-incompatible
DLLs and CMake fails with:

```
CMake Error: AUTOUIC for target bitcoinqt: Test run of "uic" executable
"...\native\bin\uic.exe" failed.
Exit code 0xc0000139
```

`0xc0000139` is the Windows "entry point not found" status — a DLL/ABI mismatch,
not a corrupt build. Fix it by launching the **MSYS2 UCRT64** shell from the Start
Menu (confirm with `echo "$MSYSTEM"` → `UCRT64`) and re-running; do not rely on a
MINGW64 shell with `/ucrt64/bin` merely appended. As a one-off you can force the
correct DLLs for a single command by prepending the tree, e.g.
`PATH="/c/msys64/ucrt64/bin:$PATH" cmake -B build -U MAKENSIS_EXECUTABLE`, but the
clean UCRT64 shell is the reliable fix (the same DLL-resolution issue affects
running `bitcoin-qt.exe` itself).

Once configuration succeeds in a UCRT64 shell, `cmake --build build -t deploy`
strips the binaries and runs `makensis`, producing `build/bitcoin-win64-setup.exe`.

**MSYS2 `makensis` path quirk (gated on `MSYS_STAGING`).** The native Windows
`makensis` is stricter about path separators than the Linux `makensis` the
official Guix build uses: its `File` directive and the MUI bitmap/icon paths
reject forward slashes, failing with `File: "...nsis-wizard.bmp" -> no files
found` and `Error in macro MUI_PAGE_WELCOME ... aborting`. The NSI template
([share/setup.nsi.in](../share/setup.nsi.in)) uses forward-slash paths (correct
for Linux), so `cmake/module/GenerateSetupNsi.cmake` rewrites the project-rooted
path tokens in the generated `bitcoin-win64-setup.nsi` to native backslashes,
leaving URLs, NSIS flags, and `$INSTDIR` paths untouched. This is a local change
(not upstream).

This fix-up is gated on the `MSYS_STAGING` environment variable — the same opt-in
knob as the depends build — so it is a no-op for every other build. The CMake
build reads it from the environment (it is not a CMake cache variable), and the
NSI is generated at **configure** time, so you must `export MSYS_STAGING=1`
*before* the `cmake -B build` in step 4. If you already configured without it,
re-run configure with the variable exported (e.g.
`export MSYS_STAGING=1 && cmake -B build`). If you see the `no files found` error,
this is almost certainly the cause: `MSYS_STAGING` was not set in the environment
when the build was configured.


Troubleshooting
---------------

- **Capture the build output to a log file.** When a build fails it is easiest to
  save the full output to a file you can search (or attach to a bug report).
  Build errors and verbose compiler output go to *stderr*, so redirect both
  streams. Use `tee` to watch progress on screen *and* write the log:

  ```bash
  make -C depends BUILD=x86_64-pc-linux-gnu HOST=x86_64-w64-mingw32 MSYS_STAGING=1 -j"$(nproc)" 2>&1 | tee build.log
  ```

  `2>&1` merges stderr into stdout; `tee build.log` writes it to the file and the
  terminal (`tee -a build.log` appends instead of overwriting). To save quietly
  without on-screen output, redirect directly: `... > build.log 2>&1`.

  Two notes specific to depends:
  - depends already hides each package's detailed output behind short status
    lines (`Configuring qt...`, `Building sqlite...`); add `V=1` to the make
    command to include that detail in the log, and `LOG=1` to also keep
    per-package log files (printed automatically on failure).
  - With `-j"$(nproc)"`, output from parallel jobs interleaves and is harder to
    read. When tracing a single failing package, re-run with `-j1` for a clean,
    ordered log.

- **`builders/mingw64.mk: No such file or directory`, plus noise like
  `echo -n "..." |  | cut ...` and `gen_id: -: command not found`.** You omitted
  `BUILD=...`, so depends derived `build_os=mingw64` from `config.guess` and
  failed to load the builder profile (which left `build_SHA256SUM` empty, causing
  the empty-pipe errors). Add `BUILD=x86_64-pc-linux-gnu` to the make command.

- **`No rule to make target 'hosts/mingw64.mk'` or no packages selected.** You
  forgot `HOST=...`. Always pass `HOST=x86_64-w64-mingw32`.

- **CMake: `Could not find the compiler specified in the environment variable
  CXX: x86_64-w64-mingw32ucrt-g++`** (or any `x86_64-w64-mingw32ucrt-*` not
  found). You passed `HOST=x86_64-w64-mingw32ucrt`. MSYS2's compilers are named
  `x86_64-w64-mingw32-*` (no `ucrt`) in *both* the UCRT64 and MINGW64 trees — the
  `*ucrt` triplet is Debian-only. Use `HOST=x86_64-w64-mingw32`; launch the
  UCRT64 shell if you want UCRT binaries.

- **`which: no x86_64-w64-mingw32-ar`, then `"" qc ...` /
  `/bin/sh: line 1: : command not found` / "compiler is not able to compile a
  simple test program".** The prefixed binutils are missing — CMake's
  `CMAKE_AR` resolved to empty. Run the binutils-copy step in
  [Create prefixed binutils](#create-prefixed-binutils-required), then re-run.

- **`x86_64-w64-mingw32-gcc: command not found`.** You are in the plain MSYS
  shell, or the toolchain is not installed. Use the UCRT64/MINGW64 shell and
  install `mingw-w64-ucrt-x86_64-toolchain`.

- **`rm: cannot remove 'include/ev*.h': No such file or directory`** (or a
  package's postprocess/cache step acting on an empty directory), with install
  logs showing a doubled path like `<staging>/.../<ver>/Users/User/.../include`.
  The staged files landed at the drive-stripped path. Add `MSYS_STAGING=1` to the
  make command (see step 3). If you started the build without it, run
  `make -C depends clean` first so the affected packages restage.

- **Source paths / autoconf failures with spaces or odd characters.** Keep the
  checkout under a short path with no spaces (e.g. `C:\src\bitcoin`).

- **A package fails to build.** Re-run with `LOG=1` to get the failing
  package's log printed automatically, and you can rebuild a single package's
  stage for debugging (see [depends/packages.md](../depends/packages.md#build-targets)),
  e.g. `make -C depends BUILD=x86_64-pc-linux-gnu HOST=x86_64-w64-mingw32 MSYS_STAGING=1 libevent_configured`.

- **Reset a partial build.** `make -C depends clean BUILD=x86_64-pc-linux-gnu` removes the work and build
  caches; `make -C depends clean-all BUILD=x86_64-pc-linux-gnu` also removes downloaded sources and the
  per-host output directory.

- **Clean the Bitcoin Core CMake build (step 4 output).** This is separate from
  the depends caches above — it only affects the `build/` directory created in
  [step 4](#4-configure-and-build-bitcoin-core-against-the-depends-output), not
  the dependencies. Pick the level you need:

  - *Recompile only* (keep the existing configuration) — recompiles changed
    sources, fastest:

    ```bash
    cmake --build build --target clean   # delete compiled objects/binaries
    cmake --build build -j"$(nproc)"     # rebuild
    ```

  - *Re-configure without deleting the tree* — picks up changed CMake options
    or a refreshed depends toolchain by reusing the build dir but discarding the
    cache:

    ```bash
    cmake -B build --fresh --toolchain depends/x86_64-w64-mingw32/toolchain.cmake
    cmake --build build -j"$(nproc)"
    ```

  - *Full clean* (recommended after changing the depends output, the toolchain
    file, or the compiler) — delete the whole build dir and reconfigure from
    scratch:

    ```bash
    rm -rf build
    cmake -B build --toolchain depends/x86_64-w64-mingw32/toolchain.cmake
    cmake --build build -j"$(nproc)"
    ```

  When in doubt, the full clean (`rm -rf build`) is the most reliable — a stale
  `build/CMakeCache.txt` is a common cause of confusing reconfigure errors,
  especially after rebuilding depends. Adjust the `x86_64-w64-mingw32` triplet to
  match what you built.
