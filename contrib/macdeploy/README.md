# MacOS Deployment

The `macdeployqtplus` script should not be run manually. Instead, after building as usual:

```bash
make deploy
```

When complete, it will have produced `Bitcoin-Core.zip`.

## SDK Extraction

### Step 1: Obtaining `Xcode.app`

A free Apple Developer Account is required to proceed.

Our macOS SDK can be extracted from
[Xcode_15.xip](https://download.developer.apple.com/Developer_Tools/Xcode_15/Xcode_15.xip).

Alternatively, after logging in to your account go to 'Downloads', then 'More'
and search for [`Xcode 15`](https://developer.apple.com/download/all/?q=Xcode%2015).

An Apple ID and cookies enabled for the hostname are needed to download this.

The `sha256sum` of the downloaded XIP archive should be `4daaed2ef2253c9661779fa40bfff50655dc7ec45801aba5a39653e7bcdde48e`.

To extract the `.xip` on Linux:

```bash
# Install/clone tools needed for extracting Xcode.app
apt install cpio
git clone https://github.com/bitcoin-core/apple-sdk-tools.git

# Unpack the .xip and place the resulting Xcode.app in your current
# working directory
python3 apple-sdk-tools/extract_xcode.py -f Xcode_15.xip | cpio -d -i
```

On macOS:

```bash
xip -x Xcode_15.xip
```

### Step 2: Generating the SDK tarball from `Xcode.app`

To generate the SDK, run the script [`gen-sdk`](./gen-sdk) with the
path to `Xcode.app` (extracted in the previous stage) as the first argument.

```bash
./contrib/macdeploy/gen-sdk '/path/to/Xcode.app'
```

The generated archive should be: `Xcode-15.0-15A240d-extracted-SDK-with-libcxx-headers.tar.gz`.
The `sha256sum` should be `c0c2e7bb92c1fee0c4e9f3a485e4530786732d6c6dd9e9f418c282aa6892f55d`.

## Deterministic macOS App Notes

macOS Applications are created in Linux by combining a recent `clang` and the Apple
`binutils` (`ld`, `ar`, etc).

Apple uses `clang` extensively for development and has upstreamed the necessary
functionality so that a vanilla clang can take advantage. It supports the use of `-F`,
`-target`, `-mmacosx-version-min`, and `-isysroot`, which are all necessary when
building for macOS.

Apple's version of `binutils` (called `cctools`) contains lots of functionality missing in the
FSF's `binutils`. In addition to extra linker options for frameworks and sysroots, several
other tools are needed as well. These do not build under Linux, so they have been patched to
do so. The work here was used as a starting point: [mingwandroid/toolchain4](https://github.com/mingwandroid/toolchain4).

In order to build a working toolchain, the following source packages are needed from
Apple: `cctools`, `dyld`, and `ld64`.

These tools inject timestamps by default, which produce non-deterministic binaries. The
`ZERO_AR_DATE` environment variable is used to disable that.

This version of `cctools` has been patched to use the current version of `clang`'s headers
and its `libLTO.so` rather than those from `llvmgcc`, as it was originally done in `toolchain4`.

To complicate things further, all builds must target an Apple SDK. These SDKs are free to
download, but not redistributable. See the SDK Extraction notes above for how to obtain it.

The Guix process builds 2 sets of files: Linux tools, then Apple binaries which are
created using these tools. The build process has been designed to avoid including the
SDK's files in Guix's outputs. All interim tarballs are fully deterministic and may be freely
redistributed.

As of OS X 10.9 Mavericks, using an Apple-blessed key to sign binaries is a requirement in
order to satisfy the new Gatekeeper requirements. Because this private key cannot be
shared, we'll have to be a bit creative in order for the build process to remain somewhat
deterministic. Here's how it works:

- Builders use Guix to create an unsigned release. This outputs an unsigned ZIP which
  users may choose to bless and run. It also outputs an unsigned app structure in the form
  of a tarball.
- The Apple keyholder uses this unsigned app to create a detached signature, using the
  script that is also included there. Detached signatures are available from this [repository](https://github.com/bitcoin-core/bitcoin-detached-sigs).
- Builders feed the unsigned app + detached signature back into Guix. It uses the
  pre-built tools to recombine the pieces into a deterministic ZIP.
