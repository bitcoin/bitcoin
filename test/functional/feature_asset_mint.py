#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
import time
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class AssetMintTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [['-assetindex=1','-mocktime=1594359054'],['-assetindex=1','-mocktime=1594359054']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()


    def run_test(self):
        self.generate(self.nodes[0], 200)
        self.sync_blocks()
        self.asset = '123456'
        spv_tx_root = "bf2fe1a8c7401fe41cd90e351d7a7b0146912e9e5a1bab17f80a602021793e60"
        spv_tx_parent_nodes = "f90147f90144822080b9013e02f9013a82164403843b9aca00843b9bcd9283061a8094a738a563f9ecb55e0b2245d1e9e380f0fe455ea1880de0b6b3a7640000b8c454c988ff0000000000000000000000000000000000000000000000000de0b6b3a7640000000000000000000000000000000000000000000000000000000000000001e2400000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000002c74737973317135736a6475717768646672727a6639656a70633572336867676761307930616e6138656178350000000000000000000000000000000000000000c001a04d4a9b05051a6cc837949f85f5707d8912b29dbe8182bd54cf8c345eaed16325a05f0b4a46d4d2daa5353ab551364e1bd13c5a5e052f17f45dab9f58d7b1d1232c"
        spv_tx_value = "f9013a82164403843b9aca00843b9bcd9283061a8094a738a563f9ecb55e0b2245d1e9e380f0fe455ea1880de0b6b3a7640000b8c454c988ff0000000000000000000000000000000000000000000000000de0b6b3a7640000000000000000000000000000000000000000000000000000000000000001e2400000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000002c74737973317135736a6475717768646672727a6639656a70633572336867676761307930616e6138656178350000000000000000000000000000000000000000c001a04d4a9b05051a6cc837949f85f5707d8912b29dbe8182bd54cf8c345eaed16325a05f0b4a46d4d2daa5353ab551364e1bd13c5a5e052f17f45dab9f58d7b1d1232c"
        spv_tx_path = "80"
        spv_receipt_root = "a6204bb80f67dc0e740514aef7ccf411c0f30d29b795b1eaa6366c0568c07c6e"
        spv_receipt_parent_nodes = "f901d1f901ce822080b901c802f901c401826cadb9010000000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000008000000000000000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000f8bbf8b994a738a563f9ecb55e0b2245d1e9e380f0fe455ea1e1a07ca654cf9212e4c3cf0164a529dd6159fc71113f867d0b09fdeb10aa65780732b880000000000000000000000000000000000000000000000000000000000001e240000000000000000000000000bf76b51ddfbe584b92d039c95f6444fabc8956a60000000000000000000000000000000000000000000000000de0b6b3a76400000000000000000000000000000000000000000000000000000000000800000012"
        spv_receipt_value = "f901c401826cadb9010000000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000400000000000000000008000000000000000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000100000000f8bbf8b994a738a563f9ecb55e0b2245d1e9e380f0fe455ea1e1a07ca654cf9212e4c3cf0164a529dd6159fc71113f867d0b09fdeb10aa65780732b880000000000000000000000000000000000000000000000000000000000001e240000000000000000000000000bf76b51ddfbe584b92d039c95f6444fabc8956a60000000000000000000000000000000000000000000000000de0b6b3a76400000000000000000000000000000000000000000000000000000000000800000012"
        blockhash = '0xd232b110d7e64d23ec6f76a500db9465c1293ed450ef3fa8e634483a2900e84e'

        self.basic_asset()
        self.generate(self.nodes[0], 1)
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        self.sync_blocks()

        # Add eth root to DB so it an validate this SPV proof, do it on both nodes so they can verify the tx
        self.nodes[0].syscoinsetethheaders([[blockhash, spv_tx_root, spv_receipt_root]])
        self.nodes[1].syscoinsetethheaders([[blockhash, spv_tx_root, spv_receipt_root]])

        newaddress = self.nodes[0].getnewaddress()
        # try to enable aux fee which should throw an error of invalid value
        assert_raises_rpc_error(-4, 'mint-mismatch-value', self.nodes[0].assetallocationmint, self.asset, newaddress, 1, blockhash, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes, True)
        assert_raises_rpc_error(-4, 'mint-mismatch-value', self.nodes[0].assetallocationmint, self.asset, newaddress, 1.00001, blockhash, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes)
        txid = self.nodes[0].assetallocationmint(self.asset, newaddress, 1, blockhash, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes)['txid']
        time.sleep(0.25)
        # cannot mint twice
        assert_raises_rpc_error(-4, 'mint-duplicate-transfer', self.nodes[0].assetallocationmint, self.asset, newaddress, 1, blockhash, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes)
        assert_raises_rpc_error(-4, 'mint-duplicate-transfer', self.nodes[0].assetallocationmint, self.asset, newaddress, 1, blockhash, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes)
        self.generate(self.nodes[0], 1)
        self.sync_blocks()
        # after a block it should show a different exists error
        assert_raises_rpc_error(-4, 'mint-exists', self.nodes[0].assetallocationmint, self.asset, newaddress, 1, blockhash, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes)
        assert_raises_rpc_error(-4, 'mint-exists', self.nodes[0].assetallocationmint, self.asset, newaddress, 1, blockhash, spv_tx_value, spv_tx_root, spv_tx_parent_nodes, spv_tx_path, spv_receipt_value, spv_receipt_root, spv_receipt_parent_nodes)
        # ensure you can lookup the mint from the NEVM txid
        mintres = self.nodes[0].syscoincheckmint('36994a0f617c8f5e6fe3197962a40c85fa4cb9b6ff51c4b533a9a25f74d04dd9')
        assert_equal(mintres['txid'], txid)
    def basic_asset(self):
        auxfees = {'auxfee_address': self.nodes[0].getnewaddress(), 'fee_struct': [[0,0.01],[10,0.004],[250,0.002],[2500,0.0007],[25000,0.00007],[250000,0]]}
        self.nodes[0].assetnewtest(self.asset, '1', 'TST', 'asset description', '0x9f90b5093f35aeac5fbaeb591f9c9de8e2844a46', 8, 10000, 127, '', {}, auxfees)

if __name__ == '__main__':
    AssetMintTest().main()
