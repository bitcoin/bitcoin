Omni Core (beta) integration/staging tree
=========================================

[![Build Status](https://travis-ci.org/OmniLayer/omnicore.svg?branch=omnicore-0.0.10)](https://travis-ci.org/OmniLayer/omnicore)

What is the Omni Layer
----------------------
The Omni Layer is a communications protocol that uses the Bitcoin block chain to enable features such as smart contracts, user currencies and decentralized peer-to-peer exchanges. A common analogy that is used to describe the relation of the Omni Layer to Bitcoin is that of HTTP to TCP/IP: HTTP, like the Omni Layer, is the application layer to the more fundamental transport and internet layer of TCP/IP, like Bitcoin.

http://www.omnilayer.org

What is Omni Core
-----------------

Omni Core is a fast, portable Omni Layer implementation that is based off the Bitcoin Core codebase (currently 0.10.4). This implementation requires no external dependencies extraneous to Bitcoin Core, and is native to the Bitcoin network just like other Bitcoin nodes. It currently supports a wallet mode and is seamlessly available on three platforms: Windows, Linux and Mac OS. Omni Layer extensions are exposed via the UI and the JSON-RPC interface. Development has been consolidated on the Omni Core product, and it is the reference client for the Omni Layer.

Disclaimer, warning
-------------------
This software is EXPERIMENTAL software. USE ON MAINNET AT YOUR OWN RISK.

By default this software will use your existing Bitcoin wallet, including spending bitcoins contained therein (for example for transaction fees or trading).
The protocol and transaction processing rules for the Omni Layer are still under active development and are subject to change in future.
Omni Core should be considered an alpha-level product, and you use it at your own risk. Neither the Omni Foundation nor the Omni Core developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this installation of Omni Core should be viewed as EXPERIMENTAL. Your wallet data, bitcoins and Omni Layer tokens may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

This software is provided open-source at no cost. You are responsible for knowing the law in your country and determining if your use of this software contravenes any local laws.

PLEASE DO NOT use wallet(s) with significant amounts of bitcoins or Omni Layer tokens while testing!

Related projects
----------------

* https://github.com/OmniLayer/OmniJ

* https://github.com/OmniLayer/omniwallet

* https://github.com/OmniLayer/spec

Support
-------

* Please open a [GitHub issue](https://github.com/OmniLayer/omnicore/issues) to file a bug submission.
