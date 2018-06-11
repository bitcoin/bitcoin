## (note: this is a temporary file, to be added-to by anybody, and moved to release-notes at release time)

Bitcoin Core version 0.16.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.16.0/>

This is a new major version release, including new features, various bugfixes
and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

The first time you run version 0.15.0 or newer, your chainstate database will be converted to a
new format, which will take anywhere from a few minutes to half an hour,
depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0 or higher. Upgrading
directly from 0.7.x and earlier without re-downloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

Wallets created in 0.16 and later are not compatible with versions prior to 0.16
and will not work if you try to use newly created wallets in older versions. Existing
wallets that were created with older versions are not affected by this.

Compatibility
==============

Bitcoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows 7 and newer (Windows XP is not supported).

Bitcoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

GUI changes
-----------

- Block storage can be limited under Preferences, in the Main tab. Undoing this setting requires downloading the full blockchain again. This mode is incompatible with -txindex and -rescan.

RPC changes
------------

### New `rescanblockchain` RPC

A new RPC `rescanblockchain` has been added to manually invoke a blockchain rescan.
The RPC supports start and end-height arguments for the rescan, and can be used in a
multiwallet environment to rescan the blockchain at runtime.

### New `savemempool` RPC
A new `savemempool` RPC has been added which allows the current mempool to be saved to
disk at any time to avoid it being lost due to crashes / power loss.

### Safe mode disabled by default

Safe mode is now disabled by default and must be manually enabled (with `-disablesafemode=0`) if you wish to use it. Safe mode is a feature that disables a subset of RPC calls - mostly related to the wallet and sending - automatically in case certain problem conditions with the network are detected. However, developers have come to regard these checks as not reliable enough to act on automatically. Even with safe mode disabled, they will still cause warnings in the `warnings` field of the `getneworkinfo` RPC and launch the `-alertnotify` command.

### Renamed script for creating JSON-RPC credentials

The `share/rpcuser/rpcuser.py` script was renamed to `share/rpcauth/rpcauth.py`. This script can be
used to create `rpcauth` credentials for a JSON-RPC user.

### Validateaddress improvements

The `validateaddress` RPC output has been extended with a few new fields, and support for segwit addresses (both P2SH and Bech32). Specifically:
* A new field `iswitness` is True for P2WPKH and P2WSH addresses ("bc1..." addresses), but not for P2SH-wrapped segwit addresses (see below).
* The existing field `isscript` will now also report True for P2WSH addresses.
* A new field `embedded` is present for all script addresses where the script is known and matches something that can be interpreted as a known address. This is particularly true for P2SH-P2WPKH and P2SH-P2WSH addresses. The value for `embedded` includes much of the information `validateaddress` would report if invoked directly on the embedded address.
* For multisig scripts a new `pubkeys` field was added that reports the full public keys involved in the script (if known). This is a replacement for the existing `addresses` field (which reports the same information but encoded as P2PKH addresses), represented in a more useful and less confusing way. The `addresses` field remains present for non-segwit addresses for backward compatibility.
* For all single-key addresses with known key (even when wrapped in P2SH or P2WSH), the `pubkey` field will be present. In particular, this means that invoking `validateaddress` on the output of `getnewaddress` will always report the `pubkey`, even when the address type is P2SH-P2WPKH.

### Low-level changes

- The `createrawtransaction` RPC will now accept an array or dictionary (kept for compatibility) for the `outputs` parameter. This means the order of transaction outputs can be specified by the client.
- The `fundrawtransaction` RPC will reject the previously deprecated `reserveChangeKey` option.
- `sendmany` now shuffles outputs to improve privacy, so any previously expected behavior with regards to output ordering can no longer be relied upon.
- The new RPC `testmempoolaccept` can be used to test acceptance of a transaction to the mempool without adding it.
- JSON transaction decomposition now includes a `weight` field which provides
  the transaction's exact weight. This is included in REST /rest/tx/ and
  /rest/block/ endpoints when in json mode. This is also included in `getblock`
  (with verbosity=2), `listsinceblock`, `listtransactions`, and
  `getrawtransaction` RPC commands.
- New `fees` field introduced in `getrawmempool`, `getmempoolancestors`, `getmempooldescendants` and
   `getmempoolentry` when verbosity is set to `true` with sub-fields `ancestor`, `base`, `modified`
   and `descendant` denominated in BTC. This new field deprecates previous fee fields, such as
   `fee`, `modifiedfee`, `ancestorfee` and `descendantfee`.

External wallet files
---------------------

The `-wallet=<path>` option now accepts full paths instead of requiring wallets
to be located in the -walletdir directory.

Newly created wallet format
---------------------------

If `-wallet=<path>` is specified with a path that does not exist, it will now
create a wallet directory at the specified location (containing a wallet.dat
data file, a db.log file, and database/log.?????????? files) instead of just
creating a data file at the path and storing log files in the parent
directory. This should make backing up wallets more straightforward than
before because the specified wallet path can just be directly archived without
having to look in the parent directory for transaction log files.

For backwards compatibility, wallet paths that are names of existing data files
in the `-walletdir` directory will continue to be accepted and interpreted the
same as before.

Low-level RPC changes
---------------------

- When bitcoin is not started with any `-wallet=<path>` options, the name of
  the default wallet returned by `getwalletinfo` and `listwallets` RPCs is
  now the empty string `""` instead of `"wallet.dat"`. If bitcoin is started
  with any `-wallet=<path>` options, there is no change in behavior, and the
  name of any wallet is just its `<path>` string.
- Passing an empty string (`""`) as the `address_type` parameter to
  `getnewaddress`, `getrawchangeaddress`, `addmultisigaddress`,
  `fundrawtransaction` RPCs is now an error. Previously, this would fall back
  to using the default address type. It is still possible to pass null or leave
  the parameter unset to use the default address type.

- Bare multisig outputs to our keys are no longer automatically treated as
  incoming payments. As this feature was only available for multisig outputs for
  which you had all private keys in your wallet, there was generally no use for
  them compared to single-key schemes. Furthermore, no address format for such
  outputs is defined, and wallet software can't easily send to it. These outputs
  will no longer show up in `listtransactions`, `listunspent`, or contribute to
  your balance, unless they are explicitly watched (using `importaddress` or
  `importmulti` with hex script argument). `signrawtransaction*` also still
  works for them.

### Logging

- The log timestamp format is now ISO 8601 (e.g. "2018-02-28T12:34:56Z").

Miner block size removed
------------------------

The `-blockmaxsize` option for miners to limit their blocks' sizes was
deprecated in V0.15.1, and has now been removed. Miners should use the
`-blockmaxweight` option if they want to limit the weight of their blocks'
weights.

Python Support
--------------

Support for Python 2 has been discontinued for all test files and tools.

Credits
=======

Thanks to everyone who directly contributed to this release:

- 251
- Aaron Clauson
- Aaron Golliver
- aaron-hanson
- Adam Langley
- Akio Nakamura
- Akira Takizawa
- Alejandro Avilés
- Alex Morcos
- Alin Rus
- Anditto Heristyo
- Andras Elso
- Andreas Schildbach
- Andrew Chow
- Anthony Towns
- azuchi
- Carl Dong
- Chris Moore
- Chris Stewart
- Christian Gentry
- Cory Fields
- Cristian Mircea Messel
- CryptAxe
- Dan Raviv
- Daniel Edgecumbe
- danra
- david60
- Donal O'Connor
- dongsamb
- Dusty Williams
- Eelis
- esneider
- Evan Klitzke
- fanquake
- Ferdinando M. Ametrano
- fivepiece
- flack
- Florian Schmaus
- gnuser
- Gregory Maxwell
- Gregory Sanders
- Henrik Jonsson
- Jack Grigg
- Jacky C
- James Evans
- James O'Beirne
- Jan Sarenik
- Jeff Rade
- Jeremiah Buddenhagen
- Jeremy Rubin
- Jim Posen
- jjz
- Joe Harvell
- Johannes Kanig
- John Newbery
- Johnson Lau
- Jonas Nick
- Jonas Schnelli
- João Barbosa
- Jorge Timón
- Karel Bílek
- Karl-Johan Alm
- klemens
- Kyuntae Ethan Kim
- laudaa
- Lawrence Nahum
- Lucas Betschart
- Luke Dashjr
- Luke Mlsna
- MarcoFalke
- Mark Friedenbach
- Marko Bencun
- Martin Ankerl
- Matt Corallo
- mruddy
- Murch
- NicolasDorier
- Pablo Fernandez
- Paul Berg
- Pedro Branco
- Pierre Rochard
- Pieter Wuille
- practicalswift
- Randolf Richardson
- Russell Yanofsky
- Samuel Dobson
- Sean Erle Johnson
- Shooter
- Sjors Provoost
- Suhas Daftuar
- Thomas Snider
- Thoragh
- Tim Shimmin
- Tomas van der Wansem
- Utsav Gupta
- Varunram Ganesh
- Vivek Ganesan
- Werner Lemberg
- William Casarin
- Willy Ko
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
