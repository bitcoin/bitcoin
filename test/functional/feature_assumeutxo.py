#!/usr/bin/env python3
# Copyright (c) 2021-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test for assumeutxo, a means of quickly bootstrapping a node using
a serialized version of the UTXO set at a certain height, which corresponds
to a hash that has been compiled into bitcoind.

The assumeutxo value generated and used here is committed to in
`CRegTestParams::m_assumeutxo_data` in `src/kernel/chainparams.cpp`.
"""
from shutil import rmtree

from dataclasses import dataclass
from test_framework.blocktools import (
        create_block,
        create_coinbase
)
from test_framework.messages import tx_from_hex
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import (
    getnewdestination,
    MiniWallet,
)

START_HEIGHT = 199
SNAPSHOT_BASE_HEIGHT = 299
FINAL_HEIGHT = 399
COMPLETE_IDX = {'synced': True, 'best_block_height': FINAL_HEIGHT}


class AssumeutxoTest(BitcoinTestFramework):

    def set_test_params(self):
        """Use the pregenerated, deterministic chain up to height 199."""
        self.num_nodes = 4
        self.rpc_timeout = 120
        self.extra_args = [
            [],
            ["-fastprune", "-prune=1", "-blockfilterindex=1", "-coinstatsindex=1"],
            ["-persistmempool=0","-txindex=1", "-blockfilterindex=1", "-coinstatsindex=1"],
            []
        ]

    def setup_network(self):
        """Start with the nodes disconnected so that one can generate a snapshot
        including blocks the other hasn't yet seen."""
        self.add_nodes(4)
        self.start_nodes(extra_args=self.extra_args)

    def test_invalid_snapshot_scenarios(self, valid_snapshot_path):
        self.log.info("Test different scenarios of loading invalid snapshot files")
        with open(valid_snapshot_path, 'rb') as f:
            valid_snapshot_contents = f.read()
        bad_snapshot_path = valid_snapshot_path + '.mod'
        node = self.nodes[1]

        def expected_error(log_msg="", rpc_details=""):
            with node.assert_debug_log([log_msg]):
                assert_raises_rpc_error(-32603, f"Unable to load UTXO snapshot{rpc_details}", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with invalid file magic")
        parsing_error_code = -22
        bad_magic = 0xf00f00f000
        with open(bad_snapshot_path, 'wb') as f:
            f.write(bad_magic.to_bytes(5, "big") + valid_snapshot_contents[5:])
        assert_raises_rpc_error(parsing_error_code, "Unable to parse metadata: Invalid UTXO set snapshot magic bytes. Please check if this is indeed a snapshot file or if you are using an outdated snapshot format.", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with unsupported version")
        for version in [0, 2]:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:5] + version.to_bytes(2, "little") + valid_snapshot_contents[7:])
            assert_raises_rpc_error(parsing_error_code, f"Unable to parse metadata: Version of snapshot {version} does not match any of the supported versions.", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with mismatching network magic")
        invalid_magics = [
            # magic, name, real
            [0xf9beb4d9, "main", True],
            [0x0b110907, "test", True],
            [0x0a03cf40, "signet", True],
            [0x00000000, "", False],
            [0xffffffff, "", False],
        ]
        for [magic, name, real] in invalid_magics:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:7] + magic.to_bytes(4, 'big') + valid_snapshot_contents[11:])
            if real:
                assert_raises_rpc_error(parsing_error_code, f"Unable to parse metadata: The network of the snapshot ({name}) does not match the network of this node (regtest).", node.loadtxoutset, bad_snapshot_path)
            else:
                assert_raises_rpc_error(parsing_error_code, "Unable to parse metadata: This snapshot has been created for an unrecognized network. This could be a custom signet, a new testnet or possibly caused by data corruption.", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file referring to a block that is not in the assumeutxo parameters")
        prev_block_hash = self.nodes[0].getblockhash(SNAPSHOT_BASE_HEIGHT - 1)
        bogus_block_hash = "0" * 64  # Represents any unknown block hash
        # The height is not used for anything critical currently, so we just
        # confirm the manipulation in the error message
        bogus_height = 1337
        for bad_block_hash in [bogus_block_hash, prev_block_hash]:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:11] + bogus_height.to_bytes(4, "little") + bytes.fromhex(bad_block_hash)[::-1] + valid_snapshot_contents[47:])

            msg = f"Unable to load UTXO snapshot: assumeutxo block hash in snapshot metadata not recognized (hash: {bad_block_hash}, height: {bogus_height}). The following snapshot heights are available: 110, 200, 299."
            assert_raises_rpc_error(-32603, msg, node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with wrong number of coins")
        valid_num_coins = int.from_bytes(valid_snapshot_contents[47:47 + 8], "little")
        for off in [-1, +1]:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:47])
                f.write((valid_num_coins + off).to_bytes(8, "little"))
                f.write(valid_snapshot_contents[47 + 8:])
            expected_error(log_msg=f"bad snapshot - coins left over after deserializing 298 coins" if off == -1 else f"bad snapshot format or truncated snapshot after deserializing 299 coins")

        self.log.info("  - snapshot file with alternated but parsable UTXO data results in different hash")
        cases = [
            # (content, offset, wrong_hash, custom_message)
            [b"\xff" * 32, 0, "7d52155c9a9fdc4525b637ef6170568e5dad6fabd0b1fdbb9432010b8453095b", None],  # wrong outpoint hash
            [(2).to_bytes(1, "little"), 32, None, "[snapshot] bad snapshot data after deserializing 1 coins"],  # wrong txid coins count
            [b"\xfd\xff\xff", 32, None, "[snapshot] mismatch in coins count in snapshot metadata and actual snapshot data"],  # txid coins count exceeds coins left
            [b"\x01", 33, "9f4d897031ab8547665b4153317ae2fdbf0130c7840b66427ebc48b881cb80ad", None],  # wrong outpoint index
            [b"\x81", 34, "3da966ba9826fb6d2604260e01607b55ba44e1a5de298606b08704bc62570ea8", None],  # wrong coin code VARINT
            [b"\x80", 34, "091e893b3ccb4334378709578025356c8bcb0a623f37c7c4e493133c988648e5", None],  # another wrong coin code
            [b"\x84\x58", 34, None, "[snapshot] bad snapshot data after deserializing 0 coins"],  # wrong coin case with height 364 and coinbase 0
            [b"\xCA\xD2\x8F\x5A", 39, None, "[snapshot] bad snapshot data after deserializing 0 coins - bad tx out value"],  # Amount exceeds MAX_MONEY
        ]

        for content, offset, wrong_hash, custom_message in cases:
            with open(bad_snapshot_path, "wb") as f:
                # Prior to offset: Snapshot magic, snapshot version, network magic, height, hash, coins count
                f.write(valid_snapshot_contents[:(5 + 2 + 4 + 4 + 32 + 8 + offset)])
                f.write(content)
                f.write(valid_snapshot_contents[(5 + 2 + 4 + 4 + 32 + 8 + offset + len(content)):])

            log_msg = custom_message if custom_message is not None else f"[snapshot] bad snapshot content hash: expected a4bf3407ccb2cc0145c49ebba8fa91199f8a3903daf0883875941497d2493c27, got {wrong_hash}"
            expected_error(log_msg=log_msg)

    def test_headers_not_synced(self, valid_snapshot_path):
        for node in self.nodes[1:]:
            msg = "Unable to load UTXO snapshot: The base block header (3bb7ce5eba0be48939b7a521ac1ba9316afee2c7bada3a0cca24188e6d7d96c0) must appear in the headers chain. Make sure all headers are syncing, and call loadtxoutset again."
            assert_raises_rpc_error(-32603, msg, node.loadtxoutset, valid_snapshot_path)

    def test_invalid_chainstate_scenarios(self):
        self.log.info("Test different scenarios of invalid snapshot chainstate in datadir")

        self.log.info("  - snapshot chainstate referring to a block that is not in the assumeutxo parameters")
        self.stop_node(0)
        chainstate_snapshot_path = self.nodes[0].chain_path / "chainstate_snapshot"
        chainstate_snapshot_path.mkdir()
        with open(chainstate_snapshot_path / "base_blockhash", 'wb') as f:
            f.write(b'z' * 32)

        def expected_error(log_msg="", error_msg=""):
            with self.nodes[0].assert_debug_log([log_msg]):
                self.nodes[0].assert_start_raises_init_error(expected_msg=error_msg)

        expected_error_msg = f"Error: A fatal internal error occurred, see debug.log for details: Assumeutxo data not found for the given blockhash '7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a'."
        error_details = f"Assumeutxo data not found for the given blockhash"
        expected_error(log_msg=error_details, error_msg=expected_error_msg)

        # resurrect node again
        rmtree(chainstate_snapshot_path)
        self.start_node(0)

    def test_invalid_mempool_state(self, dump_output_path):
        self.log.info("Test bitcoind should fail when mempool not empty.")
        node=self.nodes[2]
        tx = MiniWallet(node).send_self_transfer(from_node=node)

        assert tx['txid'] in node.getrawmempool()

        # Attempt to load the snapshot on Node 2 and expect it to fail
        msg = "Unable to load UTXO snapshot: Can't activate a snapshot when mempool not empty"
        assert_raises_rpc_error(-32603, msg, node.loadtxoutset, dump_output_path)

        self.restart_node(2, extra_args=self.extra_args[2])

    def test_invalid_file_path(self):
        self.log.info("Test bitcoind should fail when file path is invalid.")
        node = self.nodes[0]
        path = node.datadir_path / node.chain / "invalid" / "path"
        assert_raises_rpc_error(-8, "Couldn't open file {} for reading.".format(path), node.loadtxoutset, path)

    def test_snapshot_with_less_work(self, dump_output_path):
        self.log.info("Test bitcoind should fail when snapshot has less accumulated work than this node.")
        node = self.nodes[0]
        with node.assert_debug_log(expected_msgs=["[snapshot] activation failed - work does not exceed active chainstate"]):
            assert_raises_rpc_error(-32603, "Unable to load UTXO snapshot", node.loadtxoutset, dump_output_path)

    def test_snapshot_block_invalidated(self, dump_output_path):
        self.log.info("Test snapshot is not loaded when base block is invalid.")
        node = self.nodes[0]
        # We are testing the case where the base block is invalidated itself
        # and also the case where one of its parents is invalidated.
        for height in [SNAPSHOT_BASE_HEIGHT, SNAPSHOT_BASE_HEIGHT - 1]:
            block_hash = node.getblockhash(height)
            node.invalidateblock(block_hash)
            assert_equal(node.getblockcount(), height - 1)
            msg = "Unable to load UTXO snapshot: The base block header (3bb7ce5eba0be48939b7a521ac1ba9316afee2c7bada3a0cca24188e6d7d96c0) is part of an invalid chain."
            assert_raises_rpc_error(-32603, msg, node.loadtxoutset, dump_output_path)
            node.reconsiderblock(block_hash)

    def test_snapshot_in_a_divergent_chain(self, dump_output_path):
        n0 = self.nodes[0]
        n3 = self.nodes[3]
        assert_equal(n0.getblockcount(), FINAL_HEIGHT)
        assert_equal(n3.getblockcount(), START_HEIGHT)

        self.log.info("Check importing a snapshot where current chain-tip is not an ancestor of the snapshot block but has less work")
        # Generate a divergent chain in n3 up to 298
        self.generate(n3, nblocks=99, sync_fun=self.no_op)
        assert_equal(n3.getblockcount(), SNAPSHOT_BASE_HEIGHT - 1)

        # Try importing the snapshot and assert its success
        loaded = n3.loadtxoutset(dump_output_path)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)
        normal, snapshot = n3.getchainstates()["chainstates"]
        assert_equal(normal['blocks'], START_HEIGHT + 99)
        assert_equal(snapshot['blocks'], SNAPSHOT_BASE_HEIGHT)

        # Now lets sync the nodes and wait for the background validation to finish
        self.connect_nodes(0, 3)
        self.sync_blocks(nodes=(n0, n3))
        self.wait_until(lambda: len(n3.getchainstates()['chainstates']) == 1)

    def test_snapshot_not_on_most_work_chain(self, dump_output_path):
        self.log.info("Test snapshot is not loaded when the node knows the headers of another chain with more work.")
        node0 = self.nodes[0]
        node1 = self.nodes[1]
        # Create an alternative chain of 2 new blocks, forking off the main chain at the block before the snapshot block.
        # This simulates a longer chain than the main chain when submitting these two block headers to node 1 because it is only aware of
        # the main chain headers up to the snapshot height.
        parent_block_hash = node0.getblockhash(SNAPSHOT_BASE_HEIGHT - 1)
        block_time = node0.getblock(node0.getbestblockhash())['time'] + 1
        fork_block1 = create_block(int(parent_block_hash, 16), create_coinbase(SNAPSHOT_BASE_HEIGHT), block_time)
        fork_block1.solve()
        fork_block2 = create_block(fork_block1.sha256, create_coinbase(SNAPSHOT_BASE_HEIGHT + 1), block_time + 1)
        fork_block2.solve()
        node1.submitheader(fork_block1.serialize().hex())
        node1.submitheader(fork_block2.serialize().hex())
        msg = "A forked headers-chain with more work than the chain with the snapshot base block header exists. Please proceed to sync without AssumeUtxo."
        assert_raises_rpc_error(-32603, msg, node1.loadtxoutset, dump_output_path)
        # Cleanup: submit two more headers of the snapshot chain to node 1, so that it is the most-work chain again and loading
        # the snapshot in future subtests succeeds
        main_block1 = node0.getblock(node0.getblockhash(SNAPSHOT_BASE_HEIGHT + 1), 0)
        main_block2 = node0.getblock(node0.getblockhash(SNAPSHOT_BASE_HEIGHT + 2), 0)
        node1.submitheader(main_block1)
        node1.submitheader(main_block2)

    def run_test(self):
        """
        Bring up two (disconnected) nodes, mine some new blocks on the first,
        and generate a UTXO snapshot.

        Load the snapshot into the second, ensure it syncs to tip and completes
        background validation when connected to the first.
        """
        n0 = self.nodes[0]
        n1 = self.nodes[1]
        n2 = self.nodes[2]
        n3 = self.nodes[3]

        self.mini_wallet = MiniWallet(n0)

        # Mock time for a deterministic chain
        for n in self.nodes:
            n.setmocktime(n.getblockheader(n.getbestblockhash())['time'])

        # Generate a series of blocks that `n0` will have in the snapshot,
        # but that n1 and n2 don't yet see.
        assert n0.getblockcount() == START_HEIGHT
        blocks = {START_HEIGHT: Block(n0.getbestblockhash(), 1, START_HEIGHT + 1)}
        for i in range(100):
            block_tx = 1
            if i % 3 == 0:
                self.mini_wallet.send_self_transfer(from_node=n0)
                block_tx += 1
            self.generate(n0, nblocks=1, sync_fun=self.no_op)
            height = n0.getblockcount()
            hash = n0.getbestblockhash()
            blocks[height] = Block(hash, block_tx, blocks[height-1].chain_tx + block_tx)
            if i == 4:
                # Create a stale block that forks off the main chain before the snapshot.
                temp_invalid = n0.getbestblockhash()
                n0.invalidateblock(temp_invalid)
                stale_hash = self.generateblock(n0, output="raw(aaaa)", transactions=[], sync_fun=self.no_op)["hash"]
                n0.invalidateblock(stale_hash)
                n0.reconsiderblock(temp_invalid)
                stale_block = n0.getblock(stale_hash, 0)


        self.log.info("-- Testing assumeutxo + some indexes + pruning")

        assert_equal(n0.getblockcount(), SNAPSHOT_BASE_HEIGHT)
        assert_equal(n1.getblockcount(), START_HEIGHT)

        self.log.info(f"Creating a UTXO snapshot at height {SNAPSHOT_BASE_HEIGHT}")
        dump_output = n0.dumptxoutset('utxos.dat')

        self.log.info("Test loading snapshot when the node tip is on the same block as the snapshot")
        assert_equal(n0.getblockcount(), SNAPSHOT_BASE_HEIGHT)
        assert_equal(n0.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)
        self.test_snapshot_with_less_work(dump_output['path'])

        self.log.info("Test loading snapshot when headers are not synced")
        self.test_headers_not_synced(dump_output['path'])

        # In order for the snapshot to activate, we have to ferry over the new
        # headers to n1 and n2 so that they see the header of the snapshot's
        # base block while disconnected from n0.
        for i in range(1, 300):
            block = n0.getblock(n0.getblockhash(i), 0)
            # make n1 and n2 aware of the new header, but don't give them the
            # block.
            n1.submitheader(block)
            n2.submitheader(block)
            n3.submitheader(block)

        # Ensure everyone is seeing the same headers.
        for n in self.nodes:
            assert_equal(n.getblockchaininfo()["headers"], SNAPSHOT_BASE_HEIGHT)

        assert_equal(
            dump_output['txoutset_hash'],
            "a4bf3407ccb2cc0145c49ebba8fa91199f8a3903daf0883875941497d2493c27")
        assert_equal(dump_output["nchaintx"], blocks[SNAPSHOT_BASE_HEIGHT].chain_tx)
        assert_equal(n0.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

        # Mine more blocks on top of the snapshot that n1 hasn't yet seen. This
        # will allow us to test n1's sync-to-tip on top of a snapshot.
        self.generate(n0, nblocks=100, sync_fun=self.no_op)

        assert_equal(n0.getblockcount(), FINAL_HEIGHT)
        assert_equal(n1.getblockcount(), START_HEIGHT)

        assert_equal(n0.getblockchaininfo()["blocks"], FINAL_HEIGHT)

        self.test_snapshot_with_less_work(dump_output['path'])
        self.test_invalid_mempool_state(dump_output['path'])
        self.test_invalid_snapshot_scenarios(dump_output['path'])
        self.test_invalid_chainstate_scenarios()
        self.test_invalid_file_path()
        self.test_snapshot_block_invalidated(dump_output['path'])
        self.test_snapshot_not_on_most_work_chain(dump_output['path'])

        self.log.info(f"Loading snapshot into second node from {dump_output['path']}")
        # This node's tip is on an ancestor block of the snapshot, which should
        # be the normal case
        loaded = n1.loadtxoutset(dump_output['path'])
        assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        def check_tx_counts(final: bool) -> None:
            """Check nTx and nChainTx intermediate values right after loading
            the snapshot, and final values after the snapshot is validated."""
            for height, block in blocks.items():
                tx = n1.getblockheader(block.hash)["nTx"]
                stats = n1.getchaintxstats(nblocks=1, blockhash=block.hash)
                chain_tx = stats.get("txcount", None)
                window_tx_count = stats.get("window_tx_count", None)
                tx_rate = stats.get("txrate", None)
                window_interval = stats.get("window_interval")

                # Intermediate nTx of the starting block should be set, but nTx of
                # later blocks should be 0 before they are downloaded.
                # The window_tx_count of one block is equal to the blocks tx count.
                # If the window tx count is unknown, the value is missing.
                # The tx_rate is calculated from window_tx_count and window_interval
                # when possible.
                if final or height == START_HEIGHT:
                    assert_equal(tx, block.tx)
                    assert_equal(window_tx_count, tx)
                    if window_interval > 0:
                        assert_approx(tx_rate, window_tx_count / window_interval, vspan=0.1)
                    else:
                        assert_equal(tx_rate, None)
                else:
                    assert_equal(tx, 0)
                    assert_equal(window_tx_count, None)

                # Intermediate nChainTx of the starting block and snapshot block
                # should be set, but others should be None until they are downloaded.
                if final or height in (START_HEIGHT, SNAPSHOT_BASE_HEIGHT):
                    assert_equal(chain_tx, block.chain_tx)
                else:
                    assert_equal(chain_tx, None)

        check_tx_counts(final=False)

        normal, snapshot = n1.getchainstates()["chainstates"]
        assert_equal(normal['blocks'], START_HEIGHT)
        assert_equal(normal.get('snapshot_blockhash'), None)
        assert_equal(normal['validated'], True)
        assert_equal(snapshot['blocks'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(snapshot['snapshot_blockhash'], dump_output['base_hash'])
        assert_equal(snapshot['validated'], False)

        assert_equal(n1.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

        self.log.info("Submit a stale block that forked off the chain before the snapshot")
        # Normally a block like this would not be downloaded, but if it is
        # submitted early before the background chain catches up to the fork
        # point, it winds up in m_blocks_unlinked and triggers a corner case
        # that previously crashed CheckBlockIndex.
        n1.submitblock(stale_block)
        n1.getchaintips()
        n1.getblock(stale_hash)

        self.log.info("Submit a spending transaction for a snapshot chainstate coin to the mempool")
        # spend the coinbase output of the first block that is not available on node1
        spend_coin_blockhash = n1.getblockhash(START_HEIGHT + 1)
        assert_raises_rpc_error(-1, "Block not found on disk", n1.getblock, spend_coin_blockhash)
        prev_tx = n0.getblock(spend_coin_blockhash, 3)['tx'][0]
        prevout = {"txid": prev_tx['txid'], "vout": 0, "scriptPubKey": prev_tx['vout'][0]['scriptPubKey']['hex']}
        privkey = n0.get_deterministic_priv_key().key
        raw_tx = n1.createrawtransaction([prevout], {getnewdestination()[2]: 24.99})
        signed_tx = n1.signrawtransactionwithkey(raw_tx, [privkey], [prevout])['hex']
        signed_txid = tx_from_hex(signed_tx).rehash()

        assert n1.gettxout(prev_tx['txid'], 0) is not None
        n1.sendrawtransaction(signed_tx)
        assert signed_txid in n1.getrawmempool()
        assert not n1.gettxout(prev_tx['txid'], 0)

        PAUSE_HEIGHT = FINAL_HEIGHT - 40

        self.log.info("Restarting node to stop at height %d", PAUSE_HEIGHT)
        self.restart_node(1, extra_args=[
            f"-stopatheight={PAUSE_HEIGHT}", *self.extra_args[1]])

        # Finally connect the nodes and let them sync.
        #
        # Set `wait_for_connect=False` to avoid a race between performing connection
        # assertions and the -stopatheight tripping.
        self.connect_nodes(0, 1, wait_for_connect=False)

        n1.wait_until_stopped(timeout=5)

        self.log.info("Checking that blocks are segmented on disk")
        assert self.has_blockfile(n1, "00000"), "normal blockfile missing"
        assert self.has_blockfile(n1, "00001"), "assumed blockfile missing"
        assert not self.has_blockfile(n1, "00002"), "too many blockfiles"

        self.log.info("Restarted node before snapshot validation completed, reloading...")
        self.restart_node(1, extra_args=self.extra_args[1])

        # Send snapshot block to n1 out of order. This makes the test less
        # realistic because normally the snapshot block is one of the last
        # blocks downloaded, but its useful to test because it triggers more
        # corner cases in ReceivedBlockTransactions() and CheckBlockIndex()
        # setting and testing nChainTx values, and it exposed previous bugs.
        snapshot_hash = n0.getblockhash(SNAPSHOT_BASE_HEIGHT)
        snapshot_block = n0.getblock(snapshot_hash, 0)
        n1.submitblock(snapshot_block)

        self.connect_nodes(0, 1)

        self.log.info(f"Ensuring snapshot chain syncs to tip. ({FINAL_HEIGHT})")
        self.wait_until(lambda: n1.getchainstates()['chainstates'][-1]['blocks'] == FINAL_HEIGHT)
        self.sync_blocks(nodes=(n0, n1))

        self.log.info("Ensuring background validation completes")
        self.wait_until(lambda: len(n1.getchainstates()['chainstates']) == 1)

        # Ensure indexes have synced.
        completed_idx_state = {
            'basic block filter index': COMPLETE_IDX,
            'coinstatsindex': COMPLETE_IDX,
        }
        self.wait_until(lambda: n1.getindexinfo() == completed_idx_state)

        self.log.info("Re-check nTx and nChainTx values")
        check_tx_counts(final=True)

        for i in (0, 1):
            n = self.nodes[i]
            self.log.info(f"Restarting node {i} to ensure (Check|Load)BlockIndex passes")
            self.restart_node(i, extra_args=self.extra_args[i])

            assert_equal(n.getblockchaininfo()["blocks"], FINAL_HEIGHT)

            chainstate, = n.getchainstates()['chainstates']
            assert_equal(chainstate['blocks'], FINAL_HEIGHT)

            if i != 0:
                # Ensure indexes have synced for the assumeutxo node
                self.wait_until(lambda: n.getindexinfo() == completed_idx_state)


        # Node 2: all indexes + reindex
        # -----------------------------

        self.log.info("-- Testing all indexes + reindex")
        assert_equal(n2.getblockcount(), START_HEIGHT)

        self.log.info(f"Loading snapshot into third node from {dump_output['path']}")
        loaded = n2.loadtxoutset(dump_output['path'])
        assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        for reindex_arg in ['-reindex=1', '-reindex-chainstate=1']:
            self.log.info(f"Check that restarting with {reindex_arg} will delete the snapshot chainstate")
            self.restart_node(2, extra_args=[reindex_arg, *self.extra_args[2]])
            assert_equal(1, len(n2.getchainstates()["chainstates"]))
            for i in range(1, 300):
                block = n0.getblock(n0.getblockhash(i), 0)
                n2.submitheader(block)
            loaded = n2.loadtxoutset(dump_output['path'])
            assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
            assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        normal, snapshot = n2.getchainstates()['chainstates']
        assert_equal(normal['blocks'], START_HEIGHT)
        assert_equal(normal.get('snapshot_blockhash'), None)
        assert_equal(normal['validated'], True)
        assert_equal(snapshot['blocks'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(snapshot['snapshot_blockhash'], dump_output['base_hash'])
        assert_equal(snapshot['validated'], False)

        self.log.info("Check that loading the snapshot again will fail because there is already an active snapshot.")
        msg = "Unable to load UTXO snapshot: Can't activate a snapshot-based chainstate more than once"
        assert_raises_rpc_error(-32603, msg, n2.loadtxoutset, dump_output['path'])

        self.connect_nodes(0, 2)
        self.wait_until(lambda: n2.getchainstates()['chainstates'][-1]['blocks'] == FINAL_HEIGHT)
        self.sync_blocks(nodes=(n0, n2))

        self.log.info("Ensuring background validation completes")
        self.wait_until(lambda: len(n2.getchainstates()['chainstates']) == 1)

        completed_idx_state = {
            'basic block filter index': COMPLETE_IDX,
            'coinstatsindex': COMPLETE_IDX,
            'txindex': COMPLETE_IDX,
        }
        self.wait_until(lambda: n2.getindexinfo() == completed_idx_state)

        for i in (0, 2):
            n = self.nodes[i]
            self.log.info(f"Restarting node {i} to ensure (Check|Load)BlockIndex passes")
            self.restart_node(i, extra_args=self.extra_args[i])

            assert_equal(n.getblockchaininfo()["blocks"], FINAL_HEIGHT)

            chainstate, = n.getchainstates()['chainstates']
            assert_equal(chainstate['blocks'], FINAL_HEIGHT)

            if i != 0:
                # Ensure indexes have synced for the assumeutxo node
                self.wait_until(lambda: n.getindexinfo() == completed_idx_state)

        self.log.info("Test -reindex-chainstate of an assumeutxo-synced node")
        self.restart_node(2, extra_args=[
            '-reindex-chainstate=1', *self.extra_args[2]])
        assert_equal(n2.getblockchaininfo()["blocks"], FINAL_HEIGHT)
        self.wait_until(lambda: n2.getblockcount() == FINAL_HEIGHT)

        self.log.info("Test -reindex of an assumeutxo-synced node")
        self.restart_node(2, extra_args=['-reindex=1', *self.extra_args[2]])
        self.connect_nodes(0, 2)
        self.wait_until(lambda: n2.getblockcount() == FINAL_HEIGHT)

        self.test_snapshot_in_a_divergent_chain(dump_output['path'])

@dataclass
class Block:
    hash: str
    tx: int
    chain_tx: int

if __name__ == '__main__':
    AssumeutxoTest(__file__).main()
