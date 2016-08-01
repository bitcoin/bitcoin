[Website](www.bitcoinunlimited.info)  | [Download](www.bitcoinunlimited.info/download) | [Setup](doc/README.md)  |  [Xthin](bu-xthin.md)  |  [Xpedited](bu-xpedited-forwarding.md)  |   [Miner](miner.md)

Using Bitcoin Unlimited for Mining
==================================

Bitcoin Unlimited is based on the Satoshi codebase, so it is a drop in replacement for your mining pool software.  Simply configure your pool to point to the Bitcoin Unlimited daemon, in the exact same manner you would for the Bitcoin Core daemon.

But Bitcoin Unlimited has specific features to facilitate mining.

Setting your Coinbase string
----------------------------

To change the string that appears in the coinbase message of a mined block, run:

```sh
bitcoin-cli setminercomment "your mining comment"
```

To show the current string:

```sh
bitcoin-cli getminercomment
```

 - WARNING: some mining software and pools also add to the coinbase string and do not validate the total string length (it must be < 100 bytes).  This can cause the mining pool to generate invalid blocks.  Please ensure that your mining pool software validates total string length, or keep the string you add to Bitcoin Unlimited short.


Setting your maximum mined block
--------------------------------

By default, the maximum block that Bitcoin Unlimited will mine is 1MB (compatible with Bitcoin Core).
You may want to increase this block size if the bitcoin network as a whole is willing to mine larger blocks, or you may want to decrease this size if you feel that the demands on the network is exceeding capacity.

To change the largest block that Bitcoin Unlimited will generate, run:
```sh
bitcoin-cli setminingmaxblock blocksize
```
For example, to set 2MB blocks, use:
```sh
bitcoin-cli setminingmaxblock 2000000
```
To change this field in bitcoin.conf or on the command line, use:
 > blockmaxsize=<NNN>
 
for example, to set 3MB blocks use:
 > blockmaxsize=3000000

You can discover the maximum block size by running:
```sh
bitcoin-cli getminingmaxblock
```
 - WARNING: Setting this max block size parameter means that Bitcoin may mine blocks of that size on the NEXT block.  It is expected that any voting or "grace period" (like BIP109 specified) has already occurred.

Setting your block version
---------------------------
Miners can set the block version flag via CLI/RPC or config file:

From the CLI/RPC, 
```sh
bitcoin-cli setblockversion (version number or string)
```
For example:

The following all choose to vote for 2MB blocks:
```sh
bitcoin-cli setblockversion 0x30000000
bitcoin-cli setblockversion 805306368
bitcoin-cli setblockversion BIP109
```

The following does not vote for 2MB blocks:
```sh
bitcoin-cli setblockversion 0x20000000
bitcoin-cli setblockversion 536870912
bitcoin-cli setblockversion BASE
```

You can discover the current block version using:
```sh
bitcoin-cli getblockversion
```
From bitcoin.conf:
 > blockversion=805306368

Note you must specify the version in decimal format in the bitcoin.conf file.
Here is an easy conversion in Linux: python -c "print '%d' % 0x30000000"

 - WARNING: If you use nonsense numbers when calling setblockversion, you'll end up generating blocks with nonsense versions!

Setting your block retry intervals
----------------------------------

Bitcoin Unlimited tracks multiple sources for data an can rapidly request blocks or transactions from other sources if one source does not deliver the requested data.
To change the retry rate, set it in microseconds in your bitcoin.conf:

Transaction retry interval:
 > txretryinterval=2000000
 
 Block retry interval:
 > blkretryinterval=2000000

Setting your memory pool size
-----------------------------

A larger transaction memory pool allows your node to receive expedited blocks successfully (it increases the chance that you will have a transaction referenced in the expedited block) and to pick from a larger set of available transactions.  To change the memory pool size, configure it in bitcoin.conf:

 > maxmempool=<megabytes>

So a 4GB mempool would be configured like:
 > maxmempool=4096

