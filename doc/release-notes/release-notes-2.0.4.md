Raven Core version *2.0.4.0* is now available!!
==============

  <https://github.com/RavenProject/Ravencoin/releases/tag/v2.0.4>


This is a major release containing bug fixes for 2.0.3.0.  It is highly recommended that users 
upgrade to this version.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/RavenProject/Ravencoin/issues>

To receive security and update notifications, please subscribe to:

  <https://ravencoin.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the 
installer (on Windows) or just copy over `/Applications/Raven-Qt` (on Mac)
or `ravend`/`raven-qt` (on Linux).

The first time you run version 2.0.4 or higher, your chainstate database may
be converted to a new format, which will take anywhere from a few minutes to
half an hour, depending on the speed of your machine.

Downgrading warning
==============

The chainstate database for this release is not compatible with previous
releases, so if you run 2.0.4 and then decide to switch back to any
older version, you will need to run the old release with the `-reindex-chainstate`
option to rebuild the chainstate data structures in the old format.

If your node has pruning enabled, this will entail re-downloading and
processing the entire blockchain.

It is not recommended that users downgrade their version.  This version contains
changes that *will* fork the chain, users not running 2.0.4 (or later) will be not
be able to participate in this fork process and will be left on the old chain which 
will not be valid.

Compatibility
==============

Raven Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later. 32-bit versions of Windows,
and Windows XP are not supported.

Raven Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
==============

- Fix testnet chain syncing.
- Increase chaining depth.
- Fix for -txindex after immediate BIP activation
- Fix null pointer bug.
- Fix qt amount formatting.
- Python script for bulk issuance in assets/tools.
- Python script for finding signed meta data.


2.0.4.0-b175d7350 Change log
==============

Changelog available here: <https://github.com/RavenProject/Ravencoin/commits/release_2.0.4>

Credits
==============

Thanks to everyone who directly contributed to this release:

- Most importantly - The Raven Community!
- Tron Black
- Jesse Empey
- Jeremy Anderson
- Corbin Fox
- Daben Steele
- Cade Call
- @Roshii
- @underdarkskies
- Mark Ney
