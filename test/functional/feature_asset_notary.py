#!/usr/bin/env python3
# Copyright (c) 2019-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error
from decimal import Decimal
class AssetNotaryTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        self.rpc_timeout = 240
        self.extra_args = [['-assetindex=1'],['-assetindex=1'],['-assetindex=1'],['-assetindex=1'],['-assetindex=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(200)
        self.basic_notary()
        self.complex_notary()
        self.notary_nft()

    def basic_notary(self):
        self.basic_asset()
        self.nodes[0].assetsend(self.asset, self.nodes[0].getnewaddress(), 2)
        self.nodes[0].generate(1)
        # will give back hex because notarization doesn't happen in assetallocationsend
        hextx = self.nodes[0].assetallocationsend(self.asset, self.nodes[0].getnewaddress(), 0.4)['hex']
        notarysig = self.nodes[0].signhash(self.notary_address, self.nodes[0].getnotarysighash(hextx, self.asset))
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx, self.asset, notarysig)['hex']
        tx_resigned = self.nodes[0].signrawtransactionwithwallet(hextx_notarized)['hex']
        assert_equal(len(hextx), len(hextx_notarized))
        assert(hextx != hextx_notarized)
        assert(tx_resigned != hextx_notarized)
        # cannot send without notarization
        assert_raises_rpc_error(-26, 'assetallocation-notary-sig', self.nodes[0].sendrawtransaction, hextx)
        assert_raises_rpc_error(-26, 'non-mandatory-script-verify-flag', self.nodes[0].sendrawtransaction, hextx_notarized)
        self.nodes[0].sendrawtransaction(tx_resigned)
        self.nodes[0].generate(1)
        # clear notary
        self.nodes[0].assetupdate(self.asset, '','', 127, '', {}, {})
        self.nodes[0].generate(1)
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert('notary_details' not in assetInfo)
        assert_equal(assetInfo['notary_address'], '')
        assert('txid' in self.nodes[0].assetallocationsend(self.asset, self.nodes[0].getnewaddress(), 0.4))

    def CreateAssetID(self, NFTID, nBaseAsset):
        assetId = (int(NFTID) << 32) | int(nBaseAsset)
        return str(assetId)

    # create two assets requiring notarization + 1 that does not require it and create NFT's sending NFT's along with base assets
    def notary_nft(self):
        self.basic_asset()
        self.notary_address1 = self.nodes[0].getnewaddress()
        notary = {'endpoint': 'https://jsonplaceholder.typicode.com/posts/', 'instant_transfers': True, 'hd_required': True}
        self.asset1 = self.nodes[0].assetnew('1', 'TST', 'asset description', '0x', 8, 10000, 127, self.notary_address1, notary, {})['asset_guid']
        self.asset2 = self.nodes[0].assetnew('1', 'TST', 'asset description', '0x', 8, 10000, 127)['asset_guid']
        self.nodes[0].generate(1)
        self.sync_blocks()
        nftID = '1'
        nftID1 = '2'
        nftID2 = '3'
        nftGuid = self.CreateAssetID(nftID, self.asset)
        nftGuid1 = self.CreateAssetID(nftID1, self.asset)
        nftGuid2 = self.CreateAssetID(nftID2, self.asset)
        nftGuid1_1 = self.CreateAssetID(nftID, self.asset1)
        nftGuid1_2 = self.CreateAssetID(nftID1, self.asset1)
        nftGuid2_1 = self.CreateAssetID(nftID, self.asset2)
        nftGuid2_2 = self.CreateAssetID(nftID1, self.asset2)
        self.nodes[0].assetsendmany(self.asset, [{'address': self.nodes[0].getnewaddress(),'amount':10.0},{'address': self.nodes[0].getnewaddress(),'amount':0.4,'NFTID':nftID},{'address': self.nodes[0].getnewaddress(),'amount':0.5,'NFTID':nftID1},{'address': self.nodes[0].getnewaddress(),'amount':0.6,'NFTID':nftID2}])
        self.nodes[0].assetsendmany(self.asset1, [{'address': self.nodes[0].getnewaddress(),'amount':10.0},{'address': self.nodes[0].getnewaddress(),'amount':0.4,'NFTID':nftID},{'address': self.nodes[0].getnewaddress(),'amount':0.5,'NFTID':nftID1}])
        self.nodes[0].assetsendmany(self.asset2, [{'address': self.nodes[0].getnewaddress(),'amount':10.0},{'address': self.nodes[0].getnewaddress(),'amount':0.4,'NFTID':nftID},{'address': self.nodes[0].getnewaddress(),'amount':0.5,'NFTID':nftID1}])
        self.nodes[0].generate(1)
        self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        self.nodes[0].generate(1)
        # will give back hex because notarization doesn't happen in assetallocationsend
        address = self.nodes[1].getnewaddress()
        sendobj = [{"asset_guid":nftGuid,"address":address,"amount":0.4},{"asset_guid":nftGuid1,"address":address,"amount":0.5},{"asset_guid":self.asset,"address":address,"amount":2.1},{"asset_guid":nftGuid2,"address":address,"amount":0.6},{"asset_guid":nftGuid1_1,"address":address,"amount":0.4},{"asset_guid":nftGuid1_2,"address":address,"amount":0.5},{"asset_guid":self.asset1,"address":address,"amount":2.1},{"asset_guid":nftGuid2_1,"address":address,"amount":0.4},{"asset_guid":nftGuid2_2,"address":address,"amount":0.5},{"asset_guid":self.asset2,"address":address,"amount":2.1}]
        hextx = self.nodes[0].assetallocationsendmany(sendobj)['hex']
        notarysig = self.nodes[0].signhash(self.notary_address, self.nodes[0].getnotarysighash(hextx, self.asset))
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx, self.asset, notarysig)['hex']
        tx_resigned = self.nodes[0].signrawtransactionwithwallet(hextx_notarized)['hex']
        notarysig1 = self.nodes[0].signhash(self.notary_address1, self.nodes[0].getnotarysighash(tx_resigned, self.asset1))
        hextx_notarized1 = self.nodes[0].assettransactionnotarize(tx_resigned, self.asset1, notarysig1)['hex']
        tx_resigned1 = self.nodes[0].signrawtransactionwithwallet(hextx_notarized1)['hex']
        assert(hextx != hextx_notarized)
        assert(tx_resigned != hextx_notarized)
        assert(tx_resigned1 != hextx_notarized1)
        # cannot send without notarization
        assert_raises_rpc_error(-26, 'assetallocation-notary-sig', self.nodes[0].sendrawtransaction, hextx)
        assert_raises_rpc_error(-26, 'assetallocation-notary-sig', self.nodes[0].sendrawtransaction, hextx_notarized)
        assert_raises_rpc_error(-26, 'non-mandatory-script-verify-flag', self.nodes[0].sendrawtransaction, hextx_notarized1)
        # need second base asset notarized
        assert_raises_rpc_error(-26, 'assetallocation-notary-sig', self.nodes[0].sendrawtransaction, tx_resigned)
        self.nodes[0].sendrawtransaction(tx_resigned1)
        self.nodes[0].generate(1)
        # clear notary
        self.nodes[0].assetupdate(self.asset, '','', 127, '', {}, {})
        self.nodes[0].assetupdate(self.asset1, '','', 127, '', {}, {})
        self.nodes[0].generate(1)
        self.sync_blocks()
        out = self.nodes[1].listunspentasset(self.asset)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset)
        assert_equal(out[0]['asset_amount'], Decimal('2.1'))
        out = self.nodes[1].listunspentasset(nftGuid)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid)
        assert_equal(out[0]['asset_amount'], Decimal('0.4'))
        out = self.nodes[1].listunspentasset(nftGuid1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid1)
        assert_equal(out[0]['asset_amount'], Decimal('0.5'))
        out = self.nodes[1].listunspentasset(nftGuid2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid2)
        assert_equal(out[0]['asset_amount'], Decimal('0.6'))


        out = self.nodes[1].listunspentasset(self.asset1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset1)
        assert_equal(out[0]['asset_amount'], Decimal('2.1'))
        out = self.nodes[1].listunspentasset(nftGuid1_1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid1_1)
        assert_equal(out[0]['asset_amount'], Decimal('0.4'))
        out = self.nodes[1].listunspentasset(nftGuid1_2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid1_2)
        assert_equal(out[0]['asset_amount'], Decimal('0.5'))

        out = self.nodes[1].listunspentasset(self.asset2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset2)
        assert_equal(out[0]['asset_amount'], Decimal('2.1'))
        out = self.nodes[1].listunspentasset(nftGuid2_1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid2_1)
        assert_equal(out[0]['asset_amount'], Decimal('0.4'))
        out = self.nodes[1].listunspentasset(nftGuid2_2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid2_2)
        assert_equal(out[0]['asset_amount'], Decimal('0.5'))

        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert('notary_details' not in assetInfo)
        assert_equal(assetInfo['notary_address'], '')

        address = self.nodes[2].getnewaddress()
        sendobj = [{"asset_guid":nftGuid,"address":address,"amount":0.4},{"asset_guid":nftGuid1,"address":address,"amount":0.5},{"asset_guid":self.asset,"address":address,"amount":2.1},{"asset_guid":nftGuid2,"address":address,"amount":0.6},{"asset_guid":nftGuid1_1,"address":address,"amount":0.4},{"asset_guid":nftGuid1_2,"address":address,"amount":0.5},{"asset_guid":self.asset1,"address":address,"amount":2.1},{"asset_guid":nftGuid2_1,"address":address,"amount":0.4},{"asset_guid":nftGuid2_2,"address":address,"amount":0.5},{"asset_guid":self.asset2,"address":address,"amount":2.1}]
        self.nodes[1].assetallocationsendmany(sendobj, True)
        self.sync_mempools()
        self.nodes[0].generate(1)
        self.sync_blocks()
        out = self.nodes[2].listunspentasset(self.asset)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset)
        assert_equal(out[0]['asset_amount'], Decimal('2.1'))
        out = self.nodes[2].listunspentasset(nftGuid)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid)
        assert_equal(out[0]['asset_amount'], Decimal('0.4'))
        out = self.nodes[2].listunspentasset(nftGuid1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid1)
        assert_equal(out[0]['asset_amount'], Decimal('0.5'))
        out = self.nodes[2].listunspentasset(nftGuid2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid2)
        assert_equal(out[0]['asset_amount'], Decimal('0.6'))


        out = self.nodes[2].listunspentasset(self.asset1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset1)
        assert_equal(out[0]['asset_amount'], Decimal('2.1'))
        out = self.nodes[2].listunspentasset(nftGuid1_1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid1_1)
        assert_equal(out[0]['asset_amount'], Decimal('0.4'))
        out = self.nodes[2].listunspentasset(nftGuid1_2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid1_2)
        assert_equal(out[0]['asset_amount'], Decimal('0.5'))

        out = self.nodes[2].listunspentasset(self.asset2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset2)
        assert_equal(out[0]['asset_amount'], Decimal('2.1'))
        out = self.nodes[2].listunspentasset(nftGuid2_1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid2_1)
        assert_equal(out[0]['asset_amount'], Decimal('0.4'))
        out = self.nodes[2].listunspentasset(nftGuid2_2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], nftGuid2_2)
        assert_equal(out[0]['asset_amount'], Decimal('0.5'))

    def basic_asset(self):
        self.notary_address = self.nodes[0].getnewaddress()
        notary = {'endpoint': 'https://jsonplaceholder.typicode.com/posts/', 'instant_transfers': True, 'hd_required': True}
        self.asset = self.nodes[0].assetnew('1', 'TST', 'asset description', '0x', 8, 10000, 127, self.notary_address, notary, {})['asset_guid']
        self.nodes[0].generate(1)
        self.sync_blocks()
        assetInfo = self.nodes[0].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assert_equal(assetInfo['notary_details'], notary)
        assert_equal(assetInfo['notary_address'], self.notary_address)
        assetInfo = self.nodes[1].assetinfo(self.asset)
        assert_equal(assetInfo['asset_guid'], self.asset)
        assert_equal(assetInfo['notary_details'], notary)
        assert_equal(assetInfo['notary_address'], self.notary_address)

    def complex_notary(self):
        notary = {'endpoint': 'https://jsonplaceholder.typicode.com/posts/', 'instant_transfers': True, 'hd_required': True}
        self.notary_address0 = self.nodes[0].getnewaddress()
        self.notary_address1 = self.nodes[1].getnewaddress()
        self.notary_address2 = self.nodes[2].getnewaddress()
        self.notary_address3 = self.nodes[3].getnewaddress()
        self.notary_address4 = self.nodes[4].getnewaddress()
        self.asset0 = self.nodes[0].assetnew('1', 'TST0', 'asset description', '0x', 8, 10000, 127, self.notary_address0, notary, {})['asset_guid']
        self.nodes[0].generate(1)
        self.asset1 = self.nodes[0].assetnew('1', 'TST1', 'asset description', '0x', 8, 10000, 127, self.notary_address1, notary, {})['asset_guid']
        self.nodes[0].generate(1)
        self.asset2 = self.nodes[0].assetnew('1', 'TST2', 'asset description', '0x', 8, 10000, 127, self.notary_address2, notary, {})['asset_guid']
        self.nodes[0].generate(1)
        self.asset3 = self.nodes[0].assetnew('1', 'TST3', 'asset description', '0x', 8, 10000, 127, self.notary_address3, notary, {})['asset_guid']
        self.nodes[0].generate(1)
        self.asset4 = self.nodes[0].assetnew('1', 'TST4', 'asset description', '0x', 8, 10000, 127, self.notary_address4, notary, {})['asset_guid']
        self.nodes[0].generate(1)
        self.asset5 = self.nodes[0].assetnew('1', 'TST4', 'asset description', '0x', 8, 10000, 127, '', {}, {})['asset_guid']
        self.nodes[0].generate(1)
        self.nodes[0].assetsend(self.asset0, self.nodes[0].getnewaddress(), 1)
        self.nodes[0].generate(1)
        self.nodes[0].assetsend(self.asset1, self.nodes[0].getnewaddress(), 2)
        self.nodes[0].generate(1)
        self.nodes[0].assetsend(self.asset2, self.nodes[0].getnewaddress(), 3)
        self.nodes[0].generate(1)
        self.nodes[0].assetsend(self.asset3, self.nodes[0].getnewaddress(), 4)
        self.nodes[0].generate(1)
        self.nodes[0].assetsend(self.asset4, self.nodes[0].getnewaddress(), 5)
        self.nodes[0].generate(1)
        self.nodes[0].assetsend(self.asset5, self.nodes[0].getnewaddress(), 6)
        self.nodes[0].generate(1)
        sendobj = [{"asset_guid":self.asset0,"address":self.nodes[1].getnewaddress(),"amount":0.5},{"asset_guid":self.asset0,"address":self.nodes[1].getnewaddress(),"amount":0.5},{"asset_guid":self.asset1,"address":self.nodes[1].getnewaddress(),"amount":2},{"asset_guid":self.asset2,"address":self.nodes[1].getnewaddress(),"amount":2.5},{"asset_guid":self.asset3,"address":self.nodes[1].getnewaddress(),"amount":3},{"asset_guid":self.asset4,"address":self.nodes[1].getnewaddress(),"amount":3.5},{"asset_guid":self.asset5,"address":self.nodes[1].getnewaddress(),"amount":5}]
        hextx = self.nodes[0].assetallocationsendmany(sendobj)['hex']
        notarysig0 = self.nodes[0].signhash(self.notary_address0, self.nodes[0].getnotarysighash(hextx, self.asset0))
        notarysig1 = self.nodes[1].signhash(self.notary_address1, self.nodes[0].getnotarysighash(hextx, self.asset1))
        notarysig2 = self.nodes[2].signhash(self.notary_address2, self.nodes[0].getnotarysighash(hextx, self.asset2))
        notarysig3 = self.nodes[3].signhash(self.notary_address3, self.nodes[0].getnotarysighash(hextx, self.asset3))
        notarysig4 = self.nodes[4].signhash(self.notary_address4, self.nodes[0].getnotarysighash(hextx, self.asset4))
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx, self.asset0, notarysig0)['hex']
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx_notarized, self.asset1, notarysig1)['hex']
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx_notarized, self.asset2, notarysig2)['hex']
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx_notarized, self.asset3, notarysig3)['hex']
        # try without final signature
        tx_resigned = self.nodes[0].signrawtransactionwithwallet(hextx_notarized)['hex']
        assert_raises_rpc_error(-26, 'assetallocation-notary-sig', self.nodes[0].sendrawtransaction, tx_resigned)
        # try with wrong signature
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx_notarized, self.asset4, notarysig3)['hex']
        assert_raises_rpc_error(-26, 'assetallocation-notary-sig', self.nodes[0].sendrawtransaction, tx_resigned)
        hextx_notarized = self.nodes[0].assettransactionnotarize(hextx_notarized, self.asset4, notarysig4)['hex']
        tx_resigned = self.nodes[0].signrawtransactionwithwallet(hextx_notarized)['hex']
        assert_equal(len(hextx), len(hextx_notarized))
        assert(hextx != hextx_notarized)
        assert(tx_resigned != hextx_notarized)

        self.nodes[0].sendrawtransaction(tx_resigned)
        self.nodes[0].generate(1)
        self.sync_blocks()
        # check change on sender node
        out =  self.nodes[0].listunspentasset(self.asset0)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset0)
        assert_equal(out[0]['asset_amount'], 0)
        out =  self.nodes[0].listunspentasset(self.asset1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset1)
        assert_equal(out[0]['asset_amount'], 0)
        out =  self.nodes[0].listunspentasset(self.asset2)
        assert_equal(len(out), 2)
        assert(out[0]['asset_amount'] == Decimal('0.5') or out[1]['asset_amount'] == Decimal('0.5'))
        out =  self.nodes[0].listunspentasset(self.asset3)
        assert_equal(len(out), 2)
        assert(out[0]['asset_amount'] == Decimal('1.0') or out[1]['asset_amount'] == Decimal('1.0'))
        out =  self.nodes[0].listunspentasset(self.asset4)
        assert_equal(len(out), 2)
        assert(out[0]['asset_amount'] == Decimal('1.5') or out[1]['asset_amount'] == Decimal('1.5'))
        out =  self.nodes[0].listunspentasset(self.asset5)
        assert_equal(len(out), 2)
        assert(out[0]['asset_amount'] == Decimal('1.0') or out[1]['asset_amount'] == Decimal('1.0'))
        # check receiver node
        out =  self.nodes[1].listunspentasset(self.asset0)
        assert_equal(len(out), 2)
        assert_equal(out[0]['asset_guid'], self.asset0)
        assert_equal(out[0]['asset_amount'], Decimal('0.5'))
        assert_equal(out[1]['asset_guid'], self.asset0)
        assert_equal(out[1]['asset_amount'], Decimal('0.5'))
        out =  self.nodes[1].listunspentasset(self.asset1)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset1)
        assert_equal(out[0]['asset_amount'], Decimal('2.0'))
        out =  self.nodes[1].listunspentasset(self.asset2)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset2)
        assert_equal(out[0]['asset_amount'], Decimal('2.5'))
        out =  self.nodes[1].listunspentasset(self.asset3)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset3)
        assert_equal(out[0]['asset_amount'], Decimal('3.0'))
        out =  self.nodes[1].listunspentasset(self.asset4)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset4)
        assert_equal(out[0]['asset_amount'], Decimal('3.5'))
        out =  self.nodes[1].listunspentasset(self.asset5)
        assert_equal(len(out), 1)
        assert_equal(out[0]['asset_guid'], self.asset5)
        assert_equal(out[0]['asset_amount'], Decimal('5.0'))
if __name__ == '__main__':
    AssetNotaryTest().main()
