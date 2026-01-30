#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from decimal import Decimal

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from test_framework.messages import COIN
from test_framework.wallet import MiniWallet, MiniWalletMode, getnewdestination


class GetBlocksActivityTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        node.setmocktime(node.getblockheader(node.getbestblockhash())['time'])
        self.generate(wallet, 200)

        self.test_no_activity(node)
        self.test_activity_in_block(node, wallet)
        self.test_no_mempool_inclusion(node, wallet)
        self.test_multiple_addresses(node, wallet)
        self.test_invalid_blockhash(node, wallet)
        self.test_invalid_descriptor(node, wallet)
        self.test_confirmed_and_unconfirmed(node, wallet)
        self.test_receive_then_spend(node, wallet)
        self.test_no_address(node, wallet)
        self.test_required_args(node)

    def test_no_activity(self, node):
        self.log.info("Test that no activity is found for an unused address")
        _, _, addr_1 = getnewdestination()
        result = node.getdescriptoractivity([], [f"addr({addr_1})"], True)
        assert_equal(len(result['activity']), 0)

    def test_activity_in_block(self, node, wallet):
        self.log.info("Test that receive activity is correctly reported in a mined block")
        _, spk_1, addr_1 = getnewdestination(address_type='bech32m')
        txid = wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)['txid']
        blockhash = self.generate(node, 1)[0]

        # Test getdescriptoractivity with the specific blockhash
        result = node.getdescriptoractivity([blockhash], [f"addr({addr_1})"], True)
        assert_equal(list(result.keys()), ['activity'])
        [activity] = result['activity']

        for k, v in {
                'amount': Decimal('1.00000000'),
                'blockhash': blockhash,
                'height': 201,
                'txid': txid,
                'type': 'receive',
                'vout': 1,
        }.items():
            assert_equal(activity[k], v)

        outspk = activity['output_spk']

        assert_equal(outspk['asm'][:2], '1 ')
        assert_equal(outspk['desc'].split('(')[0], 'rawtr')
        assert_equal(outspk['hex'], spk_1.hex())
        assert_equal(outspk['address'], addr_1)
        assert_equal(outspk['type'], 'witness_v1_taproot')


    def test_no_mempool_inclusion(self, node, wallet):
        self.log.info("Test that mempool transactions are not included when include_mempool argument is False")
        _, spk_1, addr_1 = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)

        _, spk_2, addr_2 = getnewdestination()
        wallet.send_to(
            from_node=node, scriptPubKey=spk_2, amount=1 * COIN)

        # Do not generate a block to keep the transaction in the mempool

        result = node.getdescriptoractivity([], [f"addr({addr_1})", f"addr({addr_2})"], False)

        assert_equal(len(result['activity']), 0)

    def test_multiple_addresses(self, node, wallet):
        self.log.info("Test querying multiple addresses returns all activity correctly")
        _, spk_1, addr_1 = getnewdestination()
        _, spk_2, addr_2 = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)
        wallet.send_to(from_node=node, scriptPubKey=spk_2, amount=2 * COIN)

        blockhash = self.generate(node, 1)[0]

        result = node.getdescriptoractivity([blockhash], [f"addr({addr_1})", f"addr({addr_2})"], True)

        assert_equal(len(result['activity']), 2)

        # Duplicate address specification is fine.
        assert_equal(
            result,
            node.getdescriptoractivity([blockhash], [
                f"addr({addr_1})", f"addr({addr_1})", f"addr({addr_2})"], True))

        # Flipping descriptor order doesn't affect results.
        result_flipped = node.getdescriptoractivity(
            [blockhash], [f"addr({addr_2})", f"addr({addr_1})"], True)
        assert_equal(result, result_flipped)

        [a1] = [a for a in result['activity'] if a['output_spk']['address'] == addr_1]
        [a2] = [a for a in result['activity'] if a['output_spk']['address'] == addr_2]

        assert a1['blockhash'] == blockhash
        assert a1['amount'] == 1.0

        assert a2['blockhash'] == blockhash
        assert a2['amount'] == 2.0

    def test_invalid_blockhash(self, node, wallet):
        self.log.info("Test that passing an invalid blockhash raises appropriate RPC error")
        self.generate(node, 20) # Generate to get more fees

        _, spk_1, addr_1 = getnewdestination()
        wallet.send_to(from_node=node, scriptPubKey=spk_1, amount=1 * COIN)

        invalid_blockhash = "0000000000000000000000000000000000000000000000000000000000000000"

        assert_raises_rpc_error(
            -5, "Block not found",
            node.getdescriptoractivity, [invalid_blockhash], [f"addr({addr_1})"], True)

    def test_invalid_descriptor(self, node, wallet):
        self.log.info("Test that an invalid descriptor raises the correct RPC error")
        blockhash = self.generate(node, 1)[0]
        _, _, addr_1 = getnewdestination()

        assert_raises_rpc_error(
            -5, "is not a valid descriptor",
            node.getdescriptoractivity, [blockhash], [f"addrx({addr_1})"], True)

    def test_confirmed_and_unconfirmed(self, node, wallet):
        self.log.info("Test that both confirmed and unconfirmed transactions are reported correctly")
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

        [confirmed] = [a for a in activity if a.get('blockhash') == blockhash]
        assert confirmed['txid'] == txid_1
        assert confirmed['height'] == node.getblockchaininfo()['blocks']

        [unconfirmed] = [a for a in activity if not a.get('blockhash')]
        assert 'blockhash' not in unconfirmed
        assert 'height' not in unconfirmed

        assert any(a['txid'] == txid_2 for a in activity if not a.get('blockhash'))

    def test_receive_then_spend(self, node, wallet):
        """Also important because this tests multiple blockhashes."""
        self.log.info("Test receive and spend activities across different blocks are reported consistently")
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
        assert result['activity'][2]['spend_vin'] == 0
        assert result['activity'][2]['prevout_txid'] == sent1['txid']
        assert result['activity'][2]['blockhash'] == blockhash_2

        # Test that reversing the blockorder yields the same result.
        assert_equal(result, node.getdescriptoractivity(
            [blockhash_1, blockhash_2], [wallet.get_descriptor()], True))

        self.log.info("Test that duplicated blockhash request does not report duplicated results")
        # Test that duplicating a blockhash yields the same result.
        assert_equal(result, node.getdescriptoractivity(
            [blockhash_1, blockhash_2, blockhash_2], [wallet.get_descriptor()], True))

    def test_no_address(self, node, wallet):
        self.log.info("Test that activity is still reported for scripts without an associated address")
        raw_wallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_P2PK)
        self.generate(raw_wallet, 100)

        no_addr_tx = raw_wallet.send_self_transfer(from_node=node)
        raw_desc = raw_wallet.get_descriptor()

        blockhash = self.generate(node, 1)[0]

        result = node.getdescriptoractivity([blockhash], [raw_desc], False)

        assert_equal(len(result['activity']), 2)

        a1 = result['activity'][0]
        a2 = result['activity'][1]

        assert a1['type'] == "spend"
        assert a1['blockhash'] == blockhash
        # sPK lacks address.
        assert_equal(list(a1['prevout_spk'].keys()), ['asm', 'desc', 'hex', 'type'])
        assert a1['amount'] == no_addr_tx["fee"] + Decimal(no_addr_tx["tx"].vout[0].nValue) / COIN

        assert a2['type'] == "receive"
        assert a2['blockhash'] == blockhash
        # sPK lacks address.
        assert_equal(list(a2['output_spk'].keys()), ['asm', 'desc', 'hex', 'type'])
        assert a2['amount'] == Decimal(no_addr_tx["tx"].vout[0].nValue) / COIN

    def test_required_args(self, node):
        self.log.info("Test that required arguments must be passed")
        assert_raises_rpc_error(-1, "getdescriptoractivity", node.getdescriptoractivity)
        assert_raises_rpc_error(-1, "getdescriptoractivity", node.getdescriptoractivity, [])


if __name__ == '__main__':
    GetBlocksActivityTest(__file__).main()
