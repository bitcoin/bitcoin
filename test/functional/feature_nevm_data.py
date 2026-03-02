#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import secrets
import random
import time
from threading import Thread
from io import BytesIO
from test_framework.authproxy import JSONRPCException
from test_framework.test_framework import DashTestFramework
from test_framework.auxpow_testing import mineAuxpowBlock
from test_framework.util import assert_equal, assert_raises_rpc_error, force_finish_mnsync, set_node_times
from test_framework.messages import (
    NEVM_DATA_EXPIRE_TIME,
    MAX_DATA_BLOBS,
    MAX_NEVM_DATA_BLOB,
    CNEVMBlock,
    CNEVMBlockConnect,
    hash256,
    msg_btccsig,
    uint256_from_str,
)

class NEVMDataTest(DashTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.set_dash_test_params(5, 4, [["-disablewallet=0","-walletrejectlongchains=0"]] * 5, fast_dip3_enforcement=True)
        # Activate NEVM commitment path in this test's height range.
        for i in range(self.num_nodes):
            self.extra_args[i].append("-nevmstartheight=1")

    def skip_test_if_missing_module(self):
        self.skip_if_no_py3_zmq()
        self.skip_if_no_syscoind_zmq()
        self.skip_if_no_wallet()
        self.skip_if_no_bdb()

    def nevm_data_max_size_blob(self):
        print('Testing for max size of a blob (2MB)')
        blobDataMax = secrets.token_hex(MAX_NEVM_DATA_BLOB)
        print('Creating large blob (2MB)...')
        vh = self.nodes[0].syscoincreatenevmblob(blobDataMax)['versionhash']
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes))
        print('Generating block...')
        cl = self.nodes[0].getbestblockhash()
        self.generate_helper(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl)
        print('Testing nodes to see if blob exists...')
        assert_equal(self.nodes[0].getnevmblobdata(vh, True)['data'], blobDataMax)
        assert_equal(self.nodes[1].getnevmblobdata(vh, True)['data'], blobDataMax)
        assert_equal(self.nodes[2].getnevmblobdata(vh, True)['data'], blobDataMax)
        assert_equal(self.nodes[3].getnevmblobdata(vh, True)['data'], blobDataMax)
        assert_equal(self.nodes[4].getnevmblobdata(vh, True)['data'], blobDataMax)
        vh = secrets.token_hex(32)
        print('Trying 2MB + 1 to ensure it cannot create blob...')
        blobDataMaxPlus = secrets.token_hex(MAX_NEVM_DATA_BLOB + 1)
        txBad = self.nodes[0].syscoincreatenevmblob(blobDataMaxPlus)['txid']
        assert_raises_rpc_error(-5, "No such mempool transaction", self.nodes[1].getrawtransaction, txid=txBad)
        print('Making 2MB * MAX_DATA_BLOBS+1...')
        self.blobVHs = []
        for i in range(0, 33):
            blobDataMax = secrets.token_hex(MAX_NEVM_DATA_BLOB)
            vh = self.nodes[0].syscoincreatenevmblob(blobDataMax)['versionhash']
            self.blobVHs.append(vh)
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes))
        print('Generating block...')
        tip = self.generate(self.nodes[0], 1)[-1]
        rpc_details = self.nodes[0].getblock(tip, True)
        print('Ensure fees will be properly calculated due to the block size being correctly calculated based on PoDA policy (100x factor of blob data)...')
        assert rpc_details["size"] > 670000 and rpc_details["size"]  < 680000
        foundCount = 0
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes))
        # get the tip block's MTP
        mtp = self.nodes[0].getblockheader(tip)["mediantime"]
        foundCount = 0
        print('Testing nodes to see if MAX_DATA_BLOBS blobs exist at 2MB each in the tip...')
        for i, blobVH in enumerate(self.blobVHs):
            try:
                blob = self.nodes[1].getnevmblobdata(blobVH)
                if blob['mtp'] == mtp:
                    foundCount += 1
            except Exception:
                pass

        assert_equal(foundCount, MAX_DATA_BLOBS)
        print('Generating next block...')
        tip = self.generate(self.nodes[0], 1)[-1]
        rpc_details = self.nodes[0].getblock(tip, True)
        mtp = self.nodes[0].getblockheader(tip)["mediantime"]
        print('Testing nodes to see if MAX_DATA_BLOBS+1 blobs exist after the next block...')
        for i, blobVH in enumerate(self.blobVHs):
            try:
                blob = self.nodes[1].getnevmblobdata(blobVH)
                if blob['mtp'] == mtp:
                    foundCount += 1
            except Exception:
                pass

        assert_equal(foundCount, MAX_DATA_BLOBS+1)
        self.generate(self.nodes[0], 3)
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes))

    def nevm_data_block_max_blobs(self):
        print('Testing for max number of blobs in a block (32)')
        self.blobVHs = []
        print('Populate 2 * MAX_DATA_BLOBS blobs in mempool (64)')
        for i in range(0, MAX_DATA_BLOBS*2):
            vh = self.nodes[0].syscoincreatenevmblob(secrets.token_hex(55))['versionhash']
            self.blobVHs.append(vh)
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes))
        print('Generating block...')
        cl = self.nodes[0].getbestblockhash()
        self.generate_helper(self.nodes[0], 1)
        mtp = self.nodes[0].getblockheader(cl)["mediantime"]
        foundCount = 0
        print('Testing nodes to see if only MAX_DATA_BLOBS blobs exist...')
        for i, blobVH in enumerate(self.blobVHs):
            try:
                blob = self.nodes[1].getnevmblobdata(blobVH)
                if blob['mtp'] == mtp:
                    foundCount += 1
            except Exception:
                pass

        assert_equal(foundCount, MAX_DATA_BLOBS)
        # mine the rest of the blobs
        print('Generating next block...')
        self.generate_helper(self.nodes[0], 1)
        tip = self.nodes[0].getbestblockhash()
        print('Testing nodes to see if MAX_DATA_BLOBS*2 blobs exist...')
        mtp = self.nodes[0].getblockheader(tip)["mediantime"]
        for i, blobVH in enumerate(self.blobVHs):
            try:
                blob = self.nodes[1].getnevmblobdata(blobVH)
                if blob['mtp'] == mtp:
                    foundCount += 1
            except Exception:
                pass

        assert_equal(foundCount, MAX_DATA_BLOBS*2)
        self.generate_helper(self.nodes[0], 3)
        self.wait_for_chainlocked_block_all_nodes(cl)

    def bump_until_mtp_exceeds(self, cl, expiry_timestamp):
        max_bumps = 20  # avoid infinite loops in case something goes wrong
        bumps = 0
        mtp = self.nodes[0].getblockheader(cl)["mediantime"]
        while True:
            bump_time = (expiry_timestamp - mtp) * 10
            if (bump_time > 150):
                bump_time = 150
            self.bump_mocktime(bump_time)
            print(f"Current MTP: {mtp}, Target expiry: {expiry_timestamp}, Mocktime: {self.mocktime}")
            for i in range(len(self.nodes)):
                force_finish_mnsync(self.nodes[i])

            cl = self.nodes[0].getbestblockhash()
            self.generate(self.nodes[0], 5)
            mtp = self.nodes[0].getblockheader(cl)['mediantime']
            self.wait_for_chainlocked_block_all_nodes(cl)
            if mtp > expiry_timestamp:
                print(f"Current MTP: {mtp}, Target expiry: {expiry_timestamp}, Mocktime: {self.mocktime}, MTP expiry achieved")
                break
            bumps += 1
            if bumps >= max_bumps:
                raise RuntimeError("Exceeded max mocktime bumps without reaching expiry MTP.")

    def basic_nevm_data(self):
        print('Testing relay in mempool and compact blocks around blobs')
        # test relay with block
        print('Stop node 4 which will be used later to resync blobs to test relay from scratch')
        self.stop_node(4)
        print('Creating a few blobs across nodes...')
        startblockhash = self.nodes[0].getbestblockhash()
        self.nodes[0].syscoincreatenevmblob(secrets.token_hex(55))
        txidData = secrets.token_hex(55)
        txid = self.nodes[1].syscoincreatenevmblob(txidData)['txid']
        txid1Data = secrets.token_hex(55)
        txid1 = self.nodes[0].syscoincreatenevmblob(txid1Data)['txid']
        vhData = secrets.token_hex(55)
        res = self.nodes[3].syscoincreatenevmblob(vhData)
        vh = res['versionhash']
        vhTxid = res['txid']
        self.nodes[3].syscoincreatenevmblob(secrets.token_hex(55))
        print('Checking for duplicate versionhash...')
        assert vhTxid != self.nodes[3].syscoincreaterawnevmblob(vh, vhData)['txid']
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes[0:4]))
        self.nodes[3].syscoincreatenevmblob(secrets.token_hex(55))['txid']
        print('Generating blocks without waiting for mempools to sync...')
        # Mine on ZMQ-enabled node to avoid NEVM-data validation divergence.
        self.generate_helper(self.nodes[0], 5, sync_fun=self.no_op, nodes=self.nodes[0:4])
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes[0:4]))
        self.generate_helper(self.nodes[0], 5, sync_fun=self.no_op, nodes=self.nodes[0:4])
        print('Check for consistency...')
        self.nodes[3].syscoincreatenevmblob(secrets.token_hex(55))
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes[0:4]))
        self.nodes[3].syscoincreatenevmblob(secrets.token_hex(55))
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes[0:4]))
        assert_equal(self.nodes[0].getnevmblobdata(txid, True)['data'], txidData)
        assert_equal(self.nodes[1].getnevmblobdata(vh, True)['data'], vhData)
        assert_equal(self.nodes[1].getnevmblobdata(txid, True)['data'], txidData)
        # test relay before block creation
        print('Create more blobs...')
        self.nodes[0].syscoincreatenevmblob(secrets.token_hex(55))
        self.nodes[0].syscoincreatenevmblob(secrets.token_hex(55))
        self.nodes[0].syscoincreatenevmblob(secrets.token_hex(55))
        self.nodes[0].syscoincreatenevmblob(secrets.token_hex(55))
        self.nodes[0].syscoincreatenevmblob(secrets.token_hex(55))
        data = secrets.token_hex(55)
        self.nodes[0].syscoincreatenevmblob(data)
        self.nodes[0].syscoincreatenevmblob(vhData, True)
        self.nodes[0].syscoincreatenevmblob(data, True)
        self.nodes[0].syscoincreatenevmblob(data, True)
        self.nodes[0].syscoincreatenevmblob(data, True)
        for i in range(0, 33):
            blobDataMax = secrets.token_hex(MAX_NEVM_DATA_BLOB)
            self.nodes[0].syscoincreatenevmblob(blobDataMax)
        print('Generating blocks after waiting for mempools to sync...')
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes[0:4]))
        self.generate_helper(self.nodes[0], 5, sync_fun=self.no_op, nodes=self.nodes[0:4])
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes[0:4]))
        print('Test reindex...')
        self.restart_node(1, extra_args=["-mocktime=" + str(self.mocktime), '-reindex', *self.extra_args[1]])
        force_finish_mnsync(self.nodes[1])
        for i in range(len(self.nodes[0:4])):
            if i != 1:
                self.connect_nodes(i, 1, wait_for_connect=False)
                self.connect_nodes(1, i, wait_for_connect=False)
        self.generate_helper(self.nodes[0], 5, sync_fun=self.no_op, nodes=self.nodes[0:4])
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes[0:4]))
        assert_equal(self.nodes[1].getnevmblobdata(txid, True)['data'], txidData)
        assert_equal(self.nodes[1].getnevmblobdata(vh, True)['data'], vhData)
        assert_equal(self.nodes[1].getnevmblobdata(txid1, True)['data'], txid1Data)
        mtp = self.nodes[1].getnevmblobdata(txid1)['mtp']
        print('Start node 4...')
        self.start_node(4, extra_args=["-mocktime=" + str(self.mocktime), *self.extra_args[4]])
        force_finish_mnsync(self.nodes[4])
        for i in range(len(self.nodes)):
            if i != 4:
                self.connect_nodes(i, 4, wait_for_connect=False)
                self.connect_nodes(4, i, wait_for_connect=False)
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes))
        assert_equal(self.nodes[4].getnevmblobdata(txid, True)['data'], txidData)
        assert_equal(self.nodes[4].getnevmblobdata(vh, True)['data'], vhData)
        assert_equal(self.nodes[4].getnevmblobdata(txid1, True)['data'], txid1Data)
        # After node 4 is back online and synced, wait for next chainlock across all nodes,
        # then ensure getnevmblobdata(vh) reports chainlock
        self.generate_helper(self.nodes[0], 5)
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes), timeout=180)
        self.wait_until(
            lambda: all(
                ('chainlock' in n.getnevmblobdata(vh)) and n.getnevmblobdata(vh)['chainlock']
                for n in self.nodes
            ),
            timeout=180,
        )
        print('Test blob expiry...')
        expiry_timestamp = (mtp + NEVM_DATA_EXPIRE_TIME)
        bump_to_expiry = expiry_timestamp - self.mocktime
        self.bump_mocktime(bump_to_expiry-1) # right before expiry
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl)
        assert_equal(self.nodes[3].getnevmblobdata(txid, True)['data'], txidData)
        assert_equal(self.nodes[2].getnevmblobdata(vh, True)['data'], vhData)
        assert_equal(self.nodes[3].getnevmblobdata(txid1, True)['data'], txid1Data)
        self.bump_mocktime(3) # push median time over expiry
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        cl = self.generate(self.nodes[0], 10)[-6]
        self.wait_for_chainlocked_block_all_nodes(cl)
        self.bump_until_mtp_exceeds(cl, expiry_timestamp)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid1)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[4].getnevmblobdata, txid)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[2].getnevmblobdata, txid1)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[1].getnevmblobdata, txid1)
        # vh got recreated so its MTP was updated to a later time
        mtpbest = self.nodes[2].getblockheader(self.nodes[2].getbestblockhash())['mediantime']
        assert_equal(self.nodes[2].getnevmblobdata(vh, True)['data'], vhData)
        assert_equal(self.nodes[3].getnevmblobdata(vh, True)['data'], vhData)
        nowblockhash = self.nodes[0].getbestblockhash()
        print('Checking for reorg with chainlocks')
        print('Invalidating back to the original blockhash {}'.format(startblockhash))
        self.nodes[0].invalidateblock(startblockhash)
        print('Reconsidering block')
        self.nodes[0].reconsiderblock(startblockhash)
        assert_equal(self.nodes[0].getbestblockhash(), nowblockhash)
        assert_equal(self.nodes[2].getnevmblobdata(vh, True)['data'], vhData)
        assert_equal(self.nodes[3].getnevmblobdata(vh, True)['data'], vhData)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid1)
        cl = self.nodes[0].getbestblockhash()
        self.generate_helper(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid)
        assert_equal(self.nodes[0].getnevmblobdata(vh, True)['data'], vhData)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid1)
        print('Checking for invalid versionhash...')
        txidBad = self.nodes[3].syscoincreaterawnevmblob(secrets.token_hex(55), secrets.token_hex(55))['txid']
        # should fail and not propagate due to 'bad-txns-poda-invalid'
        assert_raises_rpc_error(-5, "No such mempool transaction", self.nodes[0].getrawtransaction, txid=txidBad)
        print('Expire updated blob...')
        mtp = self.nodes[0].getnevmblobdata(vh)['mtp']
        expiry_timestamp = (mtp + NEVM_DATA_EXPIRE_TIME)
        self.bump_until_mtp_exceeds(cl, expiry_timestamp)
        for i in range(len(self.nodes)):
            force_finish_mnsync(self.nodes[i])
        cl = self.nodes[0].getbestblockhash()
        self.generate(self.nodes[0], 5)
        self.wait_for_chainlocked_block_all_nodes(cl)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, vh)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid)
        assert_raises_rpc_error(-32602, 'Could not find blob information for versionhash', self.nodes[0].getnevmblobdata, txid1)

    def _extract_btcc_from_coinbase(self, block_hash, node=None):
        """
        Parse btcc receipt from coinbase payload, if present.
        Returns msg_btccsig() or None.
        """
        if node is None:
            node = self.nodes[0]
        coinbase_hex = node.getblock(block_hash, 2)["tx"][0]["hex"]
        raw = bytes.fromhex(coinbase_hex)
        marker = b"btcc"
        pos = -1
        start = 0
        while True:
            nxt = raw.find(marker, start)
            if nxt == -1:
                break
            pos = nxt
            start = nxt + 1
        if pos == -1:
            return None
        payload = raw[pos + len(marker):]
        receipt = msg_btccsig()
        try:
            receipt.deserialize(BytesIO(payload))
            return receipt
        except Exception:
            return None

    def _extract_btcp_from_coinbase(self, block_hash, node=None):
        """
        Parse BTCPREV commitment from coinbase payload, if present.
        Returns uint256 int or None.
        """
        if node is None:
            node = self.nodes[0]
        coinbase_hex = node.getblock(block_hash, 2)["tx"][0]["hex"]
        raw = bytes.fromhex(coinbase_hex)
        marker = b"btcp"
        pos = -1
        start = 0
        while True:
            nxt = raw.find(marker, start)
            if nxt == -1:
                break
            pos = nxt
            start = nxt + 1
        if pos == -1:
            return None
        payload = raw[pos + len(marker):]
        if len(payload) < 32:
            return None
        return uint256_from_str(payload[:32])

    @staticmethod
    def _byte_swap_uint256(value):
        raw = value.to_bytes(32, byteorder="little")
        return int.from_bytes(raw[::-1], byteorder="little")

    def btcc_anchor_forwarding_after_restart(self):
        """
        Validate deterministic BTC anchor forwarding to NEVM:
        for a carrier block, nevmconnect.btcprevhash must equal the BTCPREV
        commitment in the attested (sign-offset) Syscoin block, including after
        restarting the forwarding node.
        """
        self.log.info("Testing deterministic BTCPREV anchor forwarding after restart")
        restart_idx = 0  # regular node under test
        producer = self.nodes[0]  # mine on ZMQ-enabled regular node for NEVM-consistent blocks
        consumer = self.nodes[restart_idx]

        saw_nevmconnect = False
        saw_null_receipt_zero_anchor = False
        saw_non_null_receipt = False
        saw_non_null_receipt_anchor = False

        # Restart regular node and ensure forwarding remains deterministic.
        restart_mocktime = max(
            self.mocktime,
            producer.getblockheader(producer.getbestblockhash())["time"],
        )
        restart_args = [a for a in self.extra_args[restart_idx] if not a.startswith("-mocktime=")]
        restart_args.append(f"-mocktime={restart_mocktime}")
        self.restart_node(restart_idx, restart_args)
        consumer = self.nodes[restart_idx]
        force_finish_mnsync(consumer)
        connect_count_before = self._zmq_connect_count
        for i in range(len(self.nodes)):
            if i == restart_idx:
                continue
            self.connect_nodes(restart_idx, i, wait_for_connect=False)
            # Keep reverse dial non-blocking to avoid restart-time deadlocks on
            # duplicate/already-established links.
            self.connect_nodes(i, restart_idx, wait_for_connect=False)
        self.wait_until(lambda: producer.getconnectioncount() >= (len(self.nodes) - 1), timeout=60)
        self.wait_until(lambda: self._all_peer_versions_known(producer, len(self.nodes) - 1), timeout=60)
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes), timeout=180)

        # Mine deterministically across full sign->carrier cycles so we must
        # encounter at least one non-null BTCC receipt path in normal operation.
        MAX_CYCLES = 8
        for cycle in range(MAX_CYCLES):
            current_height = producer.getblockcount()
            sign_height = current_height + ((2 - (current_height % 10)) % 10)
            if sign_height == current_height:
                sign_height += 10
            carrier_height = sign_height + 5

            self._mine_slowly_to_height(producer, sign_height)
            # Give BTCC scheduler/signing callbacks time to register local signed request IDs
            # at the sign height before expecting aggregate propagation. Keep steps small to
            # avoid expiring LLMQ signing sessions while they exchange shares.
            for _ in range(3):
                self._advance_mock_time(1)
                self.wait_until(lambda: self.sync_blocks_helper(self.nodes), timeout=30)
            # Deterministically require BTCC aggregation for this sign-height before
            # mining through its carrier window.
            self._wait_for_btcc_with_time_advance(
                producer,
                sign_height,
                timeout=90,
                step_seconds=1,
                max_total_advance=20,
            )

            self._mine_slowly_to_height(producer, carrier_height)

            carrier_hash = producer.getblockhash(carrier_height)
            carrier_hash_int = int(carrier_hash, 16)
            seen_events = self._zmq_connect_events[connect_count_before:]
            matched = [e for e in seen_events if e[0] == carrier_hash_int]
            if not matched:
                continue
            saw_nevmconnect = True
            candidate = self._extract_btcc_from_coinbase(carrier_hash, node=producer)
            # Null/absent BTCC receipt must fail-closed and forward zero anchor.
            if candidate is None or candidate.height == 0 or candidate.sysHash == 0:
                if all(e[1] == 0 for e in matched):
                    saw_null_receipt_zero_anchor = True
                continue

            # Non-null BTCC receipt must forward the attested sign-height anchor.
            saw_non_null_receipt = True
            attested_height = int(candidate.height)
            assert_equal(attested_height, sign_height)
            attested_hash = producer.getblockhash(attested_height)
            assert_equal(candidate.sysHash, int(attested_hash, 16))
            attested_btcp = self._extract_btcp_from_coinbase(attested_hash, node=producer)
            if attested_btcp is None:
                continue
            attested_btcp_swapped = self._byte_swap_uint256(attested_btcp)
            if any(e[1] in (attested_btcp, attested_btcp_swapped) for e in matched):
                saw_non_null_receipt_anchor = True
                break

        assert saw_nevmconnect
        # Require at least one non-null BTCC receipt path in this test window and
        # verify it forwards the attested BTCPREV anchor.
        assert saw_non_null_receipt
        assert saw_non_null_receipt_anchor

    @staticmethod
    def _has_btcc_at_or_above(node, height):
        try:
            return node.getbestbtccheckpoint()["height"] >= height
        except JSONRPCException:
            return False

    def _mine_slowly_to_height(self, producer, target_height, step_seconds=6):
        """Mine one block at a time while advancing mocktime."""
        self._init_mock_time_cursor(producer)
        while producer.getblockcount() < target_height:
            self._advance_mock_time(step_seconds)
            mineAuxpowBlock(producer, None)
            self.wait_until(lambda: self.sync_blocks_helper(self.nodes), timeout=180)

    def _wait_for_btcc_with_time_advance(
        self,
        node,
        height,
        timeout=90,
        step_seconds=1,
        max_total_advance=20,
    ):
        """
        Wait for BTCC while advancing mocktime.
        BTCCSIG inv/getdata scheduling is mocktime-driven for regular nodes.
        Without advancing time in this wait window, getdata may never fire.
        """
        self._init_mock_time_cursor(node)
        deadline = time.monotonic() + timeout
        advances = 0
        while time.monotonic() < deadline:
            if self._has_btcc_at_or_above(node, height):
                return

            # Let existing share/recovery traffic settle without changing mocktime.
            for _ in range(5):
                self.wait_until(lambda: self.sync_blocks_helper(self.nodes), timeout=30)
                if self._has_btcc_at_or_above(node, height):
                    return
                if time.monotonic() >= deadline:
                    break
                time.sleep(0.1)

            if advances >= max_total_advance:
                break
            self._advance_mock_time(step_seconds)
            advances += 1

        assert False, (
            f"BTCC aggregate not observed at/above height={height} "
            f"(mocktime_advances={advances}, step_seconds={step_seconds})"
        )

    def _wait_for_mempools_with_time_advance(self, timeout=120, step_seconds=1):
        self._init_mock_time_cursor(self.nodes[0])
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.sync_mempools_helper(self.nodes):
                return
            self._advance_mock_time(step_seconds)
        assert False, "Mempools did not synchronize before timeout"

    def _init_mock_time_cursor(self, node):
        if not hasattr(self, "_mock_time_cursor"):
            self._mock_time_cursor = node.getblockheader(node.getbestblockhash())["time"]

    def _advance_mock_time(self, step_seconds):
        self._mock_time_cursor += step_seconds
        set_node_times(self.nodes, self._mock_time_cursor)

    @staticmethod
    def _has_inbound_and_outbound(node):
        peers = node.getpeerinfo()
        inbound = sum(1 for p in peers if p.get("inbound", False))
        outbound = sum(1 for p in peers if not p.get("inbound", False))
        return inbound > 0 and outbound > 0

    @staticmethod
    def _peer_directions_summary(node):
        peers = node.getpeerinfo()
        inbound = sum(1 for p in peers if p.get("inbound", False))
        outbound = sum(1 for p in peers if not p.get("inbound", False))
        return {"total": len(peers), "inbound": inbound, "outbound": outbound}

    @staticmethod
    def _all_peer_versions_known(node, min_peers):
        peers = node.getpeerinfo()
        return len(peers) >= min_peers and all(p.get("version", 0) != 0 for p in peers)

    def _reconnect_full_mesh(self):
        for i in range(len(self.nodes)):
            for j in range(len(self.nodes)):
                if i == j:
                    continue
                self.connect_nodes(i, j, wait_for_connect=False)
        for node in self.nodes:
            self.wait_until(lambda n=node: self._all_peer_versions_known(n, len(self.nodes) - 1), timeout=60)
        self.wait_until(lambda: self.sync_blocks_helper(self.nodes), timeout=180)
        self.wait_until(lambda: self.sync_mempools_helper(self.nodes), timeout=180)

    def _start_nevm_zmq_responder(self):
        import zmq

        self._zmq_running = True
        self._zmq_connect_count = 0
        self._zmq_connect_events = []
        self._zmq_ctx = zmq.Context()
        self._zmq_sock = self._zmq_ctx.socket(zmq.REP)
        self._zmq_sock.bind("tcp://127.0.0.1:29555")
        self._zmq_sock.setsockopt(zmq.RCVTIMEO, 1000)

        def _loop():
            while self._zmq_running:
                try:
                    parts = self._zmq_sock.recv_multipart()
                except zmq.Again:
                    continue
                except Exception:
                    break
                if not parts:
                    continue
                topic = parts[0]
                if topic == b"nevmcomms":
                    self._zmq_sock.send_multipart([b"nevmcomms", b"ack"])
                elif topic == b"nevmblock":
                    h = hash256(str(random.randint(-0x80000000, 0x7FFFFFFF)).encode())
                    u = uint256_from_str(h)
                    nevm_block = CNEVMBlock()
                    nevm_block.nBlockHash = u
                    nevm_block.nTxRoot = u
                    nevm_block.nReceiptRoot = u
                    nevm_block.vchNEVMBlockData = b"nevmblock"
                    self._zmq_sock.send_multipart([b"nevmblock", nevm_block.serialize()])
                elif topic == b"nevmblockinfo":
                    self._zmq_sock.send_multipart([b"nevmblockinfo", str(self.nodes[0].getblockcount()).encode()])
                elif topic == b"nevmconnect":
                    self._zmq_connect_count += 1
                    try:
                        nevm_connect = CNEVMBlockConnect()
                        nevm_connect.deserialize(BytesIO(parts[1]))
                        self._zmq_connect_events.append((nevm_connect.sysblockhash, nevm_connect.btcprevhash))
                    except Exception:
                        pass
                    self._zmq_sock.send_multipart([b"nevmconnect", b"connected"])
                elif topic == b"nevmdisconnect":
                    self._zmq_sock.send_multipart([b"nevmdisconnect", b"disconnected"])
                else:
                    self._zmq_sock.send_multipart([topic, b"ack"])

        self._zmq_thread = Thread(target=_loop)
        self._zmq_thread.start()

    def _stop_nevm_zmq_responder(self):
        self._zmq_running = False
        if hasattr(self, "_zmq_thread"):
            self._zmq_thread.join(timeout=5)
        if hasattr(self, "_zmq_sock"):
            self._zmq_sock.close(linger=0)
        if hasattr(self, "_zmq_ctx"):
            self._zmq_ctx.destroy(linger=0)

    def run_test(self):
        self._start_nevm_zmq_responder()
        try:
            # Enable ZMQ NEVM publisher only after responder is up to avoid
            # startup-time RPC stalls in framework setup_network mining.
            self.extra_args[0].append("-zmqpubnevm=tcp://127.0.0.1:29555")
            self.extra_args[0].append("-debug=zmq")
            self.restart_node(0, self.extra_args[0])
            self.nodes[1].createwallet("")
            self.nodes[2].createwallet("")
            self.nodes[3].createwallet("")
            for i in range(len(self.nodes)):
                force_finish_mnsync(self.nodes[i])
            for i in range(0, len(self.nodes)):
                for j in range(i, len(self.nodes)):
                    self.connect_nodes(i, j, wait_for_connect=False)
            self.generate_helper(self.nodes[0], 10)
            self.sync_blocks(self.nodes, timeout=60)
            self.nodes[0].spork("SPORK_17_QUORUM_DKG_ENABLED", 0)
            self.nodes[0].spork("SPORK_19_CHAINLOCKS_ENABLED", 0)
            self.wait_for_sporks_same()

            self.log.info("Mining 4 quorums")
            for i in range(4):
                self.mine_quorum()

            self.wait_for_sporks_same()
            self.log.info("Mine single block, wait for chainlock")
            cl = self.nodes[0].getbestblockhash()
            self.generate_helper(self.nodes[0], 5)
            self.wait_for_chainlocked_block_all_nodes(cl)
            # Keep mining on ZMQ-enabled node so NEVM data output is produced deterministically.
            self.generate_helper(self.nodes[0], 5)
            self.wait_until(lambda: self.sync_blocks_helper(self.nodes))
            self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
            self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1)
            self.nodes[0].sendtoaddress(self.nodes[3].getnewaddress(), 1)
            self.generate_helper(self.nodes[0], 5)
            self.wait_until(lambda: self.sync_blocks_helper(self.nodes))
            self.nevm_data_max_size_blob()
            self.nevm_data_block_max_blobs()
            self.basic_nevm_data()
            self.btcc_anchor_forwarding_after_restart()
        finally:
            self._stop_nevm_zmq_responder()


if __name__ == '__main__':
    NEVMDataTest().main()
