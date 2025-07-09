#!/usr/bin/env python3

# Copyright (c) 2022-2025 The Dash Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import copy
import struct
from decimal import Decimal
from io import BytesIO

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.authproxy import JSONRPCException
from test_framework.key import ECKey
from test_framework.messages import (
    CAssetLockTx,
    CAssetUnlockTx,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    tx_from_hex,
    hash256,
    ser_string,
)
from test_framework.script import (
    CScript,
    OP_RETURN,
)
from test_framework.script_util import (
    key_to_p2pk_script,
    key_to_p2pkh_script,
)
from test_framework.test_framework import DashTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    softfork_active,
)
from test_framework.wallet_util import bytes_to_wif

llmq_type_test = 106 # LLMQType::LLMQ_TEST_PLATFORM
tiny_amount = int(Decimal("0.0007") * COIN)
blocks_in_one_day = 100
HEIGHT_DIFF_EXPIRING = 48

class AssetLocksTest(DashTestFramework):
    def set_test_params(self):
        self.set_dash_test_params(2, 0, [[
                "-whitelist=127.0.0.1",
                "-llmqtestinstantsenddip0024=llmq_test_instantsend",
        ]] * 2, evo_count=2)
        self.mn_rr_height = 620

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def create_assetlock(self, coin, amount, pubkey):
        node_wallet = self.nodes[0]


        inputs = [CTxIn(COutPoint(int(coin["txid"], 16), coin["vout"]))]

        credit_outputs = []
        tmp_amount = amount
        if tmp_amount > COIN:
            tmp_amount -= COIN
            credit_outputs.append(CTxOut(COIN, key_to_p2pkh_script(pubkey)))
        credit_outputs.append(CTxOut(tmp_amount, key_to_p2pkh_script(pubkey)))

        lockTx_payload = CAssetLockTx(1, credit_outputs)

        remaining = int(COIN * coin['amount']) - tiny_amount - amount

        tx_output_ret = CTxOut(amount, CScript([OP_RETURN, b""]))
        tx_output = CTxOut(remaining, key_to_p2pk_script(pubkey))

        lock_tx = CTransaction()
        lock_tx.vin = inputs
        lock_tx.vout = [tx_output, tx_output_ret] if remaining > 0 else [tx_output_ret]
        lock_tx.nVersion = 3
        lock_tx.nType = 8 # asset lock type
        lock_tx.vExtraPayload = lockTx_payload.serialize()

        lock_tx = node_wallet.signrawtransactionwithwallet(lock_tx.serialize().hex())
        return tx_from_hex(lock_tx["hex"])


    def create_assetunlock(self, index, withdrawal, pubkey=None, fee=tiny_amount):
        node_wallet = self.nodes[0]
        mninfo = self.mninfo
        assert_greater_than(int(withdrawal), fee)
        tx_output = CTxOut(int(withdrawal) - fee, key_to_p2pk_script(pubkey))

        # request ID = sha256("plwdtx", index)
        request_id_buf = ser_string(b"plwdtx") + struct.pack("<Q", index)
        request_id = hash256(request_id_buf)[::-1].hex()

        height = node_wallet.getblockcount()
        self.log.info(f"Creating asset unlock: index={index} {request_id}")
        quorumHash = mninfo[0].get_node(self).quorum("selectquorum", llmq_type_test, request_id)["quorumHash"]
        self.log.info(f"Used quorum hash: {quorumHash}")
        unlockTx_payload = CAssetUnlockTx(
            version = 1,
            index = index,
            fee = fee,
            requestedHeight = height,
            quorumHash = int(quorumHash, 16),
            quorumSig = b'\00' * 96)

        unlock_tx = CTransaction()
        unlock_tx.vin = []
        unlock_tx.vout = [tx_output]
        unlock_tx.nVersion = 3
        unlock_tx.nType = 9 # asset unlock type
        unlock_tx.vExtraPayload = unlockTx_payload.serialize()

        unlock_tx.calc_sha256()
        msgHash = format(unlock_tx.sha256, '064x')

        recsig = self.get_recovered_sig(request_id, msgHash, llmq_type=llmq_type_test, use_platformsign=True)

        unlockTx_payload.quorumSig = bytearray.fromhex(recsig["sig"])
        unlock_tx.vExtraPayload = unlockTx_payload.serialize()
        return unlock_tx

    def get_credit_pool_balance(self, node = None, block_hash = None):
        if node is None:
            node = self.nodes[0]

        if block_hash is None:
            block_hash = node.getbestblockhash()
        block = node.getblock(block_hash)
        return int(COIN * block['cbTx']['creditPoolBalance'])

    def validate_credit_pool_balance(self, expected = None, block_hash = None):
        for node in self.nodes:
            locked = self.get_credit_pool_balance(node=node, block_hash=block_hash)
            if expected is None:
                expected = locked
            else:
                assert_equal(expected, locked)
        self.log.info(f"Credit pool amount matched with '{expected}'")
        return expected

    def check_mempool_size(self):
        self.sync_mempools()
        for node in self.nodes:
            assert_equal(node.getmempoolinfo()['size'], self.mempool_size)

    def check_mempool_result(self, result_expected, tx):
        """Wrapper to check result of testmempoolaccept on node_0's mempool"""
        result_expected['txid'] = tx.rehash()
        if result_expected['allowed']:
            result_expected['vsize'] = tx.get_vsize()

        result_test = self.nodes[0].testmempoolaccept([tx.serialize().hex()])

        assert_equal([result_expected], result_test)
        self.check_mempool_size()

    def create_and_check_block(self, txes, expected_error = None):
        node_wallet = self.nodes[0]
        best_block_hash = node_wallet.getbestblockhash()
        best_block = node_wallet.getblock(best_block_hash)
        tip = int(best_block_hash, 16)
        height = best_block["height"] + 1
        block_time = best_block["time"] + 1

        cbb = create_coinbase(height, dip4_activated=True, v20_activated=True)
        gbt = node_wallet.getblocktemplate()
        cbb.vExtraPayload = bytes.fromhex(gbt["coinbase_payload"])
        cbb.rehash()
        block = create_block(tip, cbb, block_time, version=4)
        # Add quorum commitments from block template
        for tx_obj in gbt["transactions"]:
            tx = tx_from_hex(tx_obj["data"])
            if tx.nType == 6:
                block.vtx.append(tx)
        for tx in txes:
            block.vtx.append(tx)
        block.hashMerkleRoot = block.calc_merkle_root()
        block.solve()
        result = node_wallet.submitblock(block.serialize().hex())
        if result != expected_error:
            raise AssertionError('mining the block should have failed with error %s, but submitblock returned %s' % (expected_error, result))

    def set_sporks(self):
        spork_enabled = 0
        spork_disabled = 4070908800

        self.nodes[0].sporkupdate("SPORK_17_QUORUM_DKG_ENABLED", spork_enabled)
        self.nodes[0].sporkupdate("SPORK_19_CHAINLOCKS_ENABLED", spork_disabled)
        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", spork_disabled)
        self.wait_for_sporks_same()

    def ensure_tx_is_not_mined(self, tx_id):
        try:
            for node in self.nodes:
                node.gettransaction(tx_id)
            raise AssertionError("Transaction should not be mined")
        except JSONRPCException as e:
            assert "Invalid or non-wallet transaction id" in e.error['message']

    def send_tx_simple(self, tx):
        return self.nodes[0].sendrawtransaction(hexstring=tx.serialize().hex(), maxfeerate=0)

    def send_tx(self, tx, expected_error = None, reason = None):
        try:
            self.log.info(f"Send tx with expected_error:'{expected_error}'...")
            tx_res = self.send_tx_simple(tx)
            if expected_error is None:
                self.sync_mempools()
                return tx_res

            # failure didn't happen, but expected:
            message = "Transaction should not be accepted"
            if reason is not None:
                message += ": " + reason

            raise AssertionError(message)
        except JSONRPCException as e:
            assert expected_error in e.error['message']

    def generate_batch(self, count):
        self.log.info(f"Generate {count} blocks")
        while count > 0:
            self.log.info(f"Generating batch of blocks {count} left")
            batch = min(50, count)
            count -= batch
            self.bump_mocktime(10 * 60 + 1)
            self.generate(self.nodes[1], batch)

    # This functional test intentionally setup only 2 MN and only 2 Evo nodes
    # to ensure that corner case of quorum with minimum amount of nodes as possible
    # does not cause any issues in Dash Core
    def mine_quorum_2_nodes(self):
        self.mine_quorum(llmq_type_name='llmq_test_platform', expected_members=2, expected_connections=1, expected_contributions=2, expected_commitments=2, llmq_type=106)

    def run_test(self):
        node_wallet = self.nodes[0]
        node = self.nodes[1]

        self.set_sporks()

        assert_equal(self.nodes[0].getblockchaininfo()['softforks']['v20']['active'], True)

        for _ in range(2):
            self.dynamically_add_masternode(evo=True)

        self.mempool_size = 0

        key = ECKey()
        key.generate()
        privkey = bytes_to_wif(key.get_bytes())
        node_wallet.importprivkey(privkey)
        pubkey = key.get_pubkey().get_bytes()

        self.test_asset_locks(node_wallet, node, pubkey)
        self.test_asset_unlocks(node_wallet, node, pubkey)
        self.test_withdrawal_limits(node_wallet, node, pubkey)
        self.test_mn_rr(node_wallet, node, pubkey)
        self.test_withdrawal_fork(node_wallet, node, pubkey)


    def test_asset_locks(self, node_wallet, node, pubkey):
        self.log.info("Testing asset lock...")
        locked_1 = 10 * COIN + 141421
        locked_2 = 10 * COIN + 314159

        coins = node_wallet.listunspent(query_options={'minimumAmount': Decimal(str(locked_2 / COIN))})
        coin = coins.pop()
        asset_lock_tx = self.create_assetlock(coin, locked_1, pubkey)


        self.check_mempool_result(tx=asset_lock_tx, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})
        self.validate_credit_pool_balance(0)
        txid_in_block = self.send_tx(asset_lock_tx)
        rpc_tx = node.getrawtransaction(txid_in_block, 1)
        assert_equal(len(rpc_tx["assetLockTx"]["creditOutputs"]), 2)
        assert_equal(rpc_tx["assetLockTx"]["creditOutputs"][0]["valueSat"] + rpc_tx["assetLockTx"]["creditOutputs"][1]["valueSat"], locked_1)
        assert_equal(rpc_tx["assetLockTx"]["creditOutputs"][0]["scriptPubKey"]["hex"], key_to_p2pkh_script(pubkey).hex())
        assert_equal(rpc_tx["assetLockTx"]["creditOutputs"][1]["scriptPubKey"]["hex"], key_to_p2pkh_script(pubkey).hex())
        self.validate_credit_pool_balance(0)
        self.generate(node, 1, sync_fun=self.no_op)
        assert_equal(self.get_credit_pool_balance(node=node), locked_1)
        self.log.info("Generate a number of blocks to ensure this is the longest chain for later in the test when we reconsiderblock")
        self.generate(node, 12)

        self.validate_credit_pool_balance(locked_1)

        # tx is mined, let's get blockhash
        self.log.info("Invalidate block with asset lock tx...")
        self.block_hash_1 = node_wallet.gettransaction(txid_in_block)['blockhash']
        for inode in self.nodes:
            inode.invalidateblock(self.block_hash_1)
            assert_equal(self.get_credit_pool_balance(node=inode), 0)
        self.generate(node, 3)
        self.validate_credit_pool_balance(0)
        self.log.info("Resubmit asset lock tx to new chain...")
        # NEW tx appears
        asset_lock_tx_2 = self.create_assetlock(coin, locked_2, pubkey)
        txid_in_block = self.send_tx(asset_lock_tx_2)
        self.generate(node, 1)
        self.validate_credit_pool_balance(locked_2)
        self.log.info("Reconsider old blocks...")
        for inode in self.nodes:
            inode.reconsiderblock(self.block_hash_1)
        self.validate_credit_pool_balance(locked_1)
        self.sync_all()

        self.log.info('Mine block with incorrect credit-pool value...')
        coin = coins.pop()
        extra_lock_tx = self.create_assetlock(coin, COIN, pubkey)
        self.create_and_check_block([extra_lock_tx], expected_error = 'bad-cbtx-assetlocked-amount')

        self.log.info("Mine IS quorum")
        if len(self.nodes[0].quorum('list')['llmq_test_instantsend']) == 0:
            self.mine_quorum(llmq_type_name='llmq_test_instantsend', expected_members=2, expected_connections=1, expected_contributions=2, expected_commitments=2, llmq_type=104)
        else:
            self.log.info("IS quorum exist")

        self.log.info("Mine a platform quorum")
        if len(self.nodes[0].quorum('list')['llmq_test_platform']) == 0:
            self.mine_quorum_2_nodes()
        else:
            self.log.info("Platform quorum exist")

        self.validate_credit_pool_balance(locked_1)


    def test_asset_unlocks(self, node_wallet, node, pubkey):
        self.log.info("Testing asset unlock...")

        self.log.info("Generating several txes by same quorum....")
        locked = self.get_credit_pool_balance()

        self.validate_credit_pool_balance(locked)
        asset_unlock_tx = self.create_assetunlock(101, COIN, pubkey)
        asset_unlock_tx_late = self.create_assetunlock(102, COIN, pubkey)
        asset_unlock_tx_too_late = self.create_assetunlock(103, COIN, pubkey)
        asset_unlock_tx_too_big_fee = self.create_assetunlock(104, COIN, pubkey, fee=int(Decimal("0.1") * COIN))
        asset_unlock_tx_zero_fee = self.create_assetunlock(105, COIN, pubkey, fee=0)
        asset_unlock_tx_duplicate_index = copy.deepcopy(asset_unlock_tx)
        # modify this tx with duplicated index to make a hash of tx different, otherwise tx would be refused too early
        asset_unlock_tx_duplicate_index.vout[0].nValue += COIN
        too_late_height = node.getblockcount() + HEIGHT_DIFF_EXPIRING

        self.log.info("Mine block to empty mempool")
        self.bump_mocktime(10 * 60 + 1)
        self.generate(self.nodes[0], 1)

        self.check_mempool_result(tx=asset_unlock_tx, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})
        self.check_mempool_result(tx=asset_unlock_tx_too_big_fee,
                result_expected={'allowed': False, 'reject-reason' : 'max-fee-exceeded'})
        self.check_mempool_result(tx=asset_unlock_tx_zero_fee,
                result_expected={'allowed': False, 'reject-reason' : 'bad-txns-assetunlock-fee-outofrange'})
        # not-verified is a correct faiure from mempool. Mempool knows nothing about CreditPool indexes and he just report that signature is not validated
        self.check_mempool_result(tx=asset_unlock_tx_duplicate_index,
                result_expected={'allowed': False, 'reject-reason' : 'bad-assetunlock-not-verified'})

        self.log.info("Validating that we calculate payload hash correctly: ask quorum forcely by message hash...")
        asset_unlock_tx_payload = CAssetUnlockTx()
        asset_unlock_tx_payload.deserialize(BytesIO(asset_unlock_tx.vExtraPayload))

        assert_equal(asset_unlock_tx_payload.quorumHash, int(self.mninfo[0].get_node(self).quorum("selectquorum", llmq_type_test, 'e6c7a809d79f78ea85b72d5df7e9bd592aecf151e679d6e976b74f053a7f9056')["quorumHash"], 16))

        self.log.info("Test no IS for asset unlock...")
        self.nodes[0].sporkupdate("SPORK_2_INSTANTSEND_ENABLED", 0)
        self.wait_for_sporks_same()

        txid = self.send_tx(asset_unlock_tx)
        assert_equal(node.getmempoolentry(txid)['fees']['base'], Decimal("0.0007"))
        is_id = node_wallet.sendtoaddress(node_wallet.getnewaddress(), 1)
        self.bump_mocktime(30)
        for node in self.nodes:
            self.wait_for_instantlock(is_id, node)


        tip = self.nodes[0].getblockcount()
        indexes_statuses_no_height = self.nodes[0].getassetunlockstatuses(["101", "102", "300"])
        assert_equal([{'index': 101, 'status': 'mempooled'}, {'index': 102, 'status': 'unknown'}, {'index': 300, 'status': 'unknown'}], indexes_statuses_no_height)
        indexes_statuses_height = self.nodes[0].getassetunlockstatuses(["101", "102", "300"], tip)
        assert_equal([{'index': 101, 'status': 'unknown'}, {'index': 102, 'status': 'unknown'}, {'index': 300, 'status': 'unknown'}], indexes_statuses_height)


        rawtx = node.getrawtransaction(txid, 1)
        rawtx_is = node.getrawtransaction(is_id, 1)
        assert_equal(rawtx["instantlock"], False)
        assert_equal(rawtx_is["instantlock"], True)
        assert_equal(rawtx["chainlock"], False)
        assert_equal(rawtx_is["chainlock"], False)
        assert not "confirmations" in rawtx
        assert not "confirmations" in rawtx_is
        self.log.info("Disable back IS")
        self.set_sporks()

        assert "assetUnlockTx" in node.getrawtransaction(txid, 1)

        self.mempool_size += 2
        self.check_mempool_size()
        self.validate_credit_pool_balance(locked)
        self.generate(node, 1)
        assert_equal(rawtx["instantlock"], False)
        assert_equal(rawtx["chainlock"], False)
        rawtx = node.getrawtransaction(txid, 1)
        assert_equal(rawtx["confirmations"], 1)
        self.validate_credit_pool_balance(locked - COIN)
        self.mempool_size -= 2
        self.check_mempool_size()
        block_asset_unlock = node.getrawtransaction(asset_unlock_tx.rehash(), 1)['blockhash']
        self.log.info("Checking rpc `getblock` and `getblockstats` succeeds as they use own fee calculation mechanism")
        assert_equal(node.getblockstats(node.getblockcount())['maxfee'], tiny_amount)
        node.getblock(block_asset_unlock, 2)

        self.send_tx(asset_unlock_tx,
            expected_error = "Transaction already in block chain",
            reason = "double copy")

        self.log.info("Mining next quorum to check tx 'asset_unlock_tx_late' is still valid...")
        self.mine_quorum_2_nodes()
        self.log.info("Checking credit pool amount is same...")
        self.validate_credit_pool_balance(locked - 1 * COIN)
        self.check_mempool_result(tx=asset_unlock_tx_late, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})
        self.log.info("Checking credit pool amount still is same...")
        self.validate_credit_pool_balance(locked - 1 * COIN)
        self.send_tx(asset_unlock_tx_late)
        self.generate(node, 1)
        self.validate_credit_pool_balance(locked - 2 * COIN)

        self.log.info("Generating many blocks to make quorum far behind (even still active)...")
        self.generate_batch(too_late_height - node.getblockcount() - 1)
        self.check_mempool_result(tx=asset_unlock_tx_too_late, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})
        self.generate(node, 1)
        self.check_mempool_result(tx=asset_unlock_tx_too_late,
                result_expected={'allowed': False, 'reject-reason' : 'bad-assetunlock-too-late'})

        self.log.info("Checking that two quorums later it is too late because quorum is not active...")
        self.mine_quorum_2_nodes()
        self.log.info("Expecting new reject-reason...")
        assert not softfork_active(self.nodes[0], 'withdrawals')
        self.check_mempool_result(tx=asset_unlock_tx_too_late,
                result_expected={'allowed': False, 'reject-reason' : 'bad-assetunlock-too-old-quorum'})

        block_to_reconsider = node.getbestblockhash()
        self.log.info("Test block invalidation with asset unlock tx...")
        for inode in self.nodes:
            inode.invalidateblock(block_asset_unlock)
        self.validate_credit_pool_balance(locked)
        self.generate_batch(50)
        self.validate_credit_pool_balance(locked)
        for inode in self.nodes:
            inode.reconsiderblock(block_to_reconsider)
        self.validate_credit_pool_balance(locked - 2 * COIN)

        self.log.info("Forcibly mining asset_unlock_tx_too_late and ensure block is invalid")
        assert not softfork_active(self.nodes[0], 'withdrawals')
        self.create_and_check_block([asset_unlock_tx_too_late], expected_error = "bad-assetunlock-too-old-quorum")

        self.generate(node, 1)

        self.validate_credit_pool_balance(locked - 2 * COIN)
        self.validate_credit_pool_balance(block_hash=self.block_hash_1, expected=locked)

        self.log.info("Forcibly mine asset_unlock_tx_duplicate_index and ensure block is invalid")
        self.create_and_check_block([asset_unlock_tx_duplicate_index], expected_error = "bad-assetunlock-duplicated-index")


    def test_withdrawal_limits(self, node_wallet, node, pubkey):
        self.log.info("Testing withdrawal limits before v22 'withdrawal fork'...")
        assert not softfork_active(node_wallet, 'withdrawals')

        self.log.info("Too big withdrawal is expected to not be mined")
        asset_unlock_tx_full = self.create_assetunlock(201, 1 + self.get_credit_pool_balance(), pubkey)

        self.log.info("Checking that transaction with exceeding amount accepted by mempool...")
        # Mempool doesn't know about the size of the credit pool
        self.check_mempool_result(tx=asset_unlock_tx_full, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})

        txid_in_block = self.send_tx(asset_unlock_tx_full)
        self.generate(node, 1)

        self.ensure_tx_is_not_mined(txid_in_block)

        self.log.info("Forcibly mine asset_unlock_tx_full and ensure block is invalid...")
        self.create_and_check_block([asset_unlock_tx_full], expected_error = "failed-creditpool-unlock-too-much")

        self.mempool_size += 1
        asset_unlock_tx_full = self.create_assetunlock(301, self.get_credit_pool_balance(), pubkey)
        self.check_mempool_result(tx=asset_unlock_tx_full, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})

        txid_in_block = self.send_tx(asset_unlock_tx_full)
        expected_balance = (Decimal(self.get_credit_pool_balance()) - Decimal(tiny_amount))
        self.generate(node, 1)
        self.log.info("Check txid_in_block was mined")
        block = node.getblock(node.getbestblockhash())
        assert txid_in_block in block['tx']
        self.validate_credit_pool_balance(0)

        self.log.info(f"Check status of withdrawal and try to spend it")
        withdrawal_status = node_wallet.gettransaction(txid_in_block)
        assert_equal(withdrawal_status['amount'] * COIN, expected_balance)
        assert_equal(withdrawal_status['details'][0]['category'], 'platform-transfer')

        spend_withdrawal_hex = node_wallet.createrawtransaction([{'txid': txid_in_block, 'vout' : 0}], { node_wallet.getnewaddress() : (expected_balance - Decimal(tiny_amount)) / COIN})
        spend_withdrawal_hex = node_wallet.signrawtransactionwithwallet(spend_withdrawal_hex)['hex']
        spend_withdrawal = tx_from_hex(spend_withdrawal_hex)
        self.check_mempool_result(tx=spend_withdrawal, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})
        spend_txid_in_block = self.send_tx(spend_withdrawal)

        self.generate(node, 1, sync_fun=self.no_op)
        block = node.getblock(node.getbestblockhash())
        assert spend_txid_in_block in block['tx']

        self.log.info("Fast forward to the next day to reset all current unlock limits...")
        self.generate_batch(blocks_in_one_day)
        self.mine_quorum_2_nodes()

        total = self.get_credit_pool_balance()
        coins = node_wallet.listunspent()
        while total <= 10_901 * COIN:
            if len(coins) == 0:
                coins = node_wallet.listunspent(query_options={'minimumAmount': 1})
            coin = coins.pop()
            to_lock = int(coin['amount'] * COIN) - tiny_amount
            if to_lock > 99 * COIN and total > 10_000 * COIN:
                to_lock = 99 * COIN

            total += to_lock
            tx = self.create_assetlock(coin, to_lock, pubkey)
            self.send_tx_simple(tx)
            self.log.info(f"Collecting coins in pool... Collected {total}/{10_901 * COIN}")
        self.sync_mempools()
        self.generate(node, 1)
        credit_pool_balance_1 = self.get_credit_pool_balance()
        assert_greater_than(credit_pool_balance_1, 10_901 * COIN)
        limit_amount_1 = 1000 * COIN
        self.log.info("Create 5 transactions and make sure that only 4 of them can be mined")
        self.log.info("because their sum is bigger than the hard-limit (1000)")
        # take most of limit by one big tx for faster testing and
        # create several tiny withdrawal with exactly 1 *invalid* / causes spend above limit tx
        withdrawals = [600 * COIN, 100 * COIN, 100 * COIN, 100 * COIN - 10000, 100 * COIN + 10001]
        amount_to_withdraw_1 = sum(withdrawals)
        index = 400
        for next_amount in withdrawals:
            index += 1
            asset_unlock_tx = self.create_assetunlock(index, next_amount, pubkey)
            last_txid = self.send_tx_simple(asset_unlock_tx)
            # make sure larger amounts are mined first simply to make this test deterministic
            node.prioritisetransaction(last_txid, next_amount // 10000)

        self.sync_mempools()
        self.generate(node, 1)

        new_total = self.get_credit_pool_balance()
        amount_actually_withdrawn = total - new_total
        self.log.info("Testing that we tried to withdraw more than we could")
        assert_greater_than(amount_to_withdraw_1, amount_actually_withdrawn)
        self.log.info("Checking that we tried to withdraw more than the hard-limit (1000)")
        assert_greater_than(amount_to_withdraw_1, limit_amount_1)
        self.log.info("Checking we didn't actually withdraw more than allowed by the limit")
        assert_greater_than_or_equal(limit_amount_1, amount_actually_withdrawn)
        assert_equal(amount_actually_withdrawn, 900 * COIN + 10001)

        self.generate(node, 1)
        self.log.info("Checking that exactly 1 tx stayed in mempool...")
        self.mempool_size = 1
        self.check_mempool_size()
        assert_equal(new_total, self.get_credit_pool_balance())
        pending_txid = node.getrawmempool()[0]

        amount_to_withdraw_2 = limit_amount_1 - amount_actually_withdrawn
        self.log.info(f"We can still consume {Decimal(str(amount_to_withdraw_2 / COIN))} before we hit the hard-limit (1000)")
        index += 1
        asset_unlock_tx = self.create_assetunlock(index, amount_to_withdraw_2, pubkey)
        self.send_tx_simple(asset_unlock_tx)
        self.sync_mempools()
        self.generate(node, 1)
        new_total = self.get_credit_pool_balance()
        amount_actually_withdrawn = total - new_total
        assert_equal(limit_amount_1, amount_actually_withdrawn)

        self.log.info("Checking that exactly the same tx as before stayed in mempool and it's the only one...")
        self.mempool_size = 1
        self.check_mempool_size()
        assert_equal(new_total, self.get_credit_pool_balance())
        assert pending_txid in node.getrawmempool()

        self.log.info("Fast forward to next day again...")
        self.generate_batch(blocks_in_one_day - 1)
        self.log.info("Checking mempool is empty now...")
        self.mempool_size = 0
        self.check_mempool_size()

        self.log.info("Creating new asset-unlock tx. It should be mined exactly 1 block after")
        credit_pool_balance_2 = self.get_credit_pool_balance()
        limit_amount_2 = credit_pool_balance_2 // 10
        index += 1
        asset_unlock_tx = self.create_assetunlock(index, limit_amount_2, pubkey)
        self.send_tx(asset_unlock_tx)
        self.generate(node, 1)
        assert_equal(new_total, self.get_credit_pool_balance())
        self.generate(node, 1)
        new_total -= limit_amount_2
        assert_equal(new_total, self.get_credit_pool_balance())
        self.log.info("Trying to withdraw more... expecting to fail")
        index += 1
        asset_unlock_tx = self.create_assetunlock(index, COIN, pubkey)
        self.send_tx(asset_unlock_tx)
        self.generate(node, 1)

        tip = self.nodes[0].getblockcount()
        indexes_statuses_no_height = self.nodes[0].getassetunlockstatuses(["101", "102", "103"])
        assert_equal([{'index': 101, 'status': 'mined'}, {'index': 102, 'status': 'mined'}, {'index': 103, 'status': 'unknown'}], indexes_statuses_no_height)
        indexes_statuses_height = self.nodes[0].getassetunlockstatuses(["101", "102", "103"], tip)
        assert_equal([{'index': 101, 'status': 'chainlocked'}, {'index': 102, 'status': 'chainlocked'}, {'index': 103, 'status': 'unknown'}], indexes_statuses_height)


        self.log.info("generate many blocks to be sure that mempool is empty after expiring txes...")
        self.generate_batch(HEIGHT_DIFF_EXPIRING)
        self.log.info("Checking that credit pool is not changed...")
        assert_equal(new_total, self.get_credit_pool_balance())
        self.check_mempool_size()
        assert not softfork_active(node_wallet, 'withdrawals')


    def test_mn_rr(self, node_wallet, node, pubkey):
        self.log.info("Activate mn_rr...")
        locked = self.get_credit_pool_balance()
        self.activate_mn_rr(expected_activation_height=620)
        self.log.info(f'mn-rr height: {node.getblockcount()} credit: {self.get_credit_pool_balance()}')
        assert_equal(locked, self.get_credit_pool_balance())

        bt = node.getblocktemplate()
        platform_reward = bt['masternode'][0]['amount']
        assert_equal(bt['masternode'][0]['script'], '6a')  # empty OP_RETURN
        owner_reward = bt['masternode'][1]['amount']
        operator_reward = bt['masternode'][2]['amount'] if len(bt['masternode']) == 3 else 0
        all_mn_rewards = platform_reward + owner_reward + operator_reward
        assert_equal(all_mn_rewards, bt['coinbasevalue'] * 3 // 4)  # 75/25 mn/miner reward split
        assert_equal(platform_reward, all_mn_rewards * 375 // 1000)  # 0.375 platform share
        assert_equal(platform_reward, 104549943)
        assert_equal(locked, self.get_credit_pool_balance())
        self.generate(node, 1)
        locked += platform_reward
        assert_equal(locked, self.get_credit_pool_balance())

        coins = node_wallet.listunspent(query_options={'minimumAmount': 1})
        coin = coins.pop()
        self.send_tx(self.create_assetlock(coin, COIN, pubkey))
        locked += platform_reward + COIN
        self.generate(node, 1)
        assert_equal(locked, self.get_credit_pool_balance())

    def test_withdrawal_fork(self, node_wallet, node, pubkey):
        self.log.info("Testing asset unlock after 'withdrawal' activation...")
        assert softfork_active(node_wallet, 'withdrawals')
        self.log.info(f'post-withdrawals height: {node.getblockcount()} credit: {self.get_credit_pool_balance()}')

        index = 501
        while index < 511:
            self.log.info(f"Generating new Asset Unlock tx, index={index}...")
            asset_unlock_tx = self.create_assetunlock(index, COIN, pubkey)
            asset_unlock_tx_payload = CAssetUnlockTx()
            asset_unlock_tx_payload.deserialize(BytesIO(asset_unlock_tx.vExtraPayload))

            self.log.info("Check that Asset Unlock tx is valid for current quorum")
            self.check_mempool_result(tx=asset_unlock_tx, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})

            quorumHash_str = format(asset_unlock_tx_payload.quorumHash, '064x')
            assert quorumHash_str in node_wallet.quorum('list')['llmq_test_platform']

            while quorumHash_str != node_wallet.quorum('list')['llmq_test_platform'][-1]:
                self.log.info("Generate one more quorum until signing quorum becomes the last one in the list")
                self.mine_quorum_2_nodes()
                self.check_mempool_result(tx=asset_unlock_tx, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})

            self.log.info("Generate one more quorum after which signing quorum is gone but Asset Unlock tx is still valid")
            assert quorumHash_str in node_wallet.quorum('list')['llmq_test_platform']
            self.mine_quorum_2_nodes()
            assert quorumHash_str not in node_wallet.quorum('list')['llmq_test_platform']

            if asset_unlock_tx_payload.requestedHeight + HEIGHT_DIFF_EXPIRING > node_wallet.getblockcount():
                self.check_mempool_result(tx=asset_unlock_tx, result_expected={'allowed': True, 'fees': {'base': Decimal(str(tiny_amount / COIN))}})
                break
            else:
                self.check_mempool_result(tx=asset_unlock_tx, result_expected={'allowed': False, 'reject-reason' : 'bad-assetunlock-too-late'})
                self.log.info("Asset Unlock tx expired, let's try again...")
                index += 1

        self.log.info("Generate one more quorum after which signing quorum becomes too old")
        self.mine_quorum_2_nodes()
        self.check_mempool_result(tx=asset_unlock_tx, result_expected={'allowed': False, 'reject-reason': 'bad-assetunlock-too-old-quorum'})

        asset_unlock_tx = self.create_assetunlock(520, 2000 * COIN + 1, pubkey)
        txid_in_block = self.send_tx(asset_unlock_tx)
        self.generate(node, 1)
        self.ensure_tx_is_not_mined(txid_in_block)

        asset_unlock_tx = self.create_assetunlock(521, 2000 * COIN, pubkey)
        txid_in_block = self.send_tx(asset_unlock_tx)
        self.generate(node, 1)
        block = node.getblock(node.getbestblockhash())
        assert txid_in_block in block['tx']

        asset_unlock_tx = self.create_assetunlock(522, COIN, pubkey)
        txid_in_block = self.send_tx(asset_unlock_tx)
        self.generate(node, 1)
        self.ensure_tx_is_not_mined(txid_in_block)

if __name__ == '__main__':
    AssetLocksTest().main()
