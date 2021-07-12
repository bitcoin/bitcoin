# Gitian

Prior to Bitcoin Core 22.0, gitian was the build system used to create reproducible builds.
Guix has been used since 22.0.
In order to ease the transition to guix, the gitian descriptors have been replaced with ones which setup and run guix inside of the gitian virtual machines.
This is not the recommended method for making Bitcoin Core release builds; new builders should setup and use guix directly.

## Security Model

Guix allows users to select their own security model, but these gitian descriptors do not give builders that option.
The security model implemented is the most trusting one - the guix installation script is used (which uses the prebuilt binaries) and packages are installed from the substitute servers when available.
Note that this security model is largely the same as the previous gitian builds as required packages are downloaded from third parties.
If this security model is insufficient, then we recommend that you install and use guix directly rather than these gitian descriptors.

## Usage

As these gitian descriptors are intended for existing gitian builders, we assume that the necessary repos have already been cloned at that builders are already familiar with the setup and build process.
However because the actual build system used is guix, the process is slightly different.

### Virtualization Notes

Guix uses some kernel features which are not always available with all of the virtualization methods supported by gitian.
These have been tested only with the Docker and KVM virtualization methods.
No changes are necessary to work with KVM.
For builders using Docker, [gitian-builder#251](https://github.com/devrandom/gitian-builder/pull/251) is required and the `GITIAN_ALLOW_PRIVILEGED=1` must be set:
```
export GITIAN_ALLOW_PRIVILEGED=1
```

### Prepare gitian-builder

1. Checkout [`guix.sigs`](https://github.com/bitcoin-core/guix.sigs)
2. Checkout [`gitian-builder`](https://github.com/devrandom/gitian-builder) and make sure it contains commit `9e97a4d5038cd61215f5243a37c06fa1734a276e`.
3. Install all of the prerequisites as per gitian's instructions.
4. Create a Ubuntu 18.04 base vm with `bin/make-base-vm --suite bionic --arch amd64` (include virtualization arguments as needed)

### Build

A build can be performed with (from the gitian-builder root):

```
bin/gbuild --commit bitcoin=<version> --allow-sudo ../bitcoin/guix-in-gitian/contrib/gitian-descriptors/gitian-guix-linux-win.yml
bin/gbuild --commit bitcoin=<version> --allow-sudo ../bitcoin/guix-in-gitian/contrib/gitian-descriptors/gitian-guix-mac.yml
```

Note that this comamnd includes `--allow-sudo`.
This is required in order for guix to work inside of the container.

### Attesting (signing)

With gitian, the binaries would be commited to (signed) using `gsign`.
However this is no longer necessary with guix.
The `guix-attest` script is used for making and signing the sha256 hashes of the build results.
This script does not require guix, and should be run on the build host.

1. Copy the build results to a safe location. This is necessary as these binaries are needed for attestation after codesigning and gitian will delete them if they remain in gitian's output directory (from the gitian-builder root):
```
mv build/out/* ../bitcoin-binaries/<version>/
```
2. Make the attestation (from bitcoin repo root):
```
env GUIX_SIGS_REPO=<path/to/sigs/repo> SIGNER=<name> OUTDIR_BASE=../bitcoin-binaries/<version> contrib/guix/guix-attest
```

### Codesign

Once detached code signatures have been pushed to the `bitcoin-detached-sigs` repo, the code signature can be attached.

1. Copy unsigned tarballs to gitian builder inputs (from the gitian-builder root):
```
cp build/out/x86_64-w64-mingw32/bitcoin-<version>-win-unsigned.tar.gz inputs/bitcoin-win-unsigned.tar.gz
cp build/out/x86_64-apple-darwin18/bitcoin-<version>-osx-unsigned.tar.gz inputs/bitcoin-osx-unsigned.tar.gz
```
2. Run the code signature builds (from the gitian-builder root):
```
bin/gbuild --commit bitcoin=<version>,signature=<version> --allow-sudo ../bitcoin/guix-in-gitian/contrib/gitian-descriptors/gitian-guix-win-signer.yml
bin/gbuild --commit bitcoin=<version>,signature=<version> --allow-sudo ../bitcoin/guix-in-gitian/contrib/gitian-descriptors/gitian-guix-mac-signer.yml
```
3. Copy the results to the same safe location as done previously.
4. Attest the results as done previously.
