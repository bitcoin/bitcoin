Bitcoin Core version 0.13.2 is now available from:

  <https://bitcoinrupee.org/bin/bitcoinrupee-core-0.13.2/>

This is a new minor version release, including various bugfixes and
performance improvements, as well as updated translations.

Please report bugs using the issue tracker at github:

  <https://github.com/bitcoinrupee/bitcoinrupee/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoinrupeecore.org/en/list/announcements/join/>

Compatibility
==============

Microsoft ended support for Windows XP on [April 8th, 2014](https://www.microsoft.com/en-us/WindowsForBusiness/end-of-xp-support),
an OS initially released in 2001. This means that not even critical security
updates will be released anymore. Without security updates, using a bitcoinrupee
wallet on a XP machine is irresponsible at least.

In addition to that, with 0.12.x there have been varied reports of Bitcoin Core
randomly crashing on Windows XP. It is [not clear](https://github.com/bitcoinrupee/bitcoinrupee/issues/7681#issuecomment-217439891)
what the source of these crashes is, but it is likely that upstream
libraries such as Qt are no longer being tested on XP.

We do not have time nor resources to provide support for an OS that is
end-of-life. From 0.13.0 on, Windows XP is no longer supported. Users are
suggested to upgrade to a newer version of Windows, or install an alternative OS
that is supported.

No attempt is made to prevent installing or running the software on Windows XP,
you can still do so at your own risk, but do not expect it to work: do not
report issues about Windows XP to the issue tracker.

From 0.13.1 onwards OS X 10.7 is no longer supported. 0.13.0 was intended to work on 10.7+, 
but severe issues with the libc++ version on 10.7.x keep it from running reliably. 
0.13.1 now requires 10.8+, and will communicate that to 10.7 users, rather than crashing unexpectedly.

Notable changes
===============

Change to wallet handling of mempool rejection
-----------------------------------------------

When a newly created transaction failed to enter the mempool due to
the limits on chains of unconfirmed transactions the sending RPC
calls would return an error.  The transaction would still be queued
in the wallet and, once some of the parent transactions were
confirmed, broadcast after the software was restarted.

This behavior has been changed to return success and to reattempt
mempool insertion at the same time transaction rebroadcast is
attempted, avoiding a need for a restart.

Transactions in the wallet which cannot be accepted into the mempool
can be abandoned with the previously existing abandontransaction RPC
(or in the GUI via a context menu on the transaction).


0.13.2 Change log
=================

Detailed release notes follow. This overview includes changes that affect
behavior, not code moves, refactors and string updates. For convenience in locating
the code changes and accompanying discussion, both the pull request and
git merge commit are mentioned.

### Consensus
- #9293 `e591c10` [0.13 Backport #9053] IBD using chainwork instead of height and not using header timestamp (gmaxwell)
- #9053 `5b93eee` IBD using chainwork instead of height and not using header timestamps (gmaxwell)

### RPC and other APIs
- #8845 `1d048b9` Don't return the address of a P2SH of a P2SH (jnewbery)
- #9041 `87fbced` keypoololdest denote Unix epoch, not GMT (s-matthew-english)
- #9122 `f82c81b` fix getnettotals RPC description about timemillis (visvirial)
- #9042 `5bcb05d` [rpc] ParseHash: Fail when length is not 64 (MarcoFalke)
- #9194 `f26dab7` Add option to return non-segwit serialization via rpc (instagibbs)
- #9347 `b711390` [0.13.2] wallet/rpc backports (MarcoFalke)
- #9292 `c365556` Complain when unknown rpcserialversion is specified (sipa)
- #9322 `49a612f` [qa] Don't set unknown rpcserialversion (MarcoFalke)

### Block and transaction handling
- #8357 `ce0d817` [mempool] Fix relaypriority calculation error (maiiz)
- #9267 `0a4aa87` [0.13 backport #9239] Disable fee estimates for a confirm target of 1 block (morcos)
- #9196 `0c09d9f` Send tip change notification from invalidateblock (ryanofsky)

### P2P protocol and network code
- #8995 `9ef3875` Add missing cs_main lock to ::GETBLOCKTXN processing (TheBlueMatt)
- #9234 `94531b5` torcontrol: Explicitly request RSA1024 private key (laanwj)
- #8637 `2cad5db` Compact Block Tweaks (rebase of #8235) (sipa)
- #9058 `286e548` Fixes for p2p-compactblocks.py test timeouts on travis (#8842) (ryanofsky)
- #8865 `4c71fc4` Decouple peer-processing-logic from block-connection-logic (TheBlueMatt)
- #9117 `6fe3981` net: don't send feefilter messages before the version handshake is complete (theuni)
- #9188 `ca1fd75` Make orphan parent fetching ask for witnesses (gmaxwell)
- #9052 `3a3bcbf` Use RelevantServices instead of node_network in AttemptToEvict (gmaxwell)
- #9048 `9460771` [0.13 backport #9026] Fix handling of invalid compact blocks (sdaftuar)
- #9357 `03b6f62` [0.13 backport #9352] Attempt reconstruction from all compact block announcements (sdaftuar)
- #9189 `b96a8f7` Always add default_witness_commitment with GBT client support (sipa)
- #9253 `28d0f22` Fix calculation of number of bound sockets to use (TheBlueMatt)
- #9199 `da5a16b` Always drop the least preferred HB peer when adding a new one (gmaxwell)

### Build system
- #9169 `d1b4da9` build: fix qt5.7 build under macOS (theuni)
- #9326 `a0f7ece` Update for OpenSSL 1.1 API (gmaxwell)
- #9224 `396c405` Prevent FD_SETSIZE error building on OpenBSD (ivdsangen)

### GUI
- #8972 `6f86b53` Make warnings label selectable (jonasschnelli) (MarcoFalke)
- #9185 `6d70a73` Fix coincontrol sort issue (jonasschnelli)
- #9094 `5f3a12c` Use correct conversion function for boost::path datadir (laanwj)
- #8908 `4a974b2` Update bitcoinrupee-qt.desktop (s-matthew-english)
- #9190 `dc46b10` Plug many memory leaks (laanwj)

### Wallet
- #9290 `35174a0` Make RelayWalletTransaction attempt to AcceptToMemoryPool (gmaxwell)
- #9295 `43bcfca` Bugfix: Fundrawtransaction: don't terminate when keypool is empty (jonasschnelli)
- #9302 `f5d606e` Return txid even if ATMP fails for new transaction (sipa)
- #9262 `fe39f26` Prefer coins that have fewer ancestors, sanity check txn before ATMP (instagibbs)

### Tests and QA
- #9159 `eca9b46` Wait for specific block announcement in p2p-compactblocks (ryanofsky)
- #9186 `dccdc3a` Fix use-after-free in scheduler tests (laanwj)
- #9168 `3107280` Add assert_raises_message to check specific error message (mrbandrews)
- #9191 `29435db` 0.13.2 Backports (MarcoFalke)
- #9077 `1d4c884` Increase wallet-dump RPC timeout (ryanofsky)
- #9098 `ecd7db5` Handle zombies and cluttered tmpdirs (MarcoFalke)
- #8927 `387ec9d` Add script tests for FindAndDelete in pre-segwit and segwit scripts (jl2012)
- #9200 `eebc699` bench: Fix subtle counting issue when rescaling iteration count (laanwj)

### Miscellaneous
- #8838 `094848b` Calculate size and weight of block correctly in CreateNewBlock() (jnewbery)
- #8920 `40169dc` Set minimum required Boost to 1.47.0 (fanquake)
- #9251 `a710a43` Improvement of documentation of command line parameter 'whitelist' (wodry)
- #8932 `106da69` Allow bitcoinrupee-tx to create v2 transactions (btcdrak)
- #8929 `12428b4` add software-properties-common (sigwo)
- #9120 `08d1c90` bug: Missed one "return false" in recent refactoring in #9067 (UdjinM6)
- #9067 `f85ee01` Fix exit codes (UdjinM6)
- #9340 `fb987b3` [0.13] Update secp256k1 subtree (MarcoFalke)
- #9229 `b172377` Remove calls to getaddrinfo_a (TheBlueMatt)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Alex Morcos
- BtcDrak
- Cory Fields
- fanquake
- Gregory Maxwell
- Gregory Sanders
- instagibbs
- Ivo van der Sangen
- jnewbery
- Johnson Lau
- Jonas Schnelli
- Luke Dashjr
- maiiz
- MarcoFalke
- Masahiko Hyuga
- Matt Corallo
- matthias
- mrbandrews
- Pavel Janík
- Pieter Wuille
- randy-waterhouse
- Russell Yanofsky
- S. Matthew English
- Steven
- Suhas Daftuar
- UdjinM6
- Wladimir J. van der Laan
- wodry

As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoinrupee/).
