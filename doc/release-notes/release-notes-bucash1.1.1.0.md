Release Notes for Bitcoin Unlimited Cash Edition 1.1.1.0
=========================================================

Bitcoin Unlimited Cash Edition version 1.1.1.0 is now available from:

  <https://bitcoinunlimited.info/download>

Please report bugs using the issue tracker at github:

  <https://github.com/BitcoinUnlimited/BitcoinUnlimited/issues>

This is a minor release version based of Bitcoin Unlimited compatible
with the Bitcoin Cash specification you could find here:

https://github.com/Bitcoin-UAHF/spec/blob/master/uahf-technical-spec.md

Upgrading
---------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Bitcoin-Qt (on Mac) or
bitcoind/bitcoin-qt (on Linux).

Main Changes
------------

Major changes and features included in this release are:

- Expose CLTV protocol feature at the GUI level. Using `bitcoin-qt` it is possible to create and send transactions not spendable until a certain block or time in the future
- Expose OP_RETURN protocol feature at the GUI level. Using `bitcoin-qt` it is possible to create and send transaction with a "public label" -- that is, a string that is embedded in your transaction
- Fix sig validation bug related to pre-fork transaction
- Improve reindexing performances
- Adapt the qa tools (functional and unit tests) to work in a post-fork scenario
- Introduce new net magic set. For a period of time the client will accept both set of net magic bits (old and new). The mid term plan is to deprecate the old sets, in the mean time leverage the NODE_CASH service bit (1 << 5) to do preferential peering (already included in 1.1.0)
- Avoid forwarding non replay protected transactions and signing new transaction only with the new SIGHASH_FORKID scheme.
- Many fixes and small enhancements: orphan pool handling, extend unit tests coverage, improve dbcache performances.


Commit details
--------------

- `9442b9926` Add BUcash 1.1.1.0 release notes (Andrea Suisani)
- `5a1a7cd95` move case to same place as the other invalid cases (Andrew Stone)
- `5ce19bfca` fix cash test error where we were expecting the excessive set to work at 1MB, but now cash is mining 2MB by default (Andrew Stone)
- `62f393917` cleanup warnings (Andrew Stone)
- `e31e50536` bump to version 1.1.1.0 (Andrew Stone)
- `d257c175d` name too long for osx (Andrew Stone)
- `319970496` fix sig validation to accept and ignore the FORKID bit prior to the fork (#740) (Andrew Stone)
- `f81d51835` remove warning condition for large invalid fork (Andrew Stone)
- `8f62c79f2` When reindexing or importing don't use chain height to set fScriptChecks (Peter Tschipper)
- `3747d5686` fix issues running the unit tests with bitcoin cash move mocktime to after cash fork fix rawtransaction -- it must supply an amount in bitcoin cash.  fix qa tests for bitcoin cash, and add drop-to-pdb on exception fix cash req for amount, add fake amounts to signrawtransaction test (Andrew Stone)
- `a3e6d9b89` Fix compilation warning in thinblock_data_tests.cpp (Andrea Suisani)
- `6af4127e3` Fix a bunch of compilation warnings for multisig_tests.cpp (Andrea Suisani)
- `dd98ae01c` fix disconnect check in mininode (Andrew Stone)
- `99bef0774` add bitcoin cash network identifying magic number (Amaury SECHET)
- `da7fa6f00` Use set the forkTime far into the future for sendheaders.py (Peter Tschipper)
- `5d61cd464` [depends] miniupnpc 2.0.20170509 (fanquake)
- `8f196dba2` better icons (Andrew Stone)
- `9fe247b07` remove bad chars from package name and reposition text on the splashscreen (Andrew Stone)
- `50dfd339f` sign proper input (Andrew Stone)
- `3f2a85bb6` some branding (Andrew Stone)
- `fbab2ff34` add unit test for IsTxProbablyNewSigHash (Andrew Stone)
- `163a40414` reduce # of getheaders nodes (Andrew Stone)
- `f67c4c916` add a guess as to whether a tx uses the newsighash so that we can choose to not include it in the orphan pool (Andrew Stone)
- `e196a729d` Add #ifdef ENABLE_WALLET around pickLabelWithAddress, plus formatting and rebase conflict (marlengit)
- `7f64e7bf3` Fix tx list public label formatting. (marlengit)
- `bf30f589f` Fix send txout order for public labels. (marlengit)
- `4123ca803` Fix tx list for public labels. (marlengit)
- `5bc999487` Fix tx description to include public label. Fix send inputs and confirmation for public label. Add Receive tx 'Copy address' menu. (marlengit)
- `254f26b49` Fixes for coin freeze input validation. (marlengit)
- `3ed097b86` Revert ListLockedCoins const. (marlengit)
- `a3ac719f7` Reformat src/main.cpp. (marlengit)
- `6cbe7a535` Merge dev fix conflicts. (marlengit)
- `dba343641` fix ctor vs declaration ordering warnsings (Andrew Stone)
- `bb71997d5` Fix Overview balance updates during coin freeze life cycle. (marlengit)
- `cae925924` fix no wallet compilation error (Andrew Stone)
- `3f7e82586` Update Freeze transaction calculation and update UI (marlengit)
- `295cad4b0` Add #ifdef ENABLE_WALLET in multisig_tests.cpp. (marlengit)
- `de930aa25` Rework opreturn_send test. (marlengit)
- `2586c18a4` Update wallet version to match freeze. (marlengit)
- `42ac82d64` Add wallet recognition of OP_RETURN, basic ui for Public Label, rebase `dev` (marlengit)
- `251075628` Update coin freeze dialog. (marlengit)
- `5e136ff9f` Update Makefiles. (marlengit)
- `e13617597` Add coin freeze dialog. (marlengit)
- `ec93f011f` Add freeze warning in Send confirmation. (marlengit)
- `c4717cb7a` Update type for nFreezeLockTime from int64_t to CScriptNum. (marlengit)
- `c76015524` Add send support for cltv lock-by-blocktime. (marlengit)
- `051b0ec0e` Add support for datetime locktimes. (marlengit)
- `ec346ae15` Update sFreezeLockTime to QString. (marlengit)
- `cbb43ca58` Add freezeBlock limit check. Update sFreezeLockTime conversion. (marlengit)
- `2764b98bd` Add freezeDateTime with calendar. Add Freeze to SendCoinsRecipient. (marlengit)
- `2b6abf95e` Rebase against `dev` (marlengit)
- `deaf0d0c2` fix UI issues (Andrew Stone)
- `324727f28` Update wallet version to match freeze. (marlengit)
- `5ec2dc109` Add wallet recognition of OP_RETURN, basic ui for Public Label, merge `dev` (marlengit)
- `7c1973157` Update coin freeze dialog. (marlengit)
- `380e2e31c` fix test to account for disallowing old tx in mempool (Andrew Stone)
- `a0409bd76` formatting (Andrew Stone)
- `2727abb8e` Update Makefiles. (marlengit)
- `73ade81c1` Add coin freeze dialog. (marlengit)
- `89d170ce9` Add freeze warning in Send confirmation. (marlengit)
- `b119495e6` Update type for nFreezeLockTime from int64_t to CScriptNum. (marlengit)
- `995f2f624` Update formatting. (marlengit)
- `5cdf7c168` Add send support for cltv lock-by-blocktime. Update cltv_freeze unit test. Update Transaction ui to display addresses for sent to self txs. (marlengit)
- `7efe1bfab` Add support for datetime locktimes. (marlengit)
- `ac9d44b0e` Update sFreezeLockTime to QString. (marlengit)
- `a176e2188` Add freezeBlock limit check. Update sFreezeLockTime conversion. (marlengit)
- `9b60af665` Add freezeDateTime with calendar. Add Freeze to SendCoinsRecipient. (marlengit)
- `5781d3ebd` Basic wallet ui with CLTV coin freeze, watch & spend. (marlengit)
- `fd6e94000` limit bucash transactions to the new format.  Add parameter to getinfo telling what fork you are on (Andrew Stone)
- `f09c01f87` remove dead code around sigop mempool admission (Andrew Stone)
- `e013f84ce` fix issue where node whose headers are downloaded is not allowed to download blocks (Andrew Stone)
- `5659b211e` Don't return an error dialog if we request a shutdown. (Peter Tschipper)
- `1601f198f` Check for fRequestShutdown in ConnectTip() (Peter Tschipper)
- `95d43ac1c` Use the more accurate DynamicMemoryUsage() when trimming the dbcache (Peter Tschipper)
- `92a76e0b4` Make sure to flush the chainstate if reindexing or importing (Peter Tschipper)
- `091e2a8a5` Periodically check and correct any drift in cachedCoinsUsage (Peter Tschipper)
- `d1a41e6a2` Fix unit tests (Peter Tschipper)
- `1649e5b20` Increment Iterator and move locks (Peter Tschipper)
- `23d447c39` Change when we flush the chainstate to disk (Peter Tschipper)
- `749a04f55` Do not UnCache entries if they are already known. (Peter Tschipper)
- `62e84ccd4` Continuously trim the dbcache while preventing continuous flushing (Peter Tschipper)
- `9bb0b140e` Performance improvement for in memory dbcache (Peter Tschipper)
- `501dc7325` clean up some comments, add a few doctests (more needed) (Andrew Stone)
- `0ea96e202` add BU messages to the mininode parser, and reorganize the mininode objects for use outside of mininode, add a simple node simulator that handles BU messages (Andrew Stone)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Amaury SECHET
- Andrea Suisani
- Andrew Stone
- marlengit
- Peter Tschipper

We also have backported changes from other projects, this is a list of PR ported from Core:

- `e61fc2d7c` Merge #10414: [depends] miniupnpc 2.0.20170509

Following all the indirect contributors whose work has been imported via the above backports:

- fanquake
