# MacOS Deployment

The `macdeployqtplus` script should not be run manually. Instead, after building as usual:

```bash
make deploy
```

During the deployment process, the disk image window will pop up briefly
when the fancy settings are applied. This is normal, please do not interfere,
the process will unmount the DMG and cleanup before finishing.

When complete, it will have produced `Litecoin-Qt.dmg`.

## SDK Extraction

### Step 1: Obtaining `Xcode.app`

Our current macOS SDK
(`Xcode-11.3.1-11C505-extracted-SDK-with-libcxx-headers.tar.gz`) can be
extracted from
[Xcode_11.3.1.xip](https://download.developer.apple.com/Developer_Tools/Xcode_11.3.1/Xcode_11.3.1.xip).
An Apple ID is needed to download this.

After Xcode version 7.x, Apple started shipping the `Xcode.app` in a `.xip`
archive. This makes the SDK less-trivial to extract on non-macOS machines. One
approach (tested on Debian Buster) is outlined below:

```bash
# Install/clone tools needed for extracting Xcode.app
apt install cpio
git clone https://github.com/bitcoin-core/apple-sdk-tools.git

# Unpack Xcode_11.3.1.xip and place the resulting Xcode.app in your current
# working directory
python3 apple-sdk-tools/extract_xcode.py -f Xcode_11.3.1.xip | cpio -d -i
```

On macOS the process is more straightforward:

```bash
xip -x Xcode_11.3.1.xip
```

### Step 2: Generating `Xcode-11.3.1-11C505-extracted-SDK-with-libcxx-headers.tar.gz` from `Xcode.app`

To generate `Xcode-11.3.1-11C505-extracted-SDK-with-libcxx-headers.tar.gz`, run
the script [`gen-sdk`](./gen-sdk) with the path to `Xcode.app` (extracted in the
previous stage) as the first argument.

```bash
# Generate a Xcode-11.3.1-11C505-extracted-SDK-with-libcxx-headers.tar.gz from
# the supplied Xcode.app
./contrib/macdeploy/gen-sdk '/path/to/Xcode.app'
```

## Deterministic macOS DMG Notes
Working macOS DMGs are created in Linux by combining a recent `clang`, the Apple
`binutils` (`ld`, `ar`, etc) and DMG authoring tools.

Apple uses `clang` extensively for development and has upstreamed the necessary
functionality so that a vanilla clang can take advantage. It supports the use of `-F`,
`-target`, `-mmacosx-version-min`, and `--sysroot`, which are all necessary when
building for macOS.

Apple's version of `binutils` (called `cctools`) contains lots of functionality missing in the
FSF's `binutils`. In addition to extra linker options for frameworks and sysroots, several
other tools are needed as well such as `install_name_tool`, `lipo`, and `nmedit`. These
do not build under Linux, so they have been patched to do so. The work here was used as
a starting point: [mingwandroid/toolchain4](https://github.com/mingwandroid/toolchain4).

In order to build a working toolchain, the following source packages are needed from
Apple: `cctools`, `dyld`, and `ld64`.

These tools inject timestamps by default, which produce non-deterministic binaries. The
`ZERO_AR_DATE` environment variable is used to disable that.

This version of `cctools` has been patched to use the current version of `clang`'s headers
and its `libLTO.so` rather than those from `llvmgcc`, as it was originally done in `toolchain4`.

To complicate things further, all builds must target an Apple SDK. These SDKs are free to
download, but not redistributable. To obtain it, register for an Apple Developer Account,
then download [Xcode_11.3.1](https://download.developer.apple.com/Developer_Tools/Xcode_11.3.1/Xcode_11.3.1.xip).

This file is many gigabytes in size, but most (but not all) of what we need is
contained only in a single directory:

```bash
Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
```

See the SDK Extraction notes above for how to obtain it.

The Gitian descriptors build 2 sets of files: Linux tools, then Apple binaries which are
created using these tools. The build process has been designed to avoid including the
SDK's files in Gitian's outputs. All interim tarballs are fully deterministic and may be freely
redistributed.

`genisoimage` is used to create the initial DMG. It is not deterministic as-is, so it has been
patched. A system `genisoimage` will work fine, but it will not be deterministic because
the file-order will change between invocations. The patch can be seen here: [cdrkit-deterministic.patch](https://github.com/bitcoin/bitcoin/blob/master/depends/patches/native_cdrkit/cdrkit-deterministic.patch).
No effort was made to fix this cleanly, so it likely leaks memory badly, however it's only used for
a single invocation, so that's no real concern.

`genisoimage` cannot compress DMGs, so afterwards, the DMG tool from the
`libdmg-hfsplus` project is used to compress it. There are several bugs in this tool and its
maintainer has seemingly abandoned the project.

The DMG tool has the ability to create DMGs from scratch as well, but this functionality is
broken. Only the compression feature is currently used. Ideally, the creation could be fixed
and `genisoimage` would no longer be necessary.

Background images and other features can be added to DMG files by inserting a
`.DS_Store` before creation. This is generated by the script `contrib/macdeploy/custom_dsstore.py`.

As of OS X 10.9 Mavericks, using an Apple-blessed key to sign binaries is a requirement in
order to satisfy the new Gatekeeper requirements. Because this private key cannot be
shared, we'll have to be a bit creative in order for the build process to remain somewhat
deterministic. Here's how it works:

- Builders use Gitian to create an unsigned release. This outputs an unsigned DMG which
  users may choose to bless and run. It also outputs an unsigned app structure in the form
  of a tarball, which also contains all of the tools that have been previously (deterministically)
  built in order to create a final DMG.
- The Apple keyholder uses this unsigned app to create a detached signature, using the
  script that is also included there. Detached signatures are available from this [repository](https://github.com/bitcoin-core/bitcoin-detached-sigs).
- Builders feed the unsigned app + detached signature back into Gitian. It uses the
  pre-built tools to recombine the pieces into a deterministic DMG.
