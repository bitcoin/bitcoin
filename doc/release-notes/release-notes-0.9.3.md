XBit Core version 0.9.3 is now available from:

  https://xbit.org/bin/0.9.3/

This is a new minor version release, bringing only bug fixes and updated
translations. Upgrading to this release is recommended.

Please report bugs using the issue tracker at github:

  https://github.com/xbit/xbit/issues

Upgrading and downgrading
==========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/XBit-Qt (on Mac) or
xbitd/xbit-qt (on Linux).

If you are upgrading from version 0.7.2 or earlier, the first time you run
0.9.3 your blockchain files will be re-indexed, which will take anywhere from 
30 minutes to several hours, depending on the speed of your machine.

Downgrading warnings
--------------------

The 'chainstate' for this release is not always compatible with previous
releases, so if you run 0.9.x and then decide to switch back to a
0.8.x release you might get a blockchain validation error when starting the
old release (due to 'pruned outputs' being omitted from the index of
unspent transaction outputs).

Running the old release with the -reindex option will rebuild the chainstate
data structures and correct the problem.

Also, the first time you run a 0.8.x release on a 0.9 wallet it will rescan
the blockchain for missing spent coins, which will take a long time (tens
of minutes on a typical machine).

0.9.3 Release notes
=======================

RPC:
- Avoid a segfault on getblock if it can't read a block from disk
- Add paranoid return value checks in base58

Protocol and network code:
- Don't poll showmyip.com, it doesn't exist anymore
- Add a way to limit deserialized string lengths and use it
- Add a new checkpoint at block 295,000
- Increase IsStandard() scriptSig length
- Avoid querying DNS seeds, if we have open connections
- Remove a useless millisleep in socket handler
- Stricter memory limits on CNode
- Better orphan transaction handling
- Add `-maxorphantx=<n>` and `-maxorphanblocks=<n>` options for control over the maximum orphan transactions and blocks

Wallet:
- Check redeemScript size does not exceed 520 byte limit
- Ignore (and warn about) too-long redeemScripts while loading wallet

GUI:
- fix 'opens in testnet mode when presented with a BIP-72 link with no fallback'
- AvailableCoins: acquire cs_main mutex
- Fix unicode character display on MacOSX

Miscellaneous:
- key.cpp: fail with a friendlier message on missing ssl EC support
- Remove bignum dependency for scripts
- Upgrade OpenSSL to 1.0.1i (see https://www.openssl.org/news/secadv_20140806.txt - just to be sure, no critical issues for XBit Core)
- Upgrade miniupnpc to 1.9.20140701
- Fix boost detection in build system on some platforms

Credits
--------

Thanks to everyone who contributed to this release:

- Andrew Poelstra
- Cory Fields
- Gavin Andresen
- Jeff Garzik
- Johnathan Corgan
- Julian Haight
- Michael Ford
- Pavel Vasin
- Peter Todd
- phantomcircuit
- Pieter Wuille
- Rose Toomey
- Ruben Dario Ponticelli
- shshshsh
- Trevin Hofmann
- Warren Togami
- Wladimir J. van der Laan
- Zak Wilcox

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/xbit/).
