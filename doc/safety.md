# SECURITY

This document describes the security recommendations and issues that can be helpful for users of Bitcoin Core.


Download Bitcoin Core only from below links, verify the integrity of the download before using:

https://bitcoincore.org/en/download/

https://github.com/bitcoin/bitcoin/releases

Downloading Bitcoin Core from untrusted sources can result in using a [malware](https://bitcoin.stackexchange.com/a/107738/) that can steal your bitcoin and compromise your computer.

### P2P

Using non-default ports and non-listening nodes can improve security.

DNS seeds are used for bootstrapping the P2P network. If one of the domains used for default DNS seeds is hacked, this can impact nodes. A node checks 3 seeds at a time, so it would require coordination for a malicious seed to affect the network. You can use own DNS seed for bootstrapping.

Difference in DNS seed and Seed node: https://bitcoin.stackexchange.com/a/17410/

Having multiple connections to the network is fundamental to safety against eclipse attacks. If a user configured the node to only have outbound block-relay-only connections, they wouldn't be participating in addr relay, so wouldn't have a diverse addrman to identify candidates to connect to. It is suggested to have at least 10 outbound connections.

`assumevalid=1` skips validation for scripts. All scripts in transactions included in blocks that are ancestors of the _assumevalid_ block are assumed to be valid. While this does introduce a little bit of trust, people can independently check that block is part of the main chain.

Known incidents when BGP hijacking was used to exploit Bitcoin users: https://www.wired.com/2014/08/isp-bitcoin-theft/

Do not use blocks and chainstate from unknown sources. Attackers could be providing you a false blockchain, i.e. one that is valid but not the blockchain that all other nodes are following.

### RPC

The JSON-RPC interface is intended for local access. By default it will only accept connections from localhost, but it can be configured to be accessed for a wider netmask; e.g. a trusted LAN network. Bitcoin Core does not allow anyone to connect to the rpcport. You will need to explicitly allow an IP address to connect to it by using the `rpcallowip=<ip>` option in the _bitcoin.conf_ file. If you set it to 0.0.0.0, it will be open to all IP addresses, but this is not recommended as it is not secure.

If you have a need to access it over an untrusted network like the internet, tunnel it through technology specifically designed for that, such as stunnel (SSL) or a VPN.

[Executable, network access, authentication and string handling](/doc/json-rpc-interface.md#security)

[Multi user systems](https://medium.com/@lukedashjr/cve-2018-20587-advisory-and-full-disclosure-a3105551e78b)

[#12763](https://github.com/bitcoin/bitcoin/pull/12763) added the RPC whitelisting feature that helps enforce application policies for services being built on top of Bitcoin Core. It suggested to not connect your Bitcoin node to arbitrary services, reduce risk and prevent unintended access.

### Wallet

[Encrypting the Wallet](/doc/managing-wallets.md#encrypting-the-wallet)

[Backing Up the Wallet](/doc/managing-wallets.md#backing-up-the-wallet)

Do not buy _wallet.dat_ files from anywhere. There are several websites that sell such files to newbies. You can read more about the issue here: https://github.com/bitcoin-core/gui/issues/77#issuecomment-721927922

Understanding the difference between legacy and descriptor wallets:

There is no XPUB in legacy wallets, `dumpwallet` will return private keys. `listdescriptors` can be used to get XPUB and XPRV for descriptor wallets.


### Other

_*notify_ config and commandline options accept shell commands to be based on different events in Bitcoin Core. Be careful while using these options and ensure that the commands are safe either by manual inspection or restrictions/alerts in the system used.
