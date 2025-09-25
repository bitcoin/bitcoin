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
import contextlib
from shutil import rmtree

from dataclasses import dataclass
from test_framework.blocktools import (
        create_block,
        create_coinbase
)
from test_framework.compressor import (
    compress_amount,
)
from test_framework.messages import (
    CBlockHeader,
    from_hex,
    MAGIC_BYTES,
    MAX_MONEY,
    msg_headers,
    ser_varint,
    tx_from_hex,
)
from test_framework.p2p import (
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_approx,
    assert_equal,
    assert_not_equal,
    assert_raises_rpc_error,
    ensure_for,
    sha256sum_file,
    try_rpc,
)
from test_framework.wallet import (
    getnewdestination,
    MiniWallet,
)
from test_framework.blocktools import (
    REGTEST_N_BITS,
    REGTEST_TARGET,
    nbits_str,
    target_str,
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

        def expected_error(msg):
            assert_raises_rpc_error(-32603, f"Unable to load UTXO snapshot: Population failed: {msg}", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with invalid file magic")
        parsing_error_code = -22
        bad_magic = 0xf00f00f000
        with open(bad_snapshot_path, 'wb') as f:
            f.write(bad_magic.to_bytes(5, "big") + valid_snapshot_contents[5:])
        assert_raises_rpc_error(parsing_error_code, "Unable to parse metadata: Invalid UTXO set snapshot magic bytes. Please check if this is indeed a snapshot file or if you are using an outdated snapshot format.", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with unsupported version")
        for version in [0, 1, 3]:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:5] + version.to_bytes(2, "little") + valid_snapshot_contents[7:])
            assert_raises_rpc_error(parsing_error_code, f"Unable to parse metadata: Version of snapshot {version} does not match any of the supported versions.", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with mismatching network magic")
        invalid_magics = [
            # magic, name, real
            [MAGIC_BYTES["mainnet"], "main", True],
            [MAGIC_BYTES["testnet4"], "testnet4", True],
            [MAGIC_BYTES["signet"], "signet", True],
            [0x00000000.to_bytes(4, 'big'), "", False],
            [0xffffffff.to_bytes(4, 'big'), "", False],
        ]
        for [magic, name, real] in invalid_magics:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:7] + magic + valid_snapshot_contents[11:])
            if real:
                assert_raises_rpc_error(parsing_error_code, f"Unable to parse metadata: The network of the snapshot ({name}) does not match the network of this node (regtest).", node.loadtxoutset, bad_snapshot_path)
            else:
                assert_raises_rpc_error(parsing_error_code, "Unable to parse metadata: This snapshot has been created for an unrecognized network. This could be a custom signet, a new testnet or possibly caused by data corruption.", node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file referring to a block that is not in the assumeutxo parameters")
        prev_block_hash = self.nodes[0].getblockhash(SNAPSHOT_BASE_HEIGHT - 1)
        bogus_block_hash = "0" * 64  # Represents any unknown block hash
        for bad_block_hash in [bogus_block_hash, prev_block_hash]:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:11] + bytes.fromhex(bad_block_hash)[::-1] + valid_snapshot_contents[43:])

            msg = f"Unable to load UTXO snapshot: assumeutxo block hash in snapshot metadata not recognized (hash: {bad_block_hash}). The following snapshot heights are available: 110, 200, 299."
            assert_raises_rpc_error(-32603, msg, node.loadtxoutset, bad_snapshot_path)

        self.log.info("  - snapshot file with wrong number of coins")
        valid_num_coins = int.from_bytes(valid_snapshot_contents[43:43 + 8], "little")
        for off in [-1, +1]:
            with open(bad_snapshot_path, 'wb') as f:
                f.write(valid_snapshot_contents[:43])
                f.write((valid_num_coins + off).to_bytes(8, "little"))
                f.write(valid_snapshot_contents[43 + 8:])
            expected_error(msg="Bad snapshot - coins left over after deserializing 298 coins." if off == -1 else "Bad snapshot format or truncated snapshot after deserializing 299 coins.")

        self.log.info("  - snapshot file with alternated but parsable UTXO data results in different hash")
        cases = [
            # (content, offset, wrong_hash, custom_message)
            [b"\xff" * 32, 0, "77874d48d932a5cb7a7f770696f5224ff05746fdcf732a58270b45da0f665934", None],  # wrong outpoint hash
            [(2).to_bytes(1, "little"), 32, None, "Bad snapshot format or truncated snapshot after deserializing 1 coins."],  # wrong txid coins count
            [b"\xfd\xff\xff", 32, None, "Mismatch in coins count in snapshot metadata and actual snapshot data"],  # txid coins count exceeds coins left
            [b"\x01", 33, "9f562925721e4f97e6fde5b590dbfede51e2204a68639525062ad064545dd0ea", None],  # wrong outpoint index
            [b"\x82", 34, "161393f07f8ad71760b3910a914f677f2cb166e5bcf5354e50d46b78c0422d15", None],  # wrong coin code VARINT
            [b"\x80", 34, "e6fae191ef851554467b68acff01ca09ad0a2e48c9b3dfea46cf7d35a7fd0ad0", None],  # another wrong coin code
            [b"\x84\x58", 34, None, "Bad snapshot data after deserializing 0 coins"],  # wrong coin case with height 364 and coinbase 0
            [
                # compressed txout value + scriptpubkey
                ser_varint(compress_amount(MAX_MONEY + 1)) + ser_varint(0),
                # txid + coins per txid + vout + coin height
                32 + 1 + 1 + 2,
                None,
                "Bad snapshot data after deserializing 0 coins - bad tx out value"
            ],  # Amount exceeds MAX_MONEY
        ]

        for content, offset, wrong_hash, custom_message in cases:
            with open(bad_snapshot_path, "wb") as f:
                # Prior to offset: Snapshot magic, snapshot version, network magic, hash, coins count
                f.write(valid_snapshot_contents[:(5 + 2 + 4 + 32 + 8 + offset)])
                f.write(content)
                f.write(valid_snapshot_contents[(5 + 2 + 4 + 32 + 8 + offset + len(content)):])

            msg = custom_message if custom_message is not None else f"Bad snapshot content hash: expected d2b051ff5e8eef46520350776f4100dd710a63447a8e01d917e92e79751a63e2, got {wrong_hash}."
            expected_error(msg)

    def test_headers_not_synced(self, valid_snapshot_path):
        for node in self.nodes[1:]:
            msg = "Unable to load UTXO snapshot: The base block header (7cc695046fec709f8c9394b6f928f81e81fd3ac20977bb68760fa1faa7916ea2) must appear in the headers chain. Make sure all headers are syncing, and call loadtxoutset again."
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

        expected_error_msg = "Error: A fatal internal error occurred, see debug.log for details: Assumeutxo data not found for the given blockhash '7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a7a'."
        error_details = "Assumeutxo data not found for the given blockhash"
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
        msg = "Unable to load UTXO snapshot: Population failed: Work does not exceed active chainstate."
        assert_raises_rpc_error(-32603, msg, node.loadtxoutset, dump_output_path)

    def test_snapshot_block_invalidated(self, dump_output_path):
        self.log.info("Test snapshot is not loaded when base block is invalid.")
        node = self.nodes[0]
        # We are testing the case where the base block is invalidated itself
        # and also the case where one of its parents is invalidated.
        for height in [SNAPSHOT_BASE_HEIGHT, SNAPSHOT_BASE_HEIGHT - 1]:
            block_hash = node.getblockhash(height)
            node.invalidateblock(block_hash)
            assert_equal(node.getblockcount(), height - 1)
            msg = "Unable to load UTXO snapshot: The base block header (7cc695046fec709f8c9394b6f928f81e81fd3ac20977bb68760fa1faa7916ea2) is part of an invalid chain."
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

        # Both states should have the same nBits and target
        assert_equal(normal['bits'], nbits_str(REGTEST_N_BITS))
        assert_equal(normal['bits'], snapshot['bits'])
        assert_equal(normal['target'], target_str(REGTEST_TARGET))
        assert_equal(normal['target'], snapshot['target'])

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
        fork_block2 = create_block(fork_block1.hash_int, create_coinbase(SNAPSHOT_BASE_HEIGHT + 1), block_time + 1)
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

    def test_sync_from_assumeutxo_node(self, snapshot):
        """
        This test verifies that:
        1. An IBD node can sync headers from an AssumeUTXO node at any time.
        2. IBD nodes do not request historical blocks from AssumeUTXO nodes while they are syncing the background-chain.
        3. The assumeUTXO node dynamically adjusts the network services it offers according to its state.
        4. IBD nodes can fully sync from AssumeUTXO nodes after they finish the background-chain sync.
        """
        self.log.info("Testing IBD-sync from assumeUTXO node")
        # Node2 starts clean and loads the snapshot.
        # Node3 starts clean and seeks to sync-up from snapshot_node.
        miner = self.nodes[0]
        snapshot_node = self.nodes[2]
        ibd_node = self.nodes[3]

        # Start test fresh by cleaning up node directories
        for node in (snapshot_node, ibd_node):
            self.stop_node(node.index)
            rmtree(node.chain_path)
            self.start_node(node.index, extra_args=self.extra_args[node.index])

        # Sync-up headers chain on snapshot_node to load snapshot
        headers_provider_conn = snapshot_node.add_p2p_connection(P2PInterface())
        headers_provider_conn.wait_for_getheaders()
        msg = msg_headers()
        for block_num in range(1, miner.getblockcount()+1):
            msg.headers.append(from_hex(CBlockHeader(), miner.getblockheader(miner.getblockhash(block_num), verbose=False)))
        headers_provider_conn.send_without_ping(msg)

        # Ensure headers arrived
        default_value = {'status': ''}  # No status
        headers_tip_hash = miner.getbestblockhash()
        self.wait_until(lambda: next(filter(lambda x: x['hash'] == headers_tip_hash, snapshot_node.getchaintips()), default_value)['status'] == "headers-only")
        snapshot_node.disconnect_p2ps()

        # Load snapshot
        snapshot_node.loadtxoutset(snapshot['path'])

        # Connect nodes and verify the ibd_node can sync-up the headers-chain from the snapshot_node
        self.connect_nodes(ibd_node.index, snapshot_node.index)
        snapshot_block_hash = snapshot['base_hash']
        self.wait_until(lambda: next(filter(lambda x: x['hash'] == snapshot_block_hash, ibd_node.getchaintips()), default_value)['status'] == "headers-only")

        # Once the headers-chain is synced, the ibd_node must avoid requesting historical blocks from the snapshot_node.
        # If it does request such blocks, the snapshot_node will ignore requests it cannot fulfill, causing the ibd_node
        # to stall. This stall could last for up to 10 min, ultimately resulting in an abrupt disconnection due to the
        # ibd_node's perceived unresponsiveness.
        ensure_for(duration=3, f=lambda: len(ibd_node.getpeerinfo()[0]['inflight']) == 0)

        # Now disconnect nodes and finish background chain sync
        self.disconnect_nodes(ibd_node.index, snapshot_node.index)
        self.connect_nodes(snapshot_node.index, miner.index)
        self.sync_blocks(nodes=(miner, snapshot_node))
        # Check the base snapshot block was stored and ensure node signals full-node service support
        self.wait_until(lambda: not try_rpc(-1, "Block not available (not fully downloaded)", snapshot_node.getblock, snapshot_block_hash))
        self.wait_until(lambda: 'NETWORK' in snapshot_node.getnetworkinfo()['localservicesnames'])

        # Now that the snapshot_node is synced, verify the ibd_node can sync from it
        self.connect_nodes(snapshot_node.index, ibd_node.index)
        assert 'NETWORK' in ibd_node.getpeerinfo()[0]['servicesnames']
        self.sync_blocks(nodes=(ibd_node, snapshot_node))

    def assert_only_network_limited_service(self, node):
        node_services = node.getnetworkinfo()['localservicesnames']
        assert 'NETWORK' not in node_services
        assert 'NETWORK_LIMITED' in node_services

    @contextlib.contextmanager
    def assert_disk_cleanup(self, node, assumeutxo_used):
        """
        Ensure an assumeutxo node is cleaning up the background chainstate
        """
        msg = []
        if assumeutxo_used:
            # Check that the snapshot actually existed before restart
            assert (node.datadir_path / node.chain / "chainstate_snapshot").exists()
            msg = ["cleaning up unneeded background chainstate"]

        with node.assert_debug_log(msg):
            yield

        assert not (node.datadir_path / node.chain / "chainstate_snapshot").exists()

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
        dump_output = n0.dumptxoutset('utxos.dat', "latest")

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

        assert_equal(n0.getblockchaininfo()["blocks"], SNAPSHOT_BASE_HEIGHT)

        def check_dump_output(output):
            assert_equal(
                output['txoutset_hash'],
                "d2b051ff5e8eef46520350776f4100dd710a63447a8e01d917e92e79751a63e2")
            assert_equal(output["nchaintx"], blocks[SNAPSHOT_BASE_HEIGHT].chain_tx)

        check_dump_output(dump_output)

        # Mine more blocks on top of the snapshot that n1 hasn't yet seen. This
        # will allow us to test n1's sync-to-tip on top of a snapshot.
        self.generate(n0, nblocks=100, sync_fun=self.no_op)

        assert_equal(n0.getblockcount(), FINAL_HEIGHT)
        assert_equal(n1.getblockcount(), START_HEIGHT)

        assert_equal(n0.getblockchaininfo()["blocks"], FINAL_HEIGHT)

        self.log.info("Check that dumptxoutset works for past block heights")
        # rollback defaults to the snapshot base height
        dump_output2 = n0.dumptxoutset('utxos2.dat', "rollback")
        check_dump_output(dump_output2)
        assert_equal(sha256sum_file(dump_output['path']), sha256sum_file(dump_output2['path']))

        # Rollback with specific height
        dump_output3 = n0.dumptxoutset('utxos3.dat', rollback=SNAPSHOT_BASE_HEIGHT)
        check_dump_output(dump_output3)
        assert_equal(sha256sum_file(dump_output['path']), sha256sum_file(dump_output3['path']))

        # Specified height that is not a snapshot height
        prev_snap_height = SNAPSHOT_BASE_HEIGHT - 1
        dump_output4 = n0.dumptxoutset(path='utxos4.dat', rollback=prev_snap_height)
        assert_equal(
            dump_output4['txoutset_hash'],
            "45ac2777b6ca96588210e2a4f14b602b41ec37b8b9370673048cc0af434a1ec8")
        assert_not_equal(sha256sum_file(dump_output['path']), sha256sum_file(dump_output4['path']))

        # Use a hash instead of a height
        prev_snap_hash = n0.getblockhash(prev_snap_height)
        dump_output5 = n0.dumptxoutset('utxos5.dat', rollback=prev_snap_hash)
        assert_equal(sha256sum_file(dump_output4['path']), sha256sum_file(dump_output5['path']))

        # Ensure n0 is back at the tip
        assert_equal(n0.getblockchaininfo()["blocks"], FINAL_HEIGHT)

        self.test_snapshot_with_less_work(dump_output['path'])
        self.test_invalid_mempool_state(dump_output['path'])
        self.test_invalid_snapshot_scenarios(dump_output['path'])
        self.test_invalid_chainstate_scenarios()
        self.test_invalid_file_path()
        self.test_snapshot_block_invalidated(dump_output['path'])
        self.test_snapshot_not_on_most_work_chain(dump_output['path'])

        # Prune-node sanity check
        assert 'NETWORK' not in n1.getnetworkinfo()['localservicesnames']

        self.log.info(f"Loading snapshot into second node from {dump_output['path']}")
        # This node's tip is on an ancestor block of the snapshot, which should
        # be the normal case
        loaded = n1.loadtxoutset(dump_output['path'])
        assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        self.log.info("Confirm that local services remain unchanged")
        # Since n1 is a pruned node, the 'NETWORK' service flag must always be unset.
        self.assert_only_network_limited_service(n1)

        self.log.info("Check that UTXO-querying RPCs operate on snapshot chainstate")
        snapshot_hash = loaded['tip_hash']
        snapshot_num_coins = loaded['coins_loaded']
        # coinstatsindex might be not caught up yet and is not relevant for this test, so don't use it
        utxo_info = n1.gettxoutsetinfo(use_index=False)
        assert_equal(utxo_info['txouts'], snapshot_num_coins)
        assert_equal(utxo_info['height'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(utxo_info['bestblock'], snapshot_hash)

        # find coinbase output at snapshot height on node0 and scan for it on node1,
        # where the block is not available, but the snapshot was loaded successfully
        coinbase_tx = n0.getblock(snapshot_hash, verbosity=2)['tx'][0]
        assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", n1.getblock, snapshot_hash)
        coinbase_output_descriptor = coinbase_tx['vout'][0]['scriptPubKey']['desc']
        scan_result = n1.scantxoutset('start', [coinbase_output_descriptor])
        assert_equal(scan_result['success'], True)
        assert_equal(scan_result['txouts'], snapshot_num_coins)
        assert_equal(scan_result['height'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(scan_result['bestblock'], snapshot_hash)
        scan_utxos = [(coin['txid'], coin['vout']) for coin in scan_result['unspents']]
        assert (coinbase_tx['txid'], 0) in scan_utxos

        txout_result = n1.gettxout(coinbase_tx['txid'], 0)
        assert_equal(txout_result['scriptPubKey']['desc'], coinbase_output_descriptor)

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
        assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", n1.getblock, spend_coin_blockhash)
        prev_tx = n0.getblock(spend_coin_blockhash, 3)['tx'][0]
        prevout = {"txid": prev_tx['txid'], "vout": 0, "scriptPubKey": prev_tx['vout'][0]['scriptPubKey']['hex']}
        privkey = n0.get_deterministic_priv_key().key
        raw_tx = n1.createrawtransaction([prevout], {getnewdestination()[2]: 24.99})
        signed_tx = n1.signrawtransactionwithkey(raw_tx, [privkey], [prevout])['hex']
        signed_txid = tx_from_hex(signed_tx).txid_hex

        assert n1.gettxout(prev_tx['txid'], 0) is not None
        n1.sendrawtransaction(signed_tx)
        assert signed_txid in n1.getrawmempool()
        assert not n1.gettxout(prev_tx['txid'], 0)

        PAUSE_HEIGHT = FINAL_HEIGHT - 40

        self.log.info("Restarting node to stop at height %d", PAUSE_HEIGHT)
        self.restart_node(1, extra_args=[
            f"-stopatheight={PAUSE_HEIGHT}", *self.extra_args[1]])

        # Upon restart during snapshot tip sync, the node must remain in 'limited' mode.
        self.assert_only_network_limited_service(n1)

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

        # Upon restart, the node must remain in 'limited' mode
        self.assert_only_network_limited_service(n1)

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

        # Since n1 is a pruned node, it will not signal NODE_NETWORK after
        # completing the background sync.
        self.assert_only_network_limited_service(n1)

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
            with self.assert_disk_cleanup(n, i == 1):
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
        assert 'NETWORK' in n2.getnetworkinfo()['localservicesnames']  # sanity check

        self.log.info(f"Loading snapshot into third node from {dump_output['path']}")
        loaded = n2.loadtxoutset(dump_output['path'])
        assert_equal(loaded['coins_loaded'], SNAPSHOT_BASE_HEIGHT)
        assert_equal(loaded['base_height'], SNAPSHOT_BASE_HEIGHT)

        # Even though n2 is a full node, it will unset the 'NETWORK' service flag during snapshot loading.
        # This indicates other peers that the node will temporarily not provide historical blocks.
        self.log.info("Check node2 updated the local services during snapshot load")
        self.assert_only_network_limited_service(n2)

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

        # Upon restart, the node must stay in 'limited' mode until the background
        # chain sync completes.
        self.restart_node(2, extra_args=self.extra_args[2])
        self.assert_only_network_limited_service(n2)

        self.connect_nodes(0, 2)
        self.wait_until(lambda: n2.getchainstates()['chainstates'][-1]['blocks'] == FINAL_HEIGHT)
        self.sync_blocks(nodes=(n0, n2))

        self.log.info("Ensuring background validation completes")
        self.wait_until(lambda: len(n2.getchainstates()['chainstates']) == 1)

        # Once background chain sync completes, the full node must start offering historical blocks again.
        self.wait_until(lambda: {'NETWORK', 'NETWORK_LIMITED'}.issubset(n2.getnetworkinfo()['localservicesnames']))

        completed_idx_state = {
            'basic block filter index': COMPLETE_IDX,
            'coinstatsindex': COMPLETE_IDX,
            'txindex': COMPLETE_IDX,
        }
        self.wait_until(lambda: n2.getindexinfo() == completed_idx_state)

        for i in (0, 2):
            n = self.nodes[i]
            self.log.info(f"Restarting node {i} to ensure (Check|Load)BlockIndex passes")
            with self.assert_disk_cleanup(n, i == 2):
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

        # The following test cleans node2 and node3 chain directories.
        self.test_sync_from_assumeutxo_node(snapshot=dump_output)

@dataclass
class Block:
    hash: str
    tx: int
    chain_tx: int

if __name__ == '__main__':
    AssumeutxoTest(__file__).main()
