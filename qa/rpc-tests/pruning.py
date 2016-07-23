#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test pruning code
# ********
# WARNING:
# This test uses 4GB of disk space.
# This test takes 30 mins or more (up to 2 hours)
# ********

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

def calc_usage(blockdir):
    return sum(os.path.getsize(blockdir+f) for f in os.listdir(blockdir) if os.path.isfile(blockdir+f)) / (1024. * 1024.)

class PruneTest(BitcoinTestFramework):

    def __init__(self):
        super().__init__()
        self.setup_clean_chain = True
        self.num_nodes = 3

        self.utxo = []
        self.address = ["",""]
        self.txouts = gen_return_txouts()

    def setup_network(self):
        self.nodes = []
        self.is_network_split = False

        # Create nodes 0 and 1 to mine
        self.nodes.append(start_node(0, self.options.tmpdir, ["-debug","-maxreceivebuffer=20000","-blockmaxsize=999000", "-checkblocks=5"], timewait=900))
        self.nodes.append(start_node(1, self.options.tmpdir, ["-debug","-maxreceivebuffer=20000","-blockmaxsize=999000", "-checkblocks=5"], timewait=900))

        # Create node 2 to test pruning
        self.nodes.append(start_node(2, self.options.tmpdir, ["-debug","-maxreceivebuffer=20000","-prune=550"], timewait=900))
        self.prunedir = self.options.tmpdir+"/node2/regtest/blocks/"

        self.address[0] = self.nodes[0].getnewaddress()
        self.address[1] = self.nodes[1].getnewaddress()

        # Determine default relay fee
        self.relayfee = self.nodes[0].getnetworkinfo()["relayfee"]

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)
        sync_blocks(self.nodes[0:3])

    def create_big_chain(self):
        # Start by creating some coinbases we can spend later
        self.nodes[1].generate(200)
        sync_blocks(self.nodes[0:2])
        self.nodes[0].generate(150)
        # Then mine enough full blocks to create more than 550MiB of data
        for i in range(645):
            self.mine_full_block(self.nodes[0], self.address[0])

        sync_blocks(self.nodes[0:3])

    def test_height_min(self):
        if not os.path.isfile(self.prunedir+"blk00000.dat"):
            raise AssertionError("blk00000.dat is missing, pruning too early")
        print("Success")
        print("Though we're already using more than 550MiB, current usage:", calc_usage(self.prunedir))
        print("Mining 25 more blocks should cause the first block file to be pruned")
        # Pruning doesn't run until we're allocating another chunk, 20 full blocks past the height cutoff will ensure this
        for i in range(25):
            self.mine_full_block(self.nodes[0],self.address[0])

        waitstart = time.time()
        while os.path.isfile(self.prunedir+"blk00000.dat"):
            time.sleep(0.1)
            if time.time() - waitstart > 30:
                raise AssertionError("blk00000.dat not pruned when it should be")

        print("Success")
        usage = calc_usage(self.prunedir)
        print("Usage should be below target:", usage)
        if (usage > 550):
            raise AssertionError("Pruning target not being met")

    def create_chain_with_staleblocks(self):
        # Create stale blocks in manageable sized chunks
        print("Mine 24 (stale) blocks on Node 1, followed by 25 (main chain) block reorg from Node 0, for 12 rounds")

        for j in range(12):
            # Disconnect node 0 so it can mine a longer reorg chain without knowing about node 1's soon-to-be-stale chain
            # Node 2 stays connected, so it hears about the stale blocks and then reorg's when node0 reconnects
            # Stopping node 0 also clears its mempool, so it doesn't have node1's transactions to accidentally mine
            stop_node(self.nodes[0],0)
            self.nodes[0]=start_node(0, self.options.tmpdir, ["-debug","-maxreceivebuffer=20000","-blockmaxsize=999000", "-checkblocks=5"], timewait=900)
            # Mine 24 blocks in node 1
            self.utxo = self.nodes[1].listunspent()
            for i in range(24):
                if j == 0:
                    self.mine_full_block(self.nodes[1],self.address[1])
                else:
                    self.nodes[1].generate(1) #tx's already in mempool from previous disconnects

            # Reorg back with 25 block chain from node 0
            self.utxo = self.nodes[0].listunspent()
            for i in range(25):
                self.mine_full_block(self.nodes[0],self.address[0])

            # Create connections in the order so both nodes can see the reorg at the same time
            connect_nodes(self.nodes[1], 0)
            connect_nodes(self.nodes[2], 0)
            sync_blocks(self.nodes[0:3])

        print("Usage can be over target because of high stale rate:", calc_usage(self.prunedir))

    def reorg_test(self):
        # Node 1 will mine a 300 block chain starting 287 blocks back from Node 0 and Node 2's tip
        # This will cause Node 2 to do a reorg requiring 288 blocks of undo data to the reorg_test chain
        # Reboot node 1 to clear its mempool (hopefully make the invalidate faster)
        # Lower the block max size so we don't keep mining all our big mempool transactions (from disconnected blocks)
        stop_node(self.nodes[1],1)
        self.nodes[1]=start_node(1, self.options.tmpdir, ["-debug","-maxreceivebuffer=20000","-blockmaxsize=5000", "-checkblocks=5", "-disablesafemode"], timewait=900)

        height = self.nodes[1].getblockcount()
        print("Current block height:", height)

        invalidheight = height-287
        badhash = self.nodes[1].getblockhash(invalidheight)
        print("Invalidating block at height:",invalidheight,badhash)
        self.nodes[1].invalidateblock(badhash)

        # We've now switched to our previously mined-24 block fork on node 1, but thats not what we want
        # So invalidate that fork as well, until we're on the same chain as node 0/2 (but at an ancestor 288 blocks ago)
        mainchainhash = self.nodes[0].getblockhash(invalidheight - 1)
        curhash = self.nodes[1].getblockhash(invalidheight - 1)
        while curhash != mainchainhash:
            self.nodes[1].invalidateblock(curhash)
            curhash = self.nodes[1].getblockhash(invalidheight - 1)

        assert(self.nodes[1].getblockcount() == invalidheight - 1)
        print("New best height", self.nodes[1].getblockcount())

        # Reboot node1 to clear those giant tx's from mempool
        stop_node(self.nodes[1],1)
        self.nodes[1]=start_node(1, self.options.tmpdir, ["-debug","-maxreceivebuffer=20000","-blockmaxsize=5000", "-checkblocks=5", "-disablesafemode"], timewait=900)

        print("Generating new longer chain of 300 more blocks")
        self.nodes[1].generate(300)

        print("Reconnect nodes")
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[2], 1)
        sync_blocks(self.nodes[0:3], timeout=120)

        print("Verify height on node 2:",self.nodes[2].getblockcount())
        print("Usage possibly still high bc of stale blocks in block files:", calc_usage(self.prunedir))

        print("Mine 220 more blocks so we have requisite history (some blocks will be big and cause pruning of previous chain)")
        self.nodes[0].generate(220) #node 0 has many large tx's in its mempool from the disconnects
        sync_blocks(self.nodes[0:3], timeout=300)

        usage = calc_usage(self.prunedir)
        print("Usage should be below target:", usage)
        if (usage > 550):
            raise AssertionError("Pruning target not being met")

        return invalidheight,badhash

    def reorg_back(self):
        # Verify that a block on the old main chain fork has been pruned away
        try:
            self.nodes[2].getblock(self.forkhash)
            raise AssertionError("Old block wasn't pruned so can't test redownload")
        except JSONRPCException as e:
            print("Will need to redownload block",self.forkheight)

        # Verify that we have enough history to reorg back to the fork point
        # Although this is more than 288 blocks, because this chain was written more recently
        # and only its other 299 small and 220 large block are in the block files after it,
        # its expected to still be retained
        self.nodes[2].getblock(self.nodes[2].getblockhash(self.forkheight))

        first_reorg_height = self.nodes[2].getblockcount()
        curchainhash = self.nodes[2].getblockhash(self.mainchainheight)
        self.nodes[2].invalidateblock(curchainhash)
        goalbestheight = self.mainchainheight
        goalbesthash = self.mainchainhash2

        # As of 0.10 the current block download logic is not able to reorg to the original chain created in
        # create_chain_with_stale_blocks because it doesn't know of any peer thats on that chain from which to
        # redownload its missing blocks.
        # Invalidate the reorg_test chain in node 0 as well, it can successfully switch to the original chain
        # because it has all the block data.
        # However it must mine enough blocks to have a more work chain than the reorg_test chain in order
        # to trigger node 2's block download logic.
        # At this point node 2 is within 288 blocks of the fork point so it will preserve its ability to reorg
        if self.nodes[2].getblockcount() < self.mainchainheight:
            blocks_to_mine = first_reorg_height + 1 - self.mainchainheight
            print("Rewind node 0 to prev main chain to mine longer chain to trigger redownload. Blocks needed:", blocks_to_mine)
            self.nodes[0].invalidateblock(curchainhash)
            assert(self.nodes[0].getblockcount() == self.mainchainheight)
            assert(self.nodes[0].getbestblockhash() == self.mainchainhash2)
            goalbesthash = self.nodes[0].generate(blocks_to_mine)[-1]
            goalbestheight = first_reorg_height + 1

        print("Verify node 2 reorged back to the main chain, some blocks of which it had to redownload")
        waitstart = time.time()
        while self.nodes[2].getblockcount() < goalbestheight:
            time.sleep(0.1)
            if time.time() - waitstart > 900:
                raise AssertionError("Node 2 didn't reorg to proper height")
        assert(self.nodes[2].getbestblockhash() == goalbesthash)
        # Verify we can now have the data for a block previously pruned
        assert(self.nodes[2].getblock(self.forkhash)["height"] == self.forkheight)

    def mine_full_block(self, node, address):
        # Want to create a full block
        # We'll generate a 66k transaction below, and 14 of them is close to the 1MB block limit
        for j in range(14):
            if len(self.utxo) < 14:
                self.utxo = node.listunspent()
            inputs=[]
            outputs = {}
            t = self.utxo.pop()
            inputs.append({ "txid" : t["txid"], "vout" : t["vout"]})
            remchange = t["amount"] - 100*self.relayfee # Fee must be above min relay rate for 66kb tx
            outputs[address]=remchange
            # Create a basic transaction that will send change back to ourself after account for a fee
            # And then insert the 128 generated transaction outs in the middle rawtx[92] is where the #
            # of txouts is stored and is the only thing we overwrite from the original transaction
            rawtx = node.createrawtransaction(inputs, outputs)
            newtx = rawtx[0:92]
            newtx = newtx + self.txouts
            newtx = newtx + rawtx[94:]
            # Appears to be ever so slightly faster to sign with SIGHASH_NONE
            signresult = node.signrawtransaction(newtx,None,None,"NONE")
            txid = node.sendrawtransaction(signresult["hex"], True)
        # Mine a full sized block which will be these transactions we just created
        node.generate(1)


    def run_test(self):
        print("Warning! This test requires 4GB of disk space and takes over 30 mins (up to 2 hours)")
        print("Mining a big blockchain of 995 blocks")
        self.create_big_chain()
        # Chain diagram key:
        # *   blocks on main chain
        # +,&,$,@ blocks on other forks
        # X   invalidated block
        # N1  Node 1
        #
        # Start by mining a simple chain that all nodes have
        # N0=N1=N2 **...*(995)

        print("Check that we haven't started pruning yet because we're below PruneAfterHeight")
        self.test_height_min()
        # Extend this chain past the PruneAfterHeight
        # N0=N1=N2 **...*(1020)

        print("Check that we'll exceed disk space target if we have a very high stale block rate")
        self.create_chain_with_staleblocks()
        # Disconnect N0
        # And mine a 24 block chain on N1 and a separate 25 block chain on N0
        # N1=N2 **...*+...+(1044)
        # N0    **...**...**(1045)
        #
        # reconnect nodes causing reorg on N1 and N2
        # N1=N2 **...*(1020) *...**(1045)
        #                   \
        #                    +...+(1044)
        #
        # repeat this process until you have 12 stale forks hanging off the
        # main chain on N1 and N2
        # N0    *************************...***************************(1320)
        #
        # N1=N2 **...*(1020) *...**(1045) *..         ..**(1295) *...**(1320)
        #                   \            \                      \
        #                    +...+(1044)  &..                    $...$(1319)

        # Save some current chain state for later use
        self.mainchainheight = self.nodes[2].getblockcount()   #1320
        self.mainchainhash2 = self.nodes[2].getblockhash(self.mainchainheight)

        print("Check that we can survive a 288 block reorg still")
        (self.forkheight,self.forkhash) = self.reorg_test() #(1033, )
        # Now create a 288 block reorg by mining a longer chain on N1
        # First disconnect N1
        # Then invalidate 1033 on main chain and 1032 on fork so height is 1032 on main chain
        # N1   **...*(1020) **...**(1032)X..
        #                  \
        #                   ++...+(1031)X..
        #
        # Now mine 300 more blocks on N1
        # N1    **...*(1020) **...**(1032) @@...@(1332)
        #                 \               \
        #                  \               X...
        #                   \                 \
        #                    ++...+(1031)X..   ..
        #
        # Reconnect nodes and mine 220 more blocks on N1
        # N1    **...*(1020) **...**(1032) @@...@@@(1552)
        #                 \               \
        #                  \               X...
        #                   \                 \
        #                    ++...+(1031)X..   ..
        #
        # N2    **...*(1020) **...**(1032) @@...@@@(1552)
        #                 \               \
        #                  \               *...**(1320)
        #                   \                 \
        #                    ++...++(1044)     ..
        #
        # N0    ********************(1032) @@...@@@(1552)
        #                                 \
        #                                  *...**(1320)

        print("Test that we can rerequest a block we previously pruned if needed for a reorg")
        self.reorg_back()
        # Verify that N2 still has block 1033 on current chain (@), but not on main chain (*)
        # Invalidate 1033 on current chain (@) on N2 and we should be able to reorg to
        # original main chain (*), but will require redownload of some blocks
        # In order to have a peer we think we can download from, must also perform this invalidation
        # on N0 and mine a new longest chain to trigger.
        # Final result:
        # N0    ********************(1032) **...****(1553)
        #                                 \
        #                                  X@...@@@(1552)
        #
        # N2    **...*(1020) **...**(1032) **...****(1553)
        #                 \               \
        #                  \               X@...@@@(1552)
        #                   \
        #                    +..
        #
        # N1 doesn't change because 1033 on main chain (*) is invalid

        print("Done")

if __name__ == '__main__':
    PruneTest().main()
