# `bitcoincore` Python bindings

This module contains a number of useful Python bindings for interaction with bitcoind.
Multiple means of communication are facilitated, including P2P messaging
(`p2p.P2PInterface`) as well as process-level bitcoind invocation
(`test_node.TestNode`).

This module can help with a variety of tasks:

  - building a local relay which adapts the user's choice of
    protocol to bitcoin P2P messages,
  - monitoring via the P2P network,
  - programmatic management of bitcoind instances,
  - general utilities related to bitcoin,
  - and more.

## Installation

This module is pip installable.

```sh
$ git clone https://github.com/bitcoin/bitcoin.git
$ cd bitcoin/test_functional/test_framework
$ pip install .

# The bitcoincore module is then importable.
$ python
>>> import bitcoincore
>>> bitcoincore.p2p.MAGIC_BYTES
{'mainnet': b'\xf9\xbe\xb4\xd9',
 'testnet3': b'\x0b\x11\t\x07',
 'regtest': b'\xfa\xbf\xb5\xda',
 'signet': b'\n\x03\xcf@'}
```

## Examples

Examples are contained in [exmaples/](examples/).
