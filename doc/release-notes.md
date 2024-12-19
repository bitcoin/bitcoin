# Dash Core version v22.0.0

This is a new major version release, bringing new features, various bugfixes
and other improvements.

This release is **mandatory** for all nodes, as it includes a hard fork.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>


# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

## Downgrade warning

### Downgrade to a version < v22.0.0

Downgrading to a version older than v22.0.0 may not be supported, and will
likely require a reindex.

# Release Notes

## Notable Changes

- **Asset Unlock Transactions (Platform Transfer)**
    - Introduces a new fork, `withdrawals`, that changes consensus rules.
    - **Validation Logic Update:**
        - Allows using all **24 active quorums** and the most recent inactive quorum.
        - Previous versions may refuse withdrawals with `bad-assetunlock-not-active-quorum` even if the quorum is active.
    - **Withdrawal Limits Increased:**
        - Flat **2000 Dash** per **576 latest blocks**. The previous limit was 1000 Dash.
- **Increased Minimum Protocol Version**
    - The minimum protocol version has been increased to **70216**.
    - Masternode minimum protocol version has increased to **70235**.

## P2P and Network Changes

- **Improved Onion Connectivity**
    - Nodes with onion connectivity will attempt to maintain at least **two outbound onion connections** and will protect these connections from eviction.
    - **Benefit:** Ensures nodes capable of accessing the onion network maintain a few onion connections, allowing network messages to propagate even if non-onion IPv4 traffic is blocked.
    - **Security Enhancement:** Enables P2P encryption for these peers.

- **DSQ Message Broadcast Update**
    - Starting in protocol version **70234**, DSQ messages are broadcast using the inventory system instead of relaying to all connected peers.
    - **Benefit:** Reduces bandwidth needs for all nodes, especially noticeable on highly connected masternodes.

- **Compressed Block Headers Request Limit**
    - Starting in protocol version **70235**, the maximum number of compressed block headers that can be requested at once has been increased from **2000** to **8000**.
    - **Applies to:** Compressed block headers only.
    - **Benefit:** Potential for improved header sync performance.

- **Multi-Network Connectivity Enhancement**
    - Nodes with multiple reachable networks will actively try to maintain at least one outbound connection to each network.
    - **Benefits:**
        - Improves resistance to eclipse attacks.
        - Enhances network-level resistance to partition attacks.
    - **User Impact:** Users no longer need to perform active measures to ensure connections to multiple enabled networks.

- **BIP324 Encrypted Communication (Experimental)**
    - Dash Core now experimentally implements [BIP324](https://github.com/bitcoin/bips/blob/master/bip-0324.mediawiki), introducing encrypted communication for P2P network traffic.
    - **Opt-In Adoption**
        - **Enable Encryption:** Users can opt-in to use BIP324 by adding `v2transport=1` to their Dash Core configuration.
        - **Default Behavior:** Encryption is **disabled by default** as this is currently experimental.
    - **Benefits**
        - **Enhanced Privacy:** Encrypts P2P messages for v2 connections, reducing the risk of traffic analysis and eavesdropping.
        - **Improved Security:** Protects against certain types of network attacks by ensuring that communication between nodes is confidential and tamper-proof.
    - **Limitations and Considerations**
        - **Compatibility:** Encrypted connections may only be established between nodes that have BIP324 enabled. Nodes without encryption support will continue to communicate over unencrypted channels.
    - **Status and Future Plans**
        - **Experimental Phase:** This feature is currently in an experimental phase. Users are encouraged to test and provide feedback to ensure stability.
        - **Future Development:** BIP324 will likely be enabled by default in the next minor version, v22.1.0.

## Compatibility

- **Dark Mode Appearance**
    - Dash Core changes appearance when macOS "dark mode" is activated.

- **glibc Requirement**
    - The minimum required glibc to run Dash Core is now **2.31**. This means that **RHEL 8** and **Ubuntu 18.04 (Bionic)** are no longer supported.

- **FreeBSD Improvements**
    - Fixed issues with building Dash Core on FreeBSD.

## New RPCs

- **`coinjoinsalt`**
    - Allows manipulation of a CoinJoin salt stored in a wallet.
        - `coinjoinsalt get`: Fetches an existing salt.
        - `coinjoinsalt set`: Allows setting a custom salt.
        - `coinjoinsalt generate`: Sets a random hash as the new salt.

## Updated RPCs

- **`getpeerinfo` Changes**
    - `getpeerinfo` no longer returns the following fields: `addnode` and `whitelisted`, which were previously deprecated in v21.
        - Instead of `addnode`, the `connection_type` field returns `manual`.
        - Instead of `whitelisted`, the `permissions` field indicates if the peer has special privileges.

- **`getblockfrompeer` Parameter Renaming**
    - The named argument `block_hash` has been renamed to `blockhash` to align with the rest of the codebase.
    - **Breaking Change:** If using named parameters, make sure to update them accordingly.

- **`coinjoin stop` Error Handling**
    - `coinjoin stop` will now return an error if there is no CoinJoin mixing session to stop.

- **`getcoinjoininfo` Adjustments**
    - `getcoinjoininfo` will no longer report `keys_left` and will not incorrectly warn about keypool depletion with descriptor wallets.

- **`creditOutputs` Format Change**
    - `creditOutputs` entries in various RPCs that output transactions as JSON are now shown as objects instead of strings.

- **`quorum dkgsimerror` Argument Update**
    - `quorum dkgsimerror` will no longer accept a decimal value between 0 and 1 for the `rate` argument.
    - It will now expect an integer between **0** to **100**.

- **Deprecated `protx *_hpmn` RPC Endpoints**
    - Deprecated `protx *_hpmn` RPC entry points have been dropped in favor of `protx *_evo`.
    - **Removed Endpoints:**
        - `protx register_fund_hpmn`
        - `protx register_hpmn`
        - `protx register_prepare_hpmn`
        - `protx update_service_hpmn`

- **`governance` Descriptor Wallet Support**
    - The `governance votemany` and `governance votealias` RPC commands now support descriptor-based wallets.

- **`createwallet` Behavior for Descriptor Wallets**
    - When creating descriptor wallets, `createwallet` now requires explicitly setting `load_on_startup`.

## Command-line Options

### Changes in Existing Command-line Options

- **`-walletnotify=<cmd>` Enhancements**
    - Introduces new format options `%h` and `%b`.
        - `%b`: Replaced by the hash of the block including the transaction (set to `'unconfirmed'` if the transaction is not included).
        - `%h`: Replaced by the block height (**-1** if not included).

- **`-maxuploadtarget` Format Update**
    - Now allows human-readable byte units `[k|K|m|M|g|G|t|T]`.
        - **Example:** `-maxuploadtarget=500g`.
        - **Constraints:** No whitespace, `+`, `-`, or fractions allowed.
        - **Default:** `M` if no suffix is provided.

## Devnet Breaking Changes

- **Hardfork Activation Changes**
    - `BRR` (`realloc`), `DIP0020`, `DIP0024`, `V19`, `V20`, and `MN_RR` hardforks are now activated at **block 2** instead of block **300** on devnets.
    - **Implications:**
        - Breaking change.
        - Inability to sync on devnets created with earlier Dash Core versions and vice versa.

- **LLMQ Type Enhancement**
    - **LLMQ_50_60** is enabled for **Devnet** networks.
    - Necessary for testing on a large Devnet.

## Tests

- **Regtest Network Softfork Activation Heights**
    - For the `regtest` network, the activation heights of several softforks have been set to **block height 1**.
    - **Customization:** Can be changed using the runtime setting `-testactivationheight=name@height`.
    - *(dash#6214)*

## Statistics

### New Features

- **Statsd Client Enhancements**
    - Supports queueing and batching messages.
        - **Benefits:**
            - Reduces the number of packets.
            - Lowers the rate at which messages are sent to the Statsd daemon.

- **Batch Configuration Options**
    - **Maximum Batch Size:**
        - Adjustable using `-statsbatchsize` (in bytes).
        - **Default:** **1KiB**.
    - **Batch Send Frequency:**
        - Adjustable using `-statsduration` (in milliseconds).
        - **Default:** **1 second**.
        - **Note:** `-statsduration` does not affect `-statsperiod`, which dictates how frequently some stats are *collected*.

### Deprecations

- **Deprecation of `-platform-user`**
    - `-platform-user` is deprecated in favor of the whitelist feature.
    - In releases **22.x** of Dash Core, it has been renamed to `-deprecated-platform-user`.
    - It will be removed in version **23.x**.

- **`-statsenabled` Deprecation**
    - Now implied by the presence of `-statshost`.
    - It will be removed in version **23.x**.

- **`-statshostname` Replacement**
    - Deprecated and replaced with `-statssuffix` for better representation of the argument's purpose.
    - **Behavior:** Behave identically to each other.
    - It will be removed in version **23.x**.

- **`-statsns` Replacement**
    - Deprecated and replaced with `-statsprefix` for better representation of the argument's purpose.
    - **Behavior:** `-statsprefix` enforces the usage of a delimiter between the prefix and key.
    - It will be removed in version **23.x**.

## GUI Changes

- **RPC Server Functionality Option**
    - A new option has been added to the "Main" tab in "Options" that allows users to enable RPC server functionality.

# v22.0.0 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- AJ ONeal
- Kittywhiskers Van Gogh
- Konstantin Akimov
- Odysseas Gabrielides
- PastaPastaPasta
- UdjinM6
- Vijaydasmp

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

- [v21.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.1.md) released Oct/22/2024
- [v21.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.0.md) released Aug/8/2024
- [v21.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.2.md) released Aug/1/2024
- [v21.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.0.md) released Jul/25/2024
- [v20.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.1.1.md) released April/3/2024
- [v20.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.1.0.md) released March/5/2024
- [v20.0.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.4.md) released Jan/13/2024
- [v20.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.3.md) released December/26/2023
- [v20.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.2.md) released December/06/2023
- [v20.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.1.md) released November/18/2023
- [v20.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.0.0.md) released November/15/2023
- [v19.3.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.3.0.md) released July/31/2023
- [v19.2.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.2.0.md) released June/19/2023
- [v19.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.1.0.md) released May/22/2023
- [v19.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-19.0.0.md) released Apr/14/2023
- [v18.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.2.md) released Mar/21/2023
- [v18.2.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.1.md) released Jan/17/2023
- [v18.2.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.2.0.md) released Jan/01/2023
- [v18.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.1.md) released January/08/2023
- [v18.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.1.0.md) released October/09/2022
- [v18.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.2.md) released October/09/2022
- [v18.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-18.0.1.md) released August/17/2022
- [v0.17.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.3.md) released June/07/2021
- [v0.17.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.17.0.2.md) released May/19/2021
- [v0.16.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.1.md) released November/17/2020
- [v0.16.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.1.0.md) released November/14/2020
- [v0.16.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.16.0.1.md) released September/30/2020
- [v0.15.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.15.0.0.md) released Febrary/18/2020
- [v0.14.0.5](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.5.md) released December/08/2019
- [v0.14.0.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.4.md) released November/22/2019
- [v0.14.0.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.3.md) released August/15/2019
- [v0.14.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.2.md) released July/4/2019
- [v0.14.0.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.1.md) released May/31/2019
- [v0.14.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.14.0.md) released May/22/2019
- [v0.13.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.3.md) released Apr/04/2019
- [v0.13.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.2.md) released Mar/15/2019
- [v0.13.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.1.md) released Feb/9/2019
- [v0.13.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.13.0.md) released Jan/14/2019
- [v0.12.3.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.4.md) released Dec/14/2018
- [v0.12.3.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.3.md) released Sep/19/2018
- [v0.12.3.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.2.md) released Jul/09/2018
- [v0.12.3.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.3.1.md) released Jul/03/2018
- [v0.12.2.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.3.md) released Jan/12/2018
- [v0.12.2.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.2.md) released Dec/17/2017
- [v0.12.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.1.md) released Feb/06/2017
- [v0.12.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.12.0.md) released Aug/15/2015
- [v0.11.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-0.9.0.md) released Mar/13/2014

[set-of-changes]: https://github.com/dashpay/dash/compare/v21.1.1...dashpay:v22.0.0
