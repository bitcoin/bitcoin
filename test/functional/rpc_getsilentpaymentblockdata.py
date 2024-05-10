#!/usr/bin/env python3
# Copyright (c) 2023-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test getsilentpaymentblockdata rpc call
#

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.blocktools import (
    TIME_GENESIS_BLOCK,
)
from test_framework.descriptors import (
    descsum_create,
)

class GetsilentpaymentblockdataTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def set_test_params(self):
        self.num_nodes = 1

        self.extra_args = [[
            '-bip352index',
        ]]

        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        mocktime = 1702294115
        node.setmocktime(mocktime)
        node.createwallet(wallet_name="w", blank=True)
        wallet = self.nodes[0].get_wallet_rpc("w")
        res = wallet.importdescriptors([{
            'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/84h/1h/0h/0/*)'),
            'timestamp': TIME_GENESIS_BLOCK,
            'active': True,
        },
        {
            'desc': descsum_create('wpkh(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/84h/1h/0h/1/*)'),
            'timestamp': TIME_GENESIS_BLOCK,
            'active': True,
            'internal': True,
        },
        {
            'desc': descsum_create('tr(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/86h/1h/0h/0/*)'),
            'timestamp': TIME_GENESIS_BLOCK,
            'active': True,
        },
        {
            'desc': descsum_create('tr(tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg/86h/1h/0h/1/*)'),
            'timestamp': TIME_GENESIS_BLOCK,
            'active': True,
            'internal': True,
        }
        ])
        assert all([r["success"] for r in res])
        self.log.info("Mine fresh coins to a taproot addresses")
        mine_tr = wallet.getnewaddress(address_type="bech32m")
        self.generatetoaddress(node, 1, mine_tr)
        self.generate(node, 100)

        self.log.info("Blocks with only a coinbase won't have any silent payment data")
        silent_data = node.getsilentpaymentblockdata(node.getbestblockhash())
        assert_equal(silent_data['bip352_tweaks'], [])

        self.log.info("Spending from taproot to segwit won't result in silent payment data")
        dest_sw = wallet.getnewaddress(address_type="bech32")
        txid = wallet.send(outputs={dest_sw: 1}, options={'change_position': 1})['txid']
        assert_equal(txid, 'bc0a188a78abaf3c512cb39c56918ffd45553055b5b8094f967bfa24d92075ca')
        self.generate(node, 1)

        silent_data = node.getsilentpaymentblockdata(node.getbestblockhash())
        assert_equal(silent_data['bip352_tweaks'], [])

        self.log.info("Spending (from taproot) to taproot results in silent payment data")
        dest_tr = wallet.getnewaddress(address_type="bech32m")
        wallet.send(outputs={dest_tr: 1}, options={'inputs': [{'txid': txid, 'vout': 1}], 'add_inputs': False, 'change_position': 1})
        self.generate(node, 1)
        silent_data = node.getsilentpaymentblockdata(node.getbestblockhash())
        assert_equal(silent_data['bip352_tweaks'], ['0386bbe197671ebf4a3e7dccf67c04fef7efdad0f999a12ac3b8197eb2e8a61f5c'])

if __name__ == '__main__':
    GetsilentpaymentblockdataTest().main()
