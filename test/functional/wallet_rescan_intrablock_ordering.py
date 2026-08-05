#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that ScanForWalletTransactions misses a transaction when a look-ahead key
   tx and a pool-expanding tx appear in the same block with the look-ahead tx first.

Background:
   A descriptor wallet pre-derives a batch of N keys ahead of actual use (the
   look-ahead pool, [next_index, next_index + keypool_size - 1]). When a payment
   is received to a key near the pool boundary, MarkUnusedAddresses() calls TopUp()
   to extend the pool by another keypool_size keys.

   ScanForWalletTransactions iterates over a block's transactions in the order the
   miner placed them (vtx order). If within one block:
     - Tx_lookahead sends to key at index end_range+1 (just outside the pool), AND
     - Tx_expand   sends to key at index end_range   (last in pool, triggers TopUp)
   and Tx_lookahead comes before Tx_expand in vtx order, then:
     - Tx_lookahead is processed while pool=[0, end_range]: IsMine returns false, missed.
     - Tx_expand is processed next: IsMine returns true, TopUp extends pool.
   FastWalletRescanFilter::UpdateIfNeeded() fires only at the start of the NEXT block
   iteration, so within the same block there is no mechanism to re-process Tx_lookahead.

   Result: Tx_lookahead is missed on the first scan and the wallet shows a wrong
   balance. However, because Tx_expand was found and extended the pool as a side
   effect, a subsequent explicit rescanblockchain call recovers Tx_lookahead.
   The user has no indication that a second rescan is needed.

   Note: this bug does NOT appear when both transactions go through the mempool
   with the wallet loaded. transactionAddedToMempool(Tx_expand) triggers TopUp
   before the block arrives, so the pool is already extended when blockConnected
   processes the block in vtx order.

   This test verifies the fix: after re-processing the block when the pool expands
   mid-block, Tx_lookahead is found on the first scan. Part 3 confirms that a second
   rescanblockchain is no longer needed to recover the missed transaction.

   Reference: https://github.com/bitcoin/bitcoin/pull/31629#issuecomment-2657775368
"""
from test_framework.address import address_to_scriptpubkey
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal
from test_framework.wallet import MiniWallet


KEYPOOL_SIZE = 5   # small keypool for faster test execution


class WalletRescanIntraBlockOrderingTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f'-keypool={KEYPOOL_SIZE}']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]
        funding_wallet = MiniWallet(node)

        self.generate(funding_wallet, COINBASE_MATURITY + 1)

        # -----------------------------------------------------------------------
        # Setup: derive the two addresses that will trigger the bug.
        #
        # Unload the wallet before submitting transactions so that
        # transactionAddedToMempool(Tx_expand) cannot pre-emptively extend the
        # pool — which would hide the bug in blockConnected.
        # -----------------------------------------------------------------------
        self.log.info("Create wallet, derive boundary addresses, then unload it")
        node.createwallet(wallet_name='w')
        w = node.get_wallet_rpc('w')

        descs = w.listdescriptors()['descriptors']
        recv_desc = next(d for d in descs if 'range' in d and not d['internal'])
        _, end_range = recv_desc['range']   # end_range == KEYPOOL_SIZE - 1

        # addr_expand: the last key in the pool. A payment here triggers TopUp,
        # extending the pool by another KEYPOOL_SIZE keys.
        addr_expand    = w.deriveaddresses(recv_desc['desc'], [end_range,     end_range    ])[0]
        # addr_lookahead: the first key just outside the pool. It enters the pool
        # only after addr_expand's TopUp fires.
        addr_lookahead = w.deriveaddresses(recv_desc['desc'], [end_range + 1, end_range + 1])[0]

        self.log.info(f"Initial pool:  [0, {end_range}]")
        self.log.info(f"addr_expand    (index {end_range}):     {addr_expand}")
        self.log.info(f"addr_lookahead (index {end_range + 1}): {addr_lookahead}")

        node.unloadwallet('w')

        # -----------------------------------------------------------------------
        # Submit both transactions and mine them in the bug-triggering order:
        #   vtx position 0: Tx_lookahead (addr outside pool)
        #   vtx position 1: Tx_expand   (addr inside pool, triggers TopUp)
        #
        # Tx_expand spends Tx_lookahead's change, so the dependency chain forces
        # Tx_lookahead into position 0 without any artificial arrangement.
        # -----------------------------------------------------------------------
        self.log.info("Submit txs and mine with Tx_lookahead before Tx_expand")
        tx_lookahead = funding_wallet.send_to(
            from_node=node,
            scriptPubKey=address_to_scriptpubkey(addr_lookahead),
            amount=10_000,
        )
        tx_expand = funding_wallet.send_to(
            from_node=node,
            scriptPubKey=address_to_scriptpubkey(addr_expand),
            amount=10_000,
        )
        assert_equal(sorted(node.getrawmempool()), sorted([tx_lookahead['txid'], tx_expand['txid']]))

        self.generateblock(node, output="raw(51)", transactions=[
            tx_lookahead['txid'],
            tx_expand['txid'],
        ])
        assert_equal(node.getrawmempool(), [])

        # -----------------------------------------------------------------------
        # Part 1: rescan triggered by loadwallet
        #
        # The wallet stored best-block = height COINBASE_MATURITY+1 when unloaded.
        # Loading it now triggers ScanForWalletTransactions over the one new block.
        # Pool starts at [0, end_range]; Tx_lookahead is processed first and missed;
        # Tx_expand is processed second, found, and extends the pool as a side effect.
        # -----------------------------------------------------------------------
        self.log.info("Part 1: reload wallet (rescan of the missed block)")
        node.loadwallet('w')
        w = node.get_wallet_rpc('w')
        node.syncwithvalidationinterfacequeue()

        txids = {tx['txid'] for tx in w.listtransactions('*', 1000)}
        assert tx_expand['txid'] in txids, \
            "tx_expand must always be found (it was within the initial pool)"
        assert tx_lookahead['txid'] in txids, \
            "tx_lookahead must be found after fix: block is re-processed when pool expands mid-block"

        # -----------------------------------------------------------------------
        # Part 2: ScanForWalletTransactions via importdescriptors (timestamp=0)
        #
        # A fresh blank wallet imports the same descriptor and rescans from genesis.
        # It has no prior transaction history, so the pool starts at [0, end_range].
        # The same vtx-order miss applies: Tx_lookahead missed, Tx_expand found.
        # -----------------------------------------------------------------------
        self.log.info("Part 2: importdescriptors on fresh wallet (full rescan from genesis)")
        node.createwallet(wallet_name='w_rescan', disable_private_keys=True, blank=True)
        w_rescan = node.get_wallet_rpc('w_rescan')
        w_rescan.importdescriptors([{
            "desc": recv_desc['desc'],
            "timestamp": 0,
        }])

        txids_rescan = {tx['txid'] for tx in w_rescan.listtransactions('*', 1000)}
        assert tx_expand['txid'] in txids_rescan, \
            "tx_expand must be found after full rescan"
        assert tx_lookahead['txid'] in txids_rescan, \
            "tx_lookahead must be found after fix: block re-processed when pool expands mid-block"

        # -----------------------------------------------------------------------
        # Part 3: verify that a second rescanblockchain recovers tx_lookahead.
        #
        # Part 2's importdescriptors scan found tx_expand and extended the pool
        # as a side effect. A second pass now sees tx_lookahead's key in the pool.
        # This matches furszy's description: "the user might need to rescan those
        # blocks too" — i.e. the tx is not permanently lost, just missed on the
        # first scan with no indication a second scan is needed.
        # -----------------------------------------------------------------------
        self.log.info("Part 3: second rescanblockchain recovers tx_lookahead")
        w_rescan.rescanblockchain()

        txids_after_second_scan = {tx['txid'] for tx in w_rescan.listtransactions('*', 1000)}
        assert tx_lookahead['txid'] in txids_after_second_scan, \
            "tx_lookahead must be recovered on a second rescan (pool was already extended)"
        assert tx_expand['txid'] in txids_after_second_scan

        # -----------------------------------------------------------------------
        # Part 4: cascading pool expansions — two levels in one block.
        #
        # This validates that the re-process loop handles multiple successive
        # TopUp events within a single block, and that the pass cap provides a
        # bounded safety net against adversarially crafted blocks.
        #
        # Setup (pool starts at [0, end_range]):
        #   addr_expand_1  (key end_range)              triggers TopUp → pool doubles
        #   addr_expand_2  (key end_range + KEYPOOL_SIZE) triggers TopUp → pool doubles again
        #   addr_lookahead_1 (key end_range + 1)        visible only after first TopUp
        #   addr_lookahead_2 (key end_range + KEYPOOL_SIZE + 1) visible only after second TopUp
        #
        # vtx order: [lookahead_2, lookahead_1, expand_1, expand_2]
        #   Pass 1: lookahead_2 and lookahead_1 missed; expand_1 and expand_2 found;
        #           pool expands twice during this single pass.
        #   Pass 2: both lookaheads now in pool → found.
        # -----------------------------------------------------------------------
        self.log.info("Part 4: cascading pool expansions — two TopUp levels in one block")
        node.createwallet(wallet_name='w_cascade', disable_private_keys=True, blank=True)
        w_cascade = node.get_wallet_rpc('w_cascade')

        # Import the same descriptor into a blank wallet so pool starts at [0, end_range].
        # Use timestamp=now so the rescan only covers blocks from this point forward;
        # mine the cascade block after import.
        w_cascade.importdescriptors([{
            "desc": recv_desc['desc'],
            "timestamp": "now",
        }])

        _, cascade_end = w_cascade.listdescriptors()['descriptors'][0]['range']

        i_e1 = cascade_end
        i_e2 = cascade_end + KEYPOOL_SIZE
        i_l1 = cascade_end + 1
        i_l2 = cascade_end + KEYPOOL_SIZE + 1
        addr_expand_1    = w_cascade.deriveaddresses(recv_desc['desc'], [i_e1, i_e1])[0]
        addr_expand_2    = w_cascade.deriveaddresses(recv_desc['desc'], [i_e2, i_e2])[0]
        addr_lookahead_1 = w_cascade.deriveaddresses(recv_desc['desc'], [i_l1, i_l1])[0]
        addr_lookahead_2 = w_cascade.deriveaddresses(recv_desc['desc'], [i_l2, i_l2])[0]

        self.log.info(f"Cascade pool:      [0, {cascade_end}]")
        self.log.info(f"addr_expand_1      (index {cascade_end}):                  {addr_expand_1}")
        self.log.info(f"addr_expand_2      (index {cascade_end + KEYPOOL_SIZE}):   {addr_expand_2}")
        self.log.info(f"addr_lookahead_1   (index {cascade_end + 1}):              {addr_lookahead_1}")
        self.log.info(f"addr_lookahead_2   (index {cascade_end + KEYPOOL_SIZE + 1}): {addr_lookahead_2}")

        node.unloadwallet('w_cascade')

        tx_c_lookahead_2 = funding_wallet.send_to(
            from_node=node,
            scriptPubKey=address_to_scriptpubkey(addr_lookahead_2),
            amount=10_000,
        )
        tx_c_lookahead_1 = funding_wallet.send_to(
            from_node=node,
            scriptPubKey=address_to_scriptpubkey(addr_lookahead_1),
            amount=10_000,
        )
        tx_c_expand_1 = funding_wallet.send_to(
            from_node=node,
            scriptPubKey=address_to_scriptpubkey(addr_expand_1),
            amount=10_000,
        )
        tx_c_expand_2 = funding_wallet.send_to(
            from_node=node,
            scriptPubKey=address_to_scriptpubkey(addr_expand_2),
            amount=10_000,
        )

        # Mine with both lookaheads before both expanders.
        # expand_1 fires TopUp (pool level 1); expand_2 is then in pool and fires
        # TopUp again (pool level 2). Both lookaheads are only coverable after
        # the second expansion — requiring at least two scan passes.
        self.generateblock(node, output="raw(51)", transactions=[
            tx_c_lookahead_2['txid'],
            tx_c_lookahead_1['txid'],
            tx_c_expand_1['txid'],
            tx_c_expand_2['txid'],
        ])

        node.loadwallet('w_cascade')
        w_cascade = node.get_wallet_rpc('w_cascade')
        node.syncwithvalidationinterfacequeue()

        txids_cascade = {tx['txid'] for tx in w_cascade.listtransactions('*', 1000)}
        assert tx_c_expand_1['txid'] in txids_cascade, \
            "expand_1 must be found (it was within the initial pool)"
        assert tx_c_expand_2['txid'] in txids_cascade, \
            "expand_2 must be found (it enters pool after expand_1 fires TopUp)"
        assert tx_c_lookahead_1['txid'] in txids_cascade, \
            "lookahead_1 must be found after fix: visible after first TopUp"
        assert tx_c_lookahead_2['txid'] in txids_cascade, \
            "lookahead_2 must be found after fix: visible after second TopUp"


if __name__ == '__main__':
    WalletRescanIntraBlockOrderingTest(__file__).main()
