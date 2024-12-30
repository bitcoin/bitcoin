#!/usr/bin/env python3
# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test checkblock RPC

Generate several (weak) blocks and test them against the checkblock RPC.
"""

from concurrent.futures import ThreadPoolExecutor

import copy

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment
)

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)

from test_framework.messages import (
    COutPoint,
    CTxIn,
    uint256_from_str,
    ser_uint256,
)

from test_framework.wallet import (
    MiniWallet,
)

class CheckBlockTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1

    def run_test(self):
        block_0_hash = self.nodes[0].getbestblockhash()
        block_0_height = self.nodes[0].getblockcount()
        self.generate(self.nodes[0], sync_fun=self.no_op, nblocks=1)
        block_1 = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        block_2 = create_block(int(block_1['hash'], 16), create_coinbase(block_0_height + 2), block_1['mediantime'] + 1)

        # Block must build on the current tip
        bad_block_2 = copy.deepcopy(block_2)
        bad_block_2.hashPrevBlock = int(block_0_hash, 16)
        bad_block_2.solve()

        assert_equal(self.nodes[0].checkblock(bad_block_2.serialize().hex()), "inconclusive-not-best-prevblk")

        self.log.info("Lowering nBits should make the block invalid")
        bad_block_2 = copy.deepcopy(block_2)
        bad_block_2.nBits = bad_block_2.nBits - 1
        bad_block_2.solve()

        assert_equal(self.nodes[0].checkblock(bad_block_2.serialize().hex()), "bad-diffbits")

        self.log.info("A weak block won't pass the check by default")
        block_2.solve(multiplier=6)
        assert_equal(self.nodes[0].checkblock(block_2.serialize().hex()), "high-hash")

        # The above solve found a nonce between 3x and 6x the target.
        self.log.info("Checking against a 3x higher target should fail.")
        target = uint256_from_str(bytes.fromhex(self.nodes[0].getmininginfo()['target']))
        target_3x = ser_uint256(3 * target).hex()
        assert_equal(self.nodes[0].checkblock(block_2.serialize().hex(), {'target': target_3x}), "high-hash")

        self.log.info("Checking against a 6x higher target should work")
        target_6x = ser_uint256(6 * target).hex()

        assert_equal(self.nodes[0].checkblock(block_2.serialize().hex(), {'target': target_6x}), None)

        self.log.info("Skip the PoW check altogether")
        assert_equal(self.nodes[0].checkblock(block_2.serialize().hex(), {'check_pow': False}), None)

        self.log.info("Add normal proof of work")
        block_2.solve()
        assert_equal(self.nodes[0].checkblock(block_2.serialize().hex()), None)

        self.log.info("checkblock does not submit the block")
        assert_equal(self.nodes[0].getblockcount(), block_0_height + 1)

        self.log.info("Submitting this block should succeed")
        assert_equal(self.nodes[0].submitblock(block_2.serialize().hex()), None)
        self.nodes[0].waitforblockheight(2)

        self.log.info("Generate a transaction")
        tx = MiniWallet(self.nodes[0]).create_self_transfer()
        block_3 = create_block(int(block_2.hash, 16), create_coinbase(block_0_height + 3), block_1['mediantime'] + 1, txlist=[tx['hex']])
        assert_equal(len(block_3.vtx), 2)
        add_witness_commitment(block_3)
        block_3.solve()
        assert_equal(self.nodes[0].checkblock(block_3.serialize().hex()), None)
        # Call again to ensure the UTXO set wasn't updated
        assert_equal(self.nodes[0].checkblock(block_3.serialize().hex()), None)

        self.log.info("Add an invalid transaction")
        bad_tx = copy.deepcopy(tx)
        bad_tx['tx'].vout[0].nValue = 10000000000
        bad_tx_hex = bad_tx['tx'].serialize().hex()
        assert_equal(self.nodes[0].testmempoolaccept([bad_tx_hex])[0]['reject-reason'], 'bad-txns-in-belowout')
        block_3 = create_block(int(block_2.hash, 16), create_coinbase(block_0_height + 3), block_1['mediantime'] + 1, txlist=[bad_tx_hex])
        assert_equal(len(block_3.vtx), 2)
        add_witness_commitment(block_3)
        block_3.solve()

        self.log.info("This can't be submitted")
        assert_equal(self.nodes[0].submitblock(block_3.serialize().hex()), 'bad-txns-in-belowout')

        self.log.info("And should also not pass checkbock")
        assert_equal(self.nodes[0].checkblock(block_3.serialize().hex()), 'bad-txns-in-belowout')

        self.log.info("Can't spend coins out of thin air")
        bad_tx = copy.deepcopy(tx)
        bad_tx['tx'].vin[0] = CTxIn(outpoint=COutPoint(hash=int('aa' * 32, 16), n=0), scriptSig=b"")
        bad_tx_hex = bad_tx['tx'].serialize().hex()
        assert_equal(self.nodes[0].testmempoolaccept([bad_tx_hex])[0]['reject-reason'], 'missing-inputs')
        block_3 = create_block(int(block_2.hash, 16), create_coinbase(block_0_height + 3), block_1['mediantime'] + 1, txlist=[bad_tx_hex])
        assert_equal(len(block_3.vtx), 2)
        add_witness_commitment(block_3)
        block_3.solve()
        assert_equal(self.nodes[0].checkblock(block_3.serialize().hex()), 'bad-txns-inputs-missingorspent')

        self.log.info("Can't spend coins twice")
        tx_hex = tx['tx'].serialize().hex()
        tx_2 = copy.deepcopy(tx)
        tx_2_hex = tx_2['tx'].serialize().hex()
        # Nothing wrong with these transactions individually
        assert_equal(self.nodes[0].testmempoolaccept([tx_hex])[0]['allowed'], True)
        assert_equal(self.nodes[0].testmempoolaccept([tx_2_hex])[0]['allowed'], True)
        # But can't be combined
        assert_equal(self.nodes[0].testmempoolaccept([tx_hex, tx_2_hex])[0]['package-error'], "package-contains-duplicates")
        block_3 = create_block(int(block_2.hash, 16), create_coinbase(block_0_height + 3), block_1['mediantime'] + 1, txlist=[tx_hex, tx_2_hex])
        assert_equal(len(block_3.vtx), 3)
        add_witness_commitment(block_3)
        block_3.solve()
        assert_equal(self.nodes[0].checkblock(block_3.serialize().hex()), 'bad-txns-inputs-missingorspent')

        # Ensure that checkblock can be called concurrently by many threads.
        self.log.info('Check blocks in parallel')
        check_50_blocks = lambda n: [assert_equal(n.checkblock(block_3.serialize().hex()),  'bad-txns-inputs-missingorspent') for _ in range(50)]
        rpcs = [self.nodes[0].cli for _ in range(6)]
        with ThreadPoolExecutor(max_workers=len(rpcs)) as threads:
            list(threads.map(check_50_blocks, rpcs))

if __name__ == '__main__':
    CheckBlockTest(__file__).main()
