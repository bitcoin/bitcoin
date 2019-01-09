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

from test_framework.script import (
    CScript,
    OP_NOP,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    bytes_to_hex_str,
)
from test_framework.wallet_util import (
    get_key,
    get_multisig,
    test_address,
)

class ImportMultiTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-addresstype=legacy"], ["-addresstype=legacy"]]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def test_importmulti(self, req, success, error_code=None, error_message=None, warnings=[]):
        """Run importmulti and assert success"""
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
                               "label": "Example label"},
                              success=False,
                              error_code=-8,
                              error_message='Internal addresses should not have a label')

        # Nonstandard scriptPubKey + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal flag")
        nonstandardScriptPubKey = key.p2pkh_script + bytes_to_hex_str(CScript([OP_NOP]))
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
                              error_message='The wallet already contains the private key for this address or script')

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
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

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
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

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
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

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
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

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
                              warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, witnessscript, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
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
                              warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, witnessscript, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
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
                               warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, witnessscript, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
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
                              warnings=["Importing as non-solvable: some required keys are missing. If this is intentional, don't provide any keys, pubkeys, witnessscript, or redeemscript.", "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
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

        # Import P2WPKH address as watch only
        self.log.info("Should import a P2WPKH address as watch only")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": {"address": key.p2wpkh_addr},
                               "timestamp": "now"},
                              success=True)
        test_address(self.nodes[1],
                     key.p2wpkh_addr,
                     iswatchonly=True,
                     solvable=False)

        # Import P2WPKH address with public key but no private key
        self.log.info("Should import a P2WPKH address and public key as solvable but not spendable")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": {"address": key.p2wpkh_addr},
                               "timestamp": "now",
                               "pubkeys": [key.pubkey]},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2wpkh_addr,
                     ismine=False,
                     solvable=True)

        # Import P2WPKH address with key and check it is spendable
        self.log.info("Should import a P2WPKH address with key")
        key = get_key(self.nodes[0])
        self.test_importmulti({"scriptPubKey": {"address": key.p2wpkh_addr},
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              success=True)
        test_address(self.nodes[1],
                     key.p2wpkh_addr,
                     iswatchonly=False,
                     ismine=True)

        # P2WSH multisig address without scripts or keys
        multisig = get_multisig(self.nodes[0])
        self.log.info("Should import a p2wsh multisig as watch only without respective redeem script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2wsh_addr},
                               "timestamp": "now"},
                              success=True)
        test_address(self.nodes[1],
                     multisig.p2sh_addr,
                     solvable=False)

        # Same P2WSH multisig address as above, but now with witnessscript + private keys
        self.log.info("Should import a p2wsh with respective witness script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2wsh_addr},
                               "timestamp": "now",
                               "witnessscript": multisig.redeem_script,
                               "keys": multisig.privkeys},
                              success=True)
        test_address(self.nodes[1],
                     multisig.p2sh_addr,
                     solvable=True,
                     ismine=True,
                     sigsrequired=2)

        # P2SH-P2WPKH address with no redeemscript or public or private key
        key = get_key(self.nodes[0])
        self.log.info("Should import a p2sh-p2wpkh without redeem script or keys")
        self.test_importmulti({"scriptPubKey": {"address": key.p2sh_p2wpkh_addr},
                               "timestamp": "now"},
                              success=True)
        test_address(self.nodes[1],
                     key.p2sh_p2wpkh_addr,
                     solvable=False,
                     ismine=False)

        # P2SH-P2WPKH address + redeemscript + public key with no private key
        self.log.info("Should import a p2sh-p2wpkh with respective redeem script and pubkey as solvable")
        self.test_importmulti({"scriptPubKey": {"address": key.p2sh_p2wpkh_addr},
                               "timestamp": "now",
                               "redeemscript": key.p2sh_p2wpkh_redeem_script,
                               "pubkeys": [key.pubkey]},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     key.p2sh_p2wpkh_addr,
                     solvable=True,
                     ismine=False)

        # P2SH-P2WPKH address + redeemscript + private key
        key = get_key(self.nodes[0])
        self.log.info("Should import a p2sh-p2wpkh with respective redeem script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": key.p2sh_p2wpkh_addr},
                               "timestamp": "now",
                               "redeemscript": key.p2sh_p2wpkh_redeem_script,
                               "keys": [key.privkey]},
                              success=True)
        test_address(self.nodes[1],
                     key.p2sh_p2wpkh_addr,
                     solvable=True,
                     ismine=True)

        # P2SH-P2WSH multisig + redeemscript with no private key
        multisig = get_multisig(self.nodes[0])
        self.log.info("Should import a p2sh-p2wsh with respective redeem script but no private key")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_p2wsh_addr},
                               "timestamp": "now",
                               "redeemscript": multisig.p2wsh_script,
                               "witnessscript": multisig.redeem_script},
                              success=True,
                              warnings=["Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag."])
        test_address(self.nodes[1],
                     multisig.p2sh_p2wsh_addr,
                     solvable=True,
                     ismine=False)

if __name__ == '__main__':
    ImportMultiTest().main()
