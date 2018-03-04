## (note: this is a temporary file, to be added-to by anybody, and moved to release-notes at release time)

Particl Core version 0.16.0.1 is now available from:

  <https://particl.io/>

This is a new major version release, including new features, various bugfixes
and performance improvements.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/particl/particl-core/issues>


How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Particl-Qt` (on Mac)
or `particld`/`particl-qt` (on Linux).

The first time you run version 0.15.0 or newer, your chainstate database will need to be converted to a
new format, start with the -reindex flag.


Downgrading warning
-------------------

Wallets created in 0.16 and later are not compatible with versions prior to 0.16
and will not work if you try to use newly created wallets in older versions. Existing
wallets that were created with older versions are not affected by this.

Compatibility
==============

Particl Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later. Windows XP is not supported.

Particl Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============


RPC changes
------------

The scanchain rpc command is deprecated, please use rescanblockchain instead.








