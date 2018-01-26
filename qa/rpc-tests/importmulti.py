#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import *

class ImportMultiTest (BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self, split=False):
        self.nodes = start_nodes(2, self.options.tmpdir)
        self.is_network_split=False

    def run_test (self):
        print ("Mining blocks...")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)

        # keyword definition
        PRIV_KEY = 'privkey'
        PUB_KEY = 'pubkey'
        ADDRESS_KEY = 'address'
        SCRIPT_KEY = 'script'


        node0_address1 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        node0_address2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        node0_address3 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())

        #Check only one address
        assert_equal(node0_address1['ismine'], True)

        #Node 1 sync test
        assert_equal(self.nodes[1].getblockcount(),1)

        #Address Test - before import
        address_info = self.nodes[1].validateaddress(node0_address1['address'])
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], False)


        # RPC importmulti -----------------------------------------------

        # Bitcoin Address
        print("Should import an address")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)

        print("Should not import an invalid address")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": "not valid address",
            },
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Invalid address')

        # ScriptPubKey + internal
        print("Should import a scriptPubKey with internal flag")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "internal": True
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)

        # ScriptPubKey + !internal
        print("Should not import a scriptPubKey without internal flag")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Internal must be set for hex scriptPubKey')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)


        # Address + Public key + !Internal
        print("Should import an address with public key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "pubkeys": [ address['pubkey'] ]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)


        # ScriptPubKey + Public key + internal
        print("Should import a scriptPubKey with internal and with public key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        request = [{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "pubkeys": [ address['pubkey'] ],
            "internal": True
        }]
        result = self.nodes[1].importmulti(request)
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)

        # ScriptPubKey + Public key + !internal
        print("Should not import a scriptPubKey without internal and with public key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        request = [{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "pubkeys": [ address['pubkey'] ]
        }]
        result = self.nodes[1].importmulti(request)
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Internal must be set for hex scriptPubKey')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)

        # Address + Private key + !watchonly
        print("Should import an address with private key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], True)

        # Address + Private key + watchonly
        print("Should not import an address with private key and with watchonly")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ],
            "watchonly": True
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Incompatibility found between watchonly and keys')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)

        # ScriptPubKey + Private key + internal
        print("Should import a scriptPubKey with internal and with private key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ],
            "internal": True
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], True)

        # ScriptPubKey + Private key + !internal
        print("Should not import a scriptPubKey without internal and with private key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ]
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Internal must be set for hex scriptPubKey')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)


        # P2SH address
        sig_address_1 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['address'], sig_address_2['address'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        transactionid = self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        transaction = self.nodes[1].gettransaction(transactionid)

        print("Should import a p2sh")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].validateaddress(multi_sig_script['address'])
        assert_equal(address_assert['isscript'], True)
        assert_equal(address_assert['iswatchonly'], True)
        p2shunspent = self.nodes[1].listunspent(0,999999, [multi_sig_script['address']])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], False)


        # P2SH + Redeem script
        sig_address_1 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['address'], sig_address_2['address'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        transactionid = self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        transaction = self.nodes[1].gettransaction(transactionid)

        print("Should import a p2sh with respective redeem script")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
            "redeemscript": multi_sig_script['redeemScript']
        }])
        assert_equal(result[0]['success'], True)

        p2shunspent = self.nodes[1].listunspent(0,999999, [multi_sig_script['address']])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)


        # P2SH + Redeem script + Private Keys + !Watchonly
        sig_address_1 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['address'], sig_address_2['address'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        transactionid = self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        transaction = self.nodes[1].gettransaction(transactionid)

        print("Should import a p2sh with respective redeem script and private keys")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
            "redeemscript": multi_sig_script['redeemScript'],
            "keys": [ self.nodes[0].dumpprivkey(sig_address_1['address']), self.nodes[0].dumpprivkey(sig_address_2['address'])]
        }])
        assert_equal(result[0]['success'], True)

        p2shunspent = self.nodes[1].listunspent(0,999999, [multi_sig_script['address']])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)

        # P2SH + Redeem script + Private Keys + Watchonly
        sig_address_1 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['address'], sig_address_2['address'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        transactionid = self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        transaction = self.nodes[1].gettransaction(transactionid)

        print("Should import a p2sh with respective redeem script and private keys")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
            "redeemscript": multi_sig_script['redeemScript'],
            "keys": [ self.nodes[0].dumpprivkey(sig_address_1['address']), self.nodes[0].dumpprivkey(sig_address_2['address'])],
            "watchonly": True
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Incompatibility found between watchonly and keys')


        # Address + Public key + !Internal + Wrong pubkey
        print("Should not import an address with a wrong public key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "pubkeys": [ address2['pubkey'] ]
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Consistency check failed')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)


        # ScriptPubKey + Public key + internal + Wrong pubkey
        print("Should not import a scriptPubKey with internal and with a wrong public key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        request = [{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "pubkeys": [ address2['pubkey'] ],
            "internal": True
        }]
        result = self.nodes[1].importmulti(request)
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Consistency check failed')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)


        # Address + Private key + !watchonly + Wrong private key
        print("Should not import an address with a wrong private key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address2['address']) ]
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Consistency check failed')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)


        # ScriptPubKey + Private key + internal + Wrong private key
        print("Should not import a scriptPubKey with internal and with a wrong private key")
        address = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].validateaddress(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address2['address']) ],
            "internal": True
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Consistency check failed')
        address_assert = self.nodes[1].validateaddress(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)

        # Bad or missing timestamps
        print("Should throw on invalid or missing timestamp values")
        assert_raises_message(JSONRPCException, 'Missing required timestamp field for key',
            self.nodes[1].importmulti, [{
                "scriptPubKey": address['scriptPubKey'],
            }])
        assert_raises_message(JSONRPCException, 'Expected number or "now" timestamp value for key. got type string',
            self.nodes[1].importmulti, [{
                "scriptPubKey": address['scriptPubKey'],
                "timestamp": "",
            }])


if __name__ == '__main__':
    ImportMultiTest ().main ()
