#!/usr/bin/env python3
# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that fast rescan using block filters for descriptor wallets detects
   top-ups correctly and finds the same transactions than the slow variant."""
from test_framework.address import address_to_scriptpubkey
from test_framework.descriptors import descsum_create
from test_framework.test_framework import BitcoinTestFramework
from test_framework.test_node import TestNode
from test_framework.util import assert_equal, assert_greater_than
from test_framework.wallet import MiniWallet
from test_framework.wallet_util import get_generate_key


KEYPOOL_SIZE = 100   # smaller than default size to speed-up test
NUM_BLOCKS = 7       # number of blocks to mine


class WalletFastRescanTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [[f'-keypool={KEYPOOL_SIZE}', '-blockfilterindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def get_wallet_txids(self, node: TestNode, wallet_name: str) -> list[str]:
        w = node.get_wallet_rpc(wallet_name)
        txs = w.listtransactions('*', 1000000)
        return [tx['txid'] for tx in txs]

    def run_test(self):
        node = self.nodes[0]
        funding_wallet = MiniWallet(node)

        self.log.info("Create descriptor wallet, import non-range descriptor, and backup wallet")
        WALLET_BACKUP_FILENAME = node.datadir_path / 'wallet.bak'
        node.createwallet(wallet_name='topup_test')
        w = node.get_wallet_rpc('topup_test')
        fixed_key = get_generate_key()
        w.importdescriptors([{"desc": descsum_create(f"wpkh({fixed_key.privkey})"), "timestamp": "now"}])

        descriptors = w.listdescriptors()['descriptors']
        non_ranged_descs = [desc for desc in descriptors if 'range' not in desc]
        assert_equal(len(non_ranged_descs), 1)

        # backup the wallet here so that the restorations later are unaware of the below transactions
        w.backupwallet(WALLET_BACKUP_FILENAME)

        def get_descriptor_end_range(desc_str):
            for descriptor in w.listdescriptors()['descriptors']:
                if descriptor['desc'] == desc_str:
                    return descriptor['range'][1]
            raise AssertionError(f"Descriptor not found: {desc_str}")

        txids_per_block = []

        self.log.info("Create and broadcast tx sending to non-ranged descriptor")
        addr = w.deriveaddresses(non_ranged_descs[0]['desc'])[0]
        spk = address_to_scriptpubkey(addr)
        self.log.debug(f"-> fixed non-range descriptor address {addr}")
        send_result = funding_wallet.send_to(from_node=node, scriptPubKey=spk, amount=10000)
        txids_per_block.append([send_result["txid"]])

        self.log.info("Create and broadcast txs sending to end range address of each descriptor, triggering top-ups")
        for i in range(1, NUM_BLOCKS-1):
            block_tx_ids = []
            # Get descriptors with updated ranges
            ranged_descs = [desc for desc in w.listdescriptors()['descriptors'] if 'range' in desc]

            for desc_info in ranged_descs:
                desc_str = desc_info['desc']
                start_range, end_range = desc_info['range']
                addr = w.deriveaddresses(desc_str, [end_range, end_range])[0]
                spk = address_to_scriptpubkey(addr)

                self.log.debug(f"-> range [{start_range},{end_range}], last address {addr}")
                send_result = funding_wallet.send_to(from_node=node, scriptPubKey=spk, amount=10000)
                assert_greater_than(get_descriptor_end_range(desc_str), end_range)
                block_tx_ids.append(send_result["txid"])

            txids_per_block.append(block_tx_ids)

        # wallet w (topup_test) is not required to be loaded from here on, unload so that it
        # doesn't needlessly process block generation notifications in the background
        w.unloadwallet()

        self.log.info("Create and broadcast tx unrelated to the wallet")
        send_result = funding_wallet.send_self_transfer(from_node=node)
        txids_per_block.append([send_result['txid']])

        self.log.info("Start generating blocks one by one with associated txs")
        expected_wallet_txids = []
        fast_rescan_messages = []
        for i, block_tx_ids in enumerate(txids_per_block):
            self.log.info(f"Block {i+1}/{NUM_BLOCKS}")
            generated_block = self.generateblock(node, output="raw(42)", transactions=block_tx_ids)
            # Not asserting the fast rescan message in the last block because it contains wallet-unrelated
            # transactions - this block may or may not be fetched due to block filters false positives.
            if i < NUM_BLOCKS-1:
                fast_rescan_messages.append(f"Fast rescan: inspect block {self.nodes[0].getblockcount()} [{generated_block['hash']}] (filter matched)")
                expected_wallet_txids.extend(block_tx_ids)

        self.log.info("Import wallet backup with block filter index")
        with node.assert_debug_log(['fast variant using block filters', *fast_rescan_messages]):
            node.restorewallet('rescan_fast', WALLET_BACKUP_FILENAME)
        txids_fast = self.get_wallet_txids(node, 'rescan_fast')

        self.log.info("Import non-active descriptors with block filter index")
        node.createwallet(wallet_name='rescan_fast_nonactive', disable_private_keys=True, blank=True)
        with node.assert_debug_log(['fast variant using block filters', *fast_rescan_messages]):
            w = node.get_wallet_rpc('rescan_fast_nonactive')
            w.importdescriptors([{"desc": descriptor['desc'], "timestamp": 0} for descriptor in descriptors])
        txids_fast_nonactive = self.get_wallet_txids(node, 'rescan_fast_nonactive')

        self.restart_node(0, [f'-keypool={KEYPOOL_SIZE}', '-blockfilterindex=0'])
        self.log.info("Import wallet backup w/o block filter index")
        with node.assert_debug_log(['slow variant inspecting all blocks']):
            node.restorewallet("rescan_slow", WALLET_BACKUP_FILENAME)
        txids_slow = self.get_wallet_txids(node, 'rescan_slow')

        self.log.info("Import non-active descriptors w/o block filter index")
        node.createwallet(wallet_name='rescan_slow_nonactive', disable_private_keys=True, blank=True)
        with node.assert_debug_log(['slow variant inspecting all blocks']):
            w = node.get_wallet_rpc('rescan_slow_nonactive')
            w.importdescriptors([{"desc": descriptor['desc'], "timestamp": 0} for descriptor in descriptors])
        txids_slow_nonactive = self.get_wallet_txids(node, 'rescan_slow_nonactive')

        self.log.info("Verify that all rescans found the same txs in slow and fast variants")
        expected_wallet_txids.sort()
        assert_equal(sorted(txids_slow), expected_wallet_txids)
        assert_equal(sorted(txids_fast), expected_wallet_txids)
        assert_equal(sorted(txids_slow_nonactive), expected_wallet_txids)
        assert_equal(sorted(txids_fast_nonactive), expected_wallet_txids)


if __name__ == '__main__':
    WalletFastRescanTest(__file__).main()
