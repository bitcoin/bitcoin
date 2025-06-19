#!/usr/bin/env python3
# Copyright (c) 2024-Present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test getblocktemplate RPC in proposal mode

Generate several blocks and test them against the getblocktemplate RPC.
"""

from concurrent.futures import ThreadPoolExecutor

import copy

from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment,
)

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)

from test_framework.messages import (
    BLOCK_HEADER_SIZE,
    uint256_from_compact,
)

from test_framework.wallet import (
    MiniWallet,
)

def assert_template(node, block, expect, *, rehash=True, submit=True, solve=True, expect_submit=None):
    if rehash:
        block.hashMerkleRoot = block.calc_merkle_root()

    rsp = node.getblocktemplate(template_request={
        'data': block.serialize().hex(),
        'mode': 'proposal',
        'rules': ['segwit'],
    })
    assert_equal(rsp, expect)
    # Only attempt to submit invalid templates
    if submit and expect is not None:
        # submitblock runs checks in a different order, so may not return
        # the same error
        if expect_submit is None:
            expect_submit = expect
        if solve:
            block.solve()
        assert_equal(node.submitblock(block.serialize().hex()), expect_submit)

class MiningTemplateVerificationTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1

    def valid_block_test(self, node, block):
        self.log.info("Valid block")
        assert_template(node, block, None)

    def cb_missing_test(self, node, block):
        self.log.info("Bad input hash for coinbase transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].vin[0].prevout.hash += 1
        assert_template(node, bad_block, 'bad-cb-missing')

    def block_without_transactions_test(self, node, block):
        self.log.info("Block with no transactions")

        no_tx_block = copy.deepcopy(block)
        no_tx_block.vtx.clear()
        no_tx_block.hashMerkleRoot = 0
        no_tx_block.solve()
        assert_template(node, no_tx_block, 'bad-blk-length', rehash=False)

    def truncated_final_transaction_test(self, node, block):
        self.log.info("Truncated final transaction")
        assert_raises_rpc_error(-22, "Block decode failed", node.getblocktemplate,
            template_request={
                "data": block.serialize()[:-1].hex(),
                "mode": "proposal",
                "rules": ["segwit"],
            }
        )

    def duplicate_transaction_test(self, node, block):
        self.log.info("Duplicate transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx.append(bad_block.vtx[0])
        assert_template(node, bad_block, 'bad-txns-duplicate')

    def thin_air_spending_test(self, node, block):
        self.log.info("Transaction that spends from thin air")
        bad_block = copy.deepcopy(block)
        bad_tx = copy.deepcopy(bad_block.vtx[0])
        bad_tx.vin[0].prevout.hash = 255
        bad_block.vtx.append(bad_tx)
        assert_template(node, bad_block, 'bad-txns-inputs-missingorspent')

    def non_final_transaction_test(self, node, block):
        self.log.info("Non-final transaction")
        bad_block = copy.deepcopy(block)
        bad_block.vtx[0].nLockTime = 2**32 - 1
        assert_template(node, bad_block, 'bad-txns-nonfinal')

    def bad_tx_count_test(self, node, block):
        self.log.info("Bad tx count")
        # The tx count is immediately after the block header
        bad_block_sn = bytearray(block.serialize())
        assert_equal(bad_block_sn[BLOCK_HEADER_SIZE], 1)
        bad_block_sn[BLOCK_HEADER_SIZE] += 1
        assert_raises_rpc_error(-22, "Block decode failed", node.getblocktemplate, {
            'data': bad_block_sn.hex(),
            'mode': 'proposal',
            'rules': ['segwit'],
        })

    def nbits_test(self, node, block):
        self.log.info("Extremely high nBits")
        bad_block = copy.deepcopy(block)
        bad_block.nBits = 469762303  # impossible in the real world
        assert_template(node, bad_block, "bad-diffbits", solve=False, expect_submit="high-hash")

        self.log.info("Lowering nBits should make the block invalid")
        bad_block = copy.deepcopy(block)
        bad_block.nBits -= 1
        assert_template(node, bad_block, "bad-diffbits")

    def merkle_root_test(self, node, block):
        self.log.info("Bad merkle root")
        bad_block = copy.deepcopy(block)
        bad_block.hashMerkleRoot += 1
        assert_template(node, bad_block, 'bad-txnmrklroot', rehash=False)

    def bad_timestamp_test(self, node, block):
        self.log.info("Bad timestamps")
        bad_block = copy.deepcopy(block)
        bad_block.nTime = 2**32 - 1
        assert_template(node, bad_block, 'time-too-new')
        bad_block.nTime = 0
        assert_template(node, bad_block, 'time-too-old')

    def current_tip_test(self, node, block):
        self.log.info("Block must build on the current tip")
        bad_block = copy.deepcopy(block)
        bad_block.hashPrevBlock = 123
        bad_block.solve()

        assert_template(node, bad_block, "inconclusive-not-best-prevblk", expect_submit="prev-blk-not-found")

    def pow_test(self, node, block):
        '''Modifies block with the generated PoW'''
        self.log.info("Generate a block")
        target = uint256_from_compact(block.nBits)
        # Ensure that it doesn't meet the target by coincidence
        while block.sha256 <= target:
            block.nNonce += 1
            block.rehash()
        self.log.debug("Found a nonce")

        self.log.info("A block template doesn't need PoW")
        assert_template(node, block, None)

        self.log.info("Add proof of work")
        block.solve()
        assert_template(node, block, None)

    def submit_test(self, node, block_0_height, block):
        self.log.info("getblocktemplate call in previous tests did not submit the block")
        assert_equal(node.getblockcount(), block_0_height + 1)

        self.log.info("Submitting this block should succeed")
        assert_equal(node.submitblock(block.serialize().hex()), None)
        node.waitforblockheight(2)

    def transaction_test(self, node, block_0_height, tx):
        self.log.info("make block template with a transaction")

        block_1 = node.getblock(node.getblockhash(block_0_height + 1))
        block_2_hash = node.getblockhash(block_0_height + 2)

        block_3 = create_block(
            int(block_2_hash, 16),
            create_coinbase(block_0_height + 3),
            block_1["mediantime"] + 1,
            txlist=[tx["hex"]],
        )
        assert_equal(len(block_3.vtx), 2)
        add_witness_commitment(block_3)
        block_3.solve()
        assert_template(node, block_3, None)

        self.log.info("checking block validity did not update the UTXO set")
        # Call again to ensure the UTXO set wasn't updated
        assert_template(node, block_3, None)

    def overspending_transaction_test(self, node, block_0_height, tx):
        self.log.info("Add an transaction that spends too much")

        block_1 = node.getblock(node.getblockhash(block_0_height + 1))
        block_2_hash = node.getblockhash(block_0_height + 2)

        bad_tx = copy.deepcopy(tx)
        bad_tx["tx"].vout[0].nValue = 10000000000
        bad_tx_hex = bad_tx["tx"].serialize().hex()
        assert_equal(
            node.testmempoolaccept([bad_tx_hex])[0]["reject-reason"],
            "bad-txns-in-belowout",
        )
        block_3 = create_block(
            int(block_2_hash, 16),
            create_coinbase(block_0_height + 3),
            block_1["mediantime"] + 1,
            txlist=[bad_tx_hex],
        )
        assert_equal(len(block_3.vtx), 2)
        add_witness_commitment(block_3)
        block_3.solve()

        assert_template(node, block_3, "bad-txns-in-belowout")

    def spend_twice_test(self, node, block_0_height, tx):
        block_1 = node.getblock(node.getblockhash(block_0_height + 1))
        block_2_hash = node.getblockhash(block_0_height + 2)

        self.log.info("Can't spend coins twice")
        tx_hex = tx["tx"].serialize().hex()
        tx_2 = copy.deepcopy(tx)
        tx_2_hex = tx_2["tx"].serialize().hex()
        # Nothing wrong with these transactions individually
        assert_equal(node.testmempoolaccept([tx_hex])[0]["allowed"], True)
        assert_equal(node.testmempoolaccept([tx_2_hex])[0]["allowed"], True)
        # But can't be combined
        assert_equal(
            node.testmempoolaccept([tx_hex, tx_2_hex])[0]["package-error"],
            "package-contains-duplicates",
        )
        block_3 = create_block(
            int(block_2_hash, 16),
            create_coinbase(block_0_height + 3),
            block_1["mediantime"] + 1,
            txlist=[tx_hex, tx_2_hex],
        )
        assert_equal(len(block_3.vtx), 3)
        add_witness_commitment(block_3)

        assert_template(node, block_3, "bad-txns-inputs-missingorspent", submit=False)

        return block_3

    def parallel_test(self, node, block_3):
        # Ensure that getblocktemplate can be called concurrently by many threads.
        self.log.info("Check blocks in parallel")
        check_50_blocks = lambda n: [
            assert_template(n, block_3, "bad-txns-inputs-missingorspent", submit=False)
            for _ in range(50)
        ]
        rpcs = [node.cli for _ in range(6)]
        with ThreadPoolExecutor(max_workers=len(rpcs)) as threads:
            list(threads.map(check_50_blocks, rpcs))

    def run_test(self):
        node = self.nodes[0]

        block_0_height = node.getblockcount()
        self.generate(node, sync_fun=self.no_op, nblocks=1)
        block_1 = node.getblock(node.getbestblockhash())
        block_2 = create_block(
            int(block_1["hash"], 16),
            create_coinbase(block_0_height + 2),
            block_1["mediantime"] + 1,
        )

        self.valid_block_test(node, block_2)
        self.cb_missing_test(node, block_2)
        self.block_without_transactions_test(node, block_2)
        self.truncated_final_transaction_test(node, block_2)
        self.duplicate_transaction_test(node, block_2)
        self.thin_air_spending_test(node, block_2)
        self.non_final_transaction_test(node, block_2)
        self.bad_tx_count_test(node, block_2)
        self.nbits_test(node, block_2)
        self.merkle_root_test(node, block_2)
        self.bad_timestamp_test(node, block_2)
        self.current_tip_test(node, block_2)
        # This sets the PoW for the next test
        self.pow_test(node, block_2)
        self.submit_test(node, block_0_height, block_2)

        self.log.info("Generate a transaction")
        tx = MiniWallet(node).create_self_transfer()

        self.transaction_test(node, block_0_height, tx)
        self.overspending_transaction_test(node, block_0_height, tx)
        block_3 = self.spend_twice_test(node, block_0_height, tx)
        self.parallel_test(node, block_3)

if __name__ == "__main__":
    MiningTemplateVerificationTest(__file__).main()
