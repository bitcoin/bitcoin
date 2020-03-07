# Crown v0.14.0 "Emerald" Release Notes

Crown Core v0.14.0 is a major release and a mandatory upgrade. 
It is available for download from:

https://www.crownplatform.com/wallet/


**ALL** nodes and wallets **MUST** upgrade before 06:00 UTC on 24 March 2020
or they will be cut off from the network. Any node rewards earned, or transactions 
initiated by previous version clients after that time will **NOT** be valid on 
the v0.14 network.

Users will have approximately two full weeks, including two weekends to upgrade
their nodes and wallets.

The cutoff will be enforced by a protocol version upgrade which means 
masternodes and systemnodes must be restarted after upgrading them and 
the controlling wallet.


Please report bugs through the Help, Guides and Support board at the
[Crown forum](https://forum.crownplatform.com/index.php?board=5.0)


## How to upgrade

The basic procedure is to shutdown your existing version, replace the 
executable with the new version, and restart. This is true for wallets, 
masternodes and systemnodes. The exact procedure depends on how you setup
the wallet and/or node in the first place. If you use a managed hosting 
service for your node(s) it is possible they will upgrade them for you, 
but you will need to upgrade your wallet. 

You are recommended to upgrade the controlling wallet before upgrading any 
nodes in order to minimise the node downtime. The protocol version is changed 
in this release so you must issue a start command from the upgraded controlling
wallet for any masternode or systemnode after upgrading the node.

When a large number of nodes concurrently go offline and come online again
it can cause network instability. If you are a node operator or individual 
with a large number of nodes we ask you to upgrade them sequentially if 
possible, or in small batches of no more than five at a time, to reduce the 
risk of destabilising the chain.


## What's new
### NFT framework
Crown Core v0.14 "Emerald" is the mainnet release of the non-fungible token
(NFT) framework. 
There is no change to the consensus rules, superblock interval or node rewards. 
There are two new RPCs to register and query NFT protocols and tokens, turning 
the Crown blockchain into an assetchain.
NFT functionality will be activated at 10:00 UTC on 24 March 2020, shortly after 
all old version nodes have been disconnected from the network.
* **nftproto** registers and queries NFT protocols.
* **nftoken** registers and queries individual non-fungible tokens.

More documentation is available at 
[NFT framework part 2](https://medium.com/crownplatform/crown-platform-nft-framework-part-2-f713fe6cd81c) and 
[NFT Framework part 1](https://medium.com/crownplatform/crown-platform-nft-framework-18d88f9db76)

 
### Other notable changes
* A testnet version of the installation/update script, crown-server-install.sh which simplifies setting up a bare testnet wallet or node.
* Some enhancements to the spendfrom.py script which simplify consolidating MN/SN rewards or other cases of multiple transactions sent to a single address.
* sandbox.py environment creation and management script for testing independent of the public testnet.
* Updated references to Crown Platform home URL wherever it is used.
* Updated mainnet seednodes.
  