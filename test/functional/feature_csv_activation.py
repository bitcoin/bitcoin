#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test CSV soft fork activation.

This soft fork will activate the following BIPS:
BIP 68  - nSequence relative lock times
BIP 112 - CHECKSEQUENCEVERIFY
BIP 113 - MedianTimePast semantics for nLockTime

mine 83 blocks whose coinbases will be used to generate inputs for our tests
mine 344 blocks and seed block chain with the 83 inputs used for our tests at height 427
mine 2 blocks and verify soft fork not yet activated
mine 1 block and test that soft fork is activated (rules enforced for next block)
Test BIP 113 is enforced
Mine 4 blocks so next height is 580 and test BIP 68 is enforced for time and height
Mine 1 block so next height is 581 and test BIP 68 now passes time but not height
Mine 1 block so next height is 582 and test BIP 68 now passes time and height
Test that BIP 112 is enforced

Various transactions will be used to test that the BIPs rules are not enforced before the soft fork activates
And that after the soft fork activates transactions pass and fail as they should according to the rules.
For each BIP, transactions of versions 1 and 2 will be tested.
----------------
BIP 113:
bip113tx - modify the nLocktime variable

BIP 68:
bip68txs - 16 txs with nSequence relative locktime of 10 with various bits set as per the relative_locktimes below

BIP 112:
bip112txs_vary_nSequence - 16 txs with nSequence relative_locktimes of 10 evaluated against 10 OP_CSV OP_DROP
bip112txs_vary_nSequence_9 - 16 txs with nSequence relative_locktimes of 9 evaluated against 10 OP_CSV OP_DROP
bip112txs_vary_OP_CSV - 16 txs with nSequence = 10 evaluated against varying {relative_locktimes of 10} OP_CSV OP_DROP
bip112txs_vary_OP_CSV_9 - 16 txs with nSequence = 9 evaluated against varying {relative_locktimes of 10} OP_CSV OP_DROP
bip112tx_special - test negative argument to OP_CSV
bip112tx_emptystack - test empty stack (= no argument) OP_CSV
"""
from itertools import product
import time

from test_framework.blocktools import (
    create_block,
    create_coinbase,
)
from test_framework.p2p import P2PDataStore
from test_framework.script import (
    CScript,
    OP_CHECKSEQUENCEVERIFY,
    OP_DROP,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    softfork_active,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)

TESTING_TX_COUNT = 83  # Number of testing transactions: 1 BIP113 tx, 16 BIP68 txs, 66 BIP112 txs (see comments above)
COINBASE_BLOCK_COUNT = TESTING_TX_COUNT  # Number of coinbase blocks we need to generate as inputs for our txs
BASE_RELATIVE_LOCKTIME = 10
SEQ_DISABLE_FLAG = 1 << 31
SEQ_RANDOM_HIGH_BIT = 1 << 25
SEQ_TYPE_FLAG = 1 << 22
SEQ_RANDOM_LOW_BIT = 1 << 18


def relative_locktime(sdf, srhb, stf, srlb):
    """Returns a locktime with certain bits set."""

    locktime = BASE_RELATIVE_LOCKTIME
    if sdf:
        locktime |= SEQ_DISABLE_FLAG
    if srhb:
        locktime |= SEQ_RANDOM_HIGH_BIT
    if stf:
        locktime |= SEQ_TYPE_FLAG
    if srlb:
        locktime |= SEQ_RANDOM_LOW_BIT
    return locktime


def all_rlt_txs(txs):
    return [tx['tx'] for tx in txs]


CSV_ACTIVATION_HEIGHT = 432


class BIP68_112_113Test(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [[
            f'-testactivationheight=csv@{CSV_ACTIVATION_HEIGHT}',
        ]]
        self.supports_cli = False

    def create_self_transfer_from_utxo(self, input_tx):
        utxo = self.miniwallet.get_utxo(txid=input_tx.rehash(), mark_as_spent=False)
        tx = self.miniwallet.create_self_transfer(utxo_to_spend=utxo)['tx']
        return tx

    def create_bip112special(self, input, txversion):
        tx = self.create_self_transfer_from_utxo(input)
        tx.version = txversion
        self.miniwallet.sign_tx(tx)
        tx.vin[0].scriptSig = CScript([-1, OP_CHECKSEQUENCEVERIFY, OP_DROP] + list(CScript(tx.vin[0].scriptSig)))
        tx.rehash()
        return tx

    def create_bip112emptystack(self, input, txversion):
        tx = self.create_self_transfer_from_utxo(input)
        tx.version = txversion
        self.miniwallet.sign_tx(tx)
        tx.vin[0].scriptSig = CScript([OP_CHECKSEQUENCEVERIFY] + list(CScript(tx.vin[0].scriptSig)))
        tx.rehash()
        return tx

    def send_generic_input_tx(self, coinbases):
        input_txid = self.nodes[0].getblock(coinbases.pop(), 2)['tx'][0]['txid']
        utxo_to_spend = self.miniwallet.get_utxo(txid=input_txid)
        return self.miniwallet.send_self_transfer(from_node=self.nodes[0], utxo_to_spend=utxo_to_spend)['tx']

    def create_bip68txs(self, bip68inputs, txversion, locktime_delta=0):
        """Returns a list of bip68 transactions with different bits set."""
        txs = []
        assert len(bip68inputs) >= 16
        for i, (sdf, srhb, stf, srlb) in enumerate(product(*[[True, False]] * 4)):
            locktime = relative_locktime(sdf, srhb, stf, srlb)
            tx = self.create_self_transfer_from_utxo(bip68inputs[i])
            tx.version = txversion
            tx.vin[0].nSequence = locktime + locktime_delta
            self.miniwallet.sign_tx(tx)
            txs.append({'tx': tx, 'sdf': sdf, 'stf': stf})

        return txs

    def create_bip112txs(self, bip112inputs, varyOP_CSV, txversion, locktime_delta=0):
        """Returns a list of bip68 transactions with different bits set."""
        txs = []
        assert len(bip112inputs) >= 16
        for i, (sdf, srhb, stf, srlb) in enumerate(product(*[[True, False]] * 4)):
            locktime = relative_locktime(sdf, srhb, stf, srlb)
            tx = self.create_self_transfer_from_utxo(bip112inputs[i])
            if varyOP_CSV:  # if varying OP_CSV, nSequence is fixed
                tx.vin[0].nSequence = BASE_RELATIVE_LOCKTIME + locktime_delta
            else:  # vary nSequence instead, OP_CSV is fixed
                tx.vin[0].nSequence = locktime + locktime_delta
            tx.version = txversion
            self.miniwallet.sign_tx(tx)
            if varyOP_CSV:
                tx.vin[0].scriptSig = CScript([locktime, OP_CHECKSEQUENCEVERIFY, OP_DROP] + list(CScript(tx.vin[0].scriptSig)))
            else:
                tx.vin[0].scriptSig = CScript([BASE_RELATIVE_LOCKTIME, OP_CHECKSEQUENCEVERIFY, OP_DROP] + list(CScript(tx.vin[0].scriptSig)))
            tx.rehash()
            txs.append({'tx': tx, 'sdf': sdf, 'stf': stf})
        return txs

    def generate_blocks(self, number):
        test_blocks = []
        for _ in range(number):
            block = self.create_test_block([])
            test_blocks.append(block)
            self.last_block_time += 600
            self.tip = block.sha256
            self.tipheight += 1
        return test_blocks

    def create_test_block(self, txs):
        block = create_block(self.tip, create_coinbase(self.tipheight + 1), self.last_block_time + 600, txlist=txs)
        block.solve()
        return block

    def send_blocks(self, blocks, success=True, reject_reason=None):
        """Sends blocks to test node. Syncs and verifies that tip has advanced to most recent block.

        Call with success = False if the tip shouldn't advance to the most recent block."""
        self.helper_peer.send_blocks_and_test(blocks, self.nodes[0], success=success, reject_reason=reject_reason)

    def run_test(self):
        self.helper_peer = self.nodes[0].add_p2p_connection(P2PDataStore())
        self.miniwallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_P2PK)

        self.log.info("Generate blocks in the past for coinbase outputs.")
        long_past_time = int(time.time()) - 600 * 1000  # enough to build up to 1000 blocks 10 minutes apart without worrying about getting into the future
        self.nodes[0].setmocktime(long_past_time - 100)  # enough so that the generated blocks will still all be before long_past_time
        self.coinbase_blocks = self.generate(self.miniwallet, COINBASE_BLOCK_COUNT)  # blocks generated for inputs
        self.nodes[0].setmocktime(0)  # set time back to present so yielded blocks aren't in the future as we advance last_block_time
        self.tipheight = COINBASE_BLOCK_COUNT  # height of the next block to build
        self.last_block_time = long_past_time
        self.tip = int(self.nodes[0].getbestblockhash(), 16)

        # Activation height is hardcoded
        # We advance to block height five below BIP112 activation for the following tests
        test_blocks = self.generate_blocks(CSV_ACTIVATION_HEIGHT - 5 - COINBASE_BLOCK_COUNT)
        self.send_blocks(test_blocks)
        assert not softfork_active(self.nodes[0], 'csv')

        # Inputs at height = 431
        #
        # Put inputs for all tests in the chain at height 431 (tip now = 430) (time increases by 600s per block)
        # Note we reuse inputs for v1 and v2 txs so must test these separately
        # 16 normal inputs
        bip68inputs = []
        for _ in range(16):
            bip68inputs.append(self.send_generic_input_tx(self.coinbase_blocks))

        # 2 sets of 16 inputs with 10 OP_CSV OP_DROP (actually will be prepended to spending scriptSig)
        bip112basicinputs = []
        for _ in range(2):
            inputs = []
            for _ in range(16):
                inputs.append(self.send_generic_input_tx(self.coinbase_blocks))
            bip112basicinputs.append(inputs)

        # 2 sets of 16 varied inputs with (relative_lock_time) OP_CSV OP_DROP (actually will be prepended to spending scriptSig)
        bip112diverseinputs = []
        for _ in range(2):
            inputs = []
            for _ in range(16):
                inputs.append(self.send_generic_input_tx(self.coinbase_blocks))
            bip112diverseinputs.append(inputs)

        # 1 special input with -1 OP_CSV OP_DROP (actually will be prepended to spending scriptSig)
        bip112specialinput = self.send_generic_input_tx(self.coinbase_blocks)
        # 1 special input with (empty stack) OP_CSV (actually will be prepended to spending scriptSig)
        bip112emptystackinput = self.send_generic_input_tx(self.coinbase_blocks)

        # 1 normal input
        bip113input = self.send_generic_input_tx(self.coinbase_blocks)

        self.nodes[0].setmocktime(self.last_block_time + 600)
        inputblockhash = self.generate(self.nodes[0], 1)[0]  # 1 block generated for inputs to be in chain at height 431
        self.nodes[0].setmocktime(0)
        self.tip = int(inputblockhash, 16)
        self.tipheight += 1
        self.last_block_time += 600
        assert_equal(len(self.nodes[0].getblock(inputblockhash, True)["tx"]), TESTING_TX_COUNT + 1)

        # 2 more version 4 blocks
        test_blocks = self.generate_blocks(2)
        self.send_blocks(test_blocks)

        assert_equal(self.tipheight, CSV_ACTIVATION_HEIGHT - 2)
        self.log.info(f"Height = {self.tipheight}, CSV not yet active (will activate for block {CSV_ACTIVATION_HEIGHT}, not {CSV_ACTIVATION_HEIGHT - 1})")
        assert not softfork_active(self.nodes[0], 'csv')

        # Test both version 1 and version 2 transactions for all tests
        # BIP113 test transaction will be modified before each use to put in appropriate block time
        bip113tx_v1 = self.create_self_transfer_from_utxo(bip113input)
        bip113tx_v1.vin[0].nSequence = 0xFFFFFFFE
        bip113tx_v1.version = 1
        bip113tx_v2 = self.create_self_transfer_from_utxo(bip113input)
        bip113tx_v2.vin[0].nSequence = 0xFFFFFFFE
        bip113tx_v2.version = 2

        # For BIP68 test all 16 relative sequence locktimes
        bip68txs_v1 = self.create_bip68txs(bip68inputs, 1)
        bip68txs_v2 = self.create_bip68txs(bip68inputs, 2)

        # For BIP112 test:
        # 16 relative sequence locktimes of 10 against 10 OP_CSV OP_DROP inputs
        bip112txs_vary_nSequence_v1 = self.create_bip112txs(bip112basicinputs[0], False, 1)
        bip112txs_vary_nSequence_v2 = self.create_bip112txs(bip112basicinputs[0], False, 2)
        # 16 relative sequence locktimes of 9 against 10 OP_CSV OP_DROP inputs
        bip112txs_vary_nSequence_9_v1 = self.create_bip112txs(bip112basicinputs[1], False, 1, -1)
        bip112txs_vary_nSequence_9_v2 = self.create_bip112txs(bip112basicinputs[1], False, 2, -1)
        # sequence lock time of 10 against 16 (relative_lock_time) OP_CSV OP_DROP inputs
        bip112txs_vary_OP_CSV_v1 = self.create_bip112txs(bip112diverseinputs[0], True, 1)
        bip112txs_vary_OP_CSV_v2 = self.create_bip112txs(bip112diverseinputs[0], True, 2)
        # sequence lock time of 9 against 16 (relative_lock_time) OP_CSV OP_DROP inputs
        bip112txs_vary_OP_CSV_9_v1 = self.create_bip112txs(bip112diverseinputs[1], True, 1, -1)
        bip112txs_vary_OP_CSV_9_v2 = self.create_bip112txs(bip112diverseinputs[1], True, 2, -1)
        # -1 OP_CSV OP_DROP input
        bip112tx_special_v1 = self.create_bip112special(bip112specialinput, 1)
        bip112tx_special_v2 = self.create_bip112special(bip112specialinput, 2)
        # (empty stack) OP_CSV input
        bip112tx_emptystack_v1 = self.create_bip112emptystack(bip112emptystackinput, 1)
        bip112tx_emptystack_v2 = self.create_bip112emptystack(bip112emptystackinput, 2)

        self.log.info("TESTING")

        self.log.info("Pre-Soft Fork Tests. All txs should pass.")
        self.log.info("Test version 1 txs")

        success_txs = []
        # BIP113 tx, -1 CSV tx and empty stack CSV tx should succeed
        bip113tx_v1.nLockTime = self.last_block_time - 600 * 5  # = MTP of prior block (not <) but < time put on current block
        self.miniwallet.sign_tx(bip113tx_v1)
        success_txs.append(bip113tx_v1)
        success_txs.append(bip112tx_special_v1)
        success_txs.append(bip112tx_emptystack_v1)
        # add BIP 68 txs
        success_txs.extend(all_rlt_txs(bip68txs_v1))
        # add BIP 112 with seq=10 txs
        success_txs.extend(all_rlt_txs(bip112txs_vary_nSequence_v1))
        success_txs.extend(all_rlt_txs(bip112txs_vary_OP_CSV_v1))
        # try BIP 112 with seq=9 txs
        success_txs.extend(all_rlt_txs(bip112txs_vary_nSequence_9_v1))
        success_txs.extend(all_rlt_txs(bip112txs_vary_OP_CSV_9_v1))
        self.send_blocks([self.create_test_block(success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        self.log.info("Test version 2 txs")

        success_txs = []
        # BIP113 tx, -1 CSV tx and empty stack CSV tx should succeed
        bip113tx_v2.nLockTime = self.last_block_time - 600 * 5  # = MTP of prior block (not <) but < time put on current block
        self.miniwallet.sign_tx(bip113tx_v2)
        success_txs.append(bip113tx_v2)
        success_txs.append(bip112tx_special_v2)
        success_txs.append(bip112tx_emptystack_v2)
        # add BIP 68 txs
        success_txs.extend(all_rlt_txs(bip68txs_v2))
        # add BIP 112 with seq=10 txs
        success_txs.extend(all_rlt_txs(bip112txs_vary_nSequence_v2))
        success_txs.extend(all_rlt_txs(bip112txs_vary_OP_CSV_v2))
        # try BIP 112 with seq=9 txs
        success_txs.extend(all_rlt_txs(bip112txs_vary_nSequence_9_v2))
        success_txs.extend(all_rlt_txs(bip112txs_vary_OP_CSV_9_v2))
        self.send_blocks([self.create_test_block(success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # 1 more version 4 block to get us to height 432 so the fork should now be active for the next block
        assert not softfork_active(self.nodes[0], 'csv')
        test_blocks = self.generate_blocks(1)
        self.send_blocks(test_blocks)
        assert softfork_active(self.nodes[0], 'csv')

        self.log.info("Post-Soft Fork Tests.")

        self.log.info("BIP 113 tests")
        # BIP 113 tests should now fail regardless of version number if nLockTime isn't satisfied by new rules
        bip113tx_v1.nLockTime = self.last_block_time - 600 * 5  # = MTP of prior block (not <) but < time put on current block
        self.miniwallet.sign_tx(bip113tx_v1)
        bip113tx_v2.nLockTime = self.last_block_time - 600 * 5  # = MTP of prior block (not <) but < time put on current block
        self.miniwallet.sign_tx(bip113tx_v2)
        for bip113tx in [bip113tx_v1, bip113tx_v2]:
            self.send_blocks([self.create_test_block([bip113tx])], success=False, reject_reason='bad-txns-nonfinal')

        # BIP 113 tests should now pass if the locktime is < MTP
        bip113tx_v1.nLockTime = self.last_block_time - 600 * 5 - 1  # < MTP of prior block
        self.miniwallet.sign_tx(bip113tx_v1)
        bip113tx_v2.nLockTime = self.last_block_time - 600 * 5 - 1  # < MTP of prior block
        self.miniwallet.sign_tx(bip113tx_v2)
        for bip113tx in [bip113tx_v1, bip113tx_v2]:
            self.send_blocks([self.create_test_block([bip113tx])])
            self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # Next block height = 437 after 4 blocks of random version
        test_blocks = self.generate_blocks(4)
        self.send_blocks(test_blocks)

        self.log.info("BIP 68 tests")
        self.log.info("Test version 1 txs - all should still pass")

        success_txs = []
        success_txs.extend(all_rlt_txs(bip68txs_v1))
        self.send_blocks([self.create_test_block(success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        self.log.info("Test version 2 txs")

        # All txs with SEQUENCE_LOCKTIME_DISABLE_FLAG set pass
        bip68success_txs = [tx['tx'] for tx in bip68txs_v2 if tx['sdf']]
        self.send_blocks([self.create_test_block(bip68success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # All txs without flag fail as we are at delta height = 8 < 10 and delta time = 8 * 600 < 10 * 512
        bip68timetxs = [tx['tx'] for tx in bip68txs_v2 if not tx['sdf'] and tx['stf']]
        for tx in bip68timetxs:
            self.send_blocks([self.create_test_block([tx])], success=False, reject_reason='bad-txns-nonfinal')

        bip68heighttxs = [tx['tx'] for tx in bip68txs_v2 if not tx['sdf'] and not tx['stf']]
        for tx in bip68heighttxs:
            self.send_blocks([self.create_test_block([tx])], success=False, reject_reason='bad-txns-nonfinal')

        # Advance one block to 438
        test_blocks = self.generate_blocks(1)
        self.send_blocks(test_blocks)

        # Height txs should fail and time txs should now pass 9 * 600 > 10 * 512
        bip68success_txs.extend(bip68timetxs)
        self.send_blocks([self.create_test_block(bip68success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())
        for tx in bip68heighttxs:
            self.send_blocks([self.create_test_block([tx])], success=False, reject_reason='bad-txns-nonfinal')

        # Advance one block to 439
        test_blocks = self.generate_blocks(1)
        self.send_blocks(test_blocks)

        # All BIP 68 txs should pass
        bip68success_txs.extend(bip68heighttxs)
        self.send_blocks([self.create_test_block(bip68success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        self.log.info("BIP 112 tests")
        self.log.info("Test version 1 txs")

        # -1 OP_CSV tx and (empty stack) OP_CSV tx should fail
        self.send_blocks([self.create_test_block([bip112tx_special_v1])], success=False,
                         reject_reason='mandatory-script-verify-flag-failed (Negative locktime)')
        self.send_blocks([self.create_test_block([bip112tx_emptystack_v1])], success=False,
                         reject_reason='mandatory-script-verify-flag-failed (Operation not valid with the current stack size)')
        # If SEQUENCE_LOCKTIME_DISABLE_FLAG is set in argument to OP_CSV, version 1 txs should still pass

        success_txs = [tx['tx'] for tx in bip112txs_vary_OP_CSV_v1 if tx['sdf']]
        success_txs += [tx['tx'] for tx in bip112txs_vary_OP_CSV_9_v1 if tx['sdf']]
        self.send_blocks([self.create_test_block(success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # If SEQUENCE_LOCKTIME_DISABLE_FLAG is unset in argument to OP_CSV, version 1 txs should now fail
        fail_txs = all_rlt_txs(bip112txs_vary_nSequence_v1)
        fail_txs += all_rlt_txs(bip112txs_vary_nSequence_9_v1)
        fail_txs += [tx['tx'] for tx in bip112txs_vary_OP_CSV_v1 if not tx['sdf']]
        fail_txs += [tx['tx'] for tx in bip112txs_vary_OP_CSV_9_v1 if not tx['sdf']]
        for tx in fail_txs:
            self.send_blocks([self.create_test_block([tx])], success=False,
                             reject_reason='mandatory-script-verify-flag-failed (Locktime requirement not satisfied)')

        self.log.info("Test version 2 txs")

        # -1 OP_CSV tx and (empty stack) OP_CSV tx should fail
        self.send_blocks([self.create_test_block([bip112tx_special_v2])], success=False,
                         reject_reason='mandatory-script-verify-flag-failed (Negative locktime)')
        self.send_blocks([self.create_test_block([bip112tx_emptystack_v2])], success=False,
                         reject_reason='mandatory-script-verify-flag-failed (Operation not valid with the current stack size)')

        # If SEQUENCE_LOCKTIME_DISABLE_FLAG is set in argument to OP_CSV, version 2 txs should pass (all sequence locks are met)
        success_txs = [tx['tx'] for tx in bip112txs_vary_OP_CSV_v2 if tx['sdf']]
        success_txs += [tx['tx'] for tx in bip112txs_vary_OP_CSV_9_v2 if tx['sdf']]

        self.send_blocks([self.create_test_block(success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # SEQUENCE_LOCKTIME_DISABLE_FLAG is unset in argument to OP_CSV for all remaining txs ##

        # All txs with nSequence 9 should fail either due to earlier mismatch or failing the CSV check
        fail_txs = all_rlt_txs(bip112txs_vary_nSequence_9_v2)
        fail_txs += [tx['tx'] for tx in bip112txs_vary_OP_CSV_9_v2 if not tx['sdf']]
        for tx in fail_txs:
            self.send_blocks([self.create_test_block([tx])], success=False,
                             reject_reason='mandatory-script-verify-flag-failed (Locktime requirement not satisfied)')

        # If SEQUENCE_LOCKTIME_DISABLE_FLAG is set in nSequence, tx should fail
        fail_txs = [tx['tx'] for tx in bip112txs_vary_nSequence_v2 if tx['sdf']]
        for tx in fail_txs:
            self.send_blocks([self.create_test_block([tx])], success=False,
                             reject_reason='mandatory-script-verify-flag-failed (Locktime requirement not satisfied)')

        # If sequencelock types mismatch, tx should fail
        fail_txs = [tx['tx'] for tx in bip112txs_vary_nSequence_v2 if not tx['sdf'] and tx['stf']]
        fail_txs += [tx['tx'] for tx in bip112txs_vary_OP_CSV_v2 if not tx['sdf'] and tx['stf']]
        for tx in fail_txs:
            self.send_blocks([self.create_test_block([tx])], success=False,
                             reject_reason='mandatory-script-verify-flag-failed (Locktime requirement not satisfied)')

        # Remaining txs should pass, just test masking works properly
        success_txs = [tx['tx'] for tx in bip112txs_vary_nSequence_v2 if not tx['sdf'] and not tx['stf']]
        success_txs += [tx['tx'] for tx in bip112txs_vary_OP_CSV_v2 if not tx['sdf'] and not tx['stf']]
        self.send_blocks([self.create_test_block(success_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())

        # Additional test, of checking that comparison of two time types works properly
        time_txs = []
        for tx in [tx['tx'] for tx in bip112txs_vary_OP_CSV_v2 if not tx['sdf'] and tx['stf']]:
            tx.vin[0].nSequence = BASE_RELATIVE_LOCKTIME | SEQ_TYPE_FLAG
            self.miniwallet.sign_tx(tx)
            time_txs.append(tx)

        self.send_blocks([self.create_test_block(time_txs)])
        self.nodes[0].invalidateblock(self.nodes[0].getbestblockhash())


if __name__ == '__main__':
    BIP68_112_113Test(__file__).main()
