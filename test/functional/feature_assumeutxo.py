#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test for assumeutxo, a means of quickly bootstrapping a node using
a serialized version of the UTXO set at a certain height, which corresponds
to a hash that has been compiled into bitcoind.

The assumeutxo value generated and used here is committed to in
`CRegTestParams::m_assumeutxo_data` in `src/chainparams.cpp`.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, wait_until_helper


class AssumeutxoTest(BitcoinTestFramework):
    def set_test_params(self):
        """Use the pregenerated, deterministic chain up to height 199."""
        self.num_nodes = 2
        self.rpc_timeout = 120

        self.extra_args = [
            ["-fastprune", "-prune=1", "-blockfilterindex=1", "-coinstatsindex=1"],
            ["-fastprune", "-prune=1", "-blockfilterindex=1", "-coinstatsindex=1"],
        ]

    def setup_network(self):
        """Start with the nodes disconnected so that one can generate a snapshot
        including blocks the other hasn't yet seen."""
        self.add_nodes(2)
        self.start_nodes(extra_args=self.extra_args)

    def run_test(self):
        """
        Bring up two (disconnected) nodes, mine some new blocks on the first,
        and generate a UTXO snapshot.

        Load the snapshot into the second, ensure it syncs to tip and completes
        background validation when connected to the first.
        """
        n1 = self.nodes[0]
        n2 = self.nodes[1]

        START_HEIGHT = 199
        SNAPSHOT_BASE_HEIGHT = 299
        FINAL_HEIGHT = 399

        # Mock time for a deterministic chain
        for n in self.nodes:
            n.setmocktime(n.getblockheader(n.getbestblockhash())['time'])

        self.sync_blocks()

        def no_sync():
            pass

        # Generate a series of blocks that `n1` will have in the snapshot,
        # but that n2 doesn't yet see. In order for the snapshot to activate,
        # though, we have to ferry over the new headers to n2 so that it
        # isn't waiting forever to see the header of the snapshot's base block
        # while disconnected from n1.
        for i in range(100):
            self.generate(n1, nblocks=1, sync_fun=no_sync)
            newblock = n1.getblock(n1.getbestblockhash(), 0)

            # make n2 aware of the new header, but don't give it the block.
            n2.submitheader(newblock)

        # Ensure everyone is seeing the same headers.
        for n in self.nodes:
            assert_equal(n.getblockchaininfo()["headers"], SNAPSHOT_BASE_HEIGHT)

        assert_equal(n1.getblockcount(), SNAPSHOT_BASE_HEIGHT)
        assert_equal(n2.getblockcount(), START_HEIGHT)

        self.log.info(f"Creating a UTXO snapshot at height {SNAPSHOT_BASE_HEIGHT}")
        dump_output = n1.dumptxoutset('utxos.dat')

        assert_equal(
            dump_output['txoutset_hash'],
            'ef45ccdca5898b6c2145e4581d2b88c56564dd389e4bd75a1aaf6961d3edd3c0')
        assert_equal(dump_output['nchaintx'], 300)
        assert_equal(n1.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

        # Mine more blocks on top of the snapshot that n2 hasn't yet seen. This
        # will allow us to test n2's sync-to-tip on top of a snapshot.
        self.generate(n1, nblocks=100, sync_fun=no_sync)

        assert_equal(n1.getblockcount(), FINAL_HEIGHT)
        assert_equal(n2.getblockcount(), START_HEIGHT)

        assert_equal(n1.getblockchaininfo()["blocks"], FINAL_HEIGHT)

        self.log.info(f"Loading snapshot into second node from {dump_output['path']}")
        loaded = n2.loadtxoutset(dump_output['path'])
        assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        monitor = n2.getchainstates()
        assert_equal(monitor['ibd']['blocks'], START_HEIGHT)
        assert_equal(monitor['snapshot']['blocks'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(monitor['snapshot']['snapshot_blockhash'], dump_output['base_hash'])

        assert_equal(n2.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

        # Finally connect the nodes and let them sync.
        self.connect_nodes(0, 1)

        # TODO: test using stopatheight to interrupt bg sync, then restart.

        self.log.info(f"Ensuring snapshot chain syncs to tip.({FINAL_HEIGHT})")
        wait_until_helper(lambda: n2.getchainstates()['snapshot']['blocks'] == FINAL_HEIGHT)
        self.sync_blocks()

        self.log.info("Ensuring background validation completes")
        # N.B.: the `ibd` key disappears once the background validation is complete.
        wait_until_helper(lambda: not n2.getchainstates().get('ibd'))

        # Ensure indexes have synced.
        expected_filter = {
            'basic block filter index': {'synced': True, 'best_block_height': FINAL_HEIGHT},
            'coinstatsindex': {'synced': True, 'best_block_height': FINAL_HEIGHT}
        }
        self.wait_until(lambda: n2.getindexinfo() == expected_filter)

        # TODO: test submitting a transaction and verifying it appears in mempool

        for i, n in enumerate(self.nodes):
            self.log.info(f"Restarting node {i} to ensure (Check|Load)BlockIndex passes")
            self.restart_node(i, extra_args=self.extra_args[i])

            assert_equal(n.getblockchaininfo()["blocks"], FINAL_HEIGHT)

            # Both chains should appear as "ibd"; i.e. fully validated.
            assert_equal(n.getchainstates()['ibd']['blocks'], FINAL_HEIGHT)
            assert_equal(n.getchainstates().get('snapshot'), None)

            # Ensure indexes have synced
            self.wait_until(lambda: n.getindexinfo() == expected_filter)
        #
        # TODO: test running a reindex

if __name__ == '__main__':
    AssumeutxoTest().main()
