#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test getblockstats rpc call
#

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet
from test_framework.script import OP_RETURN, CScript
from test_framework.messages import CTxOut, COIN
from test_framework.address import address_to_scriptpubkey
import json
import os
from decimal import Decimal

TESTSDIR = os.path.dirname(os.path.realpath(__file__))

class GetblockstatsTest(BitcoinTestFramework):

    start_height = 101
    max_stat_pos = 2

    def add_options(self, parser):
        parser.add_argument('--gen-test-data', dest='gen_test_data',
                            default=False, action='store_true',
                            help='Generate test data')
        parser.add_argument('--test-data', dest='test_data',
                            default='data/rpc_getblockstats.json',
                            action='store', metavar='FILE',
                            help='Test data file')

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def get_stats(self):
        return [self.nodes[0].getblockstats(hash_or_height=self.start_height + i) for i in range(self.max_stat_pos+1)]

    def generate_test_data(self, filename):
        MOCKTIME = 1525107225
        FEE_SATOSHIS = 31200

        TRANSACTIONS = [
            (10, "Send 10 BTC with fee subtracted"),
            (10, "Send 10 BTC with fee subtracted"),
            (10, "Send 10 BTC without subtracting fee"),
            (1, "Send 1 BTC with higher fee rate"),
        ]

        self.nodes[0].setmocktime(MOCKTIME)
        wallet = MiniWallet(self.nodes[0])

        self.generate(wallet, COINBASE_MATURITY + 1)

        external_address = self.nodes[0].get_deterministic_priv_key().address
        external_script = address_to_scriptpubkey(external_address)

        for i, (amount_btc, description) in enumerate(TRANSACTIONS):
            amount_satoshis = int(amount_btc * COIN)
            wallet.send_to(
                from_node=self.nodes[0],
                scriptPubKey=external_script,
                amount=amount_satoshis,
                fee=FEE_SATOSHIS
            )
            if i == 0:
                self.generate(wallet, 1)

        self._create_op_return_transaction(wallet)

        self.sync_all()
        self.generate(wallet, 1)

        self.expected_stats = self.get_stats()
        blocks = self._get_all_blocks()

        to_dump = {
            'blocks': blocks,
            'mocktime': int(MOCKTIME),
            'stats': self.expected_stats,
        }
        with open(filename, 'w', encoding="utf8") as f:
            json.dump(to_dump, f, sort_keys=True, indent=2)

    def _create_op_return_transaction(self, wallet):
        """Create a transaction with OP_RETURN output to test statistics exclusion."""
        tx = wallet.create_self_transfer(fee_rate=Decimal("0.003"))
        op_return_data = b'\x21'
        tx["tx"].vout.append(CTxOut(nValue=0, scriptPubKey=CScript([OP_RETURN, op_return_data])))
        wallet.sendrawtransaction(from_node=self.nodes[0], tx_hex=tx["hex"])

    def _get_all_blocks(self):
        """Get all blocks from genesis to current tip."""
        blocks = []
        tip = self.nodes[0].getbestblockhash()
        blockhash = None
        height = 0
        while tip != blockhash:
            blockhash = self.nodes[0].getblockhash(height)
            blocks.append(self.nodes[0].getblock(blockhash, 0))
            height += 1
        return blocks

    def load_test_data(self, filename):
        with open(filename, 'r', encoding="utf8") as f:
            d = json.load(f)
            blocks = d['blocks']
            mocktime = d['mocktime']
            self.expected_stats = d['stats']

        # Set the timestamps from the file so that the nodes can get out of Initial Block Download
        self.nodes[0].setmocktime(mocktime)
        self.sync_all()

        for b in blocks:
            self.nodes[0].submitblock(b)


    def run_test(self):
        test_data = os.path.join(TESTSDIR, self.options.test_data)
        if self.options.gen_test_data:
            self.generate_test_data(test_data)
        else:
            self.load_test_data(test_data)

        self.sync_all()
        stats = self.get_stats()

        # Make sure all valid statistics are included but nothing else is
        expected_keys = self.expected_stats[0].keys()
        assert_equal(set(stats[0].keys()), set(expected_keys))

        assert_equal(stats[0]['height'], self.start_height)
        assert_equal(stats[self.max_stat_pos]['height'], self.start_height + self.max_stat_pos)

        for i in range(self.max_stat_pos+1):
            self.log.info('Checking block %d' % (i))
            assert_equal(stats[i], self.expected_stats[i])

            # Check selecting block by hash too
            blockhash = self.expected_stats[i]['blockhash']
            stats_by_hash = self.nodes[0].getblockstats(hash_or_height=self.convert_to_json_for_cli(blockhash))
            assert_equal(stats_by_hash, self.expected_stats[i])

        # Make sure each stat can be queried on its own
        for stat in expected_keys:
            for i in range(self.max_stat_pos+1):
                result = self.nodes[0].getblockstats(hash_or_height=self.start_height + i, stats=[stat])
                assert_equal(list(result.keys()), [stat])
                if result[stat] != self.expected_stats[i][stat]:
                    self.log.info('result[%s] (%d) failed, %r != %r' % (
                        stat, i, result[stat], self.expected_stats[i][stat]))
                assert_equal(result[stat], self.expected_stats[i][stat])

        # Make sure only the selected statistics are included (more than one)
        some_stats = {'minfee', 'maxfee'}
        stats = self.nodes[0].getblockstats(hash_or_height=1, stats=list(some_stats))
        assert_equal(set(stats.keys()), some_stats)

        # Test invalid parameters raise the proper json exceptions
        tip = self.start_height + self.max_stat_pos
        assert_raises_rpc_error(-8, 'Target block height %d after current tip %d' % (tip+1, tip),
                                self.nodes[0].getblockstats, hash_or_height=tip+1)
        assert_raises_rpc_error(-8, 'Target block height %d is negative' % (-1),
                                self.nodes[0].getblockstats, hash_or_height=-1)

        # Make sure not valid stats aren't allowed
        inv_sel_stat = 'asdfghjkl'
        inv_stats = [
            [inv_sel_stat],
            ['minfee', inv_sel_stat],
            [inv_sel_stat, 'minfee'],
            ['minfee', inv_sel_stat, 'maxfee'],
        ]
        for inv_stat in inv_stats:
            assert_raises_rpc_error(-8, f"Invalid selected statistic '{inv_sel_stat}'",
                                    self.nodes[0].getblockstats, hash_or_height=1, stats=inv_stat)

        # Make sure we aren't always returning inv_sel_stat as the culprit stat
        assert_raises_rpc_error(-8, f"Invalid selected statistic 'aaa{inv_sel_stat}'",
                                self.nodes[0].getblockstats, hash_or_height=1, stats=['minfee', f'aaa{inv_sel_stat}'])
        # Mainchain's genesis block shouldn't be found on regtest
        assert_raises_rpc_error(-5, 'Block not found', self.nodes[0].getblockstats,
                                hash_or_height=self.convert_to_json_for_cli('000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f'))

        # Invalid number of args
        assert_raises_rpc_error(-1, 'getblockstats hash_or_height ( stats )', self.nodes[0].getblockstats, self.convert_to_json_for_cli('00'), 1, 2)
        assert_raises_rpc_error(-1, 'getblockstats hash_or_height ( stats )', self.nodes[0].getblockstats)

        self.log.info('Test block height 0')
        genesis_stats = self.nodes[0].getblockstats(0)
        assert_equal(genesis_stats["blockhash"], "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")
        assert_equal(genesis_stats["utxo_increase"], 1)
        assert_equal(genesis_stats["utxo_size_inc"], 117)
        assert_equal(genesis_stats["utxo_increase_actual"], 0)
        assert_equal(genesis_stats["utxo_size_inc_actual"], 0)

        self.log.info('Test tip including OP_RETURN')
        tip_stats = self.nodes[0].getblockstats(tip)
        expected_tip_stats = self.expected_stats[self.max_stat_pos]
        assert_equal(tip_stats["utxo_increase"], expected_tip_stats["utxo_increase"])
        assert_equal(tip_stats["utxo_size_inc"], expected_tip_stats["utxo_size_inc"])
        assert_equal(tip_stats["utxo_increase_actual"], expected_tip_stats["utxo_increase_actual"])
        assert_equal(tip_stats["utxo_size_inc_actual"], expected_tip_stats["utxo_size_inc_actual"])

        self.log.info("Test when only header is known")
        block = self.generateblock(self.nodes[0], output="raw(55)", transactions=[], submit=False)
        self.nodes[0].submitheader(block["hex"])
        assert_raises_rpc_error(-1, "Block not available (not fully downloaded)", lambda: self.nodes[0].getblockstats(self.convert_to_json_for_cli(block['hash'])))

        self.log.info('Test when block is missing')
        (self.nodes[0].blocks_path / 'blk00000.dat').rename(self.nodes[0].blocks_path / 'blk00000.dat.backup')
        assert_raises_rpc_error(-1, 'Block not found on disk', self.nodes[0].getblockstats, hash_or_height=1)
        (self.nodes[0].blocks_path / 'blk00000.dat.backup').rename(self.nodes[0].blocks_path / 'blk00000.dat')


if __name__ == '__main__':
    GetblockstatsTest(__file__).main()
