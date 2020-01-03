Bitcoin Core version 0.18.x is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.18.x/>

This is a new minor version release, including new features, various bug
fixes and performance improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has
completely shut down (which might take a few minutes for older
versions), then run the installer (on Windows) or just copy over
`/Applications/Bitcoin-Qt` (on Mac) or `bitcoind`/`bitcoin-qt` (on
Linux).

The first time you run version 0.15.0 or newer, your chainstate database
will be converted to a new format, which will take anywhere from a few
minutes to half an hour, depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and
there is no automatic upgrade code from before version 0.8 to version
0.15.0 or later. Upgrading directly from 0.7.x and earlier without
redownloading the blockchain is not supported.  However, as usual, old
wallet versions are still supported.

Compatibility
==============

Bitcoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not
recommended to use Bitcoin Core on unsupported systems.

Bitcoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From 0.17.0 onwards, macOS <10.10 is no longer supported. 0.17.0 is
built using Qt 5.9.x, which doesn't support versions of macOS older than

Mining
------

- Calls to `getblocktemplate` will fail if the segwit rule is not specified.
  Calling `getblocktemplate` without segwit specified is almost certainly
  a misconfiguration since doing so results in lower rewards for the miner.
  Failed calls will produce an error message describing how to enable the
  segwit rule.

Configuration option changes
----------------------------

- A warning is printed if an unrecognized section name is used in the
  configuration file.  Recognized sections are `[test]`, `[main]`, and
  `[regtest]`.

- Four new options are available for configuring the maximum number of
  messages that ZMQ will queue in memory (the "high water mark") before
  dropping additional messages.  The default value is 1,000, the same as
  was used for previous releases.  See the [ZMQ
  documentation](https://github.com/bitcoin/bitcoin/blob/master/doc/zmq.md#usage)
  for details.

- The `enablebip61` option (introduced in NdovuCoin Core 0.17.0) is
  used to toggle sending of BIP 61 reject messages. Reject messages have no use
  case on the P2P network and are only logged for debugging by most network
  nodes. The option will now by default be off for improved privacy and security
  as well as reduced upload usage. The option can explicitly be turned on for
  local-network debugging purposes.

- The `rpcallowip` option can no longer be used to automatically listen
  on all network interfaces.  Instead, the `rpcbind` parameter must also
  be used to specify the IP addresses to listen on.  Listening for RPC
  commands over a public network connection is insecure and should be
  disabled, so a warning is now printed if a user selects such a
  configuration.  If you need to expose RPC in order to use a tool
  like Docker, ensure you only bind RPC to your localhost, e.g. `docker
  run [...] -p 127.0.0.1:8332:8332` (this is an extra `:8332` over the
  normal Docker port specification).

- The `rpcpassword` option now causes a startup error if the password
  set in the configuration file contains a hash character (#), as it's
  ambiguous whether the hash character is meant for the password or as a
  comment.

Documentation
-------------

- A new short
  [document](https://github.com/bitcoin/bitcoin/blob/master/doc/JSON-RPC-interface.md)
  about the JSON-RPC interface describes cases where the results of an
  RPC might contain inconsistencies between data sourced from different
  subsystems, such as wallet state and mempool state.  A note is added
  to the [REST interface documentation](https://github.com/bitcoin/bitcoin/blob/master/doc/REST-interface.md)
  indicating that the same rules apply.

- A new [document](https://github.com/bitcoin/bitcoin/blob/master/doc/bitcoin-conf.md)
  about the `bitcoin.conf` file describes how to use it to configure
  NdovuCoin Core.

- A new document introduces NdovuCoin Core's BIP174
  [Partially-Signed NdovuCoin Transactions (PSBT)](https://github.com/bitcoin/bitcoin/blob/master/doc/psbt.md)
  interface, which is used to allow multiple programs to collaboratively
  work to create, sign, and broadcast new transactions.  This is useful
  for offline (cold storage) wallets, multisig wallets, coinjoin
  implementations, and many other cases where two or more programs need
  to interact to generate a complete transaction.

- The [output script descriptor](https://github.com/bitcoin/bitcoin/blob/master/doc/descriptors.md)
  documentation has been updated with information about new features in
  this still-developing language for describing the output scripts that
  a wallet or other program wants to receive notifications for, such as
  which addresses it wants to know received payments.  The language is
  currently used in the `scantxoutset` RPC and is expected to be adapted
  to other RPCs and to the underlying wallet structure.

Build system changes
--------------------

- A new `--disable-bip70` option may be passed to `./configure` to
  prevent NdovuCoin-Qt from being built with support for the BIP70 payment
  protocol or from linking libssl.  As the payment protocol has exposed
  NdovuCoin Core to libssl vulnerabilities in the past, builders who don't
  need BIP70 support are encouraged to use this option to reduce their
  exposure to future vulnerabilities.

Deprecated or removed RPCs
--------------------------

- The `signrawtransaction` RPC is removed after being deprecated and
  hidden behind a special configuration option in version 0.17.0.

- The 'account' API is removed after being deprecated in v0.17.  The
  'label' API was introduced in v0.17 as a replacement for accounts.
  See the [release notes from v0.17](https://github.com/bitcoin/bitcoin/blob/master/doc/release-notes/release-notes-0.17.0.md#label-and-account-apis-for-wallet)
  for a full description of the changes from the 'account' API to the
  'label' API.
=======
10.10. Additionally, Bitcoin Core does not yet change appearance when
macOS "dark mode" is activated.

Known issues
============

Wallet GUI
----------

For advanced users who have both (1) enabled coin control features, and
(2) are using multiple wallets loaded at the same time: The coin control
input selection dialog can erroneously retain wrong-wallet state when
switching wallets using the dropdown menu. For now, it is recommended
not to use coin control features with multiple wallets loaded.

Notable changes
===============


0.18.x change log
=================


Credits
=======

Thanks to everyone who directly contributed to this release:


As well as everyone that helped translating on [Transifex](https://www.transifex.com/projects/p/bitcoin/).
