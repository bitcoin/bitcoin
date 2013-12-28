(note: this is a temporary file, to be added-to by anybody, and deleted at
release time)

0.8.6 changes
=============

- Default block size increase for miners
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

- Fix Bitcoin-Qt startup crash when clicking dock icon on OSX 

- Fix memory leaks in CKey::SetCompactSignature() and Key::SignCompact()

- Fix rare GUI crash on send

- Various small GUI, documentation and build fixes

Warning
--------

- There have been frequent reports of users running out of virtual memory on 32-bit systems
  during the initial sync.
  Hence it is recommended to use a 64-bit executable if possible.
  A 64-bit executable for Windows is planned for 0.9.

