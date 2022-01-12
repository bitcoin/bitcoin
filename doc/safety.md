# SECURITY

This document describes the security recommendations and issues that can be helpful for users of Bitcoin Core.

Download Bitcoin Core only from the links listed below and verify the integrity of the download before using:

https://bitcoincore.org/en/download/

https://github.com/bitcoin/bitcoin/releases

For verification, please follow the instructions in the 'Verify your download' section of the download page.

Bitcoin Core downloads available on untrusted sources or solicitation for private testing is likely malware and should be avoided.

### P2P

DNS seeds are used for bootstrapping the P2P network. If one of the domains used for default DNS seeds is hacked, this can impact nodes. A node checks 3 seeds at a time, so it would require coordination for a malicious seed to affect the network. If you prefer not to trust the DNS seeds hardcoded in Bitcoin Core, you should manually make connections to nodes you do trust (using `-seednode` / `-addnode`). You can also disable the DNS seeds (`-dnsseed=0`) in that case.

Difference between DNS seed and Seed node:

_seednode_ is a peer node that is only asked to respond to GETADDR. This is the bootstrapping mechanism used for Tor nodes. DNS seeds are names that resolve to the IP addresses of full nodes.

Having multiple connections to the network is fundamental to safety against eclipse attacks. If you configure your node to only have outbound block-relay-only connections, they wouldn't be participating in addr relay, so wouldn't have a diverse addrman to identify candidates to connect to. It is suggested to have at least 10 outbound connections (8 full relay and 2 block only).

`assumevalid=<hex>` skips validation for scripts. All scripts in transactions included in blocks that are ancestors of the _assumevalid_ block are assumed to be valid. While this does introduce a little bit of trust, people can independently check that block is part of the main chain.

Do not use blocks and chainstate from unknown sources. Attackers could be providing you a false blockchain, i.e. one that is valid but not the blockchain that all other nodes are following.

### RPC

[Executable, network access, authentication and string handling](/doc/json-rpc-interface.md#security)

The JSON-RPC interface is intended for local access. By default it will only accept connections from localhost, but it can be configured to be accessed for a wider netmask; e.g. a trusted LAN network. You will need to explicitly allow an IP address to connect to it by using the `rpcallowip=<ip>` option in the _bitcoin.conf_ file. If you set it to 0.0.0.0, it will be open to all IP addresses, but this is not recommended as it is not secure.

If you have a need to access it over an untrusted network like the internet, tunnel it through technology specifically designed for that, such as stunnel (SSL) or a VPN.

CVE-2018–20587 is an Incorrect Access Control vulnerability that affects all currently released versions of Bitcoin Core, You may be affected by this issue if you have the RPC service enabled (default, with the headless bitcoind), and other users have access to run their own services on the same computer (either local or remote access). If you are the only user of the computer running your node, you are not affected. If you use the GUI but have not enabled the RPC service, you are not affected.

You can read more about the vulnerability here: https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2018-20587

[#12763](https://github.com/bitcoin/bitcoin/pull/12763) added the RPC whitelisting feature that helps enforce application policies for services being built on top of Bitcoin Core. It suggested to not connect your Bitcoin node to arbitrary services, reduce risk and prevent unintended access.

### Wallet

[Encrypting the Wallet](/doc/managing-wallets.md#encrypting-the-wallet)

[Backing Up the Wallet](/doc/managing-wallets.md#backing-up-the-wallet)

Do not buy _wallet.dat_ files from anywhere. There are several websites that sell such files to newbies. Scammers modify _wallet.dat_ file to include a bogus ckey record.

Understanding the difference between legacy and descriptor wallets:

Be careful with RPC commands like `dumpwallet`, `dumpprivkey`, and `listdescriptors` with the "private" argument set to true. These commands reveal private keys, which should never be shared with third parties.

Be careful with commands like `importdescriptor` if suggested by third parties, which can control future addresses created for incoming payments.


### Other

Be careful about accepting configuration changes and commands from third parties. Here is a non-exhaustive list of things that may go wrong:

_*notify_ config and command line options accept shell commands to be based on different events in Bitcoin Core. Ensure that the commands are safe either by manual inspection or restrictions/alerts in the system used.
