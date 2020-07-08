*After branching off for a major version release of Bitcoin Core, use this
template to create the initial release notes draft.*

*The release notes draft is a temporary file that can be added to by anyone. See
[/doc/developer-notes.md#release-notes](/doc/developer-notes.md#release-notes)
for the process.*

*Create the draft, named* "*version* Release Notes Draft"
*(e.g. "0.20.0 Release Notes Draft"), as a collaborative wiki in:*

https://github.com/bitcoin-core/bitcoin-devwiki/wiki/

*Before the final release, move the notes back to this git repository.*

*version* Release Notes Draft
===============================

Bitcoin Core version *version* is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-*version*/>

This release includes new features, various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

During this release cycle, work has been done to ensure that the codebase is fully
compatible with C++17. The intention is to begin using C++17 features starting
with the 0.22.0 release. This means that a compiler that supports C++17 will be
required to compile 0.22.0.

Bitcoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

From Bitcoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Bitcoin Core does not yet change appearance
when macOS "dark mode" is activated.

Notable changes
===============

P2P and network changes
-----------------------

- The mempool now tracks whether transactions submitted via the wallet or RPCs
  have been successfully broadcast. Every 10-15 minutes, the node will try to
  announce unbroadcast transactions until a peer requests it via a `getdata`
  message or the transaction is removed from the mempool for other reasons.
  The node will not track the broadcast status of transactions submitted to the
  node using P2P relay. This version reduces the initial broadcast guarantees
  for wallet transactions submitted via P2P to a node running the wallet. (#18038)

Updated RPCs
------------

- `getmempoolinfo` now returns an additional `unbroadcastcount` field. The
  mempool tracks locally submitted transactions until their initial broadcast
  is acknowledged by a peer. This field returns the count of transactions
  waiting for acknowledgement.

- Mempool RPCs such as `getmempoolentry` and `getrawmempool` with
  `verbose=true` now return an additional `unbroadcast` field. This indicates
  whether initial broadcast of the transaction has been acknowledged by a
  peer. `getmempoolancestors` and `getmempooldescendants` are also updated.

- The `bumpfee`, `fundrawtransaction`, `sendmany`, `sendtoaddress`, and `walletcreatefundedpsbt`
RPC commands have been updated to include two new fee estimation methods "BTC/kB" and "sat/B".
The target is the fee expressed explicitly in the given form. Note that use of this feature
will trigger BIP 125 (replace-by-fee) opt-in. (#11413)

- In addition, the `estimate_mode` parameter is now case insensitive for all of
  the above RPC commands. (#11413)

- The `bumpfee` command now uses `conf_target` rather than `confTarget` in the
  options. (#11413)

- The `getpeerinfo` RPC no longer returns the `banscore` field unless the configuration
  option `-deprecatedrpc=banscore` is used. The `banscore` field will be fully
  removed in the next major release. (#19469)

- The `walletcreatefundedpsbt` RPC call will now fail with
  `Insufficient funds` when inputs are manually selected but are not enough to cover
  the outputs and fee. Additional inputs can automatically be added through the
  new `add_inputs` option. (#16377)

- The `fundrawtransaction` RPC now supports `add_inputs` option that when `false`
  prevents adding more inputs if necessary and consequently the RPC fails.

Changes to Wallet or GUI related RPCs can be found in the GUI or Wallet section below.

New RPCs
--------

Build System
------------

Updated settings
----------------

- The `-banscore` configuration option, which modified the default threshold for
  disconnecting and discouraging misbehaving peers, has been removed as part of
  changes in 0.20.1 and in this release to the handling of misbehaving peers.
  Refer to "Changes regarding misbehaving peers" in the 0.20.1 release notes for
  details. (#19464)

- The `-debug=db` logging category, which was deprecated in 0.20 and replaced by
  `-debug=walletdb` to distinguish it from `coindb`, has been removed. (#19202)

- A `download` permission has been extracted from the `noban` permission. For
  compatibility, `noban` implies the `download` permission, but this may change
  in future releases. Refer to the help of the affected settings `-whitebind`
  and `-whitelist` for more details. (#19191)

Changes to Wallet or GUI related settings can be found in the GUI or Wallet  section below.

Tools and Utilities
-------------------

- A new `bitcoin-cli -generate` command, equivalent to RPC `generatenewaddress`
  followed by `generatetoaddress`, can generate blocks for command line testing
  purposes. This is a client-side version of the
  former `generate` RPC. See the help for details. (#19133)

- The `bitcoin-cli -getinfo` command now displays the wallet name and balance for
  each of the loaded wallets when more than one is loaded (e.g. in multiwallet
  mode) and a wallet is not specified with `-rpcwallet`. (#18594)

New settings
------------

Wallet
------

- Backwards compatibility has been dropped for two `getaddressinfo` RPC
  deprecations, as notified in the 0.20 release notes. The deprecated `label`
  field has been removed as well as the deprecated `labels` behavior of
  returning a JSON object containing `name` and `purpose` key-value pairs. Since
  0.20, the `labels` field returns a JSON array of label names. (#19200)

- To improve wallet privacy, the frequency of wallet rebroadcast attempts is
  reduced from approximately once every 15 minutes to once every 12-36 hours.
  To maintain a similar level of guarantee for initial broadcast of wallet
  transactions, the mempool tracks these transactions as a part of the newly
  introduced unbroadcast set. See the "P2P and network changes" section for
  more information on the unbroadcast set. (#18038)

- The wallet can create a transaction without change even when the keypool is
  empty. Previously it failed. (#17219)

- The `-salvagewallet` startup option has been removed. A new `salvage` command
  has been added to the `bitcoin-wallet` tool which performs the salvage
  operations that `-salvagewallet` did. (#18918)

### Experimental Descriptor Wallets

Please note that Descriptor Wallets are still experimental and not all expected functionality
is available. Additionally there may be some bugs and current functions may change in the future.
Bugs and missing functionality can be reported to the [issue tracker](https://github.com/bitcoin/bitcoin/issues).

0.21 introduces a new type of wallet - Descriptor Wallets. Descriptor Wallets store
scriptPubKey information using descriptors. This is in contrast to the Legacy Wallet
structure where keys are used to generate scriptPubKeys and addresses. Because of this
shift to being script based instead of key based, many of the confusing things that Legacy
Wallets do are not possible with Descriptor Wallets. Descriptor Wallets use a definition
of "mine" for scripts which is simpler and more intuitive than that used by Legacy Wallets.
Descriptor Wallets also uses different semantics for watch-only things and imports.

As Descriptor Wallets are a new type of wallet, their introduction does not affect existing wallets.
Users who already have a Bitcoin Core wallet can continue to use it as they did before without
any change in behavior. Newly created Legacy Wallets (which is the default type of wallet) will
behave as they did in previous versions of Bitcoin Core.

The differences between Descriptor Wallets and Legacy Wallets are largely limited to non user facing
things. They are intended to behave similarly except for the import/export and watchonly functionality
as described below.

#### Creating Descriptor Wallets

Descriptor Wallets are not created by default. They must be explicitly created using the
`createwallet` RPC or via the GUI. A `descriptors` option has been added to `createwallet`.
Setting `descriptors` to `true` will create a Descriptor Wallet instead of a Legacy Wallet.

In the GUI, a checkbox has been added to the Create Wallet Dialog to indicate that a
Descriptor Wallet should be created.

Without those options being set, a Legacy Wallet will be created instead. Additionally the
Default Wallet created upon first startup of Bitcoin Core will be a Legacy Wallet.

#### `IsMine` Semantics

`IsMine` refers to the function used to determine whether a script belongs to the wallet.
This is used to determine whether an output belongs to the wallet. `IsMine` in Legacy Wallets
returns true if the wallet would be able to sign an input that spends an output with that script.
Since keys can be involved in a variety of different scripts, this definition for `IsMine` can
lead to many unexpected scripts being considered part of the wallet.

With Descriptor Wallets, descriptors explicitly specify the set of scripts that are owned by
the wallet. Since descriptors are deterministic and easily enumerable, users will know exactly
what scripts the wallet will consider to belong to it. Additionally the implementation of `IsMine`
in Descriptor Wallets is far simpler than for Legacy Wallets. Notably, in Legacy Wallets, `IsMine`
allowed for users to take one type of address (e.g. P2PKH), mutate it into another address type
(e.g. P2WPKH), and the wallet would still detect outputs sending to the new address type
even without that address being requested from the wallet. Descriptor Wallets does not
allow for this and will only watch for the addresses that were explicitly requested from the wallet.

These changes to `IsMine` will make it easier to reason about what scripts the wallet will
actually be watching for in outputs. However for the vast majority of users, this change is
largely transparent and will not have noticeable effect.

#### Imports and Exports

In Legacy Wallets, raw scripts and keys could be imported to the wallet. Those imported scripts
and keys are treated separately from the keys generated by the wallet. This complicates the `IsMine`
logic as it has to distinguish between spendable and watchonly.

Descriptor Wallets handle importing scripts and keys differently. Only complete descriptors can be
imported. These descriptors are then added to the wallet as if it were a descriptor generated by
the wallet itself. This simplifies the `IsMine` logic so that it no longer has to distinguish
between spendable and watchonly. As such, the watchonly model for Descriptor Wallets is also
different and described in more detail in the next section.

To import into a Descriptor Wallet, a new `importdescriptors` RPC has been added that uses a syntax
similar to that of `importmulti`.

As Legacy Wallets and Descriptor Wallets use different mechanisms for storing and importing scripts and keys
the existing import RPCs have been disabled for descriptor wallets.
New export RPCs for Descriptor Wallets have not yet been added.

The following RPCs are disabled for Descriptor Wallets:

* importprivkey
* importpubkey
* importaddress
* importwallet
* dumpprivkey
* dumpwallet
* importmulti
* addmultisigaddress
* sethdseed

#### Watchonly Wallets

A Legacy Wallet contains both private keys and scripts that were being watched.
Those watched scripts would not contribute to your normal balance. In order to see the watchonly
balance and to use watchonly things in transactions, an `include_watchonly` option was added
to many RPCs that would allow users to do that. However it is easy to forget to include this option.

Descriptor Wallets move to a per-wallet watchonly model. Instead an entire wallet is considered to be
watchonly depending on whether it was created with private keys disabled. This eliminates the need
to distinguish between things that are watchonly and things that are not within a wallet itself.

This change does have a caveat. If a Descriptor Wallet with private keys *enabled* has
a multiple key descriptor without all of the private keys (e.g. `multi(...)` with only one private key),
then the wallet will fail to sign and broadcast transactions. Such wallets would need to use the PSBT
workflow but the typical GUI Send, `sendtoaddress`, etc. workflows would still be available, just
non-functional.

This issue is worsened if the wallet contains both single key (e.g. `wpkh(...)`) descriptors and such
multiple key descriptors as some transactions could be signed and broadast and others not. This is
due to some transactions containing only single key inputs, while others would contain both single
key and multiple key inputs, depending on which are available and how the coin selection algorithm
selects inputs. However this is not considered to be a supported use case; multisigs
should be in their own wallets which do not already have descriptors. Although users cannot export
descriptors with private keys for now as explained earlier.

#### BIP 44/49/84 Support

The change to using descriptors changes the default derivation paths used by Bitcoin Core
to adhere to BIP 44/49/84. Descriptors with different derivation paths can be imported without
issue.

### Wallet RPC changes

- The `upgradewallet` RPC replaces the `-upgradewallet` command line option.
  (#15761)
- The `settxfee` RPC will fail if the fee was set higher than the `-maxtxfee`
  command line setting. The wallet will already fail to create transactions
  with fees higher than `-maxtxfee`. (#18467)

GUI changes
-----------

- The GUI Peers window no longer displays a "Ban Score" field. This is part of
  changes in 0.20.1 and in this release to the handling of misbehaving
  peers. Refer to "Changes regarding misbehaving peers" in the 0.20.1 release
  notes for details. (#19512)

Low-level changes
=================

RPC
---

- To make RPC `sendtoaddress` more consistent with `sendmany` the following error
    `sendtoaddress` codes were changed from `-4` to `-6`:
  - Insufficient funds
  - Fee estimation failed
  - Transaction has too long of a mempool chain

Tests
-----

Credits
=======

Thanks to everyone who directly contributed to this release:


As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
