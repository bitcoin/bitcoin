# macOS Build Instructions and Notes

The commands in this guide should be executed in a Terminal application.
The built-in one is located in
```
/Applications/Utilities/Terminal.app
```

## Preparation
Install the macOS command line tools:

```shell
xcode-select --install
```

When the popup appears, click `Install`.

Then install [Homebrew](https://brew.sh).

## Dependencies
```shell
brew install automake berkeley-db4 libtool boost miniupnpc openssl pkg-config python qt libevent qrencode
```

See [dependencies.md](dependencies.md) for a complete overview.

If you want to build the disk image with `make deploy` (.dmg / optional), you need RSVG:
```shell
brew install librsvg
```

## Berkeley DB
It is recommended to use Berkeley DB 4.8. If you have to build it yourself,
you can use [this](/contrib/install_db4.sh) script to install it
like so:

```shell
./contrib/install_db4.sh .
```

from the root of the repository.

**Note**: You only need Berkeley DB if the wallet is enabled (see [*Disable-wallet mode*](/doc/build-osx.md#disable-wallet-mode)).

## Build Bitcoin Core

1. Clone the Bitcoin Core source code:
    ```shell
    git clone https://github.com/bitcoin/bitcoin
    cd bitcoin
    ```

2.  Build Bitcoin Core:

    Configure and build the headless Bitcoin Core binaries as well as the GUI (if Qt is found).

    You can disable the GUI build by passing `--without-gui` to configure.
    ```shell
    ./autogen.sh
    ./configure
    make
    ```

3.  It is recommended to build and run the unit tests:
    ```shell
    make check
    ```

4.  You can also create a  `.dmg` that contains the `.app` bundle (optional):
    ```shell
    make deploy
    ```

## `disable-wallet` mode
When the intention is to run only a P2P node without a wallet, Bitcoin Core may be
compiled in `disable-wallet` mode with:
```shell
./configure --disable-wallet
```

In this case there is no dependency on Berkeley DB 4.8.

Mining is also possible in disable-wallet mode using the `getblocktemplate` RPC call.

## Running
Bitcoin Core is now available at `./src/bitcoind`

Before running, you may create an empty configuration file:
```shell
mkdir -p "/Users/${USER}/Library/Application Support/Bitcoin"

touch "/Users/${USER}/Library/Application Support/Bitcoin/bitcoin.conf"

chmod 600 "/Users/${USER}/Library/Application Support/Bitcoin/bitcoin.conf"
```

The first time you run bitcoind, it will start downloading the blockchain. This process could
take many hours, or even days on slower than average systems.

You can monitor the download process by looking at the debug.log file:
```shell
tail -f $HOME/Library/Application\ Support/Bitcoin/debug.log
```

## Other commands:
```shell
./src/bitcoind -daemon      # Starts the bitcoin daemon.
./src/bitcoin-cli --help    # Outputs a list of command-line options.
./src/bitcoin-cli help      # Outputs a list of RPC commands when the daemon is running.
```

## Notes
* Tested on OS X 10.10 Yosemite through macOS 10.14 Mojave on 64-bit Intel
processors only.
* Building with downloaded Qt binaries is not officially supported. See the notes in [#7714](https://github.com/bitcoin/bitcoin/issues/7714).

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
then download the [Xcode 7.3.1 dmg](https://developer.apple.com/devcenter/download.action?path=/Developer_Tools/Xcode_7.3.1/Xcode_7.3.1.dmg).

This file is several gigabytes in size, but only a single directory inside is needed:
```
Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk
```

Unfortunately, the usual Linux tools (7zip, hpmount, loopback mount) are incapable of
opening this file. To create a tarball suitable for Gitian input, there are two options:

Using macOS, you can mount the DMG, and then create it with:
```shell
hdiutil attach Xcode_7.3.1.dmg
tar -C /Volumes/Xcode/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/ -czf MacOSX10.11.sdk.tar.gz MacOSX10.11.sdk
```

Alternatively, you can use 7zip and SleuthKit to extract the files one by one. The script
[`extract-osx-sdk.sh`](./../contrib/macdeploy/extract-osx-sdk.sh) automates this. First
ensure the DMG file is in the current directory, and then run the script. You may wish to
delete the `intermediate 5.hfs` file and `MacOSX10.11.sdk` (the directory) when you've
confirmed the extraction succeeded.

```shell
apt-get install p7zip-full sleuthkit
contrib/macdeploy/extract-osx-sdk.sh
rm -rf 5.hfs MacOSX10.11.sdk
```

The Gitian descriptors build 2 sets of files: Linux tools, then Apple binaries which are
created using these tools. The build process has been designed to avoid including the
SDK's files in Gitian's outputs. All interim tarballs are fully deterministic and may be freely
redistributed.

`genisoimage` is used to create the initial DMG. It is not deterministic as-is, so it has been
patched. A system `genisoimage` will work fine, but it will not be deterministic because
the file-order will change between invocations. The patch can be seen here:  [theuni/osx-cross-depends](https://raw.githubusercontent.com/theuni/osx-cross-depends/master/patches/cdrtools/genisoimage.diff).
No effort was made to fix this cleanly, so it likely leaks memory badly. But it's only used for
a single invocation, so that's no real concern.

`genisoimage` cannot compress DMGs, so afterwards, the DMG tool from the
`libdmg-hfsplus` project is used to compress it. There are several bugs in this tool and its
maintainer has seemingly abandoned the project. It has been forked and is available
(with fixes) here: [theuni/libdmg-hfsplus](https://github.com/theuni/libdmg-hfsplus).

The DMG tool has the ability to create DMGs from scratch as well, but this functionality is
broken. Only the compression feature is currently used. Ideally, the creation could be fixed
and `genisoimage` would no longer be necessary.

Background images and other features can be added to DMG files by inserting a
`.DS_Store` before creation. This is generated by the script
`contrib/macdeploy/custom_dsstore.py`.

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

