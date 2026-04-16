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
[Xcode_26.1.1_Apple_silicon.xip](https://download.developer.apple.com/Developer_Tools/Xcode_26.1.1/Xcode_26.1.1_Apple_silicon.xip).

Alternatively, after logging in to your account go to 'Downloads', then 'More'
and search for [`Xcode 26.1.1`](https://developer.apple.com/download/all/?q=Xcode%2026.1.1).

An Apple ID and cookies enabled for the hostname are needed to download this.

The `sha256sum` of the downloaded XIP archive should be `f4c65b01e2807372b61553c71036dbfef492d7c79d4c380a5afb61aa1018e555`.

To extract the `.xip` on Linux:

```bash
# Install/clone tools needed for extracting Xcode.app
apt install cpio
git clone https://github.com/bitcoin-core/apple-sdk-tools.git

# Unpack the .xip and place the resulting Xcode.app in your current
# working directory
python3 apple-sdk-tools/extract_xcode.py -f Xcode_26.1.1_Apple_silicon.xip | cpio -d -i
```

On macOS:

```bash
xip -x Xcode_26.1.1_Apple_silicon.xip
```

### Step 2: Generating the SDK tarball from `Xcode.app`

To generate the SDK, run the script [`gen-sdk.py`](./gen-sdk.py) with the
path to `Xcode.app` (extracted in the previous stage) as the first argument.

```bash
./contrib/macdeploy/gen-sdk.py '/path/to/Xcode.app'
```

The generated archive should be: `Xcode-26.1.1-17B100-extracted-SDK-with-libcxx-headers.tar`.
The `sha256sum` should be `9600fa93644df674ee916b5e2c8a6ba8dacf631996a65dc922d003b98b5ea3b1`.

## Deterministic macOS App Notes

macOS Applications are created on Linux using a recent LLVM.

All builds must target an Apple SDK. These SDKs are free to download, but not redistributable.
See the SDK Extraction notes above for how to obtain it.

The Guix build process has been designed to avoid including the SDK's files in Guix's outputs.
All interim tarballs are fully deterministic and may be freely redistributed.

Using an Apple-blessed key to sign binaries is a requirement to produce (distributable) macOS
binaries. Because this private key cannot be shared, we'll have to be a bit creative in order
for the build process to remain somewhat deterministic. Here's how it works:

- Builders use Guix to create an unsigned release. This outputs an unsigned ZIP which
  users may choose to bless, self-codesign, and run. It also outputs an unsigned app structure
  in the form of a tarball.
- The Apple keyholder uses this unsigned app to create a detached signature, using the
  included script. Detached signatures are available from this [repository](https://github.com/bitcoin-core/bitcoin-detached-sigs).
- Builders feed the unsigned app + detached signature back into Guix, which combines the
  pieces into a deterministic ZIP.
