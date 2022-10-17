Dash Core version v18.1.0
=========================

Release is now available from:

  <https://www.dash.org/downloads/#wallets>

This is a new minor version release, bringing new features, various bugfixes
and other improvements.

This release is optional for all nodes.

Please report bugs using the issue tracker at github:

  <https://github.com/dashpay/dash/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux). If you upgrade after DIP0003 activation and you were
using version < 0.13 you will have to reindex (start with -reindex-chainstate
or -reindex) to make sure your wallet has all the new data synced. Upgrading
from version 0.13 should not require any additional actions.

When upgrading from a version prior to 18.0.1, the
first startup of Dash Core will run a migration process which can take anywhere
from a few minutes to thirty minutes to finish. After the migration, a
downgrade to an older version is only possible with a reindex
(or reindex-chainstate).

Downgrade warning
-----------------

### Downgrade to a version < v18.1.0

Downgrading to a version older than v18.1.0 is supported.

### Downgrade to a version < v18.0.1

Downgrading to a version older than v18.0.1 is not supported due to changes in
the indexes database folder. If you need to use an older version, you must
either reindex or re-sync the whole chain.

### Downgrade of masternodes to < v18.0.1

Starting with the 0.16 release, masternodes verify the protocol version of other
masternodes. This results in PoSe punishment/banning for outdated masternodes,
so downgrading even prior to the activation of the introduced hard-fork changes
is not recommended.

Versioning
----------

Dash Core imperfectly follows semantic versioning. Breaking changes should be
expected in a major release. The number and severity of breaking changes in minor
releases are minimized, however we do not guarantee there are no breaking changes.
Bitcoin backports often introduce breaking changes, and are a likely source of
breaking changes in minor releases. Patch releases should never contain breaking changes.

This release **does** include breaking changes. Please read below to see if they will affect you.

Notable changes
===============

BIP70 Support Removed
---------------------

Support for the BIP70 Payment Protocol has been dropped from Dash Qt.
Interacting with BIP70-formatted URIs will return an error message informing them
of support removal. The `allowselfsignedrootcertificates` and `rootcertificates`
launch arguments are no longer valid.

systemd init file
-----------------

The systemd init file (`contrib/init/dashd.service`) has been changed to use
`/var/lib/dashd` as the data directory instead of `~dash/.dash`. This
change makes Dash Core more consistent with other services, and makes the
systemd init config more consistent with existing Upstart and OpenRC configs.

The configuration, PID, and data directories are now completely managed by
systemd, which will take care of their creation, permissions, etc. See
[`systemd.exec (5)`](https://www.freedesktop.org/software/systemd/man/systemd.exec.html#RuntimeDirectory=)
for more details.

When using the provided init files under `contrib/init`, overriding the
`datadir` option in `/etc/dash/dash.conf` will have no effect. This is
because the command line arguments specified in the init files take precedence
over the options specified in `/etc/dash/dash.conf`.

Account API removed
-------------------

The 'account' API was deprecated in v18.0 and has been fully removed in v18.1.
The 'label' API was introduced in v18.0 as a replacement for accounts.

See the [release notes from v18.0.1](https://github.com/dashpay/dash/blob/v18.0.1/doc/release-notes.md) for a full description of the changes from the
'account' API to the 'label' API.

Build system changes
--------------------

Python >=3.5 is now required by all aspects of the project. This includes the build
systems, test framework and linters. The previously supported minimum (3.4), was EOL
in March 2019. See bitcoin#14954 for more details.

Coin selection: Reuse Avoidance
--------------

A new wallet flag `avoid_reuse` has been added (default off). When enabled,
a wallet will distinguish between used and unused addresses, and default to not
use the former in coin selection.

(Note: rescanning the blockchain is required, to correctly mark previously
used destinations.)

Together with "avoid partial spends" (present as of Bitcoin v0.17), this
addresses a serious privacy issue where a malicious user can track spends by
peppering a previously paid to address with near-dust outputs, which would then
be inadvertently included in future payments.

Auto Loading Wallets
-------------

Wallets created or loaded in the GUI will now be automatically loaded on
startup, so they don't need to be manually reloaded next time Bitcoin is
started. The list of wallets to load on startup is stored in
`\<datadir\>/settings.json` and augments any command line or `bitcoin.conf`
`-wallet=` settings that specify more wallets to load. Wallets that are
unloaded in the GUI get removed from the settings list so they won't load again
automatically next startup. (bitcoin#19754)

The `createwallet`, `loadwallet`, and `unloadwallet` RPCs now accept
`load_on_startup` options to modify the settings list. Unless these options are
explicitly set to true or false, the list is not modified, so the RPC methods
remain backwards compatible. (bitcoin#15937)

Changes regarding misbehaving peers
-----------------------------------

Peers that misbehave (e.g. send us invalid blocks) are now referred to as
discouraged nodes in log output, as they're not (and weren't) strictly banned:
incoming connections are still allowed from them, but they're preferred for
eviction.

Furthermore, a few additional changes are introduced to how discouraged
addresses are treated:

- Discouraging an address does not time out automatically after 24 hours
  (or the `-bantime` setting). Depending on traffic from other peers,
  discouragement may time out at an indeterminate time.

- Discouragement is not persisted over restarts.

- There is no method to list discouraged addresses. They are not returned by
  the `listbanned` RPC. That RPC also no longer reports the `ban_reason`
  field, as `"manually added"` is the only remaining option.

- Discouragement cannot be removed with the `setban remove` RPC command.
  If you need to remove a discouragement, you can remove all discouragements by
  stop-starting your node.

Remote Procedure Call (RPC) Changes
-----------------------------------

Most changes here were introduced through Bitcoin backports mostly related to
the deprecation of wallet accounts in DashCore v0.17 and introduction of PSBT
format.

### The new RPCs are:
- A new `setwalletflag` RPC sets/unsets flags for an existing wallet.
- The `spork` RPC call will no longer offer both get (labelled as "basic mode") and set (labelled as "advanced mode") functionality. `spork` will now only offer "basic" functionality. "Advanced" functionality is now exposed through the `sporkupdate` RPC call.
- The `generateblock` RPC call will mine an array of ordered transactions, defined by hex array `transactions` that can contain either transaction IDs or a hex-encoded serialized raw transaction and set the coinbase destination defined by the `address/descriptor` argument.
- The `mockscheduler` is a debug RPC call that allows forwarding the scheduler by `delta_time`. This RPC call is hidden and will only be functional on mockable chains (i.e. primarily regtest). `delta_time` must be between 0 - 3600.
- `dumptxoutset` see bitcoin#16899 and the help text. This RPC is used for UTXO snapshot creation which is a part of the assume utxo project
- `generatetodescriptor` Mine blocks immediately to a specified descriptor. see bitcoin#16943

### The removed RPCs are:
- The wallet's `generate` RPC method has been removed. `generate` is only used for testing.
The RPC call reaches across multiple subsystems (wallet and mining), so is deprecated to simplify the wallet-node
interface. Projects that are using `generate` for testing purposes should
transition to using the `generatetoaddress` call, which does not require or use
the wallet component. Calling `generatetoaddress` with an address returned by
`getnewaddress` gives the same functionality as the old `generate` method.

### Changes in existing RPCs introduced through bitcoin backports:
- The `gettxoutsetinfo` RPC call now accept one optional argument (`hash_type`) that defines the algorithm used for calcluating the UTXO set hash, it will default to `hash_serialized_2` unless explicitly specified otherwise. `hash_type` will influence the key that is used to refer to refer to the UTXO set hash.
- The `generatedescriptor` RPC call has been introduced to allow mining a set number of blocks, defined by argument `num_blocks`, with the coinbase destination set to a descriptor, defined by the `descriptor` argument. The optional `maxtries` argument can be used to limit iterations.
- Descriptors with key origin information imported through `importmulti` will have their key origin information stored in the wallet for use with creating PSBTs.
- If `bip32derivs` of both `walletprocesspsbt` and `walletcreatefundedpsbt` is set to true but the key metadata for a public key has not been updated yet, then that key will have a derivation path as if it were just an independent key (i.e. no derivation path and its master fingerprint is itself)
- The `getblockstats` RPC is faster for fee calculation by using BlockUndo data. Also, `-txindex` is no longer required and `getblockstats` works for all non-pruned blocks.
- The `unloadwallet` RPC is now synchronous, meaning that it blocks until the wallet is fully unloaded.
- RPCs which have an `include_watchonly` argument or `includeWatching`
  option now default to `true` for watch-only wallets. Affected RPCs
  are: `getbalance`, `listreceivedbyaddress`, `listreceivedbylabel`,
  `listtransactions`, `listsinceblock`, `gettransaction`,
  `walletcreatefundedpsbt`, and `fundrawtransaction`.
- `createwallet` now returns a warning if an empty string is used as an encryption password, and does not encrypt the wallet, instead of raising an error.
  This makes it easier to disable encryption but also specify other options when using the `bitcoin-cli` tool.
- The RPC `joinpsbts` will shuffle the order of the inputs and outputs of the resulting joined psbt.
  Previously inputs and outputs were added in the order that the PSBTs were provided which makes correlating inputs to outputs extremely easy.
- `importprivkey`: new label behavior.
Previously, `importprivkey` automatically added the default empty label
("") to all addresses associated with the imported private key.  Now it
defaults to using any existing label for those addresses.  For example:

  - Old behavior: you import a watch-only address with the label "cold
    wallet".  Later, you import the corresponding private key using the
    default settings.  The address's label is changed from "cold wallet"
    to "".

  - New behavior: you import a watch-only address with the label "cold
    wallet".  Later, you import the corresponding private key using the
    default settings.  The address's label remains "cold wallet".

  In both the previous and current case, if you directly specify a label
    during the import, that label will override whatever previous label the
    addresses may have had.  Also in both cases, if none of the addresses
    previously had a label, they will still receive the default empty label
    ("").  Examples:
  - You import a watch-only address with the label "temporary".  Later you
    import the corresponding private key with the label "final".  The
    address's label will be changed to "final".
  - You use the default settings to import a private key for an address that
    was not previously in the wallet.  Its addresses will receive the default
    empty label ("").
- The `createwallet`, `loadwallet`, and `unloadwallet` RPCs now accept
  `load_on_startup` options to modify the settings list.
- Several RPCs have been updated to include an "avoid_reuse" flag, used to control
  whether already used addresses should be left out or included in the operation.
  These include:
  - `createwallet`
  - `getbalance`
  - `sendtoaddress`

  In addition, `sendtoaddress` has been changed to enable `-avoidpartialspends` when
  `avoid_reuse` is enabled.
  The `listunspent` RPC has also been updated to now include a "reused" bool, for nodes
  with "avoid_reuse" enabled.

### Dash-specific changes in existing RPCs:
- In rpc `upgradetohd` new parameter `rescan` was added which allows users to skip or force blockchain rescan. This params defaults to `false` when `mnemonic` parameter is empty and `true` otherwise.

Please check `help <command>` for more detailed information on specific RPCs.

Command-line options
--------------------

Most changes here were introduced through Bitcoin backports.

New cmd-line options:
- RPC Whitelist system. It can give certain RPC users permissions to only some RPC calls.
  It can be set with two command line arguments (`rpcwhitelist` and `rpcwhitelistdefault`). (bitcoin#12763)

Removed cmd-line options:
- The `-zapwallettxes` startup option has been removed and its functionality removed from the wallet.
  This option was originally intended to allow for the fee bumping of transactions that did not
  signal RBF. This functionality has been superseded with the abandon transaction feature.

Changes in existing cmd-line options:
- The `testnet` field in `dash-cli -getinfo` has been renamed to `chain` and now returns the current network name as defined in BIP70 (main, test, regtest).
- Importing blocks upon startup via the `bootstrap.dat` file no longer occurs by default. The file must now be specified with `-loadblock=<file>`.

Please check `Help -> Command-line options` in Qt wallet or `dashd --help` for
more information.

Backports from Bitcoin Core
---------------------------

This release introduces many hundreds updates from Bitcoin v0.18/v0.19/v0.20/v0.21/v22. Bitcoin changes that do not align with Dash’s product needs, such as SegWit and RBF, are excluded from our backporting. For additional detail on what’s included in Bitcoin, please refer to their release notes.

v18.1.0 Change log
==================

See detailed [set of changes](https://github.com/dashpay/dash/compare/v18.0.2...dashpay:v18.1.0).

Credits
=======

Thanks to everyone who directly contributed to this release:

- Kittywhiskers Van Gogh
- Konstantin Akimov
- Munkybooty
- Nathan Marley
- Odysseas Gabrielides
- Oleg Girko
- PastaPastaPasta
- strophy
- thephez
- UdjinM6
- Vijay

As well as everyone that submitted issues, reviewed pull requests, helped debug the release candidates, and write DIPs that were implemented in this release.

Older releases
==============

Dash was previously known as Darkcoin.

Darkcoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

Darkcoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Darkcoin tree 0.10.x used to be the closed source implementation of Darksend
which was released open source on Sep/25/2014.

Dash Core tree 0.11.x was a fork of Bitcoin Core tree 0.9,
Darkcoin was rebranded to Dash.

Dash Core tree 0.12.0.x was a fork of Bitcoin Core tree 0.10.

Dash Core tree 0.12.1.x was a fork of Bitcoin Core tree 0.12.

These release are considered obsolete. Old release notes can be found here:

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
