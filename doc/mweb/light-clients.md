# Light Clients

Since addresses are not stored on MWEB outputs, identifying coins belonging to a wallet is no longer as simple as sharing a [bloom filter](https://github.com/bitcoin/bips/blob/master/bip-0037.mediawiki) with a full node, and letting them check for outputs that _probabilistically_ belong to the wallet.

There are 2 ways that existing light clients identify coins belonging to a wallet:

## 1. Server-Side Filtering

[BIP37 - Bloom Filters](https://github.com/bitcoin/bips/blob/master/bip-0037.mediawiki): Light wallets build a bloom filter for a list of addresses belonging to them, and share it with a full node. That full node then does the work of checking for outputs that _probabilistically_ belong to the wallet.

#### MWEB Equaivalent

Since addresses are not stored on MWEB outputs, we cannot identify a wallet's transactions using server-side filtering through probabilistic data structures like bloom filters. The only server-side filtering currently supported for MWEB is filtering using knowledge of private view keys. Light wallets would share their private view keys with a full node, which would then churn through all outputs to find any belonging to the wallet. Unfortunately, this is not a probabilistic process, so full nodes will know with 100% certainty outputs belong to the wallet. To make matters worse, the node will also learn the _value_ of the wallet's outputs. This method is ideal for those who want to use MWEB from a mobile wallet connected to _their own_ full node on a desktop or server, but due to the weakened privacy, you should avoid using this method when connected to untrusted nodes.

## 2. Client-Side Filtering

[BIP157 - Client Side Block Filtering](https://github.com/bitcoin/bips/blob/master/bip-0157.mediawiki): The reverse, where full nodes build a compact filter for each block, and light wallets use those filters to identify blocks that may contain the wallet's transactions.

#### MWEB Equivalent

For those who don't have a trusted full node to connect to, we could use something similar to client side block filtering. More precisely, we could perform client side _output_ filtering. Though no method has yet been designed for building filters as compact as those defined in [BIP158 - Compact Block Filters for Light Clients](https://github.com/bitcoin/bips/blob/master/bip-0158.mediawiki), nodes can provide light clients with _compact_ MWEB outputs, allowing them to avoid downloading the large rangeproofs associated to each output. In fact, we can be even more data efficient, and download only outputs that are verifiably unspent. Following the process defined in [LIP-0006](https://github.com/DavidBurkett/lips/blob/LIP0006/LIP-0006.mediawiki), light clients can download and check all UTXOs from multiple nodes _in parallel_, avoiding the privacy and verifiability shortcomings of the server-side filtering approaches.