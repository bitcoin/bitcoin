#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.util import assert_equal


class SilentTransactioTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def init_wallet(self, *, node):
        pass

    def test_transactions(self):
        for input_type in ['bech32m', 'bech32', 'p2sh-segwit', 'legacy']:

            self.nodes[0].createwallet(wallet_name=f'sender_wallet_{input_type}', descriptors=True)
            sender_wallet = self.nodes[0].get_wallet_rpc(f'sender_wallet_{input_type}')

            self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, sender_wallet.getnewaddress())

            self.nodes[1].createwallet(wallet_name=f'recipient_wallet_01_{input_type}', descriptors=True, silent_payment=True)
            recipient_wallet_01 = self.nodes[1].get_wallet_rpc(f'recipient_wallet_01_{input_type}')

            self.nodes[2].createwallet(wallet_name=f'recipient_wallet_02_{input_type}', descriptors=True, silent_payment=True)
            recipient_wallet_02 = self.nodes[2].get_wallet_rpc(f'recipient_wallet_02_{input_type}')

            # sender wallet sends coins to itself using input_type address
            # The goal is to use the output of this tx as silent transaction inputs
            sen_addr_02 = sender_wallet.getnewaddress('', input_type)
            tx_id_01 = sender_wallet.send({sen_addr_02: 50})['txid']

            self.generate(self.nodes[0], 7)

            tx_01 = [x for x in sender_wallet.listunspent(0) if x['txid'] == tx_id_01]
            input_data = [x for x in tx_01 if x['address'] == sen_addr_02][0]

            # use two addresses so that one can be spent on a normal tx and the other on a normal transaction
            recv_addr_01 = recipient_wallet_01.getsilentaddress()['address']
            recv_addr_02 = recipient_wallet_01.getnewaddress('', input_type)
            recv_addr_03 = recipient_wallet_02.getsilentaddress()['address']

            self.log.info("[%s] create silent transaction", input_type)
            inputs = [{"txid": input_data["txid"], "vout":input_data["vout"]}]
            outputs = [{recv_addr_01: 15}, {recv_addr_02: 15}, {recv_addr_03: 15}]
            options= {"inputs": inputs }

            silent_tx_ret = sender_wallet.send(outputs=outputs, options=options)

            self.sync_mempools()

            wallet_01_utxos = [x for x in recipient_wallet_01.listunspent(0) if x['txid'] == silent_tx_ret['txid']]

            silent_txid = ''
            silent_vout = ''

            self.log.info("[%s] confirm that transaction has a different address than the original", input_type)
            assert_equal(len(wallet_01_utxos), 2)
            for utxo in wallet_01_utxos:
                if utxo['desc'].startswith('rawtr('):
                    silent_txid = utxo["txid"]
                    silent_vout = utxo["vout"]
                    assert utxo['address'] != recv_addr_01
                else:
                    assert utxo['address'] == recv_addr_02

            recv_addr_04 = recipient_wallet_02.getnewaddress('', input_type)

            self.log.info("[%s] spend the silent output to a normal address", input_type)
            normal_inputs = [{"txid": silent_txid, "vout":silent_vout}]
            normal_outputs = [{recv_addr_04: 10}]
            normal_options = {"inputs": normal_inputs}

            normal_tx_ret = recipient_wallet_01.send(outputs=normal_outputs, options=normal_options)

            self.log.info("[%s] spend the silent output to another", input_type)
            wallet_02_utxos = [x for x in recipient_wallet_02.listunspent(0) if x['txid'] == silent_tx_ret['txid']]
            assert_equal(len(wallet_02_utxos), 1)
            assert wallet_02_utxos[0]['desc'].startswith('rawtr(')
            assert wallet_02_utxos[0]['address'] != recv_addr_03

            recv_addr_05 = recipient_wallet_01.getsilentaddress()['address']
            assert_equal(recv_addr_01, recv_addr_05)

            silent_inputs = [{"txid": wallet_02_utxos[0]["txid"], "vout":wallet_02_utxos[0]["vout"]}]
            silent_outputs = [{recv_addr_05: 10}]
            silent_options = {"inputs": silent_inputs}

            silent_tx_ret = recipient_wallet_02.send(outputs=silent_outputs, options=silent_options)

            self.sync_mempools()

            normal_tx = [x for x in recipient_wallet_02.listunspent(0) if x['txid'] == normal_tx_ret['txid']][0]
            silent_tx = [x for x in recipient_wallet_01.listunspent(0) if x['txid'] == silent_tx_ret['txid']][0]

            self.log.info("[%s] confirm the silent output was spent correctly", input_type)
            assert normal_tx['address'] == recv_addr_04

            assert silent_tx['address'] != recv_addr_05
            assert silent_tx['desc'].startswith('rawtr(')

    def run_test(self):
        self.test_transactions()


if __name__ == '__main__':
    SilentTransactioTest().main()
