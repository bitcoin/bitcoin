The IoP Blockchain client version *6.0.1* is now available

HOTFIX
======
This minor release fixes a bug that might cause a client to get stuck on the shorter chain during a soft fork.


General Information
===================

This is a new major version release, including new features, various bugfixes
and performance improvements, as well as updated translations. See below for more information.
We have decided to drop the *IoP HD* name in favor of completely replacing the old client.
This software will from now on be referred to as **IoP Core**. 

While the consensus of the blockchain stays the same, the structure of the data directory **does not**. Therefore, **a full reindex of the blockchain is necessary** (see below for instructions). Additionally, the naming scheme of both executables and data files has been streamlined a bit, as detailed in the next section.

Removing all previous *IoP HD* or `iop-blockchain` packages is recommended to prevent possible confusion, but is not strictly necessary. Please be aware that you cannot use both v5.0.1 and v6.0.0 interchangeably with the same data directory.


Naming Scheme
=============

The binaries are `iopd`, `iop-cli`, `iop-tx` and `iop-qt`, while the Windows and macOS executables are `iop-qt.exe` and `IoP-Qt.app`, respectively. The configuration file is called `iop.conf`.

The default data directory for both the command line and the Qt Wallet is

- `~/.iop` on Linux,
- `%APPDATA%\IoP` on Windows, and
- `~/Library/Application Support/IoP` on macOS.

The Qt Wallet (**NOT** the command line utilities) will pick up your previously used data directory. 


How to update
=============

For most users, the relevant packages are the .dmg file (macOS), the .exe file for your CPU architecture (Windows 32-bit and 64-bit), and the .deb packages (Ubuntu 64-bit). Other Linux users should download i686-pc-linux (32-bit) or x86_64-linux (64-bit). The rest of the tar.gz files contain the command line utilities separately or are intended for uncommon architectures (these are untested, feedback is appreciated).


Upgrading from *v5.0.1* or below
================================

A full reindex of the blockchain is necessary if you upgrade from *v5.0.1* or below. The recommended procedure is as follows: 
- make a full backup of your data directory and then move it somewhere else. 
- create a new directory at the default location for your platform, containing only a copy of the files `wallet.dat` and (if applicable) `iop.conf`. 
- start the software.


How to use the new version for mining
=====================================

The miner is now multi-threaded and supports up to 128 threads. You should never use more threads than your CPU has logical cores. Some CPUs have more logical cores than physical ones, e.g. an Intel i5 dual-core processor has four logical cores. It is recommended to leave one thread free so your computer remains responsive to your input. The configuration file takes the following parameters related to mining :

```
# mine=1 tells IoP Core to use your CPU to try and find new blocks for the network
mine=0

# You need to have the private key for a whitelisted address inside your wallet
# to mine new blocks. If your wallet is encrypted, you need to unlock it for about
# ten seconds to start mining (see below).
minewhitelistaddr=YOUR_ADDRESS_HERE

# Optionally, you can also specify a target adress for the block reward associated 
# with finding a new block.
minetoaddr=TARGET_ADDRESS_HERE

# Specify the number of independent miners. They will all do unique work.
# When not specified, one thread is used.
minethreads=X
```

If you are using iopd for mining, you can unlock the wallet using
```
iop-cli -datadir=/specify/non/standard/data/directory walletpassphrase YOUR_PASSPHRASE n
```
where `n` indicates the number of seconds you want to unlock (15 is sufficient for the miners to load the private key in the memory).

