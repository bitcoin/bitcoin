#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the importmulti RPC."""

from test_framework import script
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    bytes_to_hex_str,
    hex_str_to_bytes
)
from test_framework.script import (
    CScript,
    OP_0,
    hash160
)
from test_framework.messages import sha256

class ImportMultiTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-addresstype=legacy"], ["-addresstype=legacy"]]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def run_test (self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        node0_address1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())

        #Check only one address
        assert_equal(node0_address1['ismine'], True)

        #Node 1 sync test
        assert_equal(self.nodes[1].getblockcount(),1)

        #Address Test - before import
        address_info = self.nodes[1].getaddressinfo(node0_address1['address'])
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], False)


        # RPC importmulti -----------------------------------------------

        # Bitcoin Address
        self.log.info("Should import an address")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)
        assert_equal(address_assert['timestamp'], timestamp)
        watchonly_address = address['address']
        watchonly_timestamp = timestamp

        self.log.info("Should not import an invalid address")
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
        self.log.info("Should import a scriptPubKey with internal flag")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "internal": True
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)
        assert_equal(address_assert['timestamp'], timestamp)

        # ScriptPubKey + internal + label
        self.log.info("Should not allow a label to be specified when internal is true")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "internal": True,
            "label": "Example label"
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Internal addresses should not have a label')

        # Nonstandard scriptPubKey + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal flag")
        nonstandardScriptPubKey = address['scriptPubKey'] + bytes_to_hex_str(script.CScript([script.OP_NOP]))
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": nonstandardScriptPubKey,
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Internal must be set to true for nonstandard scriptPubKey imports.')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)


        # Address + Public key + !Internal
        self.log.info("Should import an address with public key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "pubkeys": [ address['pubkey'] ]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)
        assert_equal(address_assert['timestamp'], timestamp)


        # ScriptPubKey + Public key + internal
        self.log.info("Should import a scriptPubKey with internal and with public key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        request = [{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "pubkeys": [ address['pubkey'] ],
            "internal": True
        }]
        result = self.nodes[1].importmulti(request)
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)
        assert_equal(address_assert['timestamp'], timestamp)

        # Nonstandard scriptPubKey + Public key + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal and with public key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        request = [{
            "scriptPubKey": nonstandardScriptPubKey,
            "timestamp": "now",
            "pubkeys": [ address['pubkey'] ]
        }]
        result = self.nodes[1].importmulti(request)
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Internal must be set to true for nonstandard scriptPubKey imports.')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)

        # Address + Private key + !watchonly
        self.log.info("Should import an address with private key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], True)
        assert_equal(address_assert['timestamp'], timestamp)

        self.log.info("Should not import an address with private key if is already imported")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ]
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -4)
        assert_equal(result[0]['error']['message'], 'The wallet already contains the private key for this address or script')

        # Address + Private key + watchonly
        self.log.info("Should not import an address with private key and with watchonly")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
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
        assert_equal(result[0]['error']['message'], 'Watch-only addresses should not include private keys')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)

        # ScriptPubKey + Private key + internal
        self.log.info("Should import a scriptPubKey with internal and with private key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ],
            "internal": True
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], True)
        assert_equal(address_assert['timestamp'], timestamp)

        # Nonstandard scriptPubKey + Private key + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal and with private key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": nonstandardScriptPubKey,
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address['address']) ]
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], 'Internal must be set to true for nonstandard scriptPubKey imports.')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)


        # P2SH address
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['pubkey'], sig_address_2['pubkey'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        self.log.info("Should import a p2sh")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(multi_sig_script['address'])
        assert_equal(address_assert['isscript'], True)
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['timestamp'], timestamp)
        p2shunspent = self.nodes[1].listunspent(0,999999, [multi_sig_script['address']])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], False)


        # P2SH + Redeem script
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['pubkey'], sig_address_2['pubkey'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        self.log.info("Should import a p2sh with respective redeem script")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
            "redeemscript": multi_sig_script['redeemScript']
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(multi_sig_script['address'])
        assert_equal(address_assert['timestamp'], timestamp)

        p2shunspent = self.nodes[1].listunspent(0,999999, [multi_sig_script['address']])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)


        # P2SH + Redeem script + Private Keys + !Watchonly
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['pubkey'], sig_address_2['pubkey'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        self.log.info("Should import a p2sh with respective redeem script and private keys")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
            "redeemscript": multi_sig_script['redeemScript'],
            "keys": [ self.nodes[0].dumpprivkey(sig_address_1['address']), self.nodes[0].dumpprivkey(sig_address_2['address'])]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(multi_sig_script['address'])
        assert_equal(address_assert['timestamp'], timestamp)

        p2shunspent = self.nodes[1].listunspent(0,999999, [multi_sig_script['address']])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)

        # P2SH + Redeem script + Private Keys + Watchonly
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_3 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].createmultisig(2, [sig_address_1['pubkey'], sig_address_2['pubkey'], sig_address_3['pubkey']])
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multi_sig_script['address'], 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        self.log.info("Should import a p2sh with respective redeem script and private keys")
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
        assert_equal(result[0]['error']['message'], 'Watch-only addresses should not include private keys')


        # Address + Public key + !Internal + Wrong pubkey
        self.log.info("Should not import an address with a wrong public key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "pubkeys": [ address2['pubkey'] ]
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Key does not match address destination')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)


        # ScriptPubKey + Public key + internal + Wrong pubkey
        self.log.info("Should not import a scriptPubKey with internal and with a wrong public key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        request = [{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "pubkeys": [ address2['pubkey'] ],
            "internal": True
        }]
        result = self.nodes[1].importmulti(request)
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Key does not match address destination')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)


        # Address + Private key + !watchonly + Wrong private key
        self.log.info("Should not import an address with a wrong private key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address2['address']) ]
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Key does not match address destination')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)


        # ScriptPubKey + Private key + internal + Wrong private key
        self.log.info("Should not import a scriptPubKey with internal and with a wrong private key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        address2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        result = self.nodes[1].importmulti([{
            "scriptPubKey": address['scriptPubKey'],
            "timestamp": "now",
            "keys": [ self.nodes[0].dumpprivkey(address2['address']) ],
            "internal": True
        }])
        assert_equal(result[0]['success'], False)
        assert_equal(result[0]['error']['code'], -5)
        assert_equal(result[0]['error']['message'], 'Key does not match address destination')
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], False)
        assert_equal('timestamp' in address_assert, False)


        # Importing existing watch only address with new timestamp should replace saved timestamp.
        assert_greater_than(timestamp, watchonly_timestamp)
        self.log.info("Should replace previously saved watch only timestamp.")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": watchonly_address,
            },
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(watchonly_address)
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)
        assert_equal(address_assert['timestamp'], timestamp)
        watchonly_timestamp = timestamp


        # restart nodes to check for proper serialization/deserialization of watch only address
        self.stop_nodes()
        self.start_nodes()
        address_assert = self.nodes[1].getaddressinfo(watchonly_address)
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['ismine'], False)
        assert_equal(address_assert['timestamp'], watchonly_timestamp)

        # Bad or missing timestamps
        self.log.info("Should throw on invalid or missing timestamp values")
        assert_raises_rpc_error(-3, 'Missing required timestamp field for key',
            self.nodes[1].importmulti, [{
                "scriptPubKey": address['scriptPubKey'],
            }])
        assert_raises_rpc_error(-3, 'Expected number or "now" timestamp value for key. got type string',
            self.nodes[1].importmulti, [{
                "scriptPubKey": address['scriptPubKey'],
                "timestamp": "",
            }])

        # Import P2WPKH address as watch only
        self.log.info("Should import a P2WPKH address as watch only")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress(address_type="bech32"))
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], True)
        assert_equal(address_assert['solvable'], False)

        # Import P2WPKH address with public key but no private key
        self.log.info("Should import a P2WPKH address and public key as solvable but not spendable")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress(address_type="bech32"))
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "pubkeys": [ address['pubkey'] ]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['ismine'], False)
        assert_equal(address_assert['solvable'], True)

        # Import P2WPKH address with key and check it is spendable
        self.log.info("Should import a P2WPKH address with key")
        address = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress(address_type="bech32"))
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": address['address']
            },
            "timestamp": "now",
            "keys": [self.nodes[0].dumpprivkey(address['address'])]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(address['address'])
        assert_equal(address_assert['iswatchonly'], False)
        assert_equal(address_assert['ismine'], True)

        # P2WSH multisig address without scripts or keys
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        sig_address_2 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].addmultisigaddress(2, [sig_address_1['pubkey'], sig_address_2['pubkey']], "", "bech32")
        self.log.info("Should import a p2wsh multisig as watch only without respective redeem script and private keys")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now"
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(multi_sig_script['address'])
        assert_equal(address_assert['solvable'], False)

        # Same P2WSH multisig address as above, but now with witnessscript + private keys
        self.log.info("Should import a p2wsh with respective redeem script and private keys")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
            "witnessscript": multi_sig_script['redeemScript'],
            "keys": [ self.nodes[0].dumpprivkey(sig_address_1['address']), self.nodes[0].dumpprivkey(sig_address_2['address']) ]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(multi_sig_script['address'])
        assert_equal(address_assert['solvable'], True)
        assert_equal(address_assert['ismine'], True)
        assert_equal(address_assert['sigsrequired'], 2)

        # P2SH-P2WPKH address with no redeemscript or public or private key
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress(address_type="p2sh-segwit"))
        pubkeyhash = hash160(hex_str_to_bytes(sig_address_1['pubkey']))
        pkscript = CScript([OP_0, pubkeyhash])
        self.log.info("Should import a p2sh-p2wpkh without redeem script or keys")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": sig_address_1['address']
            },
            "timestamp": "now"
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(sig_address_1['address'])
        assert_equal(address_assert['solvable'], False)
        assert_equal(address_assert['ismine'], False)

        # P2SH-P2WPKH address + redeemscript + public key with no private key
        self.log.info("Should import a p2sh-p2wpkh with respective redeem script and pubkey as solvable")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": sig_address_1['address']
            },
            "timestamp": "now",
            "redeemscript": bytes_to_hex_str(pkscript),
            "pubkeys": [ sig_address_1['pubkey'] ]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(sig_address_1['address'])
        assert_equal(address_assert['solvable'], True)
        assert_equal(address_assert['ismine'], False)

        # P2SH-P2WPKH address + redeemscript + private key
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress(address_type="p2sh-segwit"))
        pubkeyhash = hash160(hex_str_to_bytes(sig_address_1['pubkey']))
        pkscript = CScript([OP_0, pubkeyhash])
        self.log.info("Should import a p2sh-p2wpkh with respective redeem script and private keys")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": sig_address_1['address']
            },
            "timestamp": "now",
            "redeemscript": bytes_to_hex_str(pkscript),
            "keys": [ self.nodes[0].dumpprivkey(sig_address_1['address'])]
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(sig_address_1['address'])
        assert_equal(address_assert['solvable'], True)
        assert_equal(address_assert['ismine'], True)

        # P2SH-P2WSH 1-of-1 multisig + redeemscript with no private key
        sig_address_1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
        multi_sig_script = self.nodes[0].addmultisigaddress(1, [sig_address_1['pubkey']], "", "p2sh-segwit")
        scripthash = sha256(hex_str_to_bytes(multi_sig_script['redeemScript']))
        redeem_script = CScript([OP_0, scripthash])
        self.log.info("Should import a p2sh-p2wsh with respective redeem script but no private key")
        result = self.nodes[1].importmulti([{
            "scriptPubKey": {
                "address": multi_sig_script['address']
            },
            "timestamp": "now",
            "redeemscript": bytes_to_hex_str(redeem_script),
            "witnessscript": multi_sig_script['redeemScript']
        }])
        assert_equal(result[0]['success'], True)
        address_assert = self.nodes[1].getaddressinfo(multi_sig_script['address'])
        assert_equal(address_assert['solvable'], True)

if __name__ == '__main__':
    ImportMultiTest ().main ()
