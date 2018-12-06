#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the importmulti RPC."""
from collections import namedtuple

from test_framework.address import (
    key_to_p2pkh,
    key_to_p2sh_p2wpkh,
    key_to_p2wpkh,
    script_to_p2sh,
    script_to_p2sh_p2wsh,
    script_to_p2wsh,
)
from test_framework.script import (
    CScript,
    OP_0,
    OP_2,
    OP_3,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_DUP,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_HASH160,
    OP_NOP,
    hash160,
    sha256,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    bytes_to_hex_str,
    hex_str_to_bytes
)

Key = namedtuple('Key', ['privkey',
                         'pubkey',
                         'p2pkh_script',
                         'p2pkh_addr',
                         'p2wpkh_script',
                         'p2wpkh_addr',
                         'p2sh_p2wpkh_script',
                         'p2sh_p2wpkh_redeem_script',
                         'p2sh_p2wpkh_addr'])

Multisig = namedtuple('Multisig', ['privkeys',
                                   'pubkeys',
                                   'p2sh_script',
                                   'p2sh_addr',
                                   'redeem_script',
                                   'p2wsh_script',
                                   'p2wsh_addr',
                                   'p2sh_p2wsh_script',
                                   'p2sh_p2wsh_addr'])

class ImportMultiTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-addresstype=legacy"], ["-addresstype=legacy"]]
        self.setup_clean_chain = True

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self):
        self.setup_nodes()

    def get_key(self):
        """Generate a fresh key on node0

        Returns a named tuple of privkey, pubkey and all address and scripts."""
        addr = self.nodes[0].getnewaddress()
        pubkey = self.nodes[0].getaddressinfo(addr)['pubkey']
        pkh = hash160(hex_str_to_bytes(pubkey))
        return Key(self.nodes[0].dumpprivkey(addr),
                   pubkey,
                   CScript([OP_DUP, OP_HASH160, pkh, OP_EQUALVERIFY, OP_CHECKSIG]).hex(),  # p2pkh
                   key_to_p2pkh(pubkey),  # p2pkh addr
                   CScript([OP_0, pkh]).hex(),  # p2wpkh
                   key_to_p2wpkh(pubkey),  # p2wpkh addr
                   CScript([OP_HASH160, hash160(CScript([OP_0, pkh])), OP_EQUAL]).hex(),  # p2sh-p2wpkh
                   CScript([OP_0, pkh]).hex(),  # p2sh-p2wpkh redeem script
                   key_to_p2sh_p2wpkh(pubkey))  # p2sh-p2wpkh addr

    def get_multisig(self):
        """Generate a fresh multisig on node0

        Returns a named tuple of privkeys, pubkeys and all address and scripts."""
        addrs = []
        pubkeys = []
        for _ in range(3):
            addr = self.nodes[0].getaddressinfo(self.nodes[0].getnewaddress())
            addrs.append(addr['address'])
            pubkeys.append(addr['pubkey'])
        script_code = CScript([OP_2] + [hex_str_to_bytes(pubkey) for pubkey in pubkeys] + [OP_3, OP_CHECKMULTISIG])
        witness_script = CScript([OP_0, sha256(script_code)])
        return Multisig([self.nodes[0].dumpprivkey(addr) for addr in addrs],
                        pubkeys,
                        CScript([OP_HASH160, hash160(script_code), OP_EQUAL]).hex(),  # p2sh
                        script_to_p2sh(script_code),  # p2sh addr
                        script_code.hex(),  # redeem script
                        witness_script.hex(),  # p2wsh
                        script_to_p2wsh(script_code),  # p2wsh addr
                        CScript([OP_HASH160, witness_script, OP_EQUAL]).hex(),  # p2sh-p2wsh
                        script_to_p2sh_p2wsh(script_code))  # p2sh-p2wsh addr

    def test_importmulti(self, req, success, error_code=None, error_message=None):
        """Run importmulti and assert success"""
        result = self.nodes[1].importmulti([req])
        assert_equal(result[0]['success'], success)
        if error_code is not None:
            assert_equal(result[0]['error']['code'], error_code)
            assert_equal(result[0]['error']['message'], error_message)

    def test_address(self, address, **kwargs):
        """Get address info for `address` and test whether the returned values are as expected."""
        addr_info = self.nodes[1].getaddressinfo(address)
        for key, value in kwargs.items():
            if value is None:
                if key in addr_info.keys():
                    raise AssertionError("key {} unexpectedly returned in getaddressinfo.".format(key))
            elif addr_info[key] != value:
                raise AssertionError("key {} value {} did not match expected value {}".format(key, addr_info[key], value))

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
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now"},
                              True)
        self.test_address(address,
                          iswatchonly=True,
                          ismine=False,
                          timestamp=timestamp,
                          ischange=False)
        watchonly_address = address
        watchonly_timestamp = timestamp

        self.log.info("Should not import an invalid address")
        self.test_importmulti({"scriptPubKey": {"address": "not valid address"},
                               "timestamp": "now"},
                              False,
                              error_code=-5,
                              error_message='Invalid address')

        # ScriptPubKey + internal
        self.log.info("Should import a scriptPubKey with internal flag")
        key = self.get_key()
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "internal": True},
                              True)
        self.test_address(key.p2pkh_addr,
                          iswatchonly=True,
                          ismine=False,
                          timestamp=timestamp,
                          ischange=True)

        # ScriptPubKey + internal + label
        self.log.info("Should not allow a label to be specified when internal is true")
        key = self.get_key()
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "internal": True,
                               "label": "Example label"},
                              False,
                              error_code=-8,
                              error_message='Internal addresses should not have a label')

        # Nonstandard scriptPubKey + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal flag")
        nonstandardScriptPubKey = key.p2pkh_script + bytes_to_hex_str(CScript([OP_NOP]))
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": nonstandardScriptPubKey,
                               "timestamp": "now"},
                              False,
                              error_code=-8,
                              error_message='Internal must be set to true for nonstandard scriptPubKey imports.')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # Address + Public key + !Internal(explicit)
        self.log.info("Should import an address with public key")
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "pubkeys": [key.pubkey],
                               "internal": False},
                              True)
        self.test_address(address,
                          iswatchonly=True,
                          ismine=False,
                          timestamp=timestamp)

        # ScriptPubKey + Public key + internal
        self.log.info("Should import a scriptPubKey with internal and with public key")
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "pubkeys": [key.pubkey],
                               "internal": True},
                              True)
        self.test_address(address,
                          iswatchonly=True,
                          ismine=False,
                          timestamp=timestamp)

        # Nonstandard scriptPubKey + Public key + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal and with public key")
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": nonstandardScriptPubKey,
                               "timestamp": "now",
                               "pubkeys": [key.pubkey]},
                              False,
                              error_code=-8,
                              error_message='Internal must be set to true for nonstandard scriptPubKey imports.')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # Address + Private key + !watchonly
        self.log.info("Should import an address with private key")
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              True)
        self.test_address(address,
                          iswatchonly=False,
                          ismine=True,
                          timestamp=timestamp)

        self.log.info("Should not import an address with private key if is already imported")
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              False,
                              error_code=-4,
                              error_message='The wallet already contains the private key for this address or script')

        # Address + Private key + watchonly
        self.log.info("Should not import an address with private key and with watchonly")
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "keys": [key.privkey],
                               "watchonly": True},
                              False,
                              error_code=-8,
                              error_message='Watch-only addresses should not include private keys')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # ScriptPubKey + Private key + internal
        self.log.info("Should import a scriptPubKey with internal and with private key")
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "keys": [key.privkey],
                               "internal": True},
                              True)
        self.test_address(address,
                          iswatchonly=False,
                          ismine=True,
                          timestamp=timestamp)

        # Nonstandard scriptPubKey + Private key + !internal
        self.log.info("Should not import a nonstandard scriptPubKey without internal and with private key")
        key = self.get_key()
        address = key.p2pkh_addr
        self.test_importmulti({"scriptPubKey": nonstandardScriptPubKey,
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              False,
                              error_code=-8,
                              error_message='Internal must be set to true for nonstandard scriptPubKey imports.')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # P2SH address
        multisig = self.get_multisig()
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        self.log.info("Should import a p2sh")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_addr},
                               "timestamp": "now"},
                              True)
        self.test_address(multisig.p2sh_addr,
                          isscript=True,
                          iswatchonly=True,
                          timestamp=timestamp)
        p2shunspent = self.nodes[1].listunspent(0, 999999, [multisig.p2sh_addr])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], False)

        # P2SH + Redeem script
        multisig = self.get_multisig()
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        self.log.info("Should import a p2sh with respective redeem script")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_addr},
                               "timestamp": "now",
                               "redeemscript": multisig.redeem_script},
                              True)
        self.test_address(multisig.p2sh_addr, timestamp=timestamp)

        p2shunspent = self.nodes[1].listunspent(0, 999999, [multisig.p2sh_addr])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)

        # P2SH + Redeem script + Private Keys + !Watchonly
        multisig = self.get_multisig()
        self.nodes[1].generate(100)
        self.nodes[1].sendtoaddress(multisig.p2sh_addr, 10.00)
        self.nodes[1].generate(1)
        timestamp = self.nodes[1].getblock(self.nodes[1].getbestblockhash())['mediantime']

        self.log.info("Should import a p2sh with respective redeem script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_addr},
                               "timestamp": "now",
                               "redeemscript": multisig.redeem_script,
                               "keys": multisig.privkeys[0:2]},
                              True)
        self.test_address(multisig.p2sh_addr,
                          timestamp=timestamp)

        p2shunspent = self.nodes[1].listunspent(0, 999999, [multisig.p2sh_addr])[0]
        assert_equal(p2shunspent['spendable'], False)
        assert_equal(p2shunspent['solvable'], True)

        # P2SH + Redeem script + Private Keys + Watchonly
        multisig = self.get_multisig()
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
                              False,
                              error_code=-8,
                              error_message='Watch-only addresses should not include private keys')

        # Address + Public key + !Internal + Wrong pubkey
        self.log.info("Should not import an address with a wrong public key")
        key = self.get_key()
        address = key.p2pkh_addr
        wrong_key = self.get_key().pubkey
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "pubkeys": [wrong_key]},
                              False,
                              error_code=-5,
                              error_message='Key does not match address destination')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # ScriptPubKey + Public key + internal + Wrong pubkey
        self.log.info("Should not import a scriptPubKey with internal and with a wrong public key")
        key = self.get_key()
        address = key.p2pkh_addr
        wrong_key = self.get_key().pubkey
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "pubkeys": [wrong_key],
                               "internal": True},
                              False,
                              error_code=-5,
                              error_message='Key does not match address destination')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # Address + Private key + !watchonly + Wrong private key
        self.log.info("Should not import an address with a wrong private key")
        key = self.get_key()
        address = key.p2pkh_addr
        wrong_privkey = self.get_key().privkey
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "keys": [wrong_privkey]},
                              False,
                              error_code=-5,
                              error_message='Key does not match address destination')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # ScriptPubKey + Private key + internal + Wrong private key
        self.log.info("Should not import a scriptPubKey with internal and with a wrong private key")
        key = self.get_key()
        address = key.p2pkh_addr
        wrong_privkey = self.get_key().privkey
        self.test_importmulti({"scriptPubKey": key.p2pkh_script,
                               "timestamp": "now",
                               "keys": [wrong_privkey],
                               "internal": True},
                              False,
                              error_code=-5,
                              error_message='Key does not match address destination')
        self.test_address(address,
                          iswatchonly=False,
                          ismine=False,
                          timestamp=None)

        # Importing existing watch only address with new timestamp should replace saved timestamp.
        assert_greater_than(timestamp, watchonly_timestamp)
        self.log.info("Should replace previously saved watch only timestamp.")
        self.test_importmulti({"scriptPubKey": {"address": watchonly_address},
                               "timestamp": "now"},
                              True)
        self.test_address(watchonly_address,
                          iswatchonly=True,
                          ismine=False,
                          timestamp=timestamp)
        watchonly_timestamp = timestamp

        # restart nodes to check for proper serialization/deserialization of watch only address
        self.stop_nodes()
        self.start_nodes()
        self.test_address(watchonly_address,
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
        key = self.get_key()
        address = key.p2wpkh_addr
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now"},
                              True)
        self.test_address(address,
                          iswatchonly=True,
                          solvable=False)

        # Import P2WPKH address with public key but no private key
        self.log.info("Should import a P2WPKH address and public key as solvable but not spendable")
        key = self.get_key()
        address = key.p2wpkh_addr
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "pubkeys": [key.pubkey]},
                              True)
        self.test_address(address,
                          ismine=False,
                          solvable=True)

        # Import P2WPKH address with key and check it is spendable
        self.log.info("Should import a P2WPKH address with key")
        key = self.get_key()
        address = key.p2wpkh_addr
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "keys": [key.privkey]},
                              True)
        self.test_address(address,
                          iswatchonly=False,
                          ismine=True)

        # P2WSH multisig address without scripts or keys
        multisig = self.get_multisig()
        self.log.info("Should import a p2wsh multisig as watch only without respective redeem script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2wsh_addr},
                               "timestamp": "now"},
                              True)
        self.test_address(multisig.p2sh_addr,
                          solvable=False)

        # Same P2WSH multisig address as above, but now with witnessscript + private keys
        self.log.info("Should import a p2wsh with respective witness script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2wsh_addr},
                               "timestamp": "now",
                               "witnessscript": multisig.redeem_script,
                               "keys": multisig.privkeys},
                              True)
        self.test_address(multisig.p2sh_addr,
                          solvable=True,
                          ismine=True,
                          sigsrequired=2)

        # P2SH-P2WPKH address with no redeemscript or public or private key
        key = self.get_key()
        address = key.p2sh_p2wpkh_addr
        self.log.info("Should import a p2sh-p2wpkh without redeem script or keys")
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now"},
                              True)
        self.test_address(address,
                          solvable=False,
                          ismine=False)

        # P2SH-P2WPKH address + redeemscript + public key with no private key
        self.log.info("Should import a p2sh-p2wpkh with respective redeem script and pubkey as solvable")
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "redeemscript": key.p2sh_p2wpkh_redeem_script,
                               "pubkeys": [key.pubkey]},
                              True)
        self.test_address(address,
                          solvable=True,
                          ismine=False)

        # P2SH-P2WPKH address + redeemscript + private key
        key = self.get_key()
        address = key.p2sh_p2wpkh_addr
        self.log.info("Should import a p2sh-p2wpkh with respective redeem script and private keys")
        self.test_importmulti({"scriptPubKey": {"address": address},
                               "timestamp": "now",
                               "redeemscript": key.p2sh_p2wpkh_redeem_script,
                               "keys": [key.privkey]},
                              True)
        self.test_address(address,
                          solvable=True,
                          ismine=True)

        # P2SH-P2WSH multisig + redeemscript with no private key
        multisig = self.get_multisig()
        self.log.info("Should import a p2sh-p2wsh with respective redeem script but no private key")
        self.test_importmulti({"scriptPubKey": {"address": multisig.p2sh_p2wsh_addr},
                               "timestamp": "now",
                               "redeemscript": multisig.p2wsh_script,
                               "witnessscript": multisig.redeem_script},
                              True)
        self.test_address(address,
                          solvable=True)

if __name__ == '__main__':
    ImportMultiTest().main()
