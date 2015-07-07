Omni Core version 0.0.9.1-rel is available from:

  https://github.com/mastercoin-MSC/mastercore/releases/tag/v0.0.9.1

0.0.9.1 is a minor release and not consensus critical.  An upgrade is only mandatory if you are using a version prior 0.0.9.

Please report bugs using the issue tracker at GitHub:

  https://github.com/mastercoin-MSC/mastercore/issues

IMPORTANT
=========

- This is the first experimental release of Omni Layer support in the QT UI, please be vigilant with testing and do not risk large amounts of Bitcoin and Omni Layer tokens.
- The transaction index is no longer defaulted to enabled.  You will need to ensure you have "txindex=1" (without the quotes) in your configuration file.
- If you are upgrading from a version earlier than 0.0.9-rel you must start with the --startclean parameter at least once to refresh your persistence files.
- The first time Omni Core is run the startup process may take an hour or more as existing Omni Layer transactions are parsed.  This is normal and should only be required the first time Omni Core is run.

Upgrading and downgrading
==========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely shut down (which might take a few minutes for older versions), then copy the new version of mastercored/mastercore-qt.

If you are upgrading from any version earlier than 0.0.9, the first time you run you must start with the --startclean parameter at least once to refresh your persistence files.

Downgrading
-----------

Downgrading is not currently supported as older versions will not provide accurate information.

Changelog
=========

General
-------

- Extra console debugging removed
- Bitcoin 0.10 blockchain detection (will refuse to start if out of order block storage is detected)
- txindex default value now matches Bitcoin Core (false)
- Update authorized alert senders
- Added support for TX70 to RPC output
- Fix missing LOCK of cs_main in selectCoins()
- Versioning code updated


UI
--

- New signal added for changes to Omni state (emitted from block handler for blocks containing Omni transactions)
- Fix double clicking a transaction in overview does not activate the Bitcoin history tab
- Splash screen updated to reflect new branding
- Fix frame alignment in overview page
- Update send page behaviour and layout per feedback
- Fix column resizing on balances tab
- Right align amounts in balances tab
- Various rebranding to Omni Core
- Rewritten Omni transaction history tab
- Add protection against long labels growing the UI size to ridiculous proportions
- Update signalling to all Omni pages to ensure up to date info
- Override display of Mastercoin metadata for rebrand (RPC unchanged)
- Acknowledgement of disclaimer will now be remembered
- Ecosystem display fixed in property lookup
- Fix intermittent startup freezes due to locks
