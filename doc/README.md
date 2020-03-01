# Crown Platform 0.14.0

[Crown software](https://crownplatform.com/wallet) is an implementation of a full Crown Platform node, which can be used both locally, as a wallet for CRW and for running Masternodes and Systemnodes. It is a full node:  it downloads and stores the entire history of Crown transactions (which is currently several GBs); depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more. If you would like the process to go faster you can _download the blockchain directly_ **(BOOSTRAP LINK NEEDED)**.

## Install and Run Crown Wallet

Here are some helpful notes on how to run Crown on your native platform.

### Windows

Unpack the files into a directory, and then run `crown-qt.exe`.

### OSX

Drag Crown-Qt to your applications folder, and then run `Crown-Qt`.

### Unix

Unpack the files into a directory and run: `bin/crown-qt` or `bin/crownd`. You need the Qt5 run-time libraries to run Crown-Qt. On Debian or Ubuntu:

	sudo apt-get install libqtgui5

### Need Help?

* Ask for help on Discord
* Or at Crown Forum: https://forum.crownplatform.com
* Or write a mail to [support@crownplatform.com](email:support@crownplatform.com)

## Run Masternode or Systemnode

Here are some guides that will help you to setup and run an incentivized node:
* [Masternode Setup Guide](https://forum.crownplatform.com/index.php?topic=1241.0)
* [Systemnode Setup Guide](https://forum.crownplatform.com/index.php?topic=1240.0)
* [Masternode Configuration](masternode-config.md)

## Participate in Crown Decentralized Governance

As a community member you can participate in the Governance by proposing projects that will advance Crown. You are also encouraged to discuss the poposals at [Crown Forum](https://forum.crownplatform.com/index.php?board=17.0).

If you own one or more masternodes you can (and you should!) help to choose the direction in which Crown goes by carefuly studying the proposals and voting for those you consider worthy.

* [Governance and Proposals](https://forum.crownplatform.com/index.php?topic=17.0) - some frequently asqued questions
* [Console Governance Interface](governance.md) - describes how to submit a proposal, how to vote for a proposal from the console and more
* [How To Submit a Proposal](https://forum.crownplatform.com/index.php?topic=11.0) - another proposal submission guide
* [Proposal Submission Template](https://forum.crownplatform.com/index.php?topic=9.0)
* [Active Proposals](https://crown.today/proposals)

## Build Crown From Source

The following are developer notes on how to build Crown on your native platform. Right now Crown Core Development Team officially supports building only on [Unix](build-unix.md). Binaries for Windows and MacOS are generated via cross-compilation. Building under those systems natively is possible in theory - it's just not documented. If you have successful experience building Crown under those platforms, please contribute the instructions. 

## Development Documentation

The Crown repo's [root README](../README.md) contains relevant information on the development process.

- [Coding Style Guide](coding-style.md)
- [Multiwallet Qt Development](multiwallet-qt.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Unit Tests](unit-tests.md)

### Resources
* Crown Forum: https://forum.crownplatform.com
* Discord: https://discord.gg/Tcrkazc
* Telegram: https://t.me/crownplatform
* Guides & How-to: https://forum.crownplatform.com/index.php?board=5.0

## License

Crown is distributed under the terms of the [MIT/X11 software license](http://www.opensource.org/licenses/mit-license.php). 

This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard. 

Also see [Assets Attribution](assets-attribution.md)

- - -

Copyright © 2009-2018, Bitcoin Core Developers

Copyright © 2014-2016, Dash Core Developers

Copyright © 2014-2020, Crown Developers