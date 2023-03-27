P2P and network changes
-----------------------

Updated RPCs
------------

Changes to Wallet or GUI related RPCs can be found in the GUI or Wallet section below.

New RPCs
--------

Build System
------------

Updated settings
----------------

Changes to Wallet or GUI related settings can be found in the GUI or Wallet  section below.

New settings
------------

Wallet
------

#### Wallet RPC changes

- The `upgradewallet` RPC replaces the `-upgradewallet` command line option.
  (#15761)
- The `settxfee` RPC will fail if the fee was set higher than the `-maxtxfee`
  command line setting. The wallet will already fail to create transactions
  with fees higher than `-maxtxfee`. (#18467)

GUI changes
-----------

Low-level changes
=================

Tests
-----
