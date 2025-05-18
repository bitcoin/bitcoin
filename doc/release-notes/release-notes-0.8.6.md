Tortoisecoin-Qt version 0.8.6 final is now available from:

  http://sourceforge.net/projects/tortoisecoin/files/Tortoisecoin/tortoisecoin-0.8.6/

This is a maintenance release to fix a critical bug; we urge all users to upgrade.

Please report bugs using the issue tracker at github:

  https://github.com/tortoisecoin/tortoisecoin/issues

How to Upgrade
--------------

If you already downloaded 0.8.6rc1 you do not need to re-download. This release is exactly the same.

If you are running an older version, shut it down. Wait
until it has completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
/Applications/Tortoisecoin-Qt (on Mac) or tortoisecoind/tortoisecoin-qt (on Linux).

If you are upgrading from version 0.7.2 or earlier, the first time you
run 0.8.6 your blockchain files will be re-indexed, which will take
anywhere from 30 minutes to several hours, depending on the speed of
your machine.

0.8.6 Release notes
===================

- Default block size increase for miners.
  (see https://gist.github.com/gavinandresen/7670433#086-accept-into-block)

- Remove the all-outputs-must-be-greater-than-CENT-to-qualify-as-free rule for relaying
  (see https://gist.github.com/gavinandresen/7670433#086-relaying)

- Lower maximum size for free transaction creation
  (see https://gist.github.com/gavinandresen/7670433#086-wallet)

- OSX block chain database corruption fixes
  - Update leveldb to 1.13
  - Use fcntl with `F_FULLSYNC` instead of fsync on OSX
  - Use native Darwin memory barriers
  - Replace use of mmap in leveldb for improved reliability (only on OSX)

- Fix nodes forwarding transactions with empty vins and getting banned

- Network code performance and robustness improvements

- Additional debug.log logging for diagnosis of network problems, log timestamps by default

- Fix Tortoisecoin-Qt startup crash when clicking dock icon on OSX 

- Fix memory leaks in CKey::SetCompactSignature() and Key::SignCompact()

- Fix rare GUI crash on send

- Various small GUI, documentation and build fixes

Warning
-------

- There have been frequent reports of users running out of virtual memory on 32-bit systems
  during the initial sync.
  Hence it is recommended to use a 64-bit executable if possible.
  A 64-bit executable for Windows is planned for 0.9.

Note: Gavin Andresen's GPG signing key for SHA256SUMS.asc has been changed from  key id 1FC730C1 to sub key 7BF6E212 (see https://github.com/tortoisecoin/tortoisecoin.org/pull/279).
