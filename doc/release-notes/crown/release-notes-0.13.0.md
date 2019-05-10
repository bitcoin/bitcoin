# Crown v0.13.0 (Codename "Jade") Release Notes

Crown Core v0.13.0 is a major release and a mandatory upgrade.
It is available for download from:

https://crown.tech/downloads

Please report bugs using the issue tracker at Gitlab:

https://gitlab.crown.tech/crown/crown-core/issues/new?issue%5Bassignee_id%5D=&issue%5Bmilestone_id%5D=

## How to upgrade

The basic procedure is to shutdown your existing version, replace the 
executable with the new version, and restart. This is true for both wallets
and masternodes/systemnodes. The exact procedure depends on how you setup
the wallet and/or node in the first place. If you use a managed hosting 
service for your node(s) it is probable they will upgrade them for you, 
but you will need to upgrade your wallet and issue a start command for 
each node.

## What's new

v0.13.0 features 2 main changes over the previous v0.12.5.x version:
* new address prefixes
* switch from Proof of Work to Proof of Stake
 

## New address prefixes
When Crown originally forked from Bitcoin it retained the Bitcoin address format. 
Nowadays that is not a desirable legacy because it prevents use of Crown with 
hardware wallets such as Ledger and Trezor. Those devices cannot distinguish 
between different coins with the same address prefixes.

v0.13.0 introduces new prefixes for both mainnet and testnet.

Mainnet addresses now begin with the prefix "CRW", eg:```CRWP934B9FbM5RaTUfT7zCH12a259j4PYbSE```

and testnet addresses now begin with the prefix "tCRW", eg:```tCRWZpkQ8aJ4wyWx5MiZup23WG8aw5JmKRAaQ```

These unique and distinctive prefixes will enable the use of Crown with hardware 
wallets. We hope to see the first integration soon. 

The change is largely friction-free from the user's perspective. 
Once users update to v 0.13.0, they will be able to receive CRW sent to old 
addresses from old wallets.

New wallets cannot send CRW to an old address.

Old wallets cannot send CRW to a new address.

If an updated wallet wants to send any amount of CRW to the old address it needs 
to be converted via Crown command line. For example:
```
crown-cli convertaddress 15bj2HB2UbmjEZgXyEW4M8MhUL5TXGCN8L
```
Result ```CRWGZiLqSUbCTWkm3ABp5qpXdun2h8DJCKYF```

If the equivalent new address of ```15bj2HB2UbmjEZgXyEW4M8MhUL5TXGCN8L``` is
```CRWGZiLqSUbCTWkm3ABp5qpXdun2h8DJCKYF``` you can send CRW from the old wallet 
(v 0.12.5.3) using ```15bj2HB2UbmjEZgXyEW4M8MhUL5TXGCN8L``` 
and from new one (v 0.13.0) using ```CRWGZiLqSUbCTWkm3ABp5qpXdun2h8DJCKYF```.

Both old and new wallets will be able to see the CRW! This does not mean there
are 2 copies of the CRW, only that the old and new wallets have different ways
of referring to the same address.

## Block generation change from Proof of Work to Proof of Stake
New address prefixes are a substantial change but are dwarfed in scope by the
change from Proof of Work (PoW) for block generation to Proof of Stake (PoS).

Proof of Work is a computationally intensive process which consumes a lot of power.
Proof of Stake is more environmentally friendly and removes the block generation
process from miners and gives the task to stakers instead.

Several dozen coins have already made this change but Crown's implementation is 
unique and has several advantages. The first coin with PoS was Peercoin which
used a combination PoW/PoS implementation. Blackcoin was the first PoS-only coin.
The majority of PoS coins rely on hot-wallet staking and have some dependence on
coin-age or wallet balance to decide which node wins the right to generate the 
next block and receive the block reward. Hot-wallet staking means the user's 
wallet must be online and open to stake.

The Crown implementation was designed and built by Tom Bradshaw (@presstab) at
Paddington Solutions over a period of several months. It features cold-staking 
on the masternode and systemnode network. Users do not need to keep their wallets 
open and online in order to stake. Instead, their collateral balance(s) is/are 
delegated to their masternode(s) and/or systemnode(s) to be staked there.

Contemporary staking systems generally work on the entire balance of a user's
wallet. Thus in a contemporary system a user with 100,000 coins in their
wallet stakes with all of them. 
The Crown system works only on coins locked as collateral in masternodes
or systemnodes. This encourages users to support the network by running one or
more nodes in order to ensure they get a share of the staking rewards. 
In the Crown system a user with 100,000 coins in their wallet only stakes with 
those locked in a masternode or systemnode. If they choose not to support the
network by not running any nodes then they have no chance to earn staking rewards.
If they lock all of their coins in masternodes or systemnodes then all of their
coins are eligible to earn rewards. Therefore the minimum wallet balance which
can stake is 500 coins and those coins must be locked as the collateral for a 
systemnode. 

The system is described in more detail in [Crown-MNPoS.md](https://gitlab.crown.tech/crown/crown-core/blob/master/doc/Crown-MNPoS.md)

### The switchover
The switchover from PoW to PoS will take place at block height 2330000, approximately
2 weeks after release of v0.13.0.
This will allow time for users and exchanges to upgrade and get accustomed to the
new address prefixes and for the network to stabilise. At block height 2327000 
(about 2 days before the consensus change kicks in) nodes which have
not upgraded will be excluded from the network. Nodes
which have upgraded will continue on the network uninterrupted. The network will
have time to stabilise again until at block height
2330000 it will say goodbye to the miners and the first PoS block will 
be generated by one of the masternodes or systemnodes.
The block generation time will remain 1 minute, as it is now. Every minute a
masternode or systemnode will generate ("mint") a block and share it with the
network. The node will be rewarded for it's efforts with a minting reward. 
This replaces the reward previously given to the miner which generated a block. 
The node will also pay the usual per block masternode and systemnode rewards.

### PoS FAQ
#### 1. How much can I earn through staking?
Every block minted earns a staking reward of 1.5CRW for the (owner of the) node
which created it. The more coins you have locked as collateral for masternodes
and/or systemnodes the greater chance you have of winning the staking reward on
any block.
#### 2. What's the minimum amount of Crown required for staking?
One systemnode of locked coins, in other words, 500CRW.
#### 3. What do I have to do to enable staking?
Apart from locking some coins as systemnode or masternode collateral, nothing.
If you really wanted to you could choose not to stake by setting ```staking=0```
in the crown.conf of your masternode/systemnode.
#### 4. Do I need to keep my wallet open to stake?
No. Once you have activated your masternode/systemnode the coins locked for
that node will keep staking for as long as your node is running.
#### 5. What about the much publicised fake stake attack vector?
There were 2 aspects to the fake stake attack. The first involved headers-first
syncing, but the Crown PoS implementation doesn't use headers-first so this 
is a complete non-issue. The second possible attack vector involved resource
exhaustion where a valid stake is required to begin the attack. This has been
avoided by leveraging the masternode network as trusted witnesses to the 
validity of a block.
#### 6. What if I have a gazillion coins but don't want to lock them all up in masternodes or systemnodes?
In that case you won't be able to earn staking rewards for the non-collateral
coins. The chosen system is uniquely egalitarian and rewards people for 
supporting the network. 


## Changelog
```
Ashot Khachatryan (62):
      Changed block rewards. Adapted compatibility with mainnet.
      Keep auxpow from headers and use for blocks later
      Use auxpow received with headers
      Added 'CRW' and 'tCRW' prefixes
      Change old address to new one in GUI
      Convert old address to new one in walletdb
      Added address types identity, app service, title
      Added unit tests for the new pubkey address prefix
      Resolve discussions #143. Added more test cases
      Changed isOld to enum type
      Added GetVersionBytes() function
      Resolve discussion !143
      Fixed sync issue from old wallets
      Resolved discussions !137
      Fixed private key version byte size
      Used m_versionBytesPrivateKey
      Resolved discussions
      Added CLI converter for old Crown addresses
      Fixed test case
      Check if alias contains white-space characters
      Fixed out-of-memory issue caused many txlock/ix requests
      Changed 'oldtonewaddress' to 'convertaddress'
      Resolved discussions
      Deploy .dmg for MacOS
      Changed package name
      Added release version
      Changed release order
      Changed icon
      Autoconf require C++11
      Added c++11 support for boost
      Updated boost version
      Changed darwin version
      Changed darwin version in .gitlab-ci.yml
      Fixed unit test error
      Compile qt with c++11 option
      Experiment with c++11
      Added GOAL to configs
      Updated script to use v0.12.5.2 version
      Changed update.json to use v0.12.5.2 version
      Added path for .dmg
      Added -dmg postfix
      Fixed postfix
      Revert back sync optimization
      Used even build version to be release
      Changed CLIENT_VERSION_BUILD to CLIENT_VERSION_REVISION
      Changed release version
      Changed release version in configure.ac
      Fixed dmg build
      Changed build version
      Changed protocol and build versions
      Removed comments for seed nodes
      Added new spork to disable/enable masternode payments
      Fixed budget payment bug. Increased release and protocol versions
      Don't take immature coinstake for a new transaction. #281
      Removed TransactionRecord::MasterNodeReward, TransactionRecord::SystemNodeReward. #307
      Added tags to .gitlab-ci.yml
      Changed 'staking' to be true by default #308
      Changed DoS score to 50. #309
      Revert back main testnet parameters
      Changed version
      Used GetBlockValue if masternode payment is disabled
      Fixed pow check

Benjamin Allred (5):
      BlockUndo to treat coinstake as if it were coinbase
      MN/SN adds stakepointer and signs new block
      Added stake modifier generation
      Systemnode staking
      A stakepointer can only be used once in a chain

IgorDurovic (1):
      proof hash checking added to connect block

Mark Brooker (3):
      Account for 32-/64-bit environment when downloading package #224
      Clear up potential installer problems #228
      Documentation updates

Volodymyr Shamray (17):
      Change CLIENT_VERSION_IS_RELEASE to false
      Add synchronization to BudgetManager
      Fix test
      Add more synchronization
      Fix m_votes and m_obsoleteVotes maps usage
      Pulled in Dash devnets
      Make devneds work with Crown
      Make devnet MNs possible to run on localhost
      Change devnet prefixes
      Fix Windows cross-compilaion
      Fix Windows cross-compilation
      Implement basic sandbox control script
      Fix some bugs
      Merge missing checks, add devnet premine
      Fix incorrect devnet merge
      Fix devnet bugs
      Fix various bugs

presstab (70):
      basic outline of certain staking functions.
      Require c++11
      Unit tests for proof hash and stake miner
      Add Proof Pointer
      Fix Styling
      Fix error compiling with undefined class
      Add block type check and tests. Add block signature.
      Add block signature tests
      Use OP_PROOFOFSTAKE for CoinStake CTxIn.
      Add PoS to CBlock and Header
      Add coinstake checks
      in progress validation
      Stake mining thread
      New testnet genesis
      testnet premine
      Add stakepointer.h to Makefile.am
      Use signover pubkey to stake with. Fix serialization errors.
      Reduce stake weight for smoother difficulty switchover
      Fix masternode signover signature serialization
      Miner checks should only happen for PoS
      Add some spacing if blocks are coming in too fast
      Add testnet checkpoint
      Add -jumpstart flag for when the chain is stalled
      Do not pay both coinstake and coinbase
      Refactor stake pointer double spend tracking
      Only connect to the same protocol version
      Reduce rigidity of stakepointer double stake check to allow reorg
      Add null check
      Enforce that stakepointers can only be 1440 blocks deep
      Add maxreorg depth and reject stakepointers that are more recent than maxreorg
      Clarify comments
      Place connectblock() stake checks into their own method
      Remove unused proof of stake code
      Harden proofpointer checks. Rename items for legibility.
      Remove dead code. Refactor some of the mining code.
      Enforce MN/SN payment positions in coinbase tx
      Do not allow stakepointers from budget payment blocks
      Enforce 0 value coinbase
      Only reject when sporks are synced.
      Add exception for jumpstart
      Double check pointer's depth before trying to stake it
      Subtract masternode/systemnode payments from coinstake's value
      Add another jumpstart exclusion
      Blockchain sync state
      Fix miner reward
      Add final check for used stakepointer in miner.cpp
      Ensure that the full block is being processed, not just header.
      Set Proof of Stake in the block header version
      Use template for stakepointer selection
      Do not delete record of used stake pointer on disconnect if just verifying db.
      Bump protocol and client version. Add checkpoints for easier sync.
      Fix datacorruption on restart when looking at blockundo.
      Don't reject mn payment if the chain has stalled
      Adjust validation code to have knowledge of full value created in block.
      Make stakepointer check more legible.
      Do not consider coinstake txin's as conflicting spends.
      Add staking_active to getinfo RPC
      Fix partially filled out CBlockIndex. Fix first load txundo error on PoW blocks.
      Add hack to prevent segfault on ResetSync() with -jumpstart enabled
      Add tx fees to stake reward.
      Do not load coinstake transactions to mempool.
      Sort transactions by date.
      Don't try to stake too recent of stake pointers.
      Add compatibility with newer openssl versions.
      Fix timestamp for Transaction Records. Add Masternode and Systemnode txrecord types.
      Lock cs_main early if a block is found.
      Masternode based block spam filtering.
      Relay block proofs if peer requests.
      Update serialization for masternode ping.
      Make MasternodePing serialization compatible with old and new protocols.
```
