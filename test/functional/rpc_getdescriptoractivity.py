#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import COIN
from test_framework.wallet import MiniWallet, getnewdestination


class GetBlocksActivityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        node.setmocktime(node.getblockheader(node.getbestblockhash())['time'])
        wallet.generate(200, invalid_call=False)

        self.test_no_activity(node)
        self.test_activity_in_block(node, wallet)
        self.test_no_mempool_inclusion(node, wallet)
        self.test_multiple_addresses(node, wallet)
        self.test_invalid_blockhash(node, wallet)
        self.test_confirmed_and_unconfirmed(node, wallet)
        self.test_receive_then_spend(node, wallet)

    def test_no_activity(self, node):
        _, spk_1, addr_1 = getnewdestination()
        result = node.getdescriptoractivity([], [f"addr({addr_1})"], True)
        assert_equal(len(result['activity']), 0)

    def test_activity_in_block(self, node, wallet):
        _, spk_1, addr_1 = getnewdestination()
        txid = wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)['txid']
        blockhash = self.generate(node, 1)[0]

        # Test getdescriptoractivity with the specific blockhash
        result = node.getdescriptoractivity([blockhash], [f"addr({addr_1})"], True)

        for k, v in {
                'address': addr_1,
                'amount': Decimal('1.00000000'),
                'blockhash': blockhash,
                'desc': 'rawtr',  # partial
                'height': 201,
                'scriptpubkey_hex': spk_1.hex(),
                'txid': txid,
                'type': 'receive',
                'vout': 1,
        }.items():
            if k == 'desc':
                assert_equal(result['activity'][0][k].split('(')[0], v)
            else:
                assert_equal(result['activity'][0][k], v)


    def test_no_mempool_inclusion(self, node, wallet):
        _, spk_1, addr_1 = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)

        _, spk_2, addr_2 = getnewdestination()
        wallet.send_to(
            from_node=node, scriptPubKey=spk_2, amount=1 * COIN)

        # Do not generate a block to keep the transaction in the mempool

        result = node.getdescriptoractivity([], [f"addr({addr_1})", f"addr({addr_2})"], False)

        assert_equal(len(result['activity']), 0)

    def test_multiple_addresses(self, node, wallet):
        _, spk_1, addr_1 = getnewdestination()
        _, spk_2, addr_2 = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)
        wallet.send_to(from_node=node, scriptPubKey=spk_2, amount=2 * COIN)

        blockhash = self.generate(node, 1)[0]

        result = node.getdescriptoractivity([blockhash], [f"addr({addr_1})", f"addr({addr_2})"], True)

        assert_equal(len(result['activity']), 2)

        [a1] = [a for a in result['activity'] if a['address'] == addr_1]
        [a2] = [a for a in result['activity'] if a['address'] == addr_2]

        assert a1['blockhash'] == blockhash
        assert a1['amount'] == 1.0

        assert a2['blockhash'] == blockhash
        assert a2['amount'] == 2.0

    def test_invalid_blockhash(self, node, wallet):
        self.generate(node, 20) # Generate to get more fees

        _, spk_1, addr_1 = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)

        invalid_blockhash = "0000000000000000000000000000000000000000000000000000000000000000"

        assert_raises_rpc_error(
            -5, "Block not found",
            node.getdescriptoractivity, [invalid_blockhash], [f"addr({addr_1})"], True)

    def test_confirmed_and_unconfirmed(self, node, wallet):
        self.generate(node, 20) # Generate to get more fees

        _, spk_1, addr_1 = getnewdestination()
        txid_1 = wallet.send_to(
            from_node=node, scriptPubKey=spk_1, amount=1 * COIN)['txid']
        blockhash = self.generate(node, 1)[0]

        _, spk_2, to_addr = getnewdestination()
        txid_2 = wallet.send_to(
            from_node=node, scriptPubKey=spk_2, amount=1 * COIN)['txid']

        result = node.getdescriptoractivity(
            [blockhash], [f"addr({addr_1})", f"addr({to_addr})"], True)

        activity = result['activity']
        assert_equal(len(activity), 2)

        [confirmed] = [a for a in activity if a['blockhash'] == blockhash]
        assert confirmed['txid'] == txid_1
        assert confirmed['height'] == node.getblockchaininfo()['blocks']

        assert any(a['txid'] == txid_2 for a in activity if a['blockhash'] == "")

    def test_receive_then_spend(self, node, wallet):
        self.generate(node, 20) # Generate to get more fees

        sent1 = wallet.send_self_transfer(from_node=node)
        utxo = sent1['new_utxo']
        blockhash_1 = self.generate(node, 1)[0]

        sent2 = wallet.send_self_transfer(from_node=node, utxo_to_spend=utxo)
        blockhash_2 = self.generate(node, 1)[0]

        result = node.getdescriptoractivity(
            [blockhash_1, blockhash_2], [wallet.get_descriptor()], True)

        assert_equal(len(result['activity']), 4)

        assert result['activity'][1]['type'] == 'receive'
        assert result['activity'][1]['txid'] == sent1['txid']
        assert result['activity'][1]['blockhash'] == blockhash_1

        assert result['activity'][2]['type'] == 'spend'
        assert result['activity'][2]['spend_txid'] == sent2['txid']
        assert result['activity'][2]['prevout_txid'] == sent1['txid']
        assert result['activity'][2]['blockhash'] == blockhash_2


if __name__ == '__main__':
    GetBlocksActivityTest(__file__).main()
