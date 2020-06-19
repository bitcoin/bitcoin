#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPCs related to blockchainstate.

Test the following RPCs:
    - getblockchaininfo
    - gettxoutsetinfo
    - getdifficulty
    - getbestblockhash
    - getblockhash
    - getblockheader
    - getchaintxstats
    - getnetworkhashps
    - verifychain

Tests correspond to code in rpc/blockchain.cpp.
"""

from decimal import Decimal
import http.client
import subprocess

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises,
    assert_raises_rpc_error,
    assert_is_hex_string,
    assert_is_hash_string,
)
from test_framework.blocktools import (
    create_block,
    create_coinbase,
    TIME_GENESIS_BLOCK,
)
from test_framework.messages import (
    msg_block,
)
from test_framework.mininode import (
    P2PInterface,
)


class BlockchainTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.supports_cli = False

    def run_test(self):
        self.mine_chain()
        self.restart_node(0, extra_args=['-stopatheight=207', '-prune=1'])  # Set extra args with pruning after rescan is complete

        self._test_getblockchaininfo()
        self._test_getchaintxstats()
        self._test_gettxoutsetinfo()
        self._test_getblockheader()
        self._test_getdifficulty()
        self._test_getnetworkhashps()
        self._test_stopatheight()
        self._test_waitforblockheight()
        assert self.nodes[0].verifychain(4, 0)

    def mine_chain(self):
        self.log.info('Create some old blocks')
        address = self.nodes[0].get_deterministic_priv_key().address
        for t in range(TIME_GENESIS_BLOCK, TIME_GENESIS_BLOCK + 200 * 600, 600):
            # ten-minute steps from genesis block time
            self.nodes[0].setmocktime(t)
            self.nodes[0].generatetoaddress(1, address)
        assert_equal(self.nodes[0].getblockchaininfo()['blocks'], 200)

    def _test_getblockchaininfo(self):
        self.log.info("Test getblockchaininfo")

        keys = [
            'bestblockhash',
            'blocks',
            'chain',
            'chainwork',
            'difficulty',
            'headers',
            'initialblockdownload',
            'mediantime',
            'pruned',
            'size_on_disk',
            'softforks',
            'verificationprogress',
            'warnings',
        ]
        res = self.nodes[0].getblockchaininfo()

        # result should have these additional pruning keys if manual pruning is enabled
        assert_equal(sorted(res.keys()), sorted(['pruneheight', 'automatic_pruning'] + keys))

        # size_on_disk should be > 0
        assert_greater_than(res['size_on_disk'], 0)

        # pruneheight should be greater or equal to 0
        assert_greater_than_or_equal(res['pruneheight'], 0)

        # check other pruning fields given that prune=1
        assert res['pruned']
        assert not res['automatic_pruning']

        self.restart_node(0, ['-stopatheight=207'])
        res = self.nodes[0].getblockchaininfo()
        # should have exact keys
        assert_equal(sorted(res.keys()), keys)

        self.restart_node(0, ['-stopatheight=207', '-prune=550'])
        res = self.nodes[0].getblockchaininfo()
        # result should have these additional pruning keys if prune=550
        assert_equal(sorted(res.keys()), sorted(['pruneheight', 'automatic_pruning', 'prune_target_size'] + keys))

        # check related fields
        assert res['pruned']
        assert_equal(res['pruneheight'], 0)
        assert res['automatic_pruning']
        assert_equal(res['prune_target_size'], 576716800)
        assert_greater_than(res['size_on_disk'], 0)

        assert_equal(res['softforks'], {
            'bip34': {'type': 'buried', 'active': False, 'height': 500},
            'bip66': {'type': 'buried', 'active': False, 'height': 1251},
            'bip65': {'type': 'buried', 'active': False, 'height': 1351},
            'csv': {'type': 'buried', 'active': False, 'height': 432},
            'segwit': {'type': 'buried', 'active': True, 'height': 0},
            'testdummy': {
                'type': 'bip9',
                'bip9': {
                    'status': 'started',
                    'bit': 28,
                    'start_time': 0,
                    'timeout': 0x7fffffffffffffff,  # testdummy does not have a timeout so is set to the max int64 value
                    'since': 144,
                    'statistics': {
                        'period': 144,
                        'threshold': 108,
                        'elapsed': 57,
                        'count': 57,
                        'possible': True,
                    },
                },
                'active': False}
        })

    def _test_getchaintxstats(self):
        self.log.info("Test getchaintxstats")

        # Test `getchaintxstats` invalid extra parameters
        assert_raises_rpc_error(-1, 'getchaintxstats', self.nodes[0].getchaintxstats, 0, '', 0)

        # Test `getchaintxstats` invalid `nblocks`
        assert_raises_rpc_error(-1, "JSON value is not an integer as expected", self.nodes[0].getchaintxstats, '')
        assert_raises_rpc_error(-8, "Invalid block count: should be between 0 and the block's height - 1", self.nodes[0].getchaintxstats, -1)
        assert_raises_rpc_error(-8, "Invalid block count: should be between 0 and the block's height - 1", self.nodes[0].getchaintxstats, self.nodes[0].getblockcount())

        # Test `getchaintxstats` invalid `blockhash`
        assert_raises_rpc_error(-1, "JSON value is not a string as expected", self.nodes[0].getchaintxstats, blockhash=0)
        assert_raises_rpc_error(-8, "blockhash must be of length 64 (not 1, for '0')", self.nodes[0].getchaintxstats, blockhash='0')
        assert_raises_rpc_error(-8, "blockhash must be hexadecimal string (not 'ZZZ0000000000000000000000000000000000000000000000000000000000000')", self.nodes[0].getchaintxstats, blockhash='ZZZ0000000000000000000000000000000000000000000000000000000000000')
        assert_raises_rpc_error(-5, "Block not found", self.nodes[0].getchaintxstats, blockhash='0000000000000000000000000000000000000000000000000000000000000000')
        blockhash = self.nodes[0].getblockhash(200)
        self.nodes[0].invalidateblock(blockhash)
        assert_raises_rpc_error(-8, "Block is not in main chain", self.nodes[0].getchaintxstats, blockhash=blockhash)
        self.nodes[0].reconsiderblock(blockhash)

        chaintxstats = self.nodes[0].getchaintxstats(nblocks=1)
        # 200 txs plus genesis tx
        assert_equal(chaintxstats['txcount'], 201)
        # tx rate should be 1 per 10 minutes, or 1/600
        # we have to round because of binary math
        assert_equal(round(chaintxstats['txrate'] * 600, 10), Decimal(1))

        b1_hash = self.nodes[0].getblockhash(1)
        b1 = self.nodes[0].getblock(b1_hash)
        b200_hash = self.nodes[0].getblockhash(200)
        b200 = self.nodes[0].getblock(b200_hash)
        time_diff = b200['mediantime'] - b1['mediantime']

        chaintxstats = self.nodes[0].getchaintxstats()
        assert_equal(chaintxstats['time'], b200['time'])
        assert_equal(chaintxstats['txcount'], 201)
        assert_equal(chaintxstats['window_final_block_hash'], b200_hash)
        assert_equal(chaintxstats['window_final_block_height'], 200)
        assert_equal(chaintxstats['window_block_count'], 199)
        assert_equal(chaintxstats['window_tx_count'], 199)
        assert_equal(chaintxstats['window_interval'], time_diff)
        assert_equal(round(chaintxstats['txrate'] * time_diff, 10), Decimal(199))

        chaintxstats = self.nodes[0].getchaintxstats(blockhash=b1_hash)
        assert_equal(chaintxstats['time'], b1['time'])
        assert_equal(chaintxstats['txcount'], 2)
        assert_equal(chaintxstats['window_final_block_hash'], b1_hash)
        assert_equal(chaintxstats['window_final_block_height'], 1)
        assert_equal(chaintxstats['window_block_count'], 0)
        assert 'window_tx_count' not in chaintxstats
        assert 'window_interval' not in chaintxstats
        assert 'txrate' not in chaintxstats

    def _test_gettxoutsetinfo(self):
        node = self.nodes[0]
        res = node.gettxoutsetinfo()

        assert_equal(res['total_amount'], Decimal('8725.00000000'))
        assert_equal(res['transactions'], 200)
        assert_equal(res['height'], 200)
        assert_equal(res['txouts'], 200)
        assert_equal(res['bogosize'], 15000),
        assert_equal(res['bestblock'], node.getblockhash(200))
        size = res['disk_size']
        assert size > 6400
        assert size < 64000
        assert_equal(len(res['bestblock']), 64)
        assert_equal(len(res['hash_serialized_2']), 64)

        self.log.info("Test that gettxoutsetinfo() works for blockchain with just the genesis block")
        b1hash = node.getblockhash(1)
        node.invalidateblock(b1hash)

        res2 = node.gettxoutsetinfo()
        assert_equal(res2['transactions'], 0)
        assert_equal(res2['total_amount'], Decimal('0'))
        assert_equal(res2['height'], 0)
        assert_equal(res2['txouts'], 0)
        assert_equal(res2['bogosize'], 0),
        assert_equal(res2['bestblock'], node.getblockhash(0))
        assert_equal(len(res2['hash_serialized_2']), 64)

        self.log.info("Test that gettxoutsetinfo() returns the same result after invalidate/reconsider block")
        node.reconsiderblock(b1hash)

        res3 = node.gettxoutsetinfo()
        # The field 'disk_size' is non-deterministic and can thus not be
        # compared between res and res3.  Everything else should be the same.
        del res['disk_size'], res3['disk_size']
        assert_equal(res, res3)

        self.log.info("Test hash_type option for gettxoutsetinfo()")
        # Adding hash_type 'hash_serialized_2', which is the default, should
        # not change the result.
        res4 = node.gettxoutsetinfo(hash_type='hash_serialized_2')
        del res4['disk_size']
        assert_equal(res, res4)

        # hash_type none should not return a UTXO set hash.
        res5 = node.gettxoutsetinfo(hash_type='none')
        assert 'hash_serialized_2' not in res5

    def _test_getblockheader(self):
        node = self.nodes[0]

        assert_raises_rpc_error(-8, "hash must be of length 64 (not 8, for 'nonsense')", node.getblockheader, "nonsense")
        assert_raises_rpc_error(-8, "hash must be hexadecimal string (not 'ZZZ7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844')", node.getblockheader, "ZZZ7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844")
        assert_raises_rpc_error(-5, "Block not found", node.getblockheader, "0cf7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844")

        besthash = node.getbestblockhash()
        secondbesthash = node.getblockhash(199)
        header = node.getblockheader(blockhash=besthash)

        assert_equal(header['hash'], besthash)
        assert_equal(header['height'], 200)
        assert_equal(header['confirmations'], 1)
        assert_equal(header['previousblockhash'], secondbesthash)
        assert_is_hex_string(header['chainwork'])
        assert_equal(header['nTx'], 1)
        assert_is_hash_string(header['hash'])
        assert_is_hash_string(header['previousblockhash'])
        assert_is_hash_string(header['merkleroot'])
        assert_is_hash_string(header['bits'], length=None)
        assert isinstance(header['time'], int)
        assert isinstance(header['mediantime'], int)
        assert isinstance(header['nonce'], int)
        assert isinstance(header['version'], int)
        assert isinstance(int(header['versionHex'], 16), int)
        assert isinstance(header['difficulty'], Decimal)

    def _test_getdifficulty(self):
        difficulty = self.nodes[0].getdifficulty()
        # 1 hash in 2 should be valid, so difficulty should be 1/2**31
        # binary => decimal => binary math is why we do this check
        assert abs(difficulty * 2**31 - 1) < 0.0001

    def _test_getnetworkhashps(self):
        hashes_per_second = self.nodes[0].getnetworkhashps()
        # This should be 2 hashes every 10 minutes or 1/300
        assert abs(hashes_per_second * 300 - 1) < 0.0001

    def _test_stopatheight(self):
        assert_equal(self.nodes[0].getblockcount(), 200)
        self.nodes[0].generatetoaddress(6, self.nodes[0].get_deterministic_priv_key().address)
        assert_equal(self.nodes[0].getblockcount(), 206)
        self.log.debug('Node should not stop at this height')
        assert_raises(subprocess.TimeoutExpired, lambda: self.nodes[0].process.wait(timeout=3))
        try:
            self.nodes[0].generatetoaddress(1, self.nodes[0].get_deterministic_priv_key().address)
        except (ConnectionError, http.client.BadStatusLine):
            pass  # The node already shut down before response
        self.log.debug('Node should stop at this height...')
        self.nodes[0].wait_until_stopped()
        self.start_node(0)
        assert_equal(self.nodes[0].getblockcount(), 207)

    def _test_waitforblockheight(self):
        self.log.info("Test waitforblockheight")
        node = self.nodes[0]
        node.add_p2p_connection(P2PInterface())

        current_height = node.getblock(node.getbestblockhash())['height']

        # Create a fork somewhere below our current height, invalidate the tip
        # of that fork, and then ensure that waitforblockheight still
        # works as expected.
        #
        # (Previously this was broken based on setting
        # `rpc/blockchain.cpp:latestblock` incorrectly.)
        #
        b20hash = node.getblockhash(20)
        b20 = node.getblock(b20hash)

        def solve_and_send_block(prevhash, height, time):
            b = create_block(prevhash, create_coinbase(height), time)
            b.solve()
            node.p2p.send_and_ping(msg_block(b))
            return b

        b21f = solve_and_send_block(int(b20hash, 16), 21, b20['time'] + 1)
        b22f = solve_and_send_block(b21f.sha256, 22, b21f.nTime + 1)

        node.invalidateblock(b22f.hash)

        def assert_waitforheight(height, timeout=2):
            assert_equal(
                node.waitforblockheight(height=height, timeout=timeout)['height'],
                current_height)

        assert_waitforheight(0)
        assert_waitforheight(current_height - 1)
        assert_waitforheight(current_height)
        assert_waitforheight(current_height + 1)


if __name__ == '__main__':
    BlockchainTest().main()
