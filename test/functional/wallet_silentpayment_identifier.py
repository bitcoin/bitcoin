#!/usr/bin/env python3
from test_framework.test_framework import BitcoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.util import (
    assert_equal,
)

import random
import string


class SilentIdentifierTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [["-txindex=1"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def init_wallet(self, *, node):
        pass

    def random_string(self, n):
        return ''.join(random.choice(string.ascii_lowercase) for _ in range(n))

    def test_identifier(self):
        self.nodes[0].createwallet(wallet_name=f'sender_wallet', descriptors=True)
        sender_wallet = self.nodes[0].get_wallet_rpc(f'sender_wallet')

        self.generatetoaddress(self.nodes[0], COINBASE_MATURITY + 10, sender_wallet.getnewaddress())

        self.nodes[0].createwallet(wallet_name=f'recipient_wallet_01', descriptors=True, silent_payment=True)
        recipient_wallet_01 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_01')

        self.nodes[0].createwallet(wallet_name=f'recipient_wallet_02', descriptors=True, silent_payment=True)
        recipient_wallet_02 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_02')

        self.nodes[0].createwallet(wallet_name=f'recipient_wallet_03', descriptors=True, silent_payment=True)
        recipient_wallet_03 = self.nodes[0].get_wallet_rpc(f'recipient_wallet_03')

        label01 = self.random_string(8)
        recv_addr_01 = recipient_wallet_01.getsilentaddress()['address']
        recv_addr_02 = recipient_wallet_01.getsilentaddress(label01)['address']

        label02 = self.random_string(8)
        recv_addr_03 = recipient_wallet_02.getsilentaddress()['address']
        recv_addr_04 = recipient_wallet_02.getsilentaddress(label02)['address']

        # generate a large index
        recipient_wallet_03.getsilentaddress()['address']
        for _ in range(85):
            recipient_wallet_03.getsilentaddress(self.random_string(8))['address']

        label03 = self.random_string(8)
        recv_addr_05 = recipient_wallet_03.getsilentaddress(label03)['address']

        outputs = [{recv_addr_01: 3}, {recv_addr_02: 2}, {recv_addr_03: 4}, {recv_addr_04: 6},
            {recv_addr_05: 7}]

        sender_wallet.send(outputs=outputs)

        self.generatetoaddress(self.nodes[0], 1, sender_wallet.getnewaddress())

        recipient_wallet_01_utxos = recipient_wallet_01.listunspent()
        assert_equal(len(recipient_wallet_01_utxos), 2)
        assert label01 in [utxo['label'] for utxo in recipient_wallet_01_utxos]

        recipient_wallet_02_utxos = recipient_wallet_02.listunspent()
        assert_equal(len(recipient_wallet_02_utxos), 2)
        assert label02 in [utxo['label'] for utxo in recipient_wallet_02_utxos]

        recipient_wallet_03_utxos = recipient_wallet_03.listunspent()
        assert_equal(len(recipient_wallet_03_utxos), 1)
        assert label03 in [utxo['label'] for utxo in recipient_wallet_03_utxos]

        # Test if importing wallet descriptors will fetch all UTXOs
        desc_to_import = []

        for wallet in [recipient_wallet_01, recipient_wallet_02, recipient_wallet_03]:
            for desc in wallet.listdescriptors(True)['descriptors']:
                if (desc['desc'].startswith("sp(")):
                    desc_to_import.append(
                        {"desc": desc['desc'],
                        "timestamp": desc['timestamp'],
                        "next_index": desc["next"],
                        "active": True}
                    )

        self.nodes[0].createwallet(wallet_name='backup_wallet', descriptors=True, blank=True)
        backup_wallet = self.nodes[0].get_wallet_rpc('backup_wallet')

        for desc in desc_to_import:
            result = backup_wallet.importdescriptors([desc])
            assert_equal(result[0]['success'], True)

        backup_wallet_utxos = backup_wallet.listunspent()
        assert_equal(len(backup_wallet_utxos), 5)

        # Wallets with imported descriptors have no way of knowing what the original labels are,
        # so they use the identifier as labels
        backup_wallet_utxos = [utxo['label'] for utxo in backup_wallet_utxos]
        assert '0' in backup_wallet_utxos
        assert '1' in backup_wallet_utxos
        assert '86' in backup_wallet_utxos

        # Test scantxoutset using identifiers
        desc_param =  [{"desc": desc['desc'], "range": desc['next_index']} for desc in desc_to_import]
        result = self.nodes[0].scantxoutset("start", desc_param)
        assert result['success']
        assert_equal(len(result['unspents']), 5)

    def run_test(self):
        self.test_identifier()


if __name__ == '__main__':
    SilentIdentifierTest().main()
