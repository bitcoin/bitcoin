#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the importmulti RPC.

Test importmulti by generating keys on node0, importing the scriptPubKeys and
addresses on node1 and then testing the address info for the different address
variants.

- `get_key()` and `get_multisig()` are called to generate keys on node0 and
  return the privkeys, pubkeys and all variants of scriptPubKey and address.
- `test_importmulti()` is called to send an importmulti call to node1, test
  success, and (if unsuccessful) test the error code and error message returned.
- `test_address()` is called to call getaddressinfo for an address on node1
  and test the values returned."""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.script import (
    CScript,
    OP_NOP,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.descriptors import descsum_create
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
)
from test_framework.wallet_util import (
    get_key,
    get_multisig,
    test_address,
)

class ImportMultiTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [['-usehd=1']] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def test_importmulti(self, req, success, error_code=None, error_message=None, warnings=None):
        """Run importmulti and assert success"""
        if warnings is None:
            warnings = []
        result = self.nodes[1].importmulti([req])
        observed_warnings = []
        if 'warnings' in result[0]:
            observed_warnings = result[0]['warnings']
        assert_equal("\n".join(sorted(warnings)), "\n".join(sorted(observed_warnings)))
        assert_equal(result[0]['success'], success)
        if error_code is not None:
            assert_equal(result[0]['error']['code'], error_code)
            assert_equal(result[0]['error']['message'], error_message)

    def run_test(self):
        self.log.info("Mining blocks...")
        self.nodes[0].generate(1)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']
        self.nodes[1].syncwithvalidationinterfacequeue()  # Sync the timestamp to the wallet, so that importmulti works

        node0_address1 = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())

        # Check only one address
        assert_equal(node0_address1['ismine'], True)

        # Node 1 sync test
        assert_equal(self.nodes[1].getblockcount(), 1)

        # Address Test - before import
        address_info = self.nodes[1].getaddressinfo(node0_address1['address'])
        assert_equal(address_info['iswatchonly'], False)
        assert_equal(address_info['ismine'], False)

        # RPC importmulti -----------------------------------------------

        # Bitcoin Address (implicit non-internal)
        self.log.info("Should import an address")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now"},
                              success=True)
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     timestamp=timestamp,
                     ischange=False)
        watchonly_address = key.p2pkh_addr
        watchonly_timestamp = timestamp

        self.log.info("Should not import an invalid address")
        self.test_importmulti({"scriptPubKey": {"address": "not valid address"},
                               "timestamp": "now"},
                              success=False,
                              error_code=-5,
                              error_message='Invalid address \"not valid address\"')

        # ScriptPubKey + internal
        self.log.info("Should import a scriptPubKey with internal flag")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "internal": True},
                              success=True)
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     timestamp=timestamp,
                     ischange=True)

        # ScriptPubKey + internal + label
        self.log.info("Should not allow a label to be specified when internal is true")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "internal": True,
                               "label": "Unsuccessful labelling for internal addresses"},
                              success=False,
                              error_code=-8,
                              error_message='Internal addresses should not have a label')

        # Nonstandard scriptPubKey + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal flag")
        nonstandardScriptPubKey = key.p2pkh_script + CScript([OP_NOP]).hex()
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": nonstandardScriptPubKey,
                               "timestamp": "now"},
                              success=False,
                              error_code=-8,
                              error_message='Internal must be set to true for nonstandard scriptPubKey imports.')
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=False,
                     ismine=False,
                     timestamp=None)

        # Address + Public key + !Internal(explicit)
        self.log.info("Should import an address with public key")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now",
                               "pubkeys": [key.pubkey],
                               "internal": False},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     timestamp=timestamp)

        # ScriptPubKey + Public key + internal
        self.log.info("Should import a scriptPubKey with internal and with public key")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "pubkeys": [key.pubkey],
                               "internal": True},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     timestamp=timestamp)

        # Nonstandard scriptPubKey + Public key + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal and with public key")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": nonstandardScriptPubKey,
                               "timestamp": "now",
                               "pubkeys": [key.pubkey]},
                              success=False,
                              error_code=-8,
                              error_message='Internal must be set to true for nonstandard scriptPubKey imports.')
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=False,
                     ismine=False,
                     timestamp=None)

        # Address + Private key + !watchonly
        self.log.info("Should import an address with private key")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              success=True)
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=False,
                     ismine=True,
                     timestamp=timestamp)

        self.log.info("Should not import an address with private key if is already imported")
        self.test_importmulti({"scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              success=False,
                              error_code=-4,
                              error_message='The wallet already contains the private key for this address or script ("' + key.p2pkh_script + '")')

        # Address + Private key + watchonly
        self.log.info("Should import an address with private key and with watchonly")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now",
                               "keys": [key.privkey],
                               "watchonly": True},
                              success=True,
                              warnings=["All private keys are provided, outputs will be considered spendable. If this is intentional, do not specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=False,
                     ismine=True,
                     timestamp=timestamp)

        # ScriptPubKey + Private key + internal
        self.log.info("Should import a scriptPubKey with internal and with private key")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "keys": [key.privkey],
                               "internal": True},
                              success=True)
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=False,
                     ismine=True,
                     timestamp=timestamp)

        # Nonstandard scriptPubKey + Private key + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal and with private key")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": nonstandardScriptPubKey,
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              success=False,
                              error_code=-8,
                              error_message='Internal must be set to true for nonstandard scriptPubKey imports.')
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=False,
                     ismine=False,
                     timestamp=None)

        # P2SH address
        multisig = get_multisig(self.nodes[0])
        self.nodes[1].generate(COINBASE_MATURITY)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']
        self.nodes[1].syncwithvalidationinterfacequeue()

        self.log.info("Should import a p2sh")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_addr},
                               "timestamp": "now"},
                              success=True)
        test_address(self.nodes[1],
                     multisig.p2sh_addr,
                     isscript=True,
                     iswatchonly=True,
                     timestamp=timestamp)
        p2shunspent = self.nodes[1].listunspent(0, 999999, [multisig.p2sh_addr])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], False)

        # P2SH + Redeem script
        multisig = get_multisig(self.nodes[0])
        self.nodes[1].generate(COINBASE_MATURITY)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']
        self.nodes[1].syncwithvalidationinterfacequeue()

        self.log.info("Should import a p2sh with respective redeem script")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_addr},
                               "timestamp": "now",
                               "redeemscript": multisig.redeem_script},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     multisig.p2sh_addr, timestamp=timestamp, iswatchonly=True, ismine=False, solvable=True)

        p2shunspent = self.nodes[1].listunspent(0, 999999, [multisig.p2sh_addr])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)

        # P2SH + Redeem script + Private Keys + !Watchonly
        multisig = get_multisig(self.nodes[0])
        self.nodes[1].generate(COINBASE_MATURITY)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']
        self.nodes[1].syncwithvalidationinterfacequeue()

        self.log.info("Should import a p2sh with respective redeem script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_addr},
                               "timestamp": "now",
                               "redeemscript": multisig.redeem_script,
                               "keys": multisig.privkeys[0:2]},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     multisig.p2sh_addr,
                     timestamp=timestamp,
                     ismine=False,
                     iswatchonly=True,
                     solvable=True)

        p2shunspent = self.nodes[1].listunspent(0, 999999, [multisig.p2sh_addr])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)

        # P2SH + Redeem script + Private Keys + Watchonly
        multisig = get_multisig(self.nodes[0])
        self.nodes[1].generate(COINBASE_MATURITY)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']
        self.nodes[1].syncwithvalidationinterfacequeue()

        self.log.info("Should import a p2sh with respective redeem script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_addr},
                               "timestamp": "now",
                               "redeemscript": multisig.redeem_script,
                               "keys": multisig.privkeys[0:2],
                               "watchonly": True},
                              success=True)
        test_address(self.nodes[1],
                     multisig.p2sh_addr,
                     iswatchonly=True,
                     ismine=False,
                     solvable=True,
                     timestamp=timestamp)

        # Address + Public key + !Internal + Wrong pubkey
        self.log.info("Should not import an address with the wrong public key as non-solvable")
        key = get_key(self.nodes[0])
        wrong_key = get_key(self.nodes[0]).pubkey
        self.test_importmulti({"scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now",
                               "pubkeys": [wrong_key]},
                              success=True,
                              warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     solvable=False,
                     timestamp=timestamp)

        # ScriptPubKey + Public key + internal + Wrong pubkey
        self.log.info("Should import a scriptPubKey with internal and with a wrong public key as non-solvable")
        key = get_key(self.nodes[0])
        wrong_key = get_key(self.nodes[0]).pubkey
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "pubkeys": [wrong_key],
                               "internal": True},
                              success=True,
                              warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     solvable=False,
                     timestamp=timestamp)

        # Address + Private key + !watchonly + Wrong private key
        self.log.info("Should import an address with a wrong private key as non-solvable")
        key = get_key(self.nodes[0])
        wrong_privkey = get_key(self.nodes[0]).privkey
        self.test_importmulti({"scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now",
                               "keys": [wrong_privkey]},
                               success=True,
                               warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     solvable=False,
                     timestamp=timestamp)

        # ScriptPubKey + Private key + internal + Wrong private key
        self.log.info("Should import a scriptPubKey with internal and with a wrong private key as non-solvable")
        key = get_key(self.nodes[0])
        wrong_privkey = get_key(self.nodes[0]).privkey
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "keys": [wrong_privkey],
                               "internal": True},
                              success=True,
                              warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     iswatchonly=True,
                     ismine=False,
                     solvable=False,
                     timestamp=timestamp)

        # Importing existing watch only address with new timestamp should replace saved timestamp.
        assert_greater_than(timestamp, watchonly_timestamp)
        self.log.info("Should replace previously saved watch only timestamp.")
        self.test_importmulti({"scriptPubKey": {"address": watchonly_address},
                               "timestamp": "now"},
                              success=True)
        test_address(self.nodes[1],
                     watchonly_address,
                     iswatchonly=True,
                     ismine=False,
                     timestamp=timestamp)
        watchonly_timestamp = timestamp

        # restart nodes to check for proper serialization/deserialization of watch only address
        self.stop_nodes()
        self.start_nodes()
        test_address(self.nodes[1],
                     watchonly_address,
                     iswatchonly=True,
                     ismine=False,
                     timestamp=watchonly_timestamp)

        # Bad or missing timestamps
        self.log.info("Should throw on invalid or missing timestamp values")
        assert_raises_rpc_error(-3, 'Missing required timestamp field for key',
                                self.nodes[1].importmulti, [{"scriptPubKey": key.p2pkh_script}])
        assert_raises_rpc_error(-3, 'Expected number or "now" timestamp value for key. got type string',
                                self.nodes[1].importmulti, [{
                                    "scriptPubKey": key.p2pkh_script,
                                    "timestamp": ""
                                }])

        # Test ranged descriptor fails if range is not specified
        xpriv = "tprv8ZgxMBicQKsPeuVhWwi6wuMQGfPKi9Li5GtX35jVNknACgqe3CY4g5xgkfDDJcmtF7o1QnxWDRYw4H5P26PXq7sbcUkEqeR4fg3Kxp2tigg"
        desc = "sh(pkh(" + xpriv + "/0'/0'/*'" + "))"
        self.log.info("Ranged descriptor import should fail without a specified range")
        self.test_importmulti({"desc": descsum_create(desc),
                               "timestamp": "now"},
                              success=False,
                              error_code=-8,
                              error_message='Descriptor is ranged, please specify the range')

        # Test importing of a ranged descriptor with xpriv
        self.log.info("Should import the ranged descriptor with specified range as solvable")
        self.test_importmulti({"desc": descsum_create(desc),
                               "timestamp": "now",
                               "range": 1},
                              success=True)

        self.test_importmulti({"desc": descsum_create(desc), "timestamp": "now", "range": -1},
                              success=False, error_code=-8, error_message='End of range is too high')

        self.test_importmulti({"desc": descsum_create(desc), "timestamp": "now", "range": [-1, 10]},
                              success=False, error_code=-8, error_message='Range should be greater or equal than 0')

        self.test_importmulti({"desc": descsum_create(desc), "timestamp": "now", "range": [(2 << 31 + 1) - 1000000, (2 << 31 + 1)]},
                              success=False, error_code=-8, error_message='End of range is too high')

        self.test_importmulti({"desc": descsum_create(desc), "timestamp": "now", "range": [2, 1]},
                              success=False, error_code=-8, error_message='Range specified as [begin,end] must not have begin after end')

        self.test_importmulti({"desc": descsum_create(desc), "timestamp": "now", "range": [0, 1000001]},
                              success=False, error_code=-8, error_message='Range is too large')

        # Test importing a descriptor containing a WIF private key
        wif_priv = "cTT3BvHnd51YJf8fkdr2XvZTQRRUZruWhRvRyQY1raVFg5Lvam2A"
        address = "ySWABbcNKyHUgBb1ffhpuETuis9jsdR3aq"
        desc = "sh(pkh(" + wif_priv + "))"
        self.log.info("Should import a descriptor with a WIF private key as spendable")
        self.test_importmulti({"desc": descsum_create(desc),
                               "timestamp": "now"},
                              success=True)
        test_address(self.nodes[1],
                     address,
                     solvable=True,
                     ismine=True)

        # dump the private key to ensure it matches what was imported
        privkey = self.nodes[1].dumpprivkey(address)
        assert_equal(privkey, wif_priv)

        # Test importing of a P2PKH address via descriptor
        key = get_key(self.nodes[0])
        p2pkh_label = "P2PKH descriptor import"
        self.log.info("Should import a p2pkh address from descriptor")
        self.test_importmulti({"desc": descsum_create("pkh(" + key.pubkey + ")"),
                               "timestamp": "now",
                               "label": p2pkh_label},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2pkh_addr,
                     solvable=True,
                     ismine=False,
                     labels=[p2pkh_label])

        # Test import fails if both desc and scriptPubKey are provided
        key = get_key(self.nodes[0])
        self.log.info("Import should fail if both scriptPubKey and desc are provided")
        self.test_importmulti({"desc": descsum_create("pkh(" + key.pubkey + ")"),
                               "scriptPubKey": {"address": key.p2pkh_addr},
                               "timestamp": "now"},
                              success=False,
                              error_code=-8,
                              error_message='Both a descriptor and a scriptPubKey should not be provided.')

        # Test import fails if neither desc nor scriptPubKey are present
        key = get_key(self.nodes[0])
        self.log.info("Import should fail if neither a descriptor nor a scriptPubKey are provided")
        self.test_importmulti({"timestamp": "now"},
                              success=False,
                              error_code=-8,
                              error_message='Either a descriptor or scriptPubKey must be provided.')

        # Test importing of a multisig via descriptor
        key1 = get_key(self.nodes[0])
        key2 = get_key(self.nodes[0])
        self.log.info("Should import a 1-of-2 bare multisig from descriptor")
        self.test_importmulti({"desc": descsum_create("multi(1," + key1.pubkey + "," + key2.pubkey + ")"),
                               "timestamp": "now"},
                              True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        self.log.info("Should not treat individual keys from the imported bare multisig as watchonly")
        test_address(self.nodes[1],
                         key1.p2pkh_addr,
                         ismine=False,
                         iswatchonly=False)

        # Import pubkeys with key origin info
        self.log.info("Addresses should have hd keypath and master key id after import with key origin")
        pub_addr = self.nodes[1].getnewaddress()
        pub_addr = self.nodes[1].getnewaddress()
        info = self.nodes[1].getaddressinfo(pub_addr)
        pub = info['pubkey']
        pub_keypath = info['hdkeypath']
        pub_fpr = info['hdmasterfingerprint']
        result = self.nodes[0].importmulti(
            [{
                'desc' : descsum_create("pkh([" + pub_fpr + pub_keypath[1:] +"]" + pub + ")"),
                "timestamp": "now",
            }]
        )
        assert result[0]['success']
        pub_import_info = self.nodes[0].getaddressinfo(pub_addr)
        assert_equal(pub_import_info['hdmasterfingerprint'], pub_fpr)
        assert_equal(pub_import_info['pubkey'], pub)
        assert_equal(pub_import_info['hdkeypath'], pub_keypath)

        # Import privkeys with key origin info
        priv_addr = self.nodes[1].getnewaddress()
        info = self.nodes[1].getaddressinfo(priv_addr)
        priv = self.nodes[1].dumpprivkey(priv_addr)
        priv_keypath = info['hdkeypath']
        priv_fpr = info['hdmasterfingerprint']
        result = self.nodes[0].importmulti(
            [{
                'desc' : descsum_create("pkh([" + priv_fpr + priv_keypath[1:] + "]" + priv + ")"),
                "timestamp": "now",
            }]
        )
        assert result[0]['success']
        priv_import_info = self.nodes[0].getaddressinfo(priv_addr)
        assert_equal(priv_import_info['hdmasterfingerprint'], priv_fpr)
        assert_equal(priv_import_info['hdkeypath'], priv_keypath)

        # Make sure the key origin info are still there after a restart
        self.stop_nodes()
        self.start_nodes()
        import_info = self.nodes[0].getaddressinfo(pub_addr)
        assert_equal(import_info['hdmasterfingerprint'], pub_fpr)
        assert_equal(import_info['hdkeypath'], pub_keypath)
        import_info = self.nodes[0].getaddressinfo(priv_addr)
        assert_equal(import_info['hdmasterfingerprint'], priv_fpr)
        assert_equal(import_info['hdkeypath'], priv_keypath)

        # Check legacy import does not import key origin info
        self.log.info("Legacy imports don't have key origin info")
        pub_addr = self.nodes[1].getnewaddress()
        info = self.nodes[1].getaddressinfo(pub_addr)
        pub = info['pubkey']
        result = self.nodes[0].importmulti(
            [{
                'scriptPubKey': {'address': pub_addr},
                'pubkeys': [pub],
                "timestamp": "now",
            }]
        )
        assert result[0]['success']
        pub_import_info = self.nodes[0].getaddressinfo(pub_addr)
        assert_equal(pub_import_info['pubkey'], pub)
        assert 'hdmasterfingerprint' not in pub_import_info
        assert 'hdkeypath' not in pub_import_info

        # Import some public keys to the keypool of a no privkey wallet
        self.log.info("Adding pubkey to keypool of disableprivkey wallet")
        self.nodes[1].createwallet(wallet_name="noprivkeys", disable_private_keys=True)
        wrpc = self.nodes[1].get_wallet_rpc("noprivkeys")

        addr1 = self.nodes[0].getnewaddress()
        addr2 = self.nodes[0].getnewaddress()
        pub1 = self.nodes[0].getaddressinfo(addr1)['pubkey']
        pub2 = self.nodes[0].getaddressinfo(addr2)['pubkey']
        result = wrpc.importmulti(
            [{
                'desc': descsum_create('pkh(' + pub1 + ')'),
                'keypool': True,
                "timestamp": "now",
            },
            {
                'desc': descsum_create('pkh(' + pub2 + ')'),
                'keypool': True,
                "timestamp": "now",
            }]
        )
        assert result[0]['success']
        assert result[1]['success']
        assert_equal(wrpc.getwalletinfo()["keypoolsize"], 2)
        newaddr1 = wrpc.getnewaddress()
        assert_equal(addr1, newaddr1)
        newaddr2 = wrpc.getnewaddress()
        assert_equal(addr2, newaddr2)

        # Import some public keys to the internal keypool of a no privkey wallet
        self.log.info("Adding pubkey to internal keypool of disableprivkey wallet")
        addr1 = self.nodes[0].getnewaddress()
        addr2 = self.nodes[0].getnewaddress()
        pub1 = self.nodes[0].getaddressinfo(addr1)['pubkey']
        pub2 = self.nodes[0].getaddressinfo(addr2)['pubkey']
        result = wrpc.importmulti(
            [{
                'desc': descsum_create('pkh(' + pub1 + ')'),
                'keypool': True,
                'internal': True,
                "timestamp": "now",
            },
            {
                'desc': descsum_create('pkh(' + pub2 + ')'),
                'keypool': True,
                'internal': True,
                "timestamp": "now",
            }]
        )
        assert result[0]['success']
        assert result[1]['success']
        assert_equal(wrpc.getwalletinfo()["keypoolsize_hd_internal"], 2)
        newaddr1 = wrpc.getrawchangeaddress()
        assert_equal(addr1, newaddr1)
        newaddr2 = wrpc.getrawchangeaddress()
        assert_equal(addr2, newaddr2)

        # Import a multisig and make sure the keys don't go into the keypool
        self.log.info('Imported scripts with pubkeys should not have their pubkeys go into the keypool')
        addr1 = self.nodes[0].getnewaddress()
        addr2 = self.nodes[0].getnewaddress()
        pub1 = self.nodes[0].getaddressinfo(addr1)['pubkey']
        pub2 = self.nodes[0].getaddressinfo(addr2)['pubkey']
        result = wrpc.importmulti(
            [{
                'desc': descsum_create('sh(multi(2,' + pub1 + ',' + pub2 + '))'),
                'keypool': True,
                "timestamp": "now",
            }]
        )
        assert result[0]['success']
        assert_equal(wrpc.getwalletinfo()["keypoolsize"], 0)

        # Cannot import those pubkeys to keypool of wallet with privkeys
        self.log.info("Pubkeys cannot be added to the keypool of a wallet with private keys")
        wrpc = self.nodes[1].get_wallet_rpc(self.default_wallet_name)
        assert wrpc.getwalletinfo()['private_keys_enabled']
        result = wrpc.importmulti(
            [{
                'desc': descsum_create('pkh(' + pub1 + ')'),
                'keypool': True,
                "timestamp": "now",
            }]
        )
        assert_equal(result[0]['error']['code'], -8)
        assert_equal(result[0]['error']['message'], "Keys can only be imported to the keypool when private keys are disabled")

        # Make sure ranged imports import keys in order
        self.log.info('Key ranges should be imported in order')
        wrpc = self.nodes[1].get_wallet_rpc("noprivkeys")
        assert_equal(wrpc.getwalletinfo()["keypoolsize"], 0)
        assert_equal(wrpc.getwalletinfo()["private_keys_enabled"], False)
        xpub = "tpubDAXcJ7s7ZwicqjprRaEWdPoHKrCS215qxGYxpusRLLmJuT69ZSicuGdSfyvyKpvUNYBW1s2U3NSrT6vrCYB9e6nZUEvrqnwXPF8ArTCRXMY"
        addresses = [
            'yUxX4qnzWntXhEGrYB92v7ez4EZBnUjB1y', # m/0'/0'/0
            'yRhTPsPd2qYgYbFFCqY2nuPHJQBjTnMQxg', # m/0'/0'/1
            'yUyn3UV9rBdWfw6yJJ6eAoKuzDJ8RVLP1o', # m/0'/0'/2
            'yi8GEkfLBgK85wGmBFsMFdSbEvPPNCSnVx', # m/0'/0'/3
            'yYB4whdY8APWoCez6ryNdMBrrDjwzFbqMi', # m/0'/0'/4
        ]
        result = wrpc.importmulti(
            [{
                'desc': descsum_create('pkh([80002067/0h/0h]' + xpub + '/*)'),
                'keypool': True,
                'timestamp': 'now',
                'range' : [0, 4],
            }]
        )
        for i in range(0, 5):
            addr = wrpc.getnewaddress('')
            assert_equal(addr, addresses[i])


if __name__ == '__main__':
    ImportMultiTest().main()
