#!/usr/bin/env python3
# Copyright (c) 2014-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the rawtransaction RPCs.

Test the following RPCs:
   - createrawtransaction
   - signrawtransactionwithwallet
   - sendrawtransaction
   - decoderawtransaction
   - getrawtransaction
"""

from collections import OrderedDict
from decimal import Decimal

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import (
    CTransaction,
    tx_from_hex,
)
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    find_vout_for_address,
)


class multidict(dict):
    """Dictionary that allows duplicate keys.

    Constructed with a list of (key, value) tuples. When dumped by the json module,
    will output invalid json with repeated keys, eg:
    >>> json.dumps(multidict([(1,2),(1,2)])
    '{"1": 2, "1": 2}'

    Used to test calls to rpc methods with repeated keys in the json object."""

    def __init__(self, x):
        dict.__init__(self, x)
        self.x = x

    def items(self):
        return self.x


# Create one-input, one-output, no-fee transaction:
class RawTransactionsTest(SyscoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [
            ["-txindex"],
            ["-txindex"],
            ["-txindex"],
        ]
        # whitelist all peers to speed up tx relay / mempool sync
        for args in self.extra_args:
            args.append("-whitelist=noban@127.0.0.1")

        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        super().setup_network()
        self.connect_nodes(0, 2)

    def run_test(self):
        self.log.info('prepare some coins for multiple *rawtransaction commands')
        self.nodes[2].generate(1)
        self.sync_all()
        self.nodes[0].generate(COINBASE_MATURITY + 1)
        self.sync_all()
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.5)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),1.0)
        self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(),5.0)
        self.sync_all()
        self.nodes[0].generate(5)
        self.sync_all()

        self.log.info('Test getrawtransaction on genesis block coinbase returns an error')
        block = self.nodes[0].getblock(self.nodes[0].getblockhash(0))
        assert_raises_rpc_error(-5, "The genesis block coinbase is not considered an ordinary transaction", self.nodes[0].getrawtransaction, block['merkleroot'])

        self.log.info('Check parameter types and required parameters of createrawtransaction')
        # Test `createrawtransaction` required parameters
        assert_raises_rpc_error(-1, "createrawtransaction", self.nodes[0].createrawtransaction)
        assert_raises_rpc_error(-1, "createrawtransaction", self.nodes[0].createrawtransaction, [])

        # Test `createrawtransaction` invalid extra parameters
        assert_raises_rpc_error(-1, "createrawtransaction", self.nodes[0].createrawtransaction, [], {}, 0, False, 'foo')

        # Test `createrawtransaction` invalid `inputs`
        txid = '1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000'
        assert_raises_rpc_error(-3, "Expected type array", self.nodes[0].createrawtransaction, 'foo', {})
        assert_raises_rpc_error(-1, "JSON value is not an object as expected", self.nodes[0].createrawtransaction, ['foo'], {})
        assert_raises_rpc_error(-1, "JSON value is not a string as expected", self.nodes[0].createrawtransaction, [{}], {})
        assert_raises_rpc_error(-8, "txid must be of length 64 (not 3, for 'foo')", self.nodes[0].createrawtransaction, [{'txid': 'foo'}], {})
        assert_raises_rpc_error(-8, "txid must be hexadecimal string (not 'ZZZ7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844')", self.nodes[0].createrawtransaction, [{'txid': 'ZZZ7bb8b1697ea987f3b223ba7819250cae33efacb068d23dc24859824a77844'}], {})
        assert_raises_rpc_error(-8, "Invalid parameter, missing vout key", self.nodes[0].createrawtransaction, [{'txid': txid}], {})
        assert_raises_rpc_error(-8, "Invalid parameter, missing vout key", self.nodes[0].createrawtransaction, [{'txid': txid, 'vout': 'foo'}], {})
        assert_raises_rpc_error(-8, "Invalid parameter, vout cannot be negative", self.nodes[0].createrawtransaction, [{'txid': txid, 'vout': -1}], {})
        assert_raises_rpc_error(-8, "Invalid parameter, sequence number is out of range", self.nodes[0].createrawtransaction, [{'txid': txid, 'vout': 0, 'sequence': -1}], {})

        # Test `createrawtransaction` invalid `outputs`
        address = self.nodes[0].getnewaddress()
        address2 = self.nodes[0].getnewaddress()
        assert_raises_rpc_error(-1, "JSON value is not an array as expected", self.nodes[0].createrawtransaction, [], 'foo')
        self.nodes[0].createrawtransaction(inputs=[], outputs={})  # Should not throw for backwards compatibility
        self.nodes[0].createrawtransaction(inputs=[], outputs=[])
        assert_raises_rpc_error(-8, "Data must be hexadecimal string", self.nodes[0].createrawtransaction, [], {'data': 'foo'})
        assert_raises_rpc_error(-5, "Invalid Syscoin address", self.nodes[0].createrawtransaction, [], {'foo': 0})
        assert_raises_rpc_error(-3, "Invalid amount", self.nodes[0].createrawtransaction, [], {address: 'foo'})
        assert_raises_rpc_error(-3, "Amount out of range", self.nodes[0].createrawtransaction, [], {address: -1})
        assert_raises_rpc_error(-8, "Invalid parameter, duplicated address: %s" % address, self.nodes[0].createrawtransaction, [], multidict([(address, 1), (address, 1)]))
        assert_raises_rpc_error(-8, "Invalid parameter, duplicated address: %s" % address, self.nodes[0].createrawtransaction, [], [{address: 1}, {address: 1}])
        assert_raises_rpc_error(-8, "Invalid parameter, duplicate key: data", self.nodes[0].createrawtransaction, [], [{"data": 'aa'}, {"data": "bb"}])
        assert_raises_rpc_error(-8, "Invalid parameter, duplicate key: data", self.nodes[0].createrawtransaction, [], multidict([("data", 'aa'), ("data", "bb")]))
        assert_raises_rpc_error(-8, "Invalid parameter, key-value pair must contain exactly one key", self.nodes[0].createrawtransaction, [], [{'a': 1, 'b': 2}])
        assert_raises_rpc_error(-8, "Invalid parameter, key-value pair not an object as expected", self.nodes[0].createrawtransaction, [], [['key-value pair1'], ['2']])

        # Test `createrawtransaction` invalid `locktime`
        assert_raises_rpc_error(-3, "Expected type number", self.nodes[0].createrawtransaction, [], {}, 'foo')
        assert_raises_rpc_error(-8, "Invalid parameter, locktime out of range", self.nodes[0].createrawtransaction, [], {}, -1)
        assert_raises_rpc_error(-8, "Invalid parameter, locktime out of range", self.nodes[0].createrawtransaction, [], {}, 4294967296)

        # Test `createrawtransaction` invalid `replaceable`
        assert_raises_rpc_error(-3, "Expected type bool", self.nodes[0].createrawtransaction, [], {}, 0, 'foo')

        self.log.info('Check that createrawtransaction accepts an array and object as outputs')
        # One output
        tx = tx_from_hex(self.nodes[2].createrawtransaction(inputs=[{'txid': txid, 'vout': 9}], outputs={address: 99}))
        assert_equal(len(tx.vout), 1)
        assert_equal(
            tx.serialize().hex(),
            self.nodes[2].createrawtransaction(inputs=[{'txid': txid, 'vout': 9}], outputs=[{address: 99}]),
        )
        # Two outputs
        tx = tx_from_hex(self.nodes[2].createrawtransaction(inputs=[{'txid': txid, 'vout': 9}], outputs=OrderedDict([(address, 99), (address2, 99)])))
        assert_equal(len(tx.vout), 2)
        assert_equal(
            tx.serialize().hex(),
            self.nodes[2].createrawtransaction(inputs=[{'txid': txid, 'vout': 9}], outputs=[{address: 99}, {address2: 99}]),
        )
        # Multiple mixed outputs
        tx = tx_from_hex(self.nodes[2].createrawtransaction(inputs=[{'txid': txid, 'vout': 9}], outputs=multidict([(address, 99), (address2, 99), ('data', '99')])))
        assert_equal(len(tx.vout), 3)
        assert_equal(
            tx.serialize().hex(),
            self.nodes[2].createrawtransaction(inputs=[{'txid': txid, 'vout': 9}], outputs=[{address: 99}, {address2: 99}, {'data': '99'}]),
        )

        for type in ["bech32", "p2sh-segwit", "legacy"]:
            addr = self.nodes[0].getnewaddress("", type)
            addrinfo = self.nodes[0].getaddressinfo(addr)
            pubkey = addrinfo["scriptPubKey"]

            self.log.info('sendrawtransaction with missing prevtx info (%s)' %(type))

            # Test `signrawtransactionwithwallet` invalid `prevtxs`
            inputs  = [ {'txid' : txid, 'vout' : 3, 'sequence' : 1000}]
            outputs = { self.nodes[0].getnewaddress() : 1 }
            rawtx   = self.nodes[0].createrawtransaction(inputs, outputs)

            prevtx = dict(txid=txid, scriptPubKey=pubkey, vout=3, amount=1)
            succ = self.nodes[0].signrawtransactionwithwallet(rawtx, [prevtx])
            assert succ["complete"]
            if type == "legacy":
                del prevtx["amount"]
                succ = self.nodes[0].signrawtransactionwithwallet(rawtx, [prevtx])
                assert succ["complete"]

            if type != "legacy":
                assert_raises_rpc_error(-3, "Missing amount", self.nodes[0].signrawtransactionwithwallet, rawtx, [
                    {
                        "txid": txid,
                        "scriptPubKey": pubkey,
                        "vout": 3,
                    }
                ])

            assert_raises_rpc_error(-3, "Missing vout", self.nodes[0].signrawtransactionwithwallet, rawtx, [
                {
                    "txid": txid,
                    "scriptPubKey": pubkey,
                    "amount": 1,
                }
            ])
            assert_raises_rpc_error(-3, "Missing txid", self.nodes[0].signrawtransactionwithwallet, rawtx, [
                {
                    "scriptPubKey": pubkey,
                    "vout": 3,
                    "amount": 1,
                }
            ])
            assert_raises_rpc_error(-3, "Missing scriptPubKey", self.nodes[0].signrawtransactionwithwallet, rawtx, [
                {
                    "txid": txid,
                    "vout": 3,
                    "amount": 1
                }
            ])

        #########################################
        # sendrawtransaction with missing input #
        #########################################

        self.log.info('sendrawtransaction with missing input')
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1}] #won't exists
        outputs = { self.nodes[0].getnewaddress() : 4.998 }
        rawtx   = self.nodes[2].createrawtransaction(inputs, outputs)
        rawtx   = self.nodes[2].signrawtransactionwithwallet(rawtx)

        # This will raise an exception since there are missing inputs
        assert_raises_rpc_error(-25, "bad-txns-inputs-missingorspent", self.nodes[2].sendrawtransaction, rawtx['hex'])

        #####################################
        # getrawtransaction with block hash #
        #####################################

        # make a tx by sending then generate 2 blocks; block1 has the tx in it
        tx = self.nodes[2].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        block1, block2 = self.nodes[2].generate(2)
        self.sync_all()
        # We should be able to get the raw transaction by providing the correct block
        gottx = self.nodes[0].getrawtransaction(tx, True, block1)
        assert_equal(gottx['txid'], tx)
        assert_equal(gottx['in_active_chain'], True)
        # We should have the 'in_active_chain' flag when we don't provide a block due to blockindexdb
        gottx = self.nodes[0].getrawtransaction(tx, True)
        assert_equal(gottx['txid'], tx)
        # SYSCOIN
        assert 'in_active_chain' in gottx
        # We should not get the tx if we provide an unrelated block
        assert_raises_rpc_error(-5, "No such transaction found", self.nodes[0].getrawtransaction, tx, True, block2)
        # An invalid block hash should raise the correct errors
        assert_raises_rpc_error(-1, "JSON value is not a string as expected", self.nodes[0].getrawtransaction, tx, True, True)
        assert_raises_rpc_error(-8, "parameter 3 must be of length 64 (not 6, for 'foobar')", self.nodes[0].getrawtransaction, tx, True, "foobar")
        assert_raises_rpc_error(-8, "parameter 3 must be of length 64 (not 8, for 'abcd1234')", self.nodes[0].getrawtransaction, tx, True, "abcd1234")
        assert_raises_rpc_error(-8, "parameter 3 must be hexadecimal string (not 'ZZZ0000000000000000000000000000000000000000000000000000000000000')", self.nodes[0].getrawtransaction, tx, True, "ZZZ0000000000000000000000000000000000000000000000000000000000000")
        assert_raises_rpc_error(-5, "Block hash not found", self.nodes[0].getrawtransaction, tx, True, "0000000000000000000000000000000000000000000000000000000000000000")
        # Undo the blocks and check in_active_chain
        self.nodes[0].invalidateblock(block1)
        gottx = self.nodes[0].getrawtransaction(txid=tx, verbose=True, blockhash=block1)
        assert_equal(gottx['in_active_chain'], False)
        self.nodes[0].reconsiderblock(block1)
        assert_equal(self.nodes[0].getbestblockhash(), block2)

        if not self.options.descriptors:
            # The traditional multisig workflow does not work with descriptor wallets so these are legacy only.
            # The multisig workflow with descriptor wallets uses PSBTs and is tested elsewhere, no need to do them here.
            #########################
            # RAW TX MULTISIG TESTS #
            #########################
            # 2of2 test
            addr1 = self.nodes[2].getnewaddress()
            addr2 = self.nodes[2].getnewaddress()

            addr1Obj = self.nodes[2].getaddressinfo(addr1)
            addr2Obj = self.nodes[2].getaddressinfo(addr2)

            # Tests for createmultisig and addmultisigaddress
            assert_raises_rpc_error(-5, "Invalid public key", self.nodes[0].createmultisig, 1, ["01020304"])
            self.nodes[0].createmultisig(2, [addr1Obj['pubkey'], addr2Obj['pubkey']]) # createmultisig can only take public keys
            assert_raises_rpc_error(-5, "Invalid public key", self.nodes[0].createmultisig, 2, [addr1Obj['pubkey'], addr1]) # addmultisigaddress can take both pubkeys and addresses so long as they are in the wallet, which is tested here.

            mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr1])['address']

            #use balance deltas instead of absolute values
            bal = self.nodes[2].getbalance()

            # send 1.2 SYS to msig adr
            txId = self.nodes[0].sendtoaddress(mSigObj, 1.2)
            self.sync_all()
            self.nodes[0].generate(1)
            self.sync_all()
            assert_equal(self.nodes[2].getbalance(), bal+Decimal('1.20000000')) #node2 has both keys of the 2of2 ms addr., tx should affect the balance


            # 2of3 test from different nodes
            bal = self.nodes[2].getbalance()
            addr1 = self.nodes[1].getnewaddress()
            addr2 = self.nodes[2].getnewaddress()
            addr3 = self.nodes[2].getnewaddress()

            addr1Obj = self.nodes[1].getaddressinfo(addr1)
            addr2Obj = self.nodes[2].getaddressinfo(addr2)
            addr3Obj = self.nodes[2].getaddressinfo(addr3)

            mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey'], addr3Obj['pubkey']])['address']

            txId = self.nodes[0].sendtoaddress(mSigObj, 2.2)
            decTx = self.nodes[0].gettransaction(txId)
            rawTx = self.nodes[0].decoderawtransaction(decTx['hex'])
            self.sync_all()
            self.nodes[0].generate(1)
            self.sync_all()

            #THIS IS AN INCOMPLETE FEATURE
            #NODE2 HAS TWO OF THREE KEY AND THE FUNDS SHOULD BE SPENDABLE AND COUNT AT BALANCE CALCULATION
            assert_equal(self.nodes[2].getbalance(), bal) #for now, assume the funds of a 2of3 multisig tx are not marked as spendable

            txDetails = self.nodes[0].gettransaction(txId, True)
            rawTx = self.nodes[0].decoderawtransaction(txDetails['hex'])
            vout = next(o for o in rawTx['vout'] if o['value'] == Decimal('2.20000000'))

            bal = self.nodes[0].getbalance()
            inputs = [{ "txid" : txId, "vout" : vout['n'], "scriptPubKey" : vout['scriptPubKey']['hex'], "amount" : vout['value']}]
            outputs = { self.nodes[0].getnewaddress() : 2.19 }
            rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
            rawTxPartialSigned = self.nodes[1].signrawtransactionwithwallet(rawTx, inputs)
            assert_equal(rawTxPartialSigned['complete'], False) #node1 only has one key, can't comp. sign the tx

            rawTxSigned = self.nodes[2].signrawtransactionwithwallet(rawTx, inputs)
            assert_equal(rawTxSigned['complete'], True) #node2 can sign the tx compl., own two of three keys
            self.nodes[2].sendrawtransaction(rawTxSigned['hex'])
            rawTx = self.nodes[0].decoderawtransaction(rawTxSigned['hex'])
            self.sync_all()
            self.nodes[0].generate(1)
            self.sync_all()
            assert_equal(self.nodes[0].getbalance(), bal+Decimal('50.00000000')+Decimal('2.19000000')) #block reward + tx

            # 2of2 test for combining transactions
            bal = self.nodes[2].getbalance()
            addr1 = self.nodes[1].getnewaddress()
            addr2 = self.nodes[2].getnewaddress()

            addr1Obj = self.nodes[1].getaddressinfo(addr1)
            addr2Obj = self.nodes[2].getaddressinfo(addr2)

            self.nodes[1].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])['address']
            mSigObj = self.nodes[2].addmultisigaddress(2, [addr1Obj['pubkey'], addr2Obj['pubkey']])['address']
            mSigObjValid = self.nodes[2].getaddressinfo(mSigObj)

            txId = self.nodes[0].sendtoaddress(mSigObj, 2.2)
            decTx = self.nodes[0].gettransaction(txId)
            rawTx2 = self.nodes[0].decoderawtransaction(decTx['hex'])
            self.sync_all()
            self.nodes[0].generate(1)
            self.sync_all()

            assert_equal(self.nodes[2].getbalance(), bal) # the funds of a 2of2 multisig tx should not be marked as spendable

            txDetails = self.nodes[0].gettransaction(txId, True)
            rawTx2 = self.nodes[0].decoderawtransaction(txDetails['hex'])
            vout = next(o for o in rawTx2['vout'] if o['value'] == Decimal('2.20000000'))

            bal = self.nodes[0].getbalance()
            inputs = [{ "txid" : txId, "vout" : vout['n'], "scriptPubKey" : vout['scriptPubKey']['hex'], "redeemScript" : mSigObjValid['hex'], "amount" : vout['value']}]
            outputs = { self.nodes[0].getnewaddress() : 2.19 }
            rawTx2 = self.nodes[2].createrawtransaction(inputs, outputs)
            rawTxPartialSigned1 = self.nodes[1].signrawtransactionwithwallet(rawTx2, inputs)
            self.log.debug(rawTxPartialSigned1)
            assert_equal(rawTxPartialSigned1['complete'], False) #node1 only has one key, can't comp. sign the tx

            rawTxPartialSigned2 = self.nodes[2].signrawtransactionwithwallet(rawTx2, inputs)
            self.log.debug(rawTxPartialSigned2)
            assert_equal(rawTxPartialSigned2['complete'], False) #node2 only has one key, can't comp. sign the tx
            rawTxComb = self.nodes[2].combinerawtransaction([rawTxPartialSigned1['hex'], rawTxPartialSigned2['hex']])
            self.log.debug(rawTxComb)
            self.nodes[2].sendrawtransaction(rawTxComb)
            rawTx2 = self.nodes[0].decoderawtransaction(rawTxComb)
            self.sync_all()
            self.nodes[0].generate(1)
            self.sync_all()
            assert_equal(self.nodes[0].getbalance(), bal+Decimal('50.00000000')+Decimal('2.19000000')) #block reward + tx

        # decoderawtransaction tests
        # witness transaction
        encrawtx = "010000000001010000000000000072c1a6a246ae63f74f931e8365e15a089c68d61900000000000000000000ffffffff0100e1f50500000000000102616100000000"
        decrawtx = self.nodes[0].decoderawtransaction(encrawtx, True) # decode as witness transaction
        assert_equal(decrawtx['vout'][0]['value'], Decimal('1.00000000'))
        assert_raises_rpc_error(-22, 'TX decode failed', self.nodes[0].decoderawtransaction, encrawtx, False) # force decode as non-witness transaction
        # non-witness transaction
        encrawtx = "01000000010000000000000072c1a6a246ae63f74f931e8365e15a089c68d61900000000000000000000ffffffff0100e1f505000000000000000000"
        decrawtx = self.nodes[0].decoderawtransaction(encrawtx, False) # decode as non-witness transaction
        assert_equal(decrawtx['vout'][0]['value'], Decimal('1.00000000'))
        # known ambiguous transaction in the chain (see https://github.com/bitcoin/bitcoin/issues/20579)
        encrawtx = "020000000001010000000000000000000000000000000000000000000000000000000000000000ffffffff4b03c68708046ff8415c622f4254432e434f4d2ffabe6d6de1965d02c68f928e5b244ab1965115a36f56eb997633c7f690124bbf43644e23080000000ca3d3af6d005a65ff0200fd00000000ffffffff03f4c1fb4b0000000016001497cfc76442fe717f2a3f0cc9c175f7561b6619970000000000000000266a24aa21a9ed957d1036a80343e0d1b659497e1b48a38ebe876a056d45965fac4a85cda84e1900000000000000002952534b424c4f434b3a8e092581ab01986cbadc84f4b43f4fa4bb9e7a2e2a0caf9b7cf64d939028e22c0120000000000000000000000000000000000000000000000000000000000000000000000000"
        decrawtx = self.nodes[0].decoderawtransaction(encrawtx)
        decrawtx_wit = self.nodes[0].decoderawtransaction(encrawtx, True)
        assert_raises_rpc_error(-22, 'TX decode failed', self.nodes[0].decoderawtransaction, encrawtx, False) # fails to decode as non-witness transaction
        assert_equal(decrawtx, decrawtx_wit) # the witness interpretation should be chosen
        assert_equal(decrawtx['vin'][0]['coinbase'], "03c68708046ff8415c622f4254432e434f4d2ffabe6d6de1965d02c68f928e5b244ab1965115a36f56eb997633c7f690124bbf43644e23080000000ca3d3af6d005a65ff0200fd00000000")

        # Basic signrawtransaction test
        addr = self.nodes[1].getnewaddress()
        txid = self.nodes[0].sendtoaddress(addr, 10)
        self.nodes[0].generate(1)
        self.sync_all()
        vout = find_vout_for_address(self.nodes[1], txid, addr)
        rawTx = self.nodes[1].createrawtransaction([{'txid': txid, 'vout': vout}], {self.nodes[1].getnewaddress(): 9.999})
        rawTxSigned = self.nodes[1].signrawtransactionwithwallet(rawTx)
        txId = self.nodes[1].sendrawtransaction(rawTxSigned['hex'])
        self.nodes[0].generate(1)
        self.sync_all()

        # getrawtransaction tests
        # 1. valid parameters - only supply txid
        assert_equal(self.nodes[0].getrawtransaction(txId), rawTxSigned['hex'])

        # 2. valid parameters - supply txid and 0 for non-verbose
        assert_equal(self.nodes[0].getrawtransaction(txId, 0), rawTxSigned['hex'])

        # 3. valid parameters - supply txid and False for non-verbose
        assert_equal(self.nodes[0].getrawtransaction(txId, False), rawTxSigned['hex'])

        # 4. valid parameters - supply txid and 1 for verbose.
        # We only check the "hex" field of the output so we don't need to update this test every time the output format changes.
        assert_equal(self.nodes[0].getrawtransaction(txId, 1)["hex"], rawTxSigned['hex'])

        # 5. valid parameters - supply txid and True for non-verbose
        assert_equal(self.nodes[0].getrawtransaction(txId, True)["hex"], rawTxSigned['hex'])

        # 6. invalid parameters - supply txid and string "Flase"
        assert_raises_rpc_error(-1, "not a boolean", self.nodes[0].getrawtransaction, txId, "Flase")

        # 7. invalid parameters - supply txid and empty array
        assert_raises_rpc_error(-1, "not a boolean", self.nodes[0].getrawtransaction, txId, [])

        # 8. invalid parameters - supply txid and empty dict
        assert_raises_rpc_error(-1, "not a boolean", self.nodes[0].getrawtransaction, txId, {})

        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : 1000}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        rawtx   = self.nodes[0].createrawtransaction(inputs, outputs)
        decrawtx= self.nodes[0].decoderawtransaction(rawtx)
        assert_equal(decrawtx['vin'][0]['sequence'], 1000)

        # 9. invalid parameters - sequence number out of range
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : -1}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        assert_raises_rpc_error(-8, 'Invalid parameter, sequence number is out of range', self.nodes[0].createrawtransaction, inputs, outputs)

        # 10. invalid parameters - sequence number out of range
        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : 4294967296}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        assert_raises_rpc_error(-8, 'Invalid parameter, sequence number is out of range', self.nodes[0].createrawtransaction, inputs, outputs)

        inputs  = [ {'txid' : "1d1d4e24ed99057e84c3f80fd8fbec79ed9e1acee37da269356ecea000000000", 'vout' : 1, 'sequence' : 4294967294}]
        outputs = { self.nodes[0].getnewaddress() : 1 }
        rawtx   = self.nodes[0].createrawtransaction(inputs, outputs)
        decrawtx= self.nodes[0].decoderawtransaction(rawtx)
        assert_equal(decrawtx['vin'][0]['sequence'], 4294967294)

        ####################################
        # TRANSACTION VERSION NUMBER TESTS #
        ####################################

        # Test the minimum transaction version number that fits in a signed 32-bit integer.
        # As transaction version is unsigned, this should convert to its unsigned equivalent.
        tx = CTransaction()
        tx.nVersion = -0x80000000
        rawtx = tx.serialize().hex()
        decrawtx = self.nodes[0].decoderawtransaction(rawtx)
        assert_equal(decrawtx['version'], 0x80000000)

        # Test the maximum transaction version number that fits in a signed 32-bit integer.
        tx = CTransaction()
        tx.nVersion = 0x7fffffff
        rawtx = tx.serialize().hex()
        decrawtx = self.nodes[0].decoderawtransaction(rawtx)
        assert_equal(decrawtx['version'], 0x7fffffff)

        self.log.info('sendrawtransaction/testmempoolaccept with maxfeerate')

        # Test a transaction with a small fee.
        txId = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.0)
        rawTx = self.nodes[0].getrawtransaction(txId, True)
        vout = next(o for o in rawTx['vout'] if o['value'] == Decimal('1.00000000'))

        self.sync_all()
        inputs = [{ "txid" : txId, "vout" : vout['n'] }]
        # Fee 10,000 satoshis, (1 - (10000 sat * 0.00000001 SYS/sat)) = 0.9999
        outputs = { self.nodes[0].getnewaddress() : Decimal("0.99990000") }
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        rawTxSigned = self.nodes[2].signrawtransactionwithwallet(rawTx)
        assert_equal(rawTxSigned['complete'], True)
        # Fee 10,000 satoshis, ~100 b transaction, fee rate should land around 100 sat/byte = 0.00100000 SYS/kB
        # Thus, testmempoolaccept should reject
        testres = self.nodes[2].testmempoolaccept([rawTxSigned['hex']], 0.00001000)[0]
        assert_equal(testres['allowed'], False)
        assert_equal(testres['reject-reason'], 'max-fee-exceeded')
        # and sendrawtransaction should throw
        assert_raises_rpc_error(-25, 'Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)', self.nodes[2].sendrawtransaction, rawTxSigned['hex'], 0.00001000)
        # and the following calls should both succeed
        testres = self.nodes[2].testmempoolaccept(rawtxs=[rawTxSigned['hex']])[0]
        assert_equal(testres['allowed'], True)
        self.nodes[2].sendrawtransaction(hexstring=rawTxSigned['hex'])

        # Test a transaction with a large fee.
        txId = self.nodes[0].sendtoaddress(self.nodes[2].getnewaddress(), 1.0)
        rawTx = self.nodes[0].getrawtransaction(txId, True)
        vout = next(o for o in rawTx['vout'] if o['value'] == Decimal('1.00000000'))

        self.sync_all()
        inputs = [{ "txid" : txId, "vout" : vout['n'] }]
        # Fee 2,000,000 satoshis, (1 - (2000000 sat * 0.00000001 SYS/sat)) = 0.98
        outputs = { self.nodes[0].getnewaddress() : Decimal("0.98000000") }
        rawTx = self.nodes[2].createrawtransaction(inputs, outputs)
        rawTxSigned = self.nodes[2].signrawtransactionwithwallet(rawTx)
        assert_equal(rawTxSigned['complete'], True)
        # Fee 2,000,000 satoshis, ~100 b transaction, fee rate should land around 20,000 sat/byte = 0.20000000 SYS/kB
        # Thus, testmempoolaccept should reject
        testres = self.nodes[2].testmempoolaccept([rawTxSigned['hex']])[0]
        assert_equal(testres['allowed'], False)
        assert_equal(testres['reject-reason'], 'max-fee-exceeded')
        # and sendrawtransaction should throw
        assert_raises_rpc_error(-25, 'Fee exceeds maximum configured by user (e.g. -maxtxfee, maxfeerate)', self.nodes[2].sendrawtransaction, rawTxSigned['hex'])
        # and the following calls should both succeed
        testres = self.nodes[2].testmempoolaccept(rawtxs=[rawTxSigned['hex']], maxfeerate='0.20000000')[0]
        assert_equal(testres['allowed'], True)
        self.nodes[2].sendrawtransaction(hexstring=rawTxSigned['hex'], maxfeerate='0.20000000')


if __name__ == '__main__':
    RawTransactionsTest().main()
