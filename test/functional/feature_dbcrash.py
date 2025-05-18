#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test recovery from a crash during chainstate writing.

- 4 nodes
  * node0, node1, and node2 will have different dbcrash ratios, and different
    dbcache sizes
  * node3 will be a regular node, with no crashing.
  * The nodes will not connect to each other.

- use default test framework starting chain. initialize starting_tip_height to
  tip height.

- Main loop:
  * generate lots of transactions on node3, enough to fill up a block.
  * uniformly randomly pick a tip height from starting_tip_height to
    tip_height; with probability 1/(height_difference+4), invalidate this block.
  * mine enough blocks to overtake tip_height at start of loop.
  * for each node in [node0,node1,node2]:
     - for each mined block:
       * submit block to node
       * if node crashed on/after submitting:
         - restart until recovery succeeds
         - check that utxo matches node3 using gettxoutsetinfo"""

import errno
import http.client
import random
import time

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import (
    COIN,
)
from test_framework.test_framework import TortoisecoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import (
    MiniWallet,
    getnewdestination,
)


class ChainstateWriteCrashTest(TortoisecoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4
        self.rpc_timeout = 480
        self.supports_cli = False

        # Set -maxmempool=0 to turn off mempool memory sharing with dbcache
        self.base_args = [
            "-limitdescendantsize=0",
            "-maxmempool=0",
            "-dbbatchsize=200000",
        ]

        # Set different crash ratios and cache sizes.  Note that not all of
        # -dbcache goes to the in-memory coins cache.
        self.node0_args = ["-dbcrashratio=8", "-dbcache=4"] + self.base_args
        self.node1_args = ["-dbcrashratio=16", "-dbcache=8"] + self.base_args
        self.node2_args = ["-dbcrashratio=24", "-dbcache=16"] + self.base_args

        # Node3 is a normal node with default args, except will mine full blocks
        # and txs with "dust" outputs
        self.node3_args = ["-blockmaxweight=4000000", "-dustrelayfee=0"]
        self.extra_args = [self.node0_args, self.node1_args, self.node2_args, self.node3_args]

    def setup_network(self):
        self.add_nodes(self.num_nodes, extra_args=self.extra_args)
        self.start_nodes()
        # Leave them unconnected, we'll use submitblock directly in this test

    def restart_node(self, node_index, expected_tip):
        """Start up a given node id, wait for the tip to reach the given block hash, and calculate the utxo hash.

        Exceptions on startup should indicate node crash (due to -dbcrashratio), in which case we try again. Give up
        after 60 seconds. Returns the utxo hash of the given node."""

        time_start = time.time()
        while time.time() - time_start < 120:
            try:
                # Any of these RPC calls could throw due to node crash
                self.start_node(node_index)
                self.nodes[node_index].waitforblock(expected_tip)
                utxo_hash = self.nodes[node_index].gettxoutsetinfo()['hash_serialized_3']
                return utxo_hash
            except Exception:
                # An exception here should mean the node is about to crash.
                # If tortoisecoind exits, then try again.  wait_for_node_exit()
                # should raise an exception if tortoisecoind doesn't exit.
                self.wait_for_node_exit(node_index, timeout=10)
            self.crashed_on_restart += 1
            time.sleep(1)

        # If we got here, tortoisecoind isn't coming back up on restart.  Could be a
        # bug in tortoisecoind, or we've gotten unlucky with our dbcrash ratio --
        # perhaps we generated a test case that blew up our cache?
        # TODO: If this happens a lot, we should try to restart without -dbcrashratio
        # and make sure that recovery happens.
        raise AssertionError(f"Unable to successfully restart node {node_index} in allotted time")

    def submit_block_catch_error(self, node_index, block):
        """Try submitting a block to the given node.

        Catch any exceptions that indicate the node has crashed.
        Returns true if the block was submitted successfully; false otherwise."""

        try:
            self.nodes[node_index].submitblock(block)
            return True
        except (http.client.CannotSendRequest, http.client.RemoteDisconnected) as e:
            self.log.debug(f"node {node_index} submitblock raised exception: {e}")
            return False
        except OSError as e:
            self.log.debug(f"node {node_index} submitblock raised OSError exception: errno={e.errno}")
            if e.errno in [errno.EPIPE, errno.ECONNREFUSED, errno.ECONNRESET]:
                # The node has likely crashed
                return False
            else:
                # Unexpected exception, raise
                raise

    def sync_node3blocks(self, block_hashes):
        """Use submitblock to sync node3's chain with the other nodes

        If submitblock fails, restart the node and get the new utxo hash.
        If any nodes crash while updating, we'll compare utxo hashes to
        ensure recovery was successful."""

        node3_utxo_hash = self.nodes[3].gettxoutsetinfo()['hash_serialized_3']

        # Retrieve all the blocks from node3
        blocks = []
        for block_hash in block_hashes:
            blocks.append([block_hash, self.nodes[3].getblock(block_hash, 0)])

        # Deliver each block to each other node
        for i in range(3):
            nodei_utxo_hash = None
            self.log.debug(f"Syncing blocks to node {i}")
            for (block_hash, block) in blocks:
                # Get the block from node3, and submit to node_i
                self.log.debug(f"submitting block {block_hash}")
                if not self.submit_block_catch_error(i, block):
                    # TODO: more carefully check that the crash is due to -dbcrashratio
                    # (change the exit code perhaps, and check that here?)
                    self.wait_for_node_exit(i, timeout=30)
                    self.log.debug(f"Restarting node {i} after block hash {block_hash}")
                    nodei_utxo_hash = self.restart_node(i, block_hash)
                    assert nodei_utxo_hash is not None
                    self.restart_counts[i] += 1
                else:
                    # Clear it out after successful submitblock calls -- the cached
                    # utxo hash will no longer be correct
                    nodei_utxo_hash = None

            # Check that the utxo hash matches node3's utxo set
            # NOTE: we only check the utxo set if we had to restart the node
            # after the last block submitted:
            # - checking the utxo hash causes a cache flush, which we don't
            # want to do every time; so
            # - we only update the utxo cache after a node restart, since flushing
            # the cache is a no-op at that point
            if nodei_utxo_hash is not None:
                self.log.debug(f"Checking txoutsetinfo matches for node {i}")
                assert_equal(nodei_utxo_hash, node3_utxo_hash)

    def verify_utxo_hash(self):
        """Verify that the utxo hash of each node matches node3.

        Restart any nodes that crash while querying."""
        node3_utxo_hash = self.nodes[3].gettxoutsetinfo()['hash_serialized_3']
        self.log.info("Verifying utxo hash matches for all nodes")

        for i in range(3):
            try:
                nodei_utxo_hash = self.nodes[i].gettxoutsetinfo()['hash_serialized_3']
            except OSError:
                # probably a crash on db flushing
                nodei_utxo_hash = self.restart_node(i, self.nodes[3].getbestblockhash())
            assert_equal(nodei_utxo_hash, node3_utxo_hash)

    def generate_small_transactions(self, node, count, utxo_list):
        FEE = 1000  # TODO: replace this with node relay fee based calculation
        num_transactions = 0
        random.shuffle(utxo_list)
        while len(utxo_list) >= 2 and num_transactions < count:
            utxos_to_spend = [utxo_list.pop() for _ in range(2)]
            input_amount = int(sum([utxo['value'] for utxo in utxos_to_spend]) * COIN)
            if input_amount < FEE:
                # Sanity check -- if we chose inputs that are too small, skip
                continue

            self.wallet.send_self_transfer_multi(
                from_node=node,
                utxos_to_spend=utxos_to_spend,
                num_outputs=3,
                fee_per_output=FEE // 3,
            )
            num_transactions += 1

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[3])
        initial_height = self.nodes[3].getblockcount()
        self.generate(self.nodes[3], COINBASE_MATURITY, sync_fun=self.no_op)

        # Track test coverage statistics
        self.restart_counts = [0, 0, 0]  # Track the restarts for nodes 0-2
        self.crashed_on_restart = 0      # Track count of crashes during recovery

        # Start by creating a lot of utxos on node3
        utxo_list = []
        for _ in range(5):
            utxo_list.extend(self.wallet.send_self_transfer_multi(from_node=self.nodes[3], num_outputs=1000)['new_utxos'])
        self.generate(self.nodes[3], 1, sync_fun=self.no_op)
        assert_equal(len(self.nodes[3].getrawmempool()), 0)
        self.log.info(f"Prepped {len(utxo_list)} utxo entries")

        # Sync these blocks with the other nodes
        block_hashes_to_sync = []
        for height in range(initial_height + 1, self.nodes[3].getblockcount() + 1):
            block_hashes_to_sync.append(self.nodes[3].getblockhash(height))

        self.log.debug(f"Syncing {len(block_hashes_to_sync)} blocks with other nodes")
        # Syncing the blocks could cause nodes to crash, so the test begins here.
        self.sync_node3blocks(block_hashes_to_sync)

        starting_tip_height = self.nodes[3].getblockcount()

        # Main test loop:
        # each time through the loop, generate a bunch of transactions,
        # and then either mine a single new block on the tip, or some-sized reorg.
        for i in range(40):
            self.log.info(f"Iteration {i}, generating 2500 transactions {self.restart_counts}")
            # Generate a bunch of small-ish transactions
            self.generate_small_transactions(self.nodes[3], 2500, utxo_list)
            # Pick a random block between current tip, and starting tip
            current_height = self.nodes[3].getblockcount()
            random_height = random.randint(starting_tip_height, current_height)
            self.log.debug(f"At height {current_height}, considering height {random_height}")
            if random_height > starting_tip_height:
                # Randomly reorg from this point with some probability (1/4 for
                # tip, 1/5 for tip-1, ...)
                if random.random() < 1.0 / (current_height + 4 - random_height):
                    self.log.debug(f"Invalidating block at height {random_height}")
                    self.nodes[3].invalidateblock(self.nodes[3].getblockhash(random_height))

            # Now generate new blocks until we pass the old tip height
            self.log.debug("Mining longer tip")
            block_hashes = []
            while current_height + 1 > self.nodes[3].getblockcount():
                block_hashes.extend(self.generatetoaddress(
                    self.nodes[3],
                    nblocks=min(10, current_height + 1 - self.nodes[3].getblockcount()),
                    # new address to avoid mining a block that has just been invalidated
                    address=getnewdestination()[2],
                    sync_fun=self.no_op,
                ))
            self.log.debug(f"Syncing {len(block_hashes)} new blocks...")
            self.sync_node3blocks(block_hashes)
            self.wallet.rescan_utxos()
            utxo_list = self.wallet.get_utxos()
            self.log.debug(f"MiniWallet utxo count: {len(utxo_list)}")

        # Check that the utxo hashes agree with node3
        # Useful side effect: each utxo cache gets flushed here, so that we
        # won't get crashes on shutdown at the end of the test.
        self.verify_utxo_hash()

        # Check the test coverage
        self.log.info(f"Restarted nodes: {self.restart_counts}; crashes on restart: {self.crashed_on_restart}")

        # If no nodes were restarted, we didn't test anything.
        assert self.restart_counts != [0, 0, 0]

        # Make sure we tested the case of crash-during-recovery.
        assert self.crashed_on_restart > 0

        # Warn if any of the nodes escaped restart.
        for i in range(3):
            if self.restart_counts[i] == 0:
                self.log.warning(f"Node {i} never crashed during utxo flush!")


if __name__ == "__main__":
    ChainstateWriteCrashTest(__file__).main()
