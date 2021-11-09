#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test RPCs related to blockchainstate.

Test the following RPCs:
    - getblockchaininfo
    - getchaintxstats
    - gettxoutsetinfo
    - getblockheader
    - getdifficulty
    - getnetworkhashps
    - waitforblockheight
    - getblock
    - getblockhash
    - getbestblockhash
    - verifychain

Tests correspond to code in rpc/blockchain.cpp.
"""

from decimal import Decimal
import http.client
import os
import subprocess

from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.blocktools import (
    create_block,
    create_coinbase,
    TIME_GENESIS_BLOCK,
)
from test_framework.messages import (
    CBlockHeader,
    from_hex,
    msg_block,
)
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_greater_than_or_equal,
    assert_raises,
    assert_raises_rpc_error,
    assert_is_hex_string,
    assert_is_hash_string,
    get_datadir_path,
)
from test_framework.wallet import MiniWallet


HEIGHT = 200  # blocks mined
TIME_RANGE_STEP = 600  # ten-minute steps
TIME_RANGE_MTP = TIME_GENESIS_BLOCK + (HEIGHT - 6) * TIME_RANGE_STEP
TIME_RANGE_END = TIME_GENESIS_BLOCK + HEIGHT * TIME_RANGE_STEP


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
        self._test_getblock()
        assert self.nodes[0].verifychain(4, 0)

    def mine_chain(self):
        self.log.info(f"Generate {HEIGHT} blocks after the genesis block in ten-minute steps")
        for t in range(TIME_GENESIS_BLOCK, TIME_RANGE_END, TIME_RANGE_STEP):
            self.nodes[0].setmocktime(t)
            self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_P2WSH_OP_TRUE)
        assert_equal(self.nodes[0].getblockchaininfo()['blocks'], HEIGHT)

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
            'time',
            'verificationprogress',
            'warnings',
        ]
        res = self.nodes[0].getblockchaininfo()

        assert_equal(res['time'], TIME_RANGE_END - TIME_RANGE_STEP)
        assert_equal(res['mediantime'], TIME_RANGE_MTP)

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

        self.stop_node(0)
        self.nodes[0].assert_start_raises_init_error(
            extra_args=['-testactivationheight=name@2'],
            expected_msg='Error: Invalid name (name@2) for -testactivationheight=name@height.',
        )
        self.nodes[0].assert_start_raises_init_error(
            extra_args=['-testactivationheight=bip34@-2'],
            expected_msg='Error: Invalid height value (bip34@-2) for -testactivationheight=name@height.',
        )
        self.nodes[0].assert_start_raises_init_error(
            extra_args=['-testactivationheight='],
            expected_msg='Error: Invalid format () for -testactivationheight=name@height.',
        )
        self.start_node(0, extra_args=[
            '-stopatheight=207',
            '-prune=550',
            '-testactivationheight=bip34@2',
            '-testactivationheight=dersig@3',
            '-testactivationheight=cltv@4',
            '-testactivationheight=csv@5',
            '-testactivationheight=segwit@6',
        ])

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
            'bip34': {'type': 'buried', 'active': True, 'height': 2},
            'bip66': {'type': 'buried', 'active': True, 'height': 3},
            'bip65': {'type': 'buried', 'active': True, 'height': 4},
            'csv': {'type': 'buried', 'active': True, 'height': 5},
            'segwit': {'type': 'buried', 'active': True, 'height': 6},
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
                        'elapsed': HEIGHT - 143,
                        'count': HEIGHT - 143,
                        'possible': True,
                    },
                    'min_activation_height': 0,
                },
                'active': False
            },
            'taproot': {
                'type': 'bip9',
                'bip9': {
                    'status': 'active',
                    'start_time': -1,
                    'timeout': 9223372036854775807,
                    'since': 0,
                    'min_activation_height': 0,
                },
                'height': 0,
                'active': True
            }
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
        blockhash = self.nodes[0].getblockhash(HEIGHT)
        self.nodes[0].invalidateblock(blockhash)
        assert_raises_rpc_error(-8, "Block is not in main chain", self.nodes[0].getchaintxstats, blockhash=blockhash)
        self.nodes[0].reconsiderblock(blockhash)

        chaintxstats = self.nodes[0].getchaintxstats(nblocks=1)
        # 200 txs plus genesis tx
        assert_equal(chaintxstats['txcount'], HEIGHT + 1)
        # tx rate should be 1 per 10 minutes, or 1/600
        # we have to round because of binary math
        assert_equal(round(chaintxstats['txrate'] * TIME_RANGE_STEP, 10), Decimal(1))

        b1_hash = self.nodes[0].getblockhash(1)
        b1 = self.nodes[0].getblock(b1_hash)
        b200_hash = self.nodes[0].getblockhash(HEIGHT)
        b200 = self.nodes[0].getblock(b200_hash)
        time_diff = b200['mediantime'] - b1['mediantime']

        chaintxstats = self.nodes[0].getchaintxstats()
        assert_equal(chaintxstats['time'], b200['time'])
        assert_equal(chaintxstats['txcount'], HEIGHT + 1)
        assert_equal(chaintxstats['window_final_block_hash'], b200_hash)
        assert_equal(chaintxstats['window_final_block_height'], HEIGHT )
        assert_equal(chaintxstats['window_block_count'], HEIGHT - 1)
        assert_equal(chaintxstats['window_tx_count'], HEIGHT - 1)
        assert_equal(chaintxstats['window_interval'], time_diff)
        assert_equal(round(chaintxstats['txrate'] * time_diff, 10), Decimal(HEIGHT - 1))

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
        assert_equal(res['transactions'], HEIGHT)
        assert_equal(res['height'], HEIGHT)
        assert_equal(res['txouts'], HEIGHT)
        assert_equal(res['bogosize'], 16800),
        assert_equal(res['bestblock'], node.getblockhash(HEIGHT))
        size = res['disk_size']
        assert size > 6400
        assert size < 64000
        assert_equal(len(res['bestblock']), 64)
        assert_equal(len(res['hash_serialized_2']), 64)

        self.log.info("Test gettxoutsetinfo works for blockchain with just the genesis block")
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

        self.log.info("Test gettxoutsetinfo returns the same result after invalidate/reconsider block")
        node.reconsiderblock(b1hash)

        res3 = node.gettxoutsetinfo()
        # The field 'disk_size' is non-deterministic and can thus not be
        # compared between res and res3.  Everything else should be the same.
        del res['disk_size'], res3['disk_size']
        assert_equal(res, res3)

        self.log.info("Test gettxoutsetinfo hash_type option")
        # Adding hash_type 'hash_serialized_2', which is the default, should
        # not change the result.
        res4 = node.gettxoutsetinfo(hash_type='hash_serialized_2')
        del res4['disk_size']
        assert_equal(res, res4)

        # hash_type none should not return a UTXO set hash.
        res5 = node.gettxoutsetinfo(hash_type='none')
        assert 'hash_serialized_2' not in res5

        # hash_type muhash should return a different UTXO set hash.
        res6 = node.gettxoutsetinfo(hash_type='muhash')
        assert 'muhash' in res6
        assert(res['hash_serialized_2'] != res6['muhash'])

        # muhash should not be returned unless requested.
        for r in [res, res2, res3, res4, res5]:
            assert 'muhash' not in r

        # Unknown hash_type raises an error
        assert_raises_rpc_error(-8, "foohash is not a valid hash_type", node.gettxoutsetinfo, "foohash")

    def _test_getblockheader(self):
        self.log.info("Test getblockheader")
        node = self.nodes[0]

        assert_raises_rpc_error(-8, "hash must be of length 64 (not 8, for 'nonsense')", node.getblockheader, "nonsense")
        assert_raises_rpc_error(-8, "hash must be hexadecimal string (not 'ZZZ7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844')", node.getblockheader, "ZZZ7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844")
        assert_raises_rpc_error(-5, "Block not found", node.getblockheader, "0cf7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844")

        besthash = node.getbestblockhash()
        secondbesthash = node.getblockhash(HEIGHT - 1)
        header = node.getblockheader(blockhash=besthash)

        assert_equal(header['hash'], besthash)
        assert_equal(header['height'], HEIGHT)
        assert_equal(header['confirmations'], 1)
        assert_equal(header['previousblockhash'], secondbesthash)
        assert_is_hex_string(header['chainwork'])
        assert_equal(header['nTx'], 1)
        assert_is_hash_string(header['hash'])
        assert_is_hash_string(header['previousblockhash'])
        assert_is_hash_string(header['merkleroot'])
        assert_is_hash_string(header['bits'], length=None)
        assert isinstance(header['time'], int)
        assert_equal(header['mediantime'], TIME_RANGE_MTP)
        assert isinstance(header['nonce'], int)
        assert isinstance(header['version'], int)
        assert isinstance(int(header['versionHex'], 16), int)
        assert isinstance(header['difficulty'], Decimal)

        # Test with verbose=False, which should return the header as hex.
        header_hex = node.getblockheader(blockhash=besthash, verbose=False)
        assert_is_hex_string(header_hex)

        header = from_hex(CBlockHeader(), header_hex)
        header.calc_sha256()
        assert_equal(header.hash, besthash)

        assert 'previousblockhash' not in node.getblockheader(node.getblockhash(0))
        assert 'nextblockhash' not in node.getblockheader(node.getbestblockhash())

    def _test_getdifficulty(self):
        self.log.info("Test getdifficulty")
        difficulty = self.nodes[0].getdifficulty()
        # 1 hash in 2 should be valid, so difficulty should be 1/2**31
        # binary => decimal => binary math is why we do this check
        assert abs(difficulty * 2**31 - 1) < 0.0001

    def _test_getnetworkhashps(self):
        self.log.info("Test getnetworkhashps")
        hashes_per_second = self.nodes[0].getnetworkhashps()
        # This should be 2 hashes every 10 minutes or 1/300
        assert abs(hashes_per_second * 300 - 1) < 0.0001

    def _test_stopatheight(self):
        self.log.info("Test stopping at height")
        assert_equal(self.nodes[0].getblockcount(), HEIGHT)
        self.generatetoaddress(self.nodes[0], 6, ADDRESS_BCRT1_P2WSH_OP_TRUE)
        assert_equal(self.nodes[0].getblockcount(), HEIGHT + 6)
        self.log.debug('Node should not stop at this height')
        assert_raises(subprocess.TimeoutExpired, lambda: self.nodes[0].process.wait(timeout=3))
        try:
            self.generatetoaddress(self.nodes[0], 1, ADDRESS_BCRT1_P2WSH_OP_TRUE, sync_fun=self.no_op)
        except (ConnectionError, http.client.BadStatusLine):
            pass  # The node already shut down before response
        self.log.debug('Node should stop at this height...')
        self.nodes[0].wait_until_stopped()
        self.start_node(0)
        assert_equal(self.nodes[0].getblockcount(), HEIGHT + 7)

    def _test_waitforblockheight(self):
        self.log.info("Test waitforblockheight")
        node = self.nodes[0]
        peer = node.add_p2p_connection(P2PInterface())

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
            peer.send_and_ping(msg_block(b))
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

    def _test_getblock(self):
        node = self.nodes[0]

        miniwallet = MiniWallet(node)
        miniwallet.rescan_utxos()

        fee_per_byte = Decimal('0.00000010')
        fee_per_kb = 1000 * fee_per_byte

        miniwallet.send_self_transfer(fee_rate=fee_per_kb, from_node=node)
        blockhash = self.generate(node, 1)[0]

        def assert_fee_not_in_block(verbosity):
            block = node.getblock(blockhash, verbosity)
            assert 'fee' not in block['tx'][1]

        def assert_fee_in_block(verbosity):
            block = node.getblock(blockhash, verbosity)
            tx = block['tx'][1]
            assert 'fee' in tx
            assert_equal(tx['fee'], tx['vsize'] * fee_per_byte)

        def assert_vin_contains_prevout(verbosity):
            block = node.getblock(blockhash, verbosity)
            tx = block["tx"][1]
            total_vin = Decimal("0.00000000")
            total_vout = Decimal("0.00000000")
            for vin in tx["vin"]:
                assert "prevout" in vin
                assert_equal(set(vin["prevout"].keys()), set(("value", "height", "generated", "scriptPubKey")))
                assert_equal(vin["prevout"]["generated"], True)
                total_vin += vin["prevout"]["value"]
            for vout in tx["vout"]:
                total_vout += vout["value"]
            assert_equal(total_vin, total_vout + tx["fee"])

        def assert_vin_does_not_contain_prevout(verbosity):
            block = node.getblock(blockhash, verbosity)
            tx = block["tx"][1]
            if isinstance(tx, str):
                # In verbosity level 1, only the transaction hashes are written
                pass
            else:
                for vin in tx["vin"]:
                    assert "prevout" not in vin

        self.log.info("Test that getblock with verbosity 1 doesn't include fee")
        assert_fee_not_in_block(1)

        self.log.info('Test that getblock with verbosity 2 and 3 includes expected fee')
        assert_fee_in_block(2)
        assert_fee_in_block(3)

        self.log.info("Test that getblock with verbosity 1 and 2 does not include prevout")
        assert_vin_does_not_contain_prevout(1)
        assert_vin_does_not_contain_prevout(2)

        self.log.info("Test that getblock with verbosity 3 includes prevout")
        assert_vin_contains_prevout(3)

        self.log.info("Test that getblock with verbosity 2 and 3 still works with pruned Undo data")
        datadir = get_datadir_path(self.options.tmpdir, 0)

        self.log.info("Test getblock with invalid verbosity type returns proper error message")
        assert_raises_rpc_error(-1, "JSON value is not an integer as expected", node.getblock, blockhash, "2")

        def move_block_file(old, new):
            old_path = os.path.join(datadir, self.chain, 'blocks', old)
            new_path = os.path.join(datadir, self.chain, 'blocks', new)
            os.rename(old_path, new_path)

        # Move instead of deleting so we can restore chain state afterwards
        move_block_file('rev00000.dat', 'rev_wrong')

        assert_fee_not_in_block(2)
        assert_fee_not_in_block(3)
        assert_vin_does_not_contain_prevout(2)
        assert_vin_does_not_contain_prevout(3)

        # Restore chain state
        move_block_file('rev_wrong', 'rev00000.dat')

        assert 'previousblockhash' not in node.getblock(node.getblockhash(0))
        assert 'nextblockhash' not in node.getblock(node.getbestblockhash())


if __name__ == '__main__':
    BlockchainTest().main()
