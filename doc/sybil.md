## 1. Security considerations

Using CJDNS, I2P, or Tor alone without other networks may make a node more susceptible to [Sybil
attacks](https://en.bitcoin.it/wiki/Weaknesses#Sybil_attack). You can use
`bitcoin-cli -addrinfo` to see the number and type of addresses known to your node.

If all of your connections are controlled by a Sybil attacker, they can easily prevent you from seeing confirmed transactions and,
with more difficulty, even trick your node into falsely reporting a transaction as confirmed on the blockchain.

This is significantly less of a concern if you make `-addnode` connections to trusted peers (even if they're CJDNS, I2P, or Onion addresses).
It's also alleviated with IPv4/IPv6 connections (especially when using the `-asmap` configuration option) due to the cost of obtaining IPs in many networks. 
A connection to a single honest peer is enough to thwart an attempted eclipse attack.

**Network Partitioning**

If too many of the single network nodes use onlynet=[network], it could become difficult for the nodes to communicate with the rest of the entire
Bitcoin network, increasing the chance of network partitioning. It is essential that some nodes access a combination of networks.
