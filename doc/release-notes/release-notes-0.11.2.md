*This is a draft!*

0.11.2 Release notes
====================


Darkcoin Core version 0.11.2 is now available from:

  https://darkcoin.io/download

This is a new minor version release, bringing only bug fixes and updated
translations. Upgrading to this release is recommended.

Please report bugs using the issue tracker at github:

  https://github.com/darkcoin/darkcoin/issues


How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Darkcoin-Qt (on Mac) or
darkcoind/darkcoin-qt (on Linux).


Mining and relay policy enhancements
------------------------------------

Darkcoin Core's block templates are now for version 3 blocks only, and any
mining software relying on its `getblocktemplate` must be updated in parallel
to use libblkmaker either version 0.4.2 or any version from 0.5.1 onward. If you
are solo mining, this will affect you the moment you upgrade Darkcoin Core,
which must be done prior to BIP66 achieving its 951/1001 status. If you are
mining with the stratum mining protocol: this does not affect you. If you are
mining with the getblocktemplate protocol to a pool: this will affect you at the
pool operator's discretion, which must be no later than BIP66 achieving its
951/1001 status.


BIP 66: strict DER encoding for signatures
------------------------------------------

Darkcoin Core 0.11.2 implements BIP 66, which introduces block version 3, and a
new consensus rule, which prohibits non-DER signatures. Such transactions have
been non-standard since Darkcoin 0.8, but were technically still permitted
inside blocks.
This change breaks the dependency on OpenSSL's signature parsing, and is
required if implementations would want to remove all of OpenSSL from the
consensus code.
The same miner-voting mechanism as in BIP 34 is used: when 751 out of a
sequence of 1001 blocks have version number 3 or higher, the new consensus
rule becomes active for those blocks. When 951 out of a sequence of 1001
blocks have version number 3 or higher, it becomes mandatory for all blocks.
Backward compatibility with current mining software is NOT provided, thus
miners should read the first paragraph of "Mining and relay policy
enhancements" above.

Also compare with [upstream release notes](https://github.com/bitcoin/bitcoin/blob/0.10/doc/release-notes.md#mining-and-relay-policy-enhancements).
More info on [BIP 66](https://github.com/bitcoin/bips/blob/master/bip-0066.mediawiki).


0.11.2 changelog
----------------

Validation:
- `b8e81b7` consensus: guard against openssl's new strict DER checks
- `60c51f1` fail immediately on an empty signature
- `037bfef` Improve robustness of DER recoding code

Command-line options:
- `cd5164a` Make -proxy set all network types, avoiding a connect leak.

P2P:
- `bb424e4` Limit the number of new addressses to accumulate

RPC:
- `0a94661` Disable SSLv3 (in favor of TLS) for the RPC client and server.

Build system:
- `f047dfa` gitian: openssl-1.0.1i.tar.gz -> openssl-1.0.1k.tar.gz
- `5b9f78d` build: Fix OSX build when using Homebrew and qt5
- `ffab1dd` Keep symlinks when copying into .app bundle
- `613247f` osx: fix signing to make Gatekeeper happy (again)

Miscellaneous:
- `25b49b5` Refactor -alertnotify code
- `2743529` doc: Add instructions for consistent Mac OS X build names


Credits
--------

Thanks to who contributed to this release, at least:

- Evan Duffield

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/darkcoin/).
