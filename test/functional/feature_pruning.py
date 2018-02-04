#!/usr/bin/env python3
# Copyright (c) 2014-2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the pruning code.

WARNING:
This test uses 4GB of disk space.
This test takes 30 mins or more (up to 2 hours)
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *
import time
import os

MIN_BLOCKS_TO_KEEP = 288

# Rescans start at the earliest block up to 2 hours before a key timestamp, so
# the manual prune RPC avoids pruning blocks in the same window to be
# compatible with pruning based on key creation time.
TIMESTAMP_WINDOW = 2 * 60 * 60


def calc_usage(blockdir):
    return sum(os.path.getsize(blockdir+f) for f in os.listdir(blockdir) if os.path.isfile(blockdir+f)) / (1024. * 1024.)

class PruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 6

        # Create nodes 0 and 1 to mine.
        # Create node 2 to test pruning.
        self.full_node_default_args = ["-maxreceivebuffer=20000","-blockmaxsize=999000", "-checkblocks=5", "-limitdescendantcount=100", "-limitdescendantsize=5000", "-limitancestorcount=100", "-limitancestorsize=5000" ]
        # Create nodes 3 and 4 to test manual pruning (they will be re-started with manual pruning later)
        # Create nodes 5 to test wallet in prune mode, but do not connect
        self.extra_args = [self.full_node_default_args,
                           self.full_node_default_args,
                           ["-maxreceivebuffer=20000", "-prune=550"],
                           ["-maxreceivebuffer=20000", "-blockmaxsize=999000"],
                           ["-maxreceivebuffer=20000", "-blockmaxsize=999000"],
                           ["-prune=550"]]

    def setup_network(self):
        self.setup_nodes()

        self.prunedir = self.options.tmpdir + "/node2/regtest/blocks/"

        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[1], 2)
        connect_nodes(self.nodes[2], 0)
        connect_nodes(self.nodes[0], 3)
        connect_nodes(self.nodes[0], 4)
        sync_blocks(self.nodes[0:5])

    def setup_nodes(self):
        self.add_nodes(self.num_nodes, self.extra_args, timewait=900)
        self.start_nodes()

    def create_big_chain(self):
        # Start by creating some coinbases we can spend later
        self.nodes[1].generate(200)
        sync_blocks(self.nodes[0:2])
        self.nodes[0].generate(150)
        # Then mine enough full blocks to create more than 550MiB of data
        for i in range(645):
            mine_large_block(self.nodes[0], self.utxo_cache_0)

        sync_blocks(self.nodes[0:5])

    def test_height_min(self):
        if not os.path.isfile(self.prunedir+"blk00000.dat"):
            raise AssertionError("blk00000.dat is missing, pruning too early")
        self.log.info("Success")
        self.log.info("Though we're already using more than 550MiB, current usage: %d" % calc_usage(self.prunedir))
        self.log.info("Mining 25 more blocks should cause the first block file to be pruned")
        # Pruning doesn't run until we're allocating another chunk, 20 full blocks past the height cutoff will ensure this
        for i in range(25):
            mine_large_block(self.nodes[0], self.utxo_cache_0)

        waitstart = time.time()
        while os.path.isfile(self.prunedir+"blk00000.dat"):
            time.sleep(0.1)
            if time.time() - waitstart > 30:
                raise AssertionError("blk00000.dat not pruned when it should be")

        self.log.info("Success")
        usage = calc_usage(self.prunedir)
        self.log.info("Usage should be below target: %d" % usage)
        if (usage > 550):
            raise AssertionError("Pruning target not being met")

    def create_chain_with_staleblocks(self):
        # Create stale blocks in manageable sized chunks
        self.log.info("Mine 24 (stale) blocks on Node 1, followed by 25 (main chain) block reorg from Node 0, for 12 rounds")

        for j in range(12):
            # Disconnect node 0 so it can mine a longer reorg chain without knowing about node 1's soon-to-be-stale chain
            # Node 2 stays connected, so it hears about the stale blocks and then reorg's when node0 reconnects
            # Stopping node 0 also clears its mempool, so it doesn't have node1's transactions to accidentally mine
            self.stop_node(0)
            self.start_node(0, extra_args=self.full_node_default_args)
            # Mine 24 blocks in node 1
            for i in range(24):
                if j == 0:
                    mine_large_block(self.nodes[1], self.utxo_cache_1)
                else:
                    # Add node1's wallet transactions back to the mempool, to
                    # avoid the mined blocks from being too small.
                    self.nodes[1].resendwallettransactions()
                    self.nodes[1].generate(1) #tx's already in mempool from previous disconnects

            # Reorg back with 25 block chain from node 0
            for i in range(25):
                mine_large_block(self.nodes[0], self.utxo_cache_0)

            # Create connections in the order so both nodes can see the reorg at the same time
            connect_nodes(self.nodes[1], 0)
            connect_nodes(self.nodes[2], 0)
            sync_blocks(self.nodes[0:3])

        self.log.info("Usage can be over target because of high stale rate: %d" % calc_usage(self.prunedir))

    def reorg_test(self):
        # Node 1 will mine a 300 block chain starting 287 blocks back from Node 0 and Node 2's tip
        # This will cause Node 2 to do a reorg requiring 288 blocks of undo data to the reorg_test chain
        # Reboot node 1 to clear its mempool (hopefully make the invalidate faster)
        # Lower the block max size so we don't keep mining all our big mempool transactions (from disconnected blocks)
        self.stop_node(1)
        self.start_node(1, extra_args=["-maxreceivebuffer=20000","-blockmaxsize=5000", "-checkblocks=5", "-disablesafemode"])

        height = self.nodes[1].getblockcount()
        self.log.info("Current block height: %d" % height)

        invalidheight = height-287
        badhash = self.nodes[1].getblockhash(invalidheight)
        self.log.info("Invalidating block %s at height %d" % (badhash,invalidheight))
        self.nodes[1].invalidateblock(badhash)

        # We've now switched to our previously mined-24 block fork on node 1, but that's not what we want
        # So invalidate that fork as well, until we're on the same chain as node 0/2 (but at an ancestor 288 blocks ago)
        mainchainhash = self.nodes[0].getblockhash(invalidheight - 1)
        curhash = self.nodes[1].getblockhash(invalidheight - 1)
        while curhash != mainchainhash:
            self.nodes[1].invalidateblock(curhash)
            curhash = self.nodes[1].getblockhash(invalidheight - 1)

        assert(self.nodes[1].getblockcount() == invalidheight - 1)
        self.log.info("New best height: %d" % self.nodes[1].getblockcount())

        # Reboot node1 to clear those giant tx's from mempool
        self.stop_node(1)
        self.start_node(1, extra_args=["-maxreceivebuffer=20000","-blockmaxsize=5000", "-checkblocks=5", "-disablesafemode"])

        self.log.info("Generating new longer chain of 300 more blocks")
        self.nodes[1].generate(300)

        self.log.info("Reconnect nodes")
        connect_nodes(self.nodes[0], 1)
        connect_nodes(self.nodes[2], 1)
        sync_blocks(self.nodes[0:3], timeout=120)

        self.log.info("Verify height on node 2: %d" % self.nodes[2].getblockcount())
        self.log.info("Usage possibly still high bc of stale blocks in block files: %d" % calc_usage(self.prunedir))

        self.log.info("Mine 220 more blocks so we have requisite history (some blocks will be big and cause pruning of previous chain)")

        # Get node0's wallet transactions back in its mempool, to avoid the
        # mined blocks from being too small.
        self.nodes[0].resendwallettransactions()

        for i in range(22):
            # This can be slow, so do this in multiple RPC calls to avoid
            # RPC timeouts.
            self.nodes[0].generate(10) #node 0 has many large tx's in its mempool from the disconnects
        sync_blocks(self.nodes[0:3], timeout=300)

        usage = calc_usage(self.prunedir)
        self.log.info("Usage should be below target: %d" % usage)
        if (usage > 550):
            raise AssertionError("Pruning target not being met")

        return invalidheight,badhash

    def reorg_back(self):
        # Verify that a block on the old main chain fork has been pruned away
        assert_raises_rpc_error(-1, "Block not available (pruned data)", self.nodes[2].getblock, self.forkhash)
        self.log.info("Will need to redownload block %d" % self.forkheight)

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
        # create_chain_with_stale_blocks because it doesn't know of any peer that's on that chain from which to
        # redownload its missing blocks.
        # Invalidate the reorg_test chain in node 0 as well, it can successfully switch to the original chain
        # because it has all the block data.
        # However it must mine enough blocks to have a more work chain than the reorg_test chain in order
        # to trigger node 2's block download logic.
        # At this point node 2 is within 288 blocks of the fork point so it will preserve its ability to reorg
        if self.nodes[2].getblockcount() < self.mainchainheight:
            blocks_to_mine = first_reorg_height + 1 - self.mainchainheight
            self.log.info("Rewind node 0 to prev main chain to mine longer chain to trigger redownload. Blocks needed: %d" % blocks_to_mine)
            self.nodes[0].invalidateblock(curchainhash)
            assert(self.nodes[0].getblockcount() == self.mainchainheight)
            assert(self.nodes[0].getbestblockhash() == self.mainchainhash2)
            goalbesthash = self.nodes[0].generate(blocks_to_mine)[-1]
            goalbestheight = first_reorg_height + 1

        self.log.info("Verify node 2 reorged back to the main chain, some blocks of which it had to redownload")
        waitstart = time.time()
        while self.nodes[2].getblockcount() < goalbestheight:
            time.sleep(0.1)
            if time.time() - waitstart > 900:
                raise AssertionError("Node 2 didn't reorg to proper height")
        assert(self.nodes[2].getbestblockhash() == goalbesthash)
        # Verify we can now have the data for a block previously pruned
        assert(self.nodes[2].getblock(self.forkhash)["height"] == self.forkheight)

    def manual_test(self, node_number, use_timestamp):
        # at this point, node has 995 blocks and has not yet run in prune mode
        self.start_node(node_number)
        node = self.nodes[node_number]
        assert_equal(node.getblockcount(), 995)
        assert_raises_rpc_error(-1, "not in prune mode", node.pruneblockchain, 500)

        # now re-start in manual pruning mode
        self.stop_node(node_number)
        self.start_node(node_number, extra_args=["-prune=1"])
        node = self.nodes[node_number]
        assert_equal(node.getblockcount(), 995)

        def height(index):
            if use_timestamp:
                return node.getblockheader(node.getblockhash(index))["time"] + TIMESTAMP_WINDOW
            else:
                return index

        def prune(index, expected_ret=None):
            ret = node.pruneblockchain(height(index))
            # Check the return value. When use_timestamp is True, just check
            # that the return value is less than or equal to the expected
            # value, because when more than one block is generated per second,
            # a timestamp will not be granular enough to uniquely identify an
            # individual block.
            if expected_ret is None:
                expected_ret = index
            if use_timestamp:
                assert_greater_than(ret, 0)
                assert_greater_than(expected_ret + 1, ret)
            else:
                assert_equal(ret, expected_ret)

        def has_block(index):
            return os.path.isfile(self.options.tmpdir + "/node{}/regtest/blocks/blk{:05}.dat".format(node_number, index))

        # should not prune because chain tip of node 3 (995) < PruneAfterHeight (1000)
        assert_raises_rpc_error(-1, "Blockchain is too short for pruning", node.pruneblockchain, height(500))

        # mine 6 blocks so we are at height 1001 (i.e., above PruneAfterHeight)
        node.generate(6)
        assert_equal(node.getblockchaininfo()["blocks"], 1001)

        # negative heights should raise an exception
        assert_raises_rpc_error(-8, "Negative", node.pruneblockchain, -10)

        # height=100 too low to prune first block file so this is a no-op
        prune(100)
        if not has_block(0):
            raise AssertionError("blk00000.dat is missing when should still be there")

        # Does nothing
        node.pruneblockchain(height(0))
        if not has_block(0):
            raise AssertionError("blk00000.dat is missing when should still be there")

        # height=500 should prune first file
        prune(500)
        if has_block(0):
            raise AssertionError("blk00000.dat is still there, should be pruned by now")
        if not has_block(1):
            raise AssertionError("blk00001.dat is missing when should still be there")

        # height=650 should prune second file
        prune(650)
        if has_block(1):
            raise AssertionError("blk00001.dat is still there, should be pruned by now")

        # height=1000 should not prune anything more, because tip-288 is in blk00002.dat.
        prune(1000, 1001 - MIN_BLOCKS_TO_KEEP)
        if not has_block(2):
            raise AssertionError("blk00002.dat is still there, should be pruned by now")

        # advance the tip so blk00002.dat and blk00003.dat can be pruned (the last 288 blocks should now be in blk00004.dat)
        node.generate(288)
        prune(1000)
        if has_block(2):
            raise AssertionError("blk00002.dat is still there, should be pruned by now")
        if has_block(3):
            raise AssertionError("blk00003.dat is still there, should be pruned by now")

        # stop node, start back up with auto-prune at 550MB, make sure still runs
        self.stop_node(node_number)
        self.start_node(node_number, extra_args=["-prune=550"])

        self.log.info("Success")

    def wallet_test(self):
        # check that the pruning node's wallet is still in good shape
        self.log.info("Stop and start pruning node to trigger wallet rescan")
        self.stop_node(2)
        self.start_node(2, extra_args=["-prune=550"])
        self.log.info("Success")

        # check that wallet loads successfully when restarting a pruned node after IBD.
        # this was reported to fail in #7494.
        self.log.info("Syncing node 5 to test wallet")
        connect_nodes(self.nodes[0], 5)
        nds = [self.nodes[0], self.nodes[5]]
        sync_blocks(nds, wait=5, timeout=300)
        self.stop_node(5) #stop and start to trigger rescan
        self.start_node(5, extra_args=["-prune=550"])
        self.log.info("Success")

    def run_test(self):
        self.log.info("Warning! This test requires 4GB of disk space and takes over 30 mins (up to 2 hours)")
        self.log.info("Mining a big blockchain of 995 blocks")

        # Determine default relay fee
        self.relayfee = self.nodes[0].getnetworkinfo()["relayfee"]

        # Cache for utxos, as the listunspent may take a long time later in the test
        self.utxo_cache_0 = []
        self.utxo_cache_1 = []

        self.create_big_chain()
        # Chain diagram key:
        # *   blocks on main chain
        # +,&,$,@ blocks on other forks
        # X   invalidated block
        # N1  Node 1
        #
        # Start by mining a simple chain that all nodes have
        # N0=N1=N2 **...*(995)

        # stop manual-pruning node with 995 blocks
        self.stop_node(3)
        self.stop_node(4)

        self.log.info("Check that we haven't started pruning yet because we're below PruneAfterHeight")
        self.test_height_min()
        # Extend this chain past the PruneAfterHeight
        # N0=N1=N2 **...*(1020)

        self.log.info("Check that we'll exceed disk space target if we have a very high stale block rate")
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

        self.log.info("Check that we can survive a 288 block reorg still")
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

        self.log.info("Test that we can rerequest a block we previously pruned if needed for a reorg")
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

        self.log.info("Test manual pruning with block indices")
        self.manual_test(3, use_timestamp=False)

        self.log.info("Test manual pruning with timestamps")
        self.manual_test(4, use_timestamp=True)

        self.log.info("Test wallet re-scan")
        self.wallet_test()

        self.log.info("Done")

if __name__ == '__main__':
    PruneTest().main()
