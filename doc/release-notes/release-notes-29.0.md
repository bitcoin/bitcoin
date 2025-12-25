Bitcoin Core version 29.0 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-29.0/>

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
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on macOS)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

Bitcoin Core is supported and tested on operating systems using the
Linux Kernel 3.17+, macOS 13+, and Windows 10+. Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them. It is not recommended to use Bitcoin Core on
unsupported systems.

Notable changes
===============

### P2P and Network Changes

- Support for UPnP was dropped. If you want to open a port automatically, consider using the `-natpmp`
option instead, which uses PCP or NAT-PMP depending on router support. (#31130)

- libnatpmp was replaced with a built-in implementation of PCP and NAT-PMP (still enabled using the `-natpmp` option). This supports automatic IPv4 port forwarding as well as IPv6 pinholing. (#30043)

- When the `-port` configuration option is used, the default onion listening port will now
be derived to be that port + 1 instead of being set to a fixed value (8334 on mainnet).
This re-allows setups with multiple local nodes using different `-port` and not using `-bind`,
which would lead to a startup failure in v28.0 due to a port collision.
Note that a `HiddenServicePort` manually configured in `torrc` may need adjustment if used in
connection with the `-port` option.
For example, if you are using `-port=5555` with a non-standard value and not using `-bind=...=onion`,
previously Bitcoin Core would listen for incoming Tor connections on `127.0.0.1:8334`.
Now it would listen on `127.0.0.1:5556` (`-port` plus one). If you configured the hidden service manually
in torrc now you have to change it from `HiddenServicePort 8333 127.0.0.1:8334` to `HiddenServicePort 8333
127.0.0.1:5556`, or configure bitcoind with `-bind=127.0.0.1:8334=onion` to get the previous behavior.
(#31223)

- Upon receiving an orphan transaction (an unconfirmed transaction that spends unknown inputs), the node will attempt to download missing parents from all peers who announced the orphan. This change may increase bandwidth usage but make orphan-handling more reliable. (#31397)

### Mempool Policy and Mining Changes

- Ephemeral dust is a new concept that allows a single
dust output in a transaction, provided the transaction
is zero fee. In order to spend any unconfirmed outputs
from this transaction, the spender must also spend
this dust in addition to any other desired outputs.
In other words, this type of transaction
should be created in a transaction package where
the dust is both created and spent simultaneously. (#30239)

- Due to a bug, the default block reserved weight (`4,000 WU`) for fixed-size block header, transactions count, and coinbase transaction was reserved twice and could not be lowered. As a result the total reserved weight was always `8,000 WU`, meaning that even when specifying a `-blockmaxweight` higher than the default (even to the max of `4,000,000 WU`), the actual block size will never exceed `3,992,000 WU`.
The fix consolidates the reservation into a single place and introduces a new startup option, `-blockreservedweight` which specifies the reserved weight directly. The default value of `-blockreservedweight` is set to `8,000 WU` to ensure backward compatibility for users who relied on the previous behavior of `-blockmaxweight`.
The minimum value of `-blockreservedweight` is set to `2,000 WU`. Users setting `-blockreservedweight` below the default should ensure that the total weight of their block header, transaction count, and coinbase transaction does not exceed the reduced value or they may risk mining an invalid block. (#31384)

### Updated RPCs

- The RPC `testmempoolaccept` response now includes a `reject-details` field in some cases,
similar to the complete error messages returned by `sendrawtransaction` (#28121)

- Duplicate blocks submitted with `submitblock` will now persist their block data
even if it was previously pruned. If pruning is activated, the data will be
pruned again eventually once the block file it is persisted in is selected for
pruning. This is consistent with the behaviour of `getblockfrompeer` where the
block is persisted as well even when pruning. (#31175)

- `getmininginfo` now returns `nBits` and the current target in the `target` field. It also returns a `next` object which specifies the `height`, `nBits`, `difficulty`, and `target` for the next block. (#31583)

- `getblock` and `getblockheader` now return the current target in the `target` field (#31583)

- `getblockchaininfo` and `getchainstates` now return `nBits` and the current target in the `target` field (#31583)

- the `getblocktemplate` RPC `curtime` (BIP22) and `mintime` (BIP23) fields now
  account for the timewarp fix proposed in BIP94 on all networks. This ensures
  that, in the event a timewarp fix softfork activates on mainnet, un-upgraded
  miners will not accidentally violate the timewarp rule. (#31376, #31600)
As a reminder, it's important that any software which uses the `getblocktemplate`
RPC takes these values into account (either `curtime` or `mintime` is fine).
Relying only on a clock can lead to invalid blocks under some circumstances,
especially once a timewarp fix is deployed. (#31600)

### New RPCs

- `getdescriptoractivity` can be used to find all spend/receive activity relevant to
  a given set of descriptors within a set of specified blocks. This call can be used with
  `scanblocks` to lessen the need for additional indexing programs. (#30708)

### Updated REST APIs

- `GET /rest/block/<BLOCK-HASH>.json` and `GET /rest/headers/<BLOCK-HASH>.json` now return the current target in the `target` field

### Updated Settings

- The maximum allowed value for the `-dbcache` configuration option has been
  dropped due to recent UTXO set growth. Note that before this change, large `-dbcache`
  values were automatically reduced to 16 GiB (1 GiB on 32 bit systems). (#28358)

- Handling of negated `-noseednode`, `-nobind`, `-nowhitebind`, `-norpcbind`, `-norpcallowip`, `-norpcwhitelist`, `-notest`, `-noasmap`, `-norpcwallet`, `-noonlynet`, and `-noexternalip` options has changed. Previously negating these options had various confusing and undocumented side effects. Now negating them just resets the settings and restores default behaviors, as if the options were not specified.

- Starting with v28.0, the `-mempoolfullrbf` startup option was set to
default to `1`. With widespread adoption of this policy, users no longer
benefit from disabling it, so the option has been removed, making full
replace-by-fee the standard behavior. (#30592)

- Setting `-upnp` will now log a warning and be interpreted as `-natpmp`. Consider using `-natpmp` directly instead. (#31130, #31916)

- As a safety check, Bitcoin core will **fail to start** when `-blockreservedweight` init parameter value is lower than `2000` weight units. Bitcoin Core will also **fail to start** if the `-blockmaxweight` or `-blockreservedweight` init parameter exceeds consensus limit of `4,000,000 WU`.

- Passing `-debug=0` or `-debug=none` now behaves like `-nodebug`: previously set debug categories will be cleared, but subsequent `-debug` options will still be applied.

- The default for `-rpcthreads` has been changed from 4 to 16, and the default for `-rpcworkqueue` has been changed from 16 to 64. (#31215).

### Build System

The build system has been migrated from Autotools to CMake:

1. The minimum required CMake version is 3.22.
2. In-source builds are not allowed. When using a subdirectory within the root source tree as a build directory, it is recommended that its name includes the substring "build".
3. CMake variables may be used to configure the build system. **Some defaults have changed.** For example, you will now need to add `-DWITH_ZMQ=ON` to build with zmq and `-DBUILD_GUI=ON` to build `bitcoin-qt`. See [Autotools to CMake Options Mapping](https://github.com/bitcoin-core/bitcoin-devwiki/wiki/Autotools-to-CMake-Options-Mapping) for details.
4. For single-configuration generators, the default build configuration (`CMAKE_BUILD_TYPE`) is "RelWithDebInfo". However, for the "Release" configuration, CMake defaults to the compiler optimization flag `-O3`, which has not been extensively tested with Bitcoin Core. Therefore, the build system replaces it with `-O2`.
5. By default, the built executables and libraries are located in the `bin/` and `lib/` subdirectories of the build directory.
6. The build system supports component‐based installation. The names of the installable components coincide with the build target names. For example:
```
cmake -B build
cmake --build build --target bitcoind
cmake --install build --component bitcoind
```

7. If any of the `CPPFLAGS`, `CFLAGS`, `CXXFLAGS` or `LDFLAGS` environment variables were used in your Autotools-based build process, you should instead use the corresponding CMake variables (`APPEND_CPPFLAGS`, `APPEND_CFLAGS`, `APPEND_CXXFLAGS` and `APPEND_LDFLAGS`). Alternatively, if you opt to use the dedicated `CMAKE_<...>_FLAGS` variables, you must ensure that the resulting compiler or linker invocations are as expected.

For more detailed guidance on configuring and using CMake, please refer to the official [CMake documentation](https://cmake.org/cmake/help/latest/) and [CMake’s User Interaction Guide](https://cmake.org/cmake/help/latest/guide/user-interaction/index.html). Additionally, consult platform-specific `doc/build-*.md` build guides for instructions tailored to your operating system.

## Low-Level Changes

### Tools and Utilities

- A new tool [`utxo_to_sqlite.py`](https://github.com/bitcoin/bitcoin/blob/v29.0/contrib/utxo-tools/utxo_to_sqlite.py)
  converts a compact-serialized UTXO snapshot (as created with the
  `dumptxoutset` RPC) to a SQLite3 database. Refer to the script's `--help`
  output for more details. (#27432)

### Tests

- The BIP94 timewarp attack mitigation (designed for testnet4) is no longer active on the regtest network. (#31156)

### Dependencies

- MiniUPnPc and libnatpmp have been removed as dependencies (#31130, #30043).

Credits
=======

Thanks to everyone who directly contributed to this release:

- 0xb10c
- Adlai Chandrasekhar
- Afanti
- Alfonso Roman Zubeldia
- am-sq
- Andre
- Andre Alves
- Anthony Towns
- Antoine Poinsot
- Ash Manning
- Ava Chow
- Boris Nagaev
- Brandon Odiwuor
- brunoerg
- Chris Stewart
- Cory Fields
- costcould
- Daniel Pfeifer
- Daniela Brozzoni
- David Gumberg
- dergoegge
- epysqyli
- espi3
- Eval EXEC
- Fabian Jahr
- fanquake
- furszy
- Gabriele Bocchi
- glozow
- Greg Sanders
- Gutflo
- Hennadii Stepanov
- Hodlinator
- i-am-yuvi
- ion-
- ismaelsadeeq
- Jadi
- James O'Beirne
- Jeremy Rand
- Jon Atack
- jurraca
- Kay
- kevkevinpal
- l0rinc
- laanwj
- Larry Ruane
- Lőrinc
- Maciej S. Szmigiero
- Mackain
- MarcoFalke
- marcofleon
- Marnix
- Martin Leitner-Ankerl
- Martin Saposnic
- Martin Zumsande
- Matthew Zipkin
- Max Edwards
- Michael Dietz
- naiyoma
- Nicola Leonardo Susca
- omahs
- pablomartin4btc
- Pieter Wuille
- Randall Naar
- RiceChuan
- rkrux
- Roman Zeyde
- Ryan Ofsky
- Sebastian Falbesoner
- secp512k2
- Sergi Delgado Segura
- Simon
- Sjors Provoost
- stickies-v
- Suhas Daftuar
- tdb3
- TheCharlatan
- tianzedavid
- Torkel Rogstad
- Vasil Dimov
- wgyt
- willcl-ark
- yancy

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
