Detailed Protocol Design for Xtreme Thin blocks (Xthinblocks)
=============================================================

The following is the updated and current design spec for Xthinblocks and may be slightly different than the original BUIP010
proposal which was voted on.


1. Summary
---------------------------------
---------------------------------

In order to scale the Bitcoin network, a faster less bandwidth intensive method is needed in order to send larger blocks. The 
thinblock strategy is designed to speed up the relay of blocks by using the transactions that already exist in the requester's 
memory pool as a way to rebuild the block, rather than download it in its entirety. 

Differing from other thinblock strategies, "Xtreme Thinblocks" uses simple bloom filtering and a new class of transactions to
ensure that almost all block requests only require a single round trip, which is essential for good performance on a system 
where transaction rates are steadily climbing and only a synchronous network layer exists. In addition, Xtreme Thinblocks 
uses a 64 bit transaction hash which further reduces thinblock size; in doing so a typical 1MB block can be reduced to a 10KB 
to 25KB thinblock.


2. High Level Design
--------------------
--------------------

For various reasons memory pools are not in perfect sync which makes it difficult to reconstruct blocks in a fast and reliable 
manner and with a single round trip. Creating a bloom filter at the time of sending the getdata request is an easy and fast way 
around that problem which allows the remote node to know with high certainty what transactions are missing in the requester's
memory pool and which need to be sent back in order to re-assemble the block.

The reference implementation works in the following way:

Node A is behind by one block and makes a thinblock request to Node B in the following way:

1. Node A creates a bloom filter seeded with the contents of its memory pool.
2. Node A sends the bloom filter along with a getdata request to Node B.
3. Node B sends back a "thinblock" transaction which contains the block header information, all the transaction hashes that were 
contained in the block, and any transactions that do not match the bloom filter which Node A had sent.
4. Node A receives the "thinblock" and reconstructs the block using transactions that exist from its own memory pool as well as 
the transactions that were supplied in the thinblock.?

In the unlikely event that there are transactions still missing, they will be re-requested as follows.

5. If there are still any transactions missing then a GET_XBLOCKTX message is sent from Node A.
6. Node B upon receiving the GET_XBLOCKTX request, will get the missing transactions 
from the block on disk rather than memory (in this way we can be sure the transactions can be accessed as they may already have been 
purged from memory or they may have been unannounced). An XBLOCKTX message which contains the transactions is then sent back.

Generally only one round trip is required to complete the block, but on occasion a transaction has to be re-requested initiating a 
second round trip.

In addition to the above, the following functionalities and configurations will be needed.

- An XTHIN service bit

- If the thinblocks service bit is turned off then your node you will not be able to request thinkblocks or receive thinblocks. 

- The coinbase transaction will always be included in the thinblock.

- During startup when the memory pool has few transactions in it, or when a block is very small and has only 1 or 2 transactions a 
thinblock may end up being larger than the regular block. In that case it is recommended that a regular block be returned to the
requestor instead of a thinblock. This typically happens when a new block is mined just seconds after the previous one.

- Bloom Size Decay algorithm: A useful phenomena occurs as the memory pools grow and get closer in sync; the bloom filter can be allowed 
to become less sparse. That means more false positives but because the memory pool has been "warmed up" there is now a very low likelihood
of missing a transaction. This bears out in practice and a simple linear decay algorithm was developed which alters both the number of 
elements and the false positive rate. However, not knowing how far out of sync our pools are in practice means we can not calculate the 
with certainty the probability of a false positive and a memory pool miss which will result in a re-requested transaction, so we need to 
be careful in not cutting too fine a line. Using this approach significantly reduces the needed size of the bloom filter by 50% (NOTE: 
any bloom filter can be sent of type CBloomFilter, however which transactions go into the filter is up to each implementation. Keep in mind 
the transactions that are chosen to be inserted into bloom filter are critical in determining how **thin** the xthinblocks will be when 
they return to the requester).

- Bloom Filter Targeting (Beginning in version 12.1): Optional but recommended `bloom filter targeting` is effective in reducing the 
size of the bloom filters during times when the memory pool is over run with transactions.  This targeting process is memory pool size 
independent and yields reliably smaller bloom filters. It works essentially by narrowing down the transactions to include in the bloom filter 
to just those that are most likely to be mined in the next block.

- A 64 bit transaction hash is used instead of the full 256 bit hash to further reduce thinblock size while still preventing hash collisions 
in the memory pool.  This is done using the GET_XTHIN p2p netmessage.  If the block can not be constructed with the transactions in the mempool
then a GET_XBLOCKTX netmessage will be requested and will contain a list of the missing transaction hashes to be retrieved.  The returning
XBLOCKTX message will contain a list of the transactions requested.

- In the event of a 64 bit hash collision the receiving peer will re-request a normal THINBLOCK with the full 256 bit transaction hashes.
This is done using a GETDATA with an inventory message containing the MSG_THINBLOCK message type.

- In the event that the re-requested THINBLOCK containing the full transaction hashes is not received or is missing transactions then a
normal block is requested.


3. Detailed Protocol Specification
----------------------------------
----------------------------------


XTHIN Service Bit
-----------------


        NODE_XTHIN = (1 << 4),




New p2p  message types
----------------------

The p2p message format is as follows:

    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data

With the exception of `thinblock`, which is optional, the following new p2p messages types must be used in order to request and receive 
Xthinblocks. These strings must to be inserted into the 12 byte command string of the p2p message.

    thinblock
    xthinblock
    xblocktx
    get_xblocktx
    get_xthin


New Enumerated message types
----------------------------

Two new enumerated messages types are added.  MSG_THINBLOCK and MSG_XTHINBLOCK and are used
for creating inventory messages which are either requested by GETDATA for the MSG_THINBLOCK, or
appended to the GET_XTHIN message, as in the case of teh MSG_XTHINBLOCK inventory message.

They are as follows:

MSG_THINBLOCK == 4  and MSG_XTHINBLOCK == 5



New P2P message construction:
-----------------------------

The following is a description of how to construct each of the new p2p message types.


### **NetMsgType::GET_XTHIN**
  
Constructed by concatenating an inventory message of type MSG_XTHINBLOCK and the bloom filter generated in BuildSeededBloomFilter() and
then sending the datastream as a NetMsgType::GET_XTHIN.
        
Construction of `GET_XTHIN` is as follows:

    CInv(MSG_XTHINBLOCK, pindex->GetBlockHash());   // Inventory message with MSG_XTHINBLOCK message type
    CBloomFilter filterMemPool;                     // a bloom filter seeded with all the transactions in the mempool



    The following illustrates the coding required for this:

    // create datastream and bloom filter         
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    CBloomFilter filterMemPool;
    BuildSeededBloomFilter(filterMemPool, vOrphanHashes, pindex->GetBlockHash());

    // concatenate data
    ss << CInv(MSG_XTHINBLOCK, pindex->GetBlockHash());
    ss << filterMemPool;
    
    // send get_xthin
    pto->PushMessage(NetMsgType::GET_XTHIN, ss);

    NOTE: when the `GET_XTHIN` is received the xthin bloom filter is stored separately from Core's filterload message which uses "pfilter".

        found in net.h:

        CBloomFilter* pfilter;
        CBloomFilter* pThinBlockFilter;         // BU - Xtreme Thinblocks: a bloom filter which is separate from the one used by SPV wallets




### **NetMsgType::XTHINBLOCK**
 
An xthinblock contains the block header followed by all the transaction hashes in the block, but only the first 64 bits of the hash is included. Finally
after checking the block against the bloom filter provided in the GET_XTHIN message, we attach any missing transactions that we believe the requesting
peer needs.

NOTE:  You must keep the order of the hashes identical to what is in the block!
NOTE:  The first transaction in the block, the `coinbase`, must always be added to vMissingTx.

Construction of `XTHINBLOCK` is as follows:

    CBlockHeader header;                        // block header
    std::vector<uint64_t> vTxHashes;            // vector of all 64 bit transactions id's in the block
    std::vector<CTransaction> vMissingTx;       // vector of transactions that did not match the bloom filter and the requesting peer needs




### **NetMsgType::THINBLOCK**
 
A thinblock contains the block header followed by all the full transaction hashes in the block. Finally after checking the 
block against the bloom filter provided in the GET_XTHIN message, we attach any missing transactions that we believe the requesting
peer needs.

NOTE:  You must keep the order of the hashes identical to what is in the block!
NOTE:  The first transaction in the block, the `coinbase`, must always be added to vMissingTx.

Construction of `THINBLOCK` is as follows:

    CBlockHeader header;                        // block header
    std::vector<uint256> vTxHashes;             // vector of all transactions id's in the block
    std::vector<CTransaction> vMissingTx;       // vector of transactions that did not match the bloom filter and the requesting peer needs





### **NetMsgType::GET_XBLOCKTX**
  
This message is used to re-request any missing transactions that were not in the mempool but are still needed in order to reconstruct
the block.

Construction of `GET_XBLOCKTX` is as follows:

    uint256 blockhash;                          // the block hash
    std::set<uint64_t> setCheapHashesToRequest; // 64bit transactions hashes to be requested that were missing from the mempool




### **NetMsgType::XBLOCKTX**
 
This message is used to send back the transactions that were re-requested and missing from the requesting peer's memory pool

Construction of `XBLOCKTX` is as follows:

    uint256 blockhash;                          // the block hash
    std::vector<CTransaction> vMissingTx;       // missing transactions to be returned




Order and logic of P2P message requests
---------------------------------------

- `NODE A:`  receives block announcement from NODE B by an INV message and then creates and issues a GET_XTHIN to NODE B

- `NODE B:`  receives GET_XTHIN and loads the attached bloom filter and creates an XTHINBLOCK which is sent back to NODE A
         (Before sending, the xthinblock **must** be checked to see if a hash collision is within the block since the
          receiving node may not be able to re-construct the block if such a collision exists, causing an addition re-request and possibly
          be flagged as node misbehavior. Therefore, if a collision is detected then immediately send preferably a THINBLOCK, but
          at minimum a full BLOCK)

- `NODE A:`  receives an XTHINBLOCK with 64 bit txn hashes
    - if there is a hash collision with the mempool then issue either a GETDATA(MSG_THINBLOCK), or a GETDATA(MSG_BLOCK) to NODE B
        - `NODE B:` returns a THINBLOCK with full 256 bit txn hashes, or the full BLOCK depending on which request was made.
        - `NODE A:` receives THINBLOCK and tries to reconstruct block, or receives the full BLOCK.
            - if THINBLOCK was received and block is still incomplete then 
                - `NODE A:` issues a GETDATA(MSG_BLOCK) for the full block

    - else if missing transactions then re-request any missing transactions which can happen with a bloom filter false positive match.
        - `NODE A:` issues a GET_XBLOCKTX to NODE B
        - `NODE B:` returns an XBLOCKTX
        - `NODE A:` receives XBLOCKTX and completes the block

- `NODE A:` Accepts the final full block, re-constructed xthinblock or re-constructed thinblock



The Importance of the Orphan Cache
----------------------------------

Orphans are transactions which arrive before their parent transaction arrives.  As such they are not eligible to be accepted into the memory pool
and are cached separately in a temporary Orphan Cache.  Once the parent arrives the orphaned transactions can then be moved from the orphan cache
to the main transaction memory pool.  It was found that the orphan cache can play an important role in the "thinness" of an xthinblock.  The 
current behavior of the orphan caching is broken.  The cache is too small and evictions happen too frequently to be of any use.  Therefore, two 
recommendations of major importance in relation to xthinblocks are:

- do not purge orphans when a node disconnects.  In prior versions, when a node disconnects then all orphans that were received from it get
purged from the orphan cache.  While this was likely put into place as some sort of attack mitigation it has no need or place in the context of
xthinblocks and only serves to limit its effectives.  Therefore currently, we do not delete associated orphans form the cache when a node disconnects.

- Allow for a much larger orphan cache.  Currently BU has implemented a default maximum of 5000 orphans.  With memory so cheap these days there is not much
of an attack possible, even if the orphans are large.  Even so, with a 5000 orphan limit, we can see the orphan cache on occasion being overrun.

- Beginning in v12.1 we also purge any orphans greater than the -mempoolexpiry which defaults to 72 hours. This is recommended in order to align
with the time when transactions get expired from the memory pool; in other words, there is no need to keep orphans when the parent transactions 
no longer exist and helps to keep the size of the orphan cache to a minimum.



Bloom Filtering for Xthinblocks
-------------------------------

A simple bloom filter is used as a compact way to make the node we are requesting an Xthinblock from aware of the contents of our own memory pool. Before
sending the `GET_XTHIN` message request we create a bloom filter by first getting all the transaction hashes in our own memory pool as well as the
transaction hashes in the Orphan Cache and then add them to our filter one by one.  The process is very fast and takes just a few milliseconds even
for a large memory pool.  Once the bloom filter is created, it is appended to the `GET_XTHIN` message before being sent.

Once the receiving node receives the `GET_XTHIN` message is removes the filter and stores it in memory as part of the CNode data structure.

        CBloomFilter* pThinBlockFilter;

NOTE: Storing the filter is recommended (if your implementation uses the thinblock re-request via the MSG_THINBLOCK getdata) because in the event of a hash 
collision the requester doesn't have to rebuild the bloom filter before requesting a standard thinblock and can use the same filter that was already
previously attached to the GET_XTHIN request.  (There is no point to sending a standard thinblock request without previously sending a GET_XTHIN request
as you will only receive a full block in return)

Once the filter is loaded the `XTHINBLOCK` can be constructed by adding transactions to the xthin that we can be `almost certain` the other node doesn't 
have.  We do this by comparing the hashes in the block to be sent with the bloom filter pThinBlockFilter.  If there is no match for a transaction
then we add it to the `XTHINBLOCK`. Once all transactions in the block have been checked we can send the `XTHINBLOCK` back to the requester.

NOTE:  We always add the `coinbase` transaction to the `XTHINBLOCK`.  It is the first transaction which shows up in a block.


Bloom Filter Targeting (BU v12.1 and above)
-------------------------------------------

Beginning in v12.1 we have almost completely re-designed BuildSeededBloomFilter().  There are two notable changes in how the bloom filter is
created.

- The bloom filter decay algorithm is simplified and works better with the larger memory pools we would expect when/if block sizes are allowed to
increase.  What we do here is simply increase the false positive rate, gradually, and exponentially, over a 72 hour period.  This is much simpler
and more intuitive and easier to modify and tune in the future.

- The list of transactions that goes into making the bloom filter is selected from the group of transactions in the memory pool that are most likely
to be mined.  We select those transactions in a fashion that is not too rigid and leaves room for error such that the bloom filter transactions tend
to be 2 or 3 times more transactions than will be in the block however they will be much less than in a mempool that has been over run. So if you had
15000 transactions in your mempool, perhaps only 5000 will be selected to be entered into the bloom filter, resulting in a much smaller filter.  Also
as the number of transactions in the mempool continues to rise, the number of transactions selected for the filter will generally not rise, making
this approach `memory pool size independent`.
