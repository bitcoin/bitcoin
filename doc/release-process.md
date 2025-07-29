Release Process
====================

* [ ] Update translations, see [translation_process.md](https://github.com/dashpay/dash/blob/master/doc/translation_process.md#synchronising-translations).
* [ ] Update manpages (after rebuilding the binaries), see [gen-manpages.py](https://github.com/bitcoin/bitcoin/blob/master/contrib/devtools/README.md#gen-manpagespy).

Before every minor and major release:

* [ ] Review ["Needs backport" labels](https://github.com/dashpay/dash/labels?q=backport).
* [ ] Update DIPs with any changes introduced by this release (see [this pull request](https://github.com/dashpay/dips/pull/142) for an example)
* [ ] Update version in `configure.ac` (don't forget to set `CLIENT_VERSION_IS_RELEASE` to `true`)
* [ ] Write release notes (see below)
* [ ] Update `src/chainparams.cpp` `nMinimumChainWork` with information from the `getblockchaininfo` rpc.
* [ ] Update `src/chainparams.cpp` `defaultAssumeValid` with information from the `getblockhash` rpc.
  - The selected value must not be orphaned so it may be useful to set the value two blocks back from the tip.
  - Testnet should be set some tens of thousands back from the tip due to reorgs there.
  - This update should be reviewed with a `reindex-chainstate` with `assumevalid=0` to catch any defect
    that causes rejection of blocks in the past history.
* [ ] Ensure all TODOs are evaluated and resolved if needed
* [ ] Verify Insight works
* [ ] Verify p2pool works (unmaintained; no responsible party)
* [ ] Tag version and push (see below)
* [ ] Validate that CI passes

Before every major release:

* [ ] Update hardcoded [seeds](/contrib/seeds/README.md), see [this pull request](https://github.com/dashpay/dash/pull/5914) for an example.
* [ ] Update [`src/chainparams.cpp`](/src/chainparams.cpp) `m_assumed_blockchain_size` and `m_assumed_chain_state_size` with the current size plus some overhead (see [this](#how-to-calculate-assumed-blockchain-and-chain-state-size) for information on how to calculate them).
* [ ] Update [`src/chainparams.cpp`](/src/chainparams.cpp) `chainTxData` with statistics about the transaction count and rate. Use the output of the `getchaintxstats` RPC, see
  [this pull request](https://github.com/dashpay/dash/pull/5692) for an example. Reviewers can verify the results by running `getchaintxstats <window_block_count> <window_last_block_hash>` with the `window_block_count` and `window_last_block_hash` from your output.

### First time / New builders

Install Guix using one of the installation methods detailed in
[contrib/guix/INSTALL.md](/contrib/guix/INSTALL.md).

Check out the source code in the following directory hierarchy.

```sh
cd /path/to/your/toplevel/build
git clone https://github.com/dashpay/guix.sigs.git
git clone https://github.com/dashpay/dash-detached-sigs.git
git clone https://github.com/dashpay/dash.git
```

### Dash Core maintainers/release engineers, suggestion for writing release notes

Write release notes. git shortlog helps a lot, for example:

```sh
git shortlog --no-merges v(current version, e.g. 19.3.0)..v(new version, e.g. 20.0.0)
```

Generate list of authors:

```sh
git log --format='- %aN' v(current version, e.g. 19.3.0)..v(new version, e.g. 20.0.0) | sort -fiu
```

Tag version (or release candidate) in git

```sh
git tag -s v(new version, e.g. 20.0.0)
```

### Setup and perform Guix builds

Checkout the Dash Core version you'd like to build:

```sh
pushd ./dash
export SIGNER='(your builder key, ie udjinm6, pasta, etc)'
export VERSION='(new version, e.g. 20.0.0)'
git fetch origin "v${VERSION}"
git checkout "v${VERSION}"
popd
```

Ensure your guix.sigs are up-to-date if you wish to `guix-verify` your builds
against other `guix-attest` signatures.

```sh
git -C ./guix.sigs pull
```

### Create the macOS SDK tarball: (first time, or when SDK version changes)

_Note: this step can be skipped if [our CI](https://github.com/dashpay/dash/blob/master/ci/test/00_setup_env.sh#L64) still uses bitcoin's SDK package (see SDK_URL)_

Create the macOS SDK tarball, see the [macOS build
instructions](build-osx.md#deterministic-macos-app-notes) for
details.

### Build and attest to build outputs:

Follow the relevant Guix README.md sections:
- [Building](/contrib/guix/README.md#building)
- [Attesting to build outputs](/contrib/guix/README.md#attesting-to-build-outputs)

_Note: we ship releases for only some supported HOSTs so consider providing limited `HOSTS` variable or run `./contrib/containers/guix/scripts/guix-start` instead of `./contrib/guix/guix-build` when building binaries for quicker builds that exclude the supported but not shipped HOSTs_

### Verify other builders' signatures to your own. (Optional)

Add other builders keys to your gpg keyring, and/or refresh keys: See `../dash/contrib/builder-keys/README.md`.

Follow the relevant Guix README.md sections:
- [Verifying build output attestations](/contrib/guix/README.md#verifying-build-output-attestations)

### Next steps:

Commit your signature to `guix.sigs`:

```sh
pushd guix.sigs
git add "${VERSION}/${SIGNER}/noncodesigned.SHA256SUMS{,.asc}"
git commit -a
git push  # Assuming you can push to the guix.sigs tree
popd
```

Codesigner only: Create Windows/macOS detached signatures:
- Only one person handles codesigning. Everyone else should skip to the next step.
- Only once the Windows/macOS builds each have 3 matching signatures may they be signed with their respective release keys.

Codesigner only: Sign the macOS binary:

* Transfer `dashcore-osx-unsigned.tar.gz` to macOS for signing
* Extract and sign:

    ```sh
    tar xf dashcore-osx-unsigned.tar.gz
    ./detached-sig-create.sh -s "Key ID" -o runtime
    ```

* Enter the keychain password and authorize the signature
* Move `signature-osx.tar.gz` back to the guix-build host

Codesigner only: Sign the windows binaries:

* Extract and sign:

    ```sh
    tar xf dashcore-win-unsigned.tar.gz
    ./detached-sig-create.sh -key /path/to/codesign.key
    ```

* Enter the passphrase for the key when prompted
* `signature-win.tar.gz` will be created

Code-signer only: It is advised to test that the code signature attaches properly prior to tagging by performing the `guix-codesign` step.
However if this is done, once the release has been tagged in the bitcoin-detached-sigs repo, the `guix-codesign` step must be performed again in order for the guix attestation to be valid when compared against the attestations of non-codesigner builds.

Codesigner only: Commit the detached codesign payloads:

```sh
pushd ~/dashcore-detached-sigs
# checkout the appropriate branch for this release series
git checkout "v${VERSION}"
rm -rf *
tar xf signature-osx.tar.gz
tar xf signature-win.tar.gz
git add -A
git commit -m "add detached sigs for win/osx for ${VERSION}"
git push
popd
```

Non-codesigners: wait for Windows/macOS detached signatures:

- Once the Windows/macOS builds each have 3 matching signatures, they will be signed with their respective release keys.
- Detached signatures will then be committed to the [dash-detached-sigs](https://github.com/dashpay/dash-detached-sigs) repository, which can be combined with the unsigned apps to create signed binaries.

Create (and optionally verify) the codesigned outputs:
- [Codesigning](/contrib/guix/README.md#codesigning)

Commit your signature for the signed macOS/Windows binaries:

```sh
pushd ./guix.sigs
git add "${VERSION}/${SIGNER}"/all.SHA256SUMS{,.asc}
git commit -m "Add attestations by ${SIGNER} for ${VERSION} codesigned"
git push  # Assuming you can push to the guix.sigs tree
popd
```

### After 3 or more people have guix-built and their results match:

* [ ] Combine the `all.SHA256SUMS.asc` file from all signers into `SHA256SUMS.asc`:
    ```sh
    cat "$VERSION"/*/all.SHA256SUMS.asc > SHA256SUMS.asc
    ```
* [ ] GPG sign each download / binary
* [ ] Upload zips and installers, as well as `SHA256SUMS.asc` from last step, to GitHub as GitHub draft release.
    1. The contents of each `./dash/guix-build-${VERSION}/output/${HOST}/` directory, except for
       `*-debug*` files.

       Guix will output all of the results into host subdirectories, but the `SHA256SUMS`
       file does not include these subdirectories. In order for downloads via torrent
       to verify without directory structure modification, all of the uploaded files
       need to be in the same directory as the `SHA256SUMS` file.

       The `*-debug*` files generated by the guix build contain debug symbols
       for troubleshooting by developers. It is assumed that anyone that is
       interested in debugging can run guix to generate the files for
       themselves. To avoid end-user confusion about which file to pick, as well
       as save storage space *do not upload these to the dash.org server*.

       ```sh
       find guix-build-${VERSION}/output/ -maxdepth 2 -type f -not -name "SHA256SUMS.part" -and -not -name "*debug*" -exec scp {} user@dash.org:/var/www/bin/dash-core-${VERSION} \;
       ```

    2. The `SHA256SUMS` file

    3. The `SHA256SUMS.asc` combined signature file you just created
* [ ] Validate `SHA256SUMS.asc` and all binaries attached to GitHub draft release are correct
* [ ] Notarize macOS binaries
* [ ] Publish release on GitHub
* [ ] Fast-forward `master` branch on GitHub
* [ ] Update the dash.org download links
* [ ] Ensure that docker hub images are up to date

### Announce the release:
* [ ] Release on Dash forum: https://www.dash.org/forum/topic/official-announcements.54/ (necessary so we have a permalink to use on twitter, reddit, etc.)
* [ ] Prepare product brief (major versions only)
* [ ] Prepare a release announcement tweet
* [ ] Follow-up tweets with any important block heights for consensus changes
* [ ] Post on Reddit
* [ ] Celebrate

### After the release:
* [ ] Submit patches to BTCPay to ensure they use latest / compatible version see https://github.com/dashpay/dash/issues/4211#issuecomment-966608207
* [ ] Update Core and User docs (docs.dash.org)
* [ ] Test Docker build runs without error in Dashmate
* [ ] Add new Release Process items to repo [Release Process](release-process.md) document
* [ ] Merge `master` branch back into `develop` so that `master` could be fast-forwarded on next release again

### MacOS Notarization

#### Prerequisites
Make sure you have the latest Xcode installed on your macOS device. You can download it from the Apple Developer website.
You should have a valid Apple Developer ID under the team you are using which is necessary for the notarization process.
To avoid including your password as cleartext in a notarization script, you can provide a reference to a keychain item. You can add a new keychain item named `AC_PASSWORD` from the command line using the `notarytool` utility:

```sh
xcrun notarytool store-credentials "AC_PASSWORD" --apple-id "AC_USERNAME" --team-id <WWDRTeamID> --password <secret_2FA_password>
```

#### Notarization
Open Terminal, and navigate to the location of the .dmg file.

Then, run the following command to notarize the .dmg file:

```sh
xcrun notarytool submit dashcore-{version}-{x86_64, arm64}-apple-darwin.dmg --keychain-profile "AC_PASSWORD" --wait
```

Replace `{version}` with the version you are notarizing. This command uploads the .dmg file to Apple's notary service.

The `--wait` option makes the command wait to return until the notarization process is complete.

If the notarization process is successful, the notary service generates a log file URL. Please save this URL, as it contains valuable information regarding the notarization process.

#### Notarization Validation

After successfully notarizing the .dmg file, extract `Dash-Qt.app` from the .dmg.
To verify that the notarization process was successful, run the following command:

```sh
spctl -a -vv -t install Dash-Qt.app
```

Replace `Dash-Qt.app` with the path to your .app file. This command checks whether your .app file passes Gatekeeper’s
checks. If the app is successfully notarized, the command line will include a line stating `source=<Notarized Developer ID>`.

### Additional information

#### <a name="how-to-calculate-assumed-blockchain-and-chain-state-size"></a>How to calculate `m_assumed_blockchain_size` and `m_assumed_chain_state_size`

Both variables are used as a guideline for how much space the user needs on their drive in total, not just strictly for the blockchain.
Note that all values should be taken from a **fully synced** node and have an overhead of 5-10% added on top of its base value.

To calculate `m_assumed_blockchain_size`:
- For `mainnet` -> Take the size of the data directory, excluding `/regtest` and `/testnet3` directories.
- For `testnet` -> Take the size of the `/testnet3` directory.


To calculate `m_assumed_chain_state_size`:
- For `mainnet` -> Take the size of the `/chainstate` directory.
- For `testnet` -> Take the size of the `/testnet3/chainstate` directory.

Notes:
- When taking the size for `m_assumed_blockchain_size`, there's no need to exclude the `/chainstate` directory since it's a guideline value and an overhead will be added anyway.
- The expected overhead for growth may change over time, so it may not be the same value as last release; pay attention to that when changing the variables.
