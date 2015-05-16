Credits Core version 0.9.1.50 is now available from:

  http://credits-currency.org

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Credits-Qt (on Mac) or
creditsd/credits-qt (on Linux).

0.9.1.50 Release notes
=======================

0.9.1.50 is a major functionality extension release:
- Version number bumped to *.50 to indicate major new functionality included.
- The client now has support for using sgminer (AMD graphic cards) for GPU mining.
- Several problematic areas with the getwork command has been fixed.
- The rpc command getwork has been fixed in preparation for adaption to pooled mining.
- A problematic bug that will force users to add extra deposits after block 32200 has been fixed.
- The deposit protocol has been adjusted to shift to exponential enforcement of deposits instead of linear at monetary base 8.160 MCRE (block 140000).
- Bug fixes.
