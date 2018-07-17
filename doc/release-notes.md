Bitcoin Core version 0.16.1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.16.1/>

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
the Linux kernel, macOS 10.8+, and Windows Vista and later. Windows XP is not supported.

Bitcoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============

Miner block size removed
------------------------

The `-blockmaxsize` option for miners to limit their blocks' sizes was
deprecated in version 0.15.1, and has now been removed. Miners should use the
`-blockmaxweight` option if they want to limit the weight of their blocks'
weights.

0.16.1 change log
------------------

### Policy
- #11423 `d353dd1` [Policy] Several transaction standardness rules (jl2012)

### Mining
- #12756 `e802c22` [config] Remove blockmaxsize option (jnewbery)

### Block and transaction handling
- #13199 `c71e535` Bugfix: ensure consistency of m_failed_blocks after reconsiderblock (sdaftuar)
- #13023 `bb79aaf` Fix some concurrency issues in ActivateBestChain() (skeees)

### P2P protocol and network code
- #12626 `f60e84d` Limit the number of IPs addrman learns from each DNS seeder (EthanHeilman)

### Wallet
- #13265 `5d8de76` Exit SyncMetaData if there are no transactions to sync (laanwj)
- #13030 `5ff571e` Fix zapwallettxes/multiwallet interaction. (jnewbery)

### GUI
- #12999 `1720eb3` Show the Window when double clicking the taskbar icon (ken2812221)
- #12650 `f118a7a` Fix issue: "default port not shown correctly in settings dialog" (251Labs)
- #13251 `ea487f9` Rephrase Bech32 checkbox texts, and enable it with legacy address default (fanquake)

### Build system
- #12474 `b0f692f` Allow depends system to support armv7l (hkjn)
- #12585 `72a3290` depends: Switch to downloading expat from GitHub (fanquake)
- #12648 `46ca8f3` test: Update trusted git root (MarcoFalke)
- #11995 `686cb86` depends: Fix Qt build with Xcode 9 (fanquake)
- #12636 `845838c` backport: #11995 Fix Qt build with Xcode 9 (fanquake)
- #12946 `e055bc0` depends: Fix Qt build with XCode 9.3 (fanquake)
- #12998 `7847b92` Default to defining endian-conversion DECLs in compat w/o config (TheBlueMatt)

### Tests and QA
- #12447 `01f931b` Add missing signal.h header (laanwj)
- #12545 `1286f3e` Use wait_until to ensure ping goes out (Empact)
- #12804 `4bdb0ce` Fix intermittent rpc_net.py failure. (jnewbery)
- #12553 `0e98f96` Prefer wait_until over polling with time.sleep (Empact)
- #12486 `cfebd40` Round target fee to 8 decimals in assert_fee_amount (kallewoof)
- #12843 `df38b13` Test starting bitcoind with -h and -version (jnewbery)
- #12475 `41c29f6` Fix python TypeError in script.py (MarcoFalke)
- #12638 `0a76ed2` Cache only chain and wallet for regtest datadir (MarcoFalke)
- #12902 `7460945` Handle potential cookie race when starting node (sdaftuar)
- #12904 `6c26df0` Ensure bitcoind processes are cleaned up when tests end (sdaftuar)
- #13049 `9ea62a3` Backports (MarcoFalke)
- #13201 `b8aacd6` Handle disconnect_node race (sdaftuar)

### Miscellaneous
- #12518 `a17fecf` Bump leveldb subtree (MarcoFalke)
- #12442 `f3b8d85` devtools: Exclude patches from lint-whitespace (MarcoFalke)
- #12988 `acdf433` Hold cs_main while calling UpdatedBlockTip() signal (skeees)
- #12985 `0684cf9` Windows: Avoid launching as admin when NSIS installer ends. (JeremyRand)

### Documentation
- #12637 `60086dd` backport: #12556 fix version typo in getpeerinfo RPC call help (fanquake)
- #13184 `4087dd0` RPC Docs: `gettxout*`: clarify bestblock and unspent counts (harding)
- #13246 `6de7543` Bump to Ubuntu Bionic 18.04 in build-windows.md (ken2812221)
- #12556 `e730b82` Fix version typo in getpeerinfo RPC call help (tamasblummer)

Credits
=======

Thanks to everyone who directly contributed to this release:

- 251
- Ben Woosley
- Chun Kuan Lee
- David A. Harding
- e0
- fanquake
- Henrik Jonsson
- JeremyRand
- Jesse Cohen
- John Newbery
- Johnson Lau
- Karl-Johan Alm
- Luke Dashjr
- MarcoFalke
- Matt Corallo
- Pieter Wuille
- Suhas Daftuar
- Tamas Blummer
- Wladimir J. van der Laan

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
