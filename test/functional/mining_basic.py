#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test mining RPCs

- getmininginfo
- getblocktemplate proposal mode
- submitblock"""

import copy
from decimal import Decimal

from test_framework.blocktools import (
    create_coinbase,
    get_witness_script,
    NORMAL_GBT_REQUEST_PARAMS,
    TIME_GENESIS_BLOCK,
    REGTEST_N_BITS,
    REGTEST_TARGET,
    nbits_str,
    target_str,
)
from test_framework.messages import (
    BLOCK_HEADER_SIZE,
    CBlock,
    CBlockHeader,
    COIN,
    DEFAULT_BLOCK_RESERVED_WEIGHT,
    MAX_BLOCK_WEIGHT,
    MAX_SEQUENCE_NONFINAL,
    MINIMUM_BLOCK_RESERVED_WEIGHT,
    ser_uint256,
    WITNESS_SCALE_FACTOR,
)
from test_framework.p2p import P2PDataStore
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
    assert_raises_rpc_error,
    get_fee,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)


DIFFICULTY_ADJUSTMENT_INTERVAL = 144
MAX_FUTURE_BLOCK_TIME = 2 * 3600
MAX_TIMEWARP = 600
VERSIONBITS_TOP_BITS = 0x20000000
VERSIONBITS_DEPLOYMENT_TESTDUMMY_BIT = 28
DEFAULT_BLOCK_MIN_TX_FEE = 1000  # default `-blockmintxfee` setting [sat/kvB]


def assert_template(node, block, expect, rehash=True):
    if rehash:
        block.hashMerkleRoot = block.calc_merkle_root()
    rsp = node.getblocktemplate(template_request={
        'data': block.serialize().hex(),
        'mode': 'proposal',
        'rules': ['segwit'],
    })
    assert_equal(rsp, expect)


class MiningTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [
            [],
            [],
            ["-fastprune", "-prune=1"]
        ]
        self.setup_clean_chain = True
        self.supports_cli = False

    def mine_chain(self):
        self.log.info('Create some old blocks')
        for t in range(TIME_GENESIS_BLOCK, TIME_GENESIS_BLOCK + 200 * 600, 600):
            self.nodes[0].setmocktime(t)
            self.generate(self.wallet, 1, sync_fun=self.no_op)
        mining_info = self.nodes[0].getmininginfo()
        assert_equal(mining_info['blocks'], 200)
        assert_equal(mining_info['currentblocktx'], 0)
        assert_equal(mining_info['currentblockweight'], DEFAULT_BLOCK_RESERVED_WEIGHT)

        self.log.info('test blockversion')
        self.restart_node(0, extra_args=[f'-mocktime={t}', '-blockversion=1337'])
        self.connect_nodes(0, 1)
        assert_equal(1337, self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)['version'])
        self.restart_node(0, extra_args=[f'-mocktime={t}'])
        self.connect_nodes(0, 1)
        assert_equal(VERSIONBITS_TOP_BITS + (1 << VERSIONBITS_DEPLOYMENT_TESTDUMMY_BIT), self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)['version'])
        self.restart_node(0)
        self.connect_nodes(0, 1)

    def test_fees_and_sigops(self):
        self.log.info("Test fees and sigops in getblocktemplate result")
        node = self.nodes[0]

        # Generate a coinbases with p2pk transactions for its sigops.
        wallet_sigops = MiniWallet(node, mode=MiniWalletMode.RAW_P2PK)
        self.generate(wallet_sigops, 1, sync_fun=self.no_op)

        # Mature with regular coinbases to prevent interference with other tests
        self.generate(self.wallet, 100, sync_fun=self.no_op)

        # Generate three transactions that must be mined in sequence
        #
        #      tx_a (1 sat/vbyte)
        #        |
        #        |
        #      tx_b (2 sat/vbyte)
        #        |
        #        |
        #      tx_c (3 sat/vbyte)
        #
        tx_a = wallet_sigops.send_self_transfer(from_node=node,
                                                fee_rate=Decimal("0.00001"))
        tx_b = wallet_sigops.send_self_transfer(from_node=node,
                                                fee_rate=Decimal("0.00002"),
                                                utxo_to_spend=tx_a["new_utxo"])
        tx_c = wallet_sigops.send_self_transfer(from_node=node,
                                                fee_rate=Decimal("0.00003"),
                                                utxo_to_spend=tx_b["new_utxo"])

        # Generate transaction without sigops. It will go first because it pays
        # higher fees (100 sat/vbyte) and descends from a different coinbase.
        tx_d = self.wallet.send_self_transfer(from_node=node,
                                              fee_rate=Decimal("0.00100"))

        block_template_txs = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)['transactions']

        block_template_fees = [tx['fee'] for tx in block_template_txs]
        assert_equal(block_template_fees, [
            tx_d["fee"] * COIN,
            tx_a["fee"] * COIN,
            tx_b["fee"] * COIN,
            tx_c["fee"] * COIN
        ])

        block_template_sigops = [tx['sigops'] for tx in block_template_txs]
        assert_equal(block_template_sigops, [0, 4, 4, 4])

        # Clear mempool
        self.generate(self.wallet, 1, sync_fun=self.no_op)

    def test_blockmintxfee_parameter(self):
        self.log.info("Test -blockmintxfee setting")
        self.restart_node(0, extra_args=['-minrelaytxfee=0', '-persistmempool=0'])
        node = self.nodes[0]

        # test default (no parameter), zero and a bunch of arbitrary blockmintxfee rates [sat/kvB]
        for blockmintxfee_sat_kvb in (DEFAULT_BLOCK_MIN_TX_FEE, 0, 50, 100, 500, 2500, 5000, 21000, 333333, 2500000):
            blockmintxfee_btc_kvb = blockmintxfee_sat_kvb / Decimal(COIN)
            if blockmintxfee_sat_kvb == DEFAULT_BLOCK_MIN_TX_FEE:
                self.log.info(f"-> Default -blockmintxfee setting ({blockmintxfee_sat_kvb} sat/kvB)...")
            else:
                blockmintxfee_parameter = f"-blockmintxfee={blockmintxfee_btc_kvb:.8f}"
                self.log.info(f"-> Test {blockmintxfee_parameter} ({blockmintxfee_sat_kvb} sat/kvB)...")
                self.restart_node(0, extra_args=[blockmintxfee_parameter, '-minrelaytxfee=0', '-persistmempool=0'])
                self.wallet.rescan_utxos()  # to avoid spending outputs of txs that are not in mempool anymore after restart

            # submit one tx with exactly the blockmintxfee rate, and one slightly below
            tx_with_min_feerate = self.wallet.send_self_transfer(from_node=node, fee_rate=blockmintxfee_btc_kvb)
            assert_equal(tx_with_min_feerate["fee"], get_fee(tx_with_min_feerate["tx"].get_vsize(), blockmintxfee_btc_kvb))
            if blockmintxfee_btc_kvb > 0:
                lowerfee_btc_kvb = blockmintxfee_btc_kvb - Decimal(10)/COIN  # 0.01 sat/vbyte lower
                tx_below_min_feerate = self.wallet.send_self_transfer(from_node=node, fee_rate=lowerfee_btc_kvb)
                assert_equal(tx_below_min_feerate["fee"], get_fee(tx_below_min_feerate["tx"].get_vsize(), lowerfee_btc_kvb))
            else:  # go below zero fee by using modified fees
                tx_below_min_feerate = self.wallet.send_self_transfer(from_node=node, fee_rate=blockmintxfee_btc_kvb)
                node.prioritisetransaction(tx_below_min_feerate["txid"], 0, -1)

            # check that tx below specified fee-rate is neither in template nor in the actual block
            block_template = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
            block_template_txids = [tx['txid'] for tx in block_template['transactions']]
            self.generate(self.wallet, 1, sync_fun=self.no_op)
            block = node.getblock(node.getbestblockhash(), verbosity=2)
            block_txids = [tx['txid'] for tx in block['tx']]

            assert tx_with_min_feerate['txid'] in block_template_txids
            assert tx_with_min_feerate['txid'] in block_txids
            assert tx_below_min_feerate['txid'] not in block_template_txids
            assert tx_below_min_feerate['txid'] not in block_txids

            # Restart node to clear mempool for the next test
            self.restart_node(0)

    def test_timewarp(self):
        self.log.info("Test timewarp attack mitigation (BIP94)")
        node = self.nodes[0]
        self.restart_node(0, extra_args=['-test=bip94'])

        self.log.info("Mine until the last block of the retarget period")
        blockchain_info = self.nodes[0].getblockchaininfo()
        n = DIFFICULTY_ADJUSTMENT_INTERVAL - blockchain_info['blocks'] % DIFFICULTY_ADJUSTMENT_INTERVAL - 2
        t = blockchain_info['time']

        for _ in range(n):
            t += 600
            self.nodes[0].setmocktime(t)
            self.generate(self.wallet, 1, sync_fun=self.no_op)

        self.log.info("Create block two hours in the future")
        self.nodes[0].setmocktime(t + MAX_FUTURE_BLOCK_TIME)
        self.generate(self.wallet, 1, sync_fun=self.no_op)
        assert_equal(node.getblock(node.getbestblockhash())['time'], t + MAX_FUTURE_BLOCK_TIME)

        self.log.info("First block template of retarget period can't use wall clock time")
        self.nodes[0].setmocktime(t)
        # The template will have an adjusted timestamp, which we then modify
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_greater_than_or_equal(tmpl['curtime'], t + MAX_FUTURE_BLOCK_TIME - MAX_TIMEWARP)
        # mintime and curtime should match
        assert_equal(tmpl['mintime'], tmpl['curtime'])

        block = CBlock()
        block.nVersion = tmpl["version"]
        block.hashPrevBlock = int(tmpl["previousblockhash"], 16)
        block.nTime = tmpl["curtime"]
        block.nBits = int(tmpl["bits"], 16)
        block.nNonce = 0
        block.vtx = [create_coinbase(height=int(tmpl["height"]))]
        block.solve()
        assert_template(node, block, None)

        bad_block = copy.deepcopy(block)
        bad_block.nTime = t
        bad_block.solve()
        assert_raises_rpc_error(-25, 'time-timewarp-attack', lambda: node.submitheader(hexdata=CBlockHeader(bad_block).serialize().hex()))

        self.log.info("Test timewarp protection boundary")
        bad_block.nTime = t + MAX_FUTURE_BLOCK_TIME - MAX_TIMEWARP - 1
        bad_block.solve()
        assert_raises_rpc_error(-25, 'time-timewarp-attack', lambda: node.submitheader(hexdata=CBlockHeader(bad_block).serialize().hex()))

        bad_block.nTime = t + MAX_FUTURE_BLOCK_TIME - MAX_TIMEWARP
        bad_block.solve()
        node.submitheader(hexdata=CBlockHeader(bad_block).serialize().hex())

    def test_pruning(self):
        self.log.info("Test that submitblock stores previously pruned block")
        prune_node = self.nodes[2]
        self.generate(prune_node, 400, sync_fun=self.no_op)
        pruned_block = prune_node.getblock(prune_node.getblockhash(2), verbosity=0)
        pruned_height = prune_node.pruneblockchain(400)
        assert_greater_than_or_equal(pruned_height, 2)
        pruned_blockhash = prune_node.getblockhash(2)

        assert_raises_rpc_error(-1, 'Block not available (pruned data)', prune_node.getblock, pruned_blockhash)

        result = prune_node.submitblock(pruned_block)
        assert_equal(result, "inconclusive")
        assert_equal(prune_node.getblock(pruned_blockhash, verbosity=0), pruned_block)


    def send_transactions(self, utxos, fee_rate, target_vsize):
        """
        Helper to create and send transactions with the specified target virtual size and fee rate.
        """
        for utxo in utxos:
            self.wallet.send_self_transfer(
                from_node=self.nodes[0],
                utxo_to_spend=utxo,
                target_vsize=target_vsize,
                fee_rate=fee_rate,
            )

    def verify_block_template(self, expected_tx_count, expected_weight):
        """
        Create a block template and check that it satisfies the expected transaction count and total weight.
        """
        response = self.nodes[0].getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        self.log.info(f"Testing block template: contains {expected_tx_count} transactions, and total weight <= {expected_weight}")
        assert_equal(len(response["transactions"]), expected_tx_count)
        total_weight = sum(transaction["weight"] for transaction in response["transactions"])
        assert_greater_than_or_equal(expected_weight, total_weight)

    def test_block_max_weight(self):
        self.log.info("Testing default and custom -blockmaxweight startup options.")

        LARGE_TXS_COUNT = 10
        LARGE_VSIZE = int(((MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT) / WITNESS_SCALE_FACTOR) / LARGE_TXS_COUNT)
        HIGH_FEERATE = Decimal("0.0003")

        # Ensure the mempool is empty
        assert_equal(len(self.nodes[0].getrawmempool()), 0)

        # Generate UTXOs and send 10 large transactions with a high fee rate
        utxos = [self.wallet.get_utxo(confirmed_only=True) for _ in range(LARGE_TXS_COUNT + 4)] # Add 4 more utxos that will be used in the test later
        self.send_transactions(utxos[:LARGE_TXS_COUNT], HIGH_FEERATE, LARGE_VSIZE)

        # Send 2 normal transactions with a lower fee rate
        NORMAL_VSIZE = int(2000 / WITNESS_SCALE_FACTOR)
        NORMAL_FEERATE = Decimal("0.0001")
        self.send_transactions(utxos[LARGE_TXS_COUNT:LARGE_TXS_COUNT + 2], NORMAL_FEERATE, NORMAL_VSIZE)

        # Check that the mempool contains all transactions
        self.log.info(f"Testing that the mempool contains {LARGE_TXS_COUNT + 2} transactions.")
        assert_equal(len(self.nodes[0].getrawmempool()), LARGE_TXS_COUNT + 2)

        # Verify the block template includes only the 10 high-fee transactions
        self.log.info("Testing that the block template includes only the 10 large transactions.")
        self.verify_block_template(
            expected_tx_count=LARGE_TXS_COUNT,
            expected_weight=MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT,
        )

        # Test block template creation with custom -blockmaxweight
        custom_block_weight = MAX_BLOCK_WEIGHT - 2000
        # Reducing the weight by 2000 units will prevent 1 large transaction from fitting into the block.
        self.restart_node(0, extra_args=[f"-blockmaxweight={custom_block_weight}"])

        self.log.info("Testing the block template with custom -blockmaxweight to include 9 large and 2 normal transactions.")
        self.verify_block_template(
            expected_tx_count=11,
            expected_weight=MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT - 2000,
        )

        # Ensure the block weight does not exceed the maximum
        self.log.info(f"Testing that the block weight will never exceed {MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT}.")
        self.restart_node(0, extra_args=[f"-blockmaxweight={MAX_BLOCK_WEIGHT}"])
        self.log.info("Sending 2 additional normal transactions to fill the mempool to the maximum block weight.")
        self.send_transactions(utxos[LARGE_TXS_COUNT + 2:], NORMAL_FEERATE, NORMAL_VSIZE)
        self.log.info(f"Testing that the mempool's weight matches the maximum block weight: {MAX_BLOCK_WEIGHT}.")
        assert_equal(self.nodes[0].getmempoolinfo()['bytes'] * WITNESS_SCALE_FACTOR, MAX_BLOCK_WEIGHT)

        self.log.info("Testing that the block template includes only 10 transactions and cannot reach full block weight.")
        self.verify_block_template(
            expected_tx_count=LARGE_TXS_COUNT,
            expected_weight=MAX_BLOCK_WEIGHT - DEFAULT_BLOCK_RESERVED_WEIGHT,
        )

        self.log.info("Test -blockreservedweight startup option.")
        # Lowering the -blockreservedweight by 4000 will allow for two more transactions.
        self.restart_node(0, extra_args=["-blockreservedweight=4000"])
        self.verify_block_template(
            expected_tx_count=12,
            expected_weight=MAX_BLOCK_WEIGHT - 4000,
        )

        self.log.info("Test that node will fail to start when user provide invalid -blockreservedweight")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f"-blockreservedweight={MAX_BLOCK_WEIGHT + 1}"],
            expected_msg=f"Error: Specified -blockreservedweight ({MAX_BLOCK_WEIGHT + 1}) exceeds consensus maximum block weight ({MAX_BLOCK_WEIGHT})",
        )

        self.log.info(f"Test that node will fail to start when user provide -blockreservedweight below {MINIMUM_BLOCK_RESERVED_WEIGHT}")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f"-blockreservedweight={MINIMUM_BLOCK_RESERVED_WEIGHT - 1}"],
            expected_msg=f"Error: Specified -blockreservedweight ({MINIMUM_BLOCK_RESERVED_WEIGHT - 1}) is lower than minimum safety value of ({MINIMUM_BLOCK_RESERVED_WEIGHT})",
        )

        self.log.info("Test that node will fail to start when user provide invalid -blockmaxweight")
        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=[f"-blockmaxweight={MAX_BLOCK_WEIGHT + 1}"],
            expected_msg=f"Error: Specified -blockmaxweight ({MAX_BLOCK_WEIGHT + 1}) exceeds consensus maximum block weight ({MAX_BLOCK_WEIGHT})",
        )

    def test_height_in_locktime(self):
        self.log.info("Sanity check generated blocks have their coinbase timelocked to their height.")
        self.generate(self.nodes[0], 1, sync_fun=self.no_op)
        block = self.nodes[0].getblock(self.nodes[0].getbestblockhash(), 2)
        assert_equal(block["tx"][0]["locktime"], block["height"] - 1)
        assert_equal(block["tx"][0]["vin"][0]["sequence"], MAX_SEQUENCE_NONFINAL)

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.mine_chain()

        def assert_submitblock(block, result_str_1, result_str_2=None):
            block.solve()
            result_str_2 = result_str_2 or 'duplicate-invalid'
            assert_equal(result_str_1, node.submitblock(hexdata=block.serialize().hex()))
            assert_equal(result_str_2, node.submitblock(hexdata=block.serialize().hex()))

        self.log.info('getmininginfo')
        mining_info = node.getmininginfo()
        assert_equal(mining_info['blocks'], 200)
        assert_equal(mining_info['chain'], self.chain)
        assert 'currentblocktx' not in mining_info
        assert 'currentblockweight' not in mining_info
        assert_equal(mining_info['bits'], nbits_str(REGTEST_N_BITS))
        assert_equal(mining_info['target'], target_str(REGTEST_TARGET))
        assert_equal(mining_info['difficulty'], Decimal('4.656542373906925E-10'))
        assert_equal(mining_info['next'], {
            'height': 201,
            'target': target_str(REGTEST_TARGET),
            'bits': nbits_str(REGTEST_N_BITS),
            'difficulty': Decimal('4.656542373906925E-10')
        })
        assert_equal(mining_info['networkhashps'], Decimal('0.003333333333333334'))
        assert_equal(mining_info['pooledtx'], 0)

        self.log.info("getblocktemplate: Test default witness commitment")
        txid = int(self.wallet.send_self_transfer(from_node=node)['wtxid'], 16)
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)

        # Check that default_witness_commitment is present.
        assert 'default_witness_commitment' in tmpl
        witness_commitment = tmpl['default_witness_commitment']

        # Check that default_witness_commitment is correct.
        witness_root = CBlock.get_merkle_root([ser_uint256(0),
                                               ser_uint256(txid)])
        script = get_witness_script(witness_root, 0)
        assert_equal(witness_commitment, script.hex())

        # Mine a block to leave initial block download and clear the mempool
        self.generatetoaddress(node, 1, node.get_deterministic_priv_key().address)
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        self.log.info("getblocktemplate: Test capability advertised")
        assert 'proposal' in tmpl['capabilities']
        assert 'coinbasetxn' not in tmpl

        next_height = int(tmpl["height"])
        coinbase_tx = create_coinbase(height=next_height)
        # sequence numbers must not be max for nLockTime to have effect
        coinbase_tx.vin[0].nSequence = 2**32 - 2
        coinbase_tx.rehash()

        block = CBlock()
        block.nVersion = tmpl["version"]
        block.hashPrevBlock = int(tmpl["previousblockhash"], 16)
        block.nTime = tmpl["curtime"]
        block.nBits = int(tmpl["bits"], 16)
        block.nNonce = 0
        block.vtx = [coinbase_tx]

        self.log.info("getblocktemplate: segwit rule must be set")
        assert_raises_rpc_error(-8, "getblocktemplate must be called with the segwit rule set", node.getblocktemplate, {})

        self.log.info("getblocktemplate: Test valid block")
        assert_template(node, block, None)

        self.log.info("submitblock: Test block decode failure")
        assert_raises_rpc_error(-22, "Block decode failed", node.submitblock, block.serialize()[:-15].hex())

        self.log.info("getblocktemplate: Test bad input hash for coinbase transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].vin[0].prevout.hash += 1
        bad_block.vtx[0].rehash()
        assert_template(node, bad_block, 'bad-cb-missing')

        self.log.info("submitblock: Test bad input hash for coinbase transaction")
        bad_block.solve()
        assert_equal("bad-cb-missing", node.submitblock(hexdata=bad_block.serialize().hex()))

        self.log.info("submitblock: Test block with no transactions")
        no_tx_block = copy.deepcopy(block)
        no_tx_block.vtx.clear()
        no_tx_block.hashMerkleRoot = 0
        no_tx_block.solve()
        assert_equal("bad-blk-length", node.submitblock(hexdata=no_tx_block.serialize().hex()))

        self.log.info("submitblock: Test empty block")
        assert_equal('high-hash', node.submitblock(hexdata=CBlock().serialize().hex()))

        self.log.info("getblocktemplate: Test truncated final transaction")
        assert_raises_rpc_error(-22, "Block decode failed", node.getblocktemplate, {
            'data': block.serialize()[:-1].hex(),
            'mode': 'proposal',
            'rules': ['segwit'],
        })

        self.log.info("getblocktemplate: Test duplicate transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx.append(bad_block.vtx[0])
        assert_template(node, bad_block, 'bad-txns-duplicate')
        assert_submitblock(bad_block, 'bad-txns-duplicate', 'bad-txns-duplicate')

        self.log.info("getblocktemplate: Test invalid transaction")
        bad_block = copy.deepcopy(block)
        bad_tx = copy.deepcopy(bad_block.vtx[0])
        bad_tx.vin[0].prevout.hash = 255
        bad_tx.rehash()
        bad_block.vtx.append(bad_tx)
        assert_template(node, bad_block, 'bad-txns-inputs-missingorspent')
        assert_submitblock(bad_block, 'bad-txns-inputs-missingorspent')

        self.log.info("getblocktemplate: Test nonfinal transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].nLockTime = 2**32 - 1
        bad_block.vtx[0].rehash()
        assert_template(node, bad_block, 'bad-txns-nonfinal')
        assert_submitblock(bad_block, 'bad-txns-nonfinal')

        self.log.info("getblocktemplate: Test bad tx count")
        # The tx count is immediately after the block header
        bad_block_sn = bytearray(block.serialize())
        assert_equal(bad_block_sn[BLOCK_HEADER_SIZE], 1)
        bad_block_sn[BLOCK_HEADER_SIZE] += 1
        assert_raises_rpc_error(-22, "Block decode failed", node.getblocktemplate, {
            'data': bad_block_sn.hex(),
            'mode': 'proposal',
            'rules': ['segwit'],
        })

        self.log.info("getblocktemplate: Test bad bits")
        bad_block = copy.deepcopy(block)
        bad_block.nBits = 469762303  # impossible in the real world
        assert_template(node, bad_block, 'bad-diffbits')

        self.log.info("getblocktemplate: Test bad merkle root")
        bad_block = copy.deepcopy(block)
        bad_block.hashMerkleRoot += 1
        assert_template(node, bad_block, 'bad-txnmrklroot', False)
        assert_submitblock(bad_block, 'bad-txnmrklroot', 'bad-txnmrklroot')

        self.log.info("getblocktemplate: Test bad timestamps")
        bad_block = copy.deepcopy(block)
        bad_block.nTime = 2**32 - 1
        assert_template(node, bad_block, 'time-too-new')
        assert_submitblock(bad_block, 'time-too-new', 'time-too-new')
        bad_block.nTime = 0
        assert_template(node, bad_block, 'time-too-old')
        assert_submitblock(bad_block, 'time-too-old', 'time-too-old')

        self.log.info("getblocktemplate: Test not best block")
        bad_block = copy.deepcopy(block)
        bad_block.hashPrevBlock = 123
        assert_template(node, bad_block, 'inconclusive-not-best-prevblk')
        assert_submitblock(bad_block, 'prev-blk-not-found', 'prev-blk-not-found')

        self.log.info('submitheader tests')
        assert_raises_rpc_error(-22, 'Block header decode failed', lambda: node.submitheader(hexdata='xx' * BLOCK_HEADER_SIZE))
        assert_raises_rpc_error(-22, 'Block header decode failed', lambda: node.submitheader(hexdata='ff' * (BLOCK_HEADER_SIZE-2)))
        assert_raises_rpc_error(-25, 'Must submit previous header', lambda: node.submitheader(hexdata=super(CBlock, bad_block).serialize().hex()))

        block.nTime += 1
        block.solve()

        def chain_tip(b_hash, *, status='headers-only', branchlen=1):
            return {'hash': b_hash, 'height': 202, 'branchlen': branchlen, 'status': status}

        assert chain_tip(block.hash) not in node.getchaintips()
        node.submitheader(hexdata=block.serialize().hex())
        assert chain_tip(block.hash) in node.getchaintips()
        node.submitheader(hexdata=CBlockHeader(block).serialize().hex())  # Noop
        assert chain_tip(block.hash) in node.getchaintips()

        bad_block_root = copy.deepcopy(block)
        bad_block_root.hashMerkleRoot += 2
        bad_block_root.solve()
        assert chain_tip(bad_block_root.hash) not in node.getchaintips()
        node.submitheader(hexdata=CBlockHeader(bad_block_root).serialize().hex())
        assert chain_tip(bad_block_root.hash) in node.getchaintips()
        # Should still reject invalid blocks, even if we have the header:
        assert_equal(node.submitblock(hexdata=bad_block_root.serialize().hex()), 'bad-txnmrklroot')
        assert_equal(node.submitblock(hexdata=bad_block_root.serialize().hex()), 'bad-txnmrklroot')
        assert chain_tip(bad_block_root.hash) in node.getchaintips()
        # We know the header for this invalid block, so should just return early without error:
        node.submitheader(hexdata=CBlockHeader(bad_block_root).serialize().hex())
        assert chain_tip(bad_block_root.hash) in node.getchaintips()

        bad_block_lock = copy.deepcopy(block)
        bad_block_lock.vtx[0].nLockTime = 2**32 - 1
        bad_block_lock.vtx[0].rehash()
        bad_block_lock.hashMerkleRoot = bad_block_lock.calc_merkle_root()
        bad_block_lock.solve()
        assert_equal(node.submitblock(hexdata=bad_block_lock.serialize().hex()), 'bad-txns-nonfinal')
        assert_equal(node.submitblock(hexdata=bad_block_lock.serialize().hex()), 'duplicate-invalid')
        # Build a "good" block on top of the submitted bad block
        bad_block2 = copy.deepcopy(block)
        bad_block2.hashPrevBlock = bad_block_lock.sha256
        bad_block2.solve()
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(hexdata=CBlockHeader(bad_block2).serialize().hex()))

        # Should reject invalid header right away
        bad_block_time = copy.deepcopy(block)
        bad_block_time.nTime = 1
        bad_block_time.solve()
        assert_raises_rpc_error(-25, 'time-too-old', lambda: node.submitheader(hexdata=CBlockHeader(bad_block_time).serialize().hex()))

        # Should ask for the block from a p2p node, if they announce the header as well:
        peer = node.add_p2p_connection(P2PDataStore())
        peer.wait_for_getheaders(timeout=5, block_hash=block.hashPrevBlock)
        peer.send_blocks_and_test(blocks=[block], node=node)
        # Must be active now:
        assert chain_tip(block.hash, status='active', branchlen=0) in node.getchaintips()

        # Building a few blocks should give the same results
        self.generatetoaddress(node, 10, node.get_deterministic_priv_key().address)
        assert_raises_rpc_error(-25, 'time-too-old', lambda: node.submitheader(hexdata=CBlockHeader(bad_block_time).serialize().hex()))
        assert_raises_rpc_error(-25, 'bad-prevblk', lambda: node.submitheader(hexdata=CBlockHeader(bad_block2).serialize().hex()))
        node.submitheader(hexdata=CBlockHeader(block).serialize().hex())
        node.submitheader(hexdata=CBlockHeader(bad_block_root).serialize().hex())
        assert_equal(node.submitblock(hexdata=block.serialize().hex()), 'duplicate')  # valid

        self.test_fees_and_sigops()
        self.test_blockmintxfee_parameter()
        self.test_block_max_weight()
        self.test_timewarp()
        self.test_pruning()
        self.test_height_in_locktime()


if __name__ == '__main__':
    MiningTest(__file__).main()
