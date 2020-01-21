#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test miniscript implementation."""

import hashlib
from io import BytesIO

from test_framework.address import script_to_p2wsh
from test_framework.key import ECKey
from test_framework.miniscript import Node, SatType
from test_framework.messages import CTransaction, CTxInWitness, COutPoint, \
    CTxIn, CTxOut, hash256, sha256
from test_framework.script import CScript, hash160, \
    SegwitV0SignatureHash, SIGHASH_ALL
from test_framework.test_framework import BitcoinTestFramework


class MiniscriptTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def test_miniscript_expr(self, desc_str, property_str,
                             alt_desc_str=None):
        node = Node.from_desc(desc_str)
        node_from_script = Node.from_script(node.script)
        assert(''.join(sorted(property_str)) ==
               ''.join(sorted(node.p.to_string()))
               )
        assert(node.desc == desc_str)
        assert(node_from_script.script == node.script)
        if alt_desc_str is None:
            assert(node_from_script.desc == node.desc)
        else:
            assert(node_from_script.desc == alt_desc_str)
        return node

    def test_miniscript_satisfaction(self, node, keyhash_dict, sha256_dict,
                                     hash256_dict, ripemd160_dict,
                                     hash160_dict):
        # Send funds to p2wsh(miniscript.script).
        v0_address = script_to_p2wsh(node.script)
        tx = self.generate_and_send_coin(v0_address)
        # Construct spending transaction.
        minfee_sat = int(self.nodes[0].getmempoolinfo()[
            'mempoolminfee'] * 100000000)
        spending_tx = self.create_spending_transaction(
            tx.hash, amount=tx.vout[0].nValue-minfee_sat)
        spending_tx.wit.vtxinwit = [CTxInWitness()]
        node_script = CScript(node.script)
        sig_hash = SegwitV0SignatureHash(node_script,
                                               spending_tx, 0,
                                               SIGHASH_ALL,
                                               tx.vout[0].nValue)
        # Test each canonical AND non-canonical
        # satisfying witness for spending transaction.
        for wit_sat in node.sat:
            witness_arr = []
            for sat_element in wit_sat:
                # Set sequence for input at index 0.
                if sat_element[0] is SatType.OLDER:
                    delay = max(spending_tx.vin[0].nSequence, sat_element[1])
                    spending_tx.vin[0].nSequence = delay
                    # Recompute sighash.
                    sig_hash = SegwitV0SignatureHash(node_script,
                                                           spending_tx, 0,
                                                           SIGHASH_ALL,
                                                           tx.vout[0].nValue)
                # Set locktime.
                if sat_element[0] is SatType.AFTER:
                    locktime = max(spending_tx.nLockTime, sat_element[1])
                    spending_tx.nLockTime = locktime
                    # Recompute sighash.
                    sig_hash = SegwitV0SignatureHash(node_script,
                                                           spending_tx, 0,
                                                           SIGHASH_ALL,
                                                           tx.vout[0].nValue)
                # Add signature witness element.
                if sat_element[0] is SatType.SIGNATURE:
                    # Value: 33B Pubkey
                    if len(sat_element[1]) == 33:
                        pkhash = hash160(sat_element[1])
                        signature = keyhash_dict[pkhash].sign_ecdsa(
                            sig_hash)+b'\x01'
                        witness_arr.append(signature)
                    # Value: 20B Pubkeyhash
                    elif len(sat_element[1]) == 20:
                        signature = keyhash_dict[sat_element[1]].sign_ecdsa(
                            sig_hash) + b'\x01'
                        witness_arr.append(signature)
                # Add public key witness element.
                elif sat_element[0] is SatType.KEY_AND_HASH160_PREIMAGE:
                    secret_key = keyhash_dict[sat_element[1]]
                    witness_arr.append(secret_key.get_pubkey().get_bytes())
                # Add sha256 preimage witness element.
                elif sat_element[0] is SatType.SHA256_PREIMAGE:
                    sha256_preimage = sha256_dict[sat_element[1]]
                    witness_arr.append(sha256_preimage)
                # Add hash256 preimage witness element.
                elif sat_element[0] is SatType.HASH256_PREIMAGE:
                    hash256_preimage = hash256_dict[sat_element[1]]
                    witness_arr.append(hash256_preimage)
                # Add ripemd160 preimage witness element.
                elif sat_element[0] is SatType.RIPEMD160_PREIMAGE:
                    ripemd160_preimage = ripemd160_dict[sat_element[1]]
                    witness_arr.append(ripemd160_preimage)
                # Add hash160 preimage witness element.
                elif sat_element[0] is SatType.HASH160_PREIMAGE:
                    hash160_preimage = hash160_dict[sat_element[1]]
                    witness_arr.append(hash160_preimage)
                # Add data witness element
                elif sat_element[0] is SatType.DATA:
                    witness_arr.append(sat_element[1])
            # Set witness and test mempool acceptance.
            spending_tx.wit.vtxinwit[0].scriptWitness.stack = witness_arr + [
                node_script]
            spending_tx.rehash()
            tx_str = spending_tx.serialize().hex()
            # Mine if delay or timelock.
            if spending_tx.vin[0].nSequence > 0 or spending_tx.nLockTime > 0:
                height = self.nodes[0].getblockchaininfo()['blocks']
                blocks_required = max(spending_tx.nLockTime-height,
                                      spending_tx.vin[0].nSequence)
                self.nodes[0].generate(blocks_required)
            # Test mempool acceptance.
            ret = self.nodes[0].testmempoolaccept(rawtxs=[tx_str],
                                                  maxfeerate=0)[0]
            assert(ret['allowed'])

    # Test Utility Methods.
    def generate_key_pair(self, keyhash_dict):
        privkey = ECKey()
        privkey.generate()
        pubkeyhash = hash160(privkey.get_pubkey().get_bytes())
        keyhash_dict[pubkeyhash] = privkey

    def get_pubkey_hex_list(self, n, keyhash_dict):
        key_hex_list = []
        for pkh in list(keyhash_dict)[0:n]:
            pk_hex = keyhash_dict[pkh].get_pubkey().get_bytes().hex()
            key_hex_list.append(pk_hex)
        return key_hex_list

    def generate_and_send_coin(self, address):
        if self.nodes[0].getblockchaininfo()['blocks'] == 0:
            self.coinbase_address = self.nodes[0].getnewaddress(
                address_type="bech32")
            self.nodes[0].generatetoaddress(101, self.coinbase_address)

        balance = self.nodes[0].getbalance()

        while balance < 1:
            self.nodes[0].generatetoaddress(1, self.coinbase_address)
            balance = self.nodes[0].getbalance()

        unspent_txid = self.nodes[0].listunspent(1)[-1]["txid"]
        inputs = [{"txid": unspent_txid, "vout": 0}]

        # Send funds to address.
        tx_hex = self.nodes[0].createrawtransaction(inputs=inputs,
                                                    outputs=[{address: 1}])
        res = self.nodes[0].signrawtransactionwithwallet(hexstring=tx_hex)
        tx_hex = res["hex"]
        assert res["complete"]
        assert 'errors' not in res

        # Max feerate set to 0 since no change output created.
        txid = self.nodes[0].sendrawtransaction(hexstring=tx_hex, maxfeerate=0)
        tx_hex = self.nodes[0].getrawtransaction(txid)
        tx = CTransaction()
        tx.deserialize(BytesIO(bytes.fromhex(tx_hex)))
        tx.rehash()
        return tx

    def create_spending_transaction(self, txid, amount=0, version=2,
                                    nSequence=0, nLocktime=0):
        spending_tx = CTransaction()
        spending_tx.nVersion = version
        spending_tx.nLockTime = nLocktime
        outpoint = COutPoint(int(txid, 16), 0)
        spending_tx_in = CTxIn(outpoint=outpoint, nSequence=nSequence)
        spending_tx.vin = [spending_tx_in]
        dest_addr = self.nodes[0].getnewaddress(address_type="bech32")
        scriptpubkey = bytes.fromhex(self.nodes[0].getaddressinfo(dest_addr)[
            'scriptPubKey'])
        amount_sat = amount
        dest_output = CTxOut(nValue=amount_sat, scriptPubKey=scriptpubkey)
        spending_tx.vout = [dest_output]
        return spending_tx

    def run_test(self):
        # Generate key pairs.
        keyhash_dict = {}
        for i in range(5):
            self.generate_key_pair(keyhash_dict)

        # Generate hashlocks.
        sha256_dict = {}
        hash256_dict = {}
        ripemd160_dict = {}
        hash160_dict = {}
        for i in range(5):
            secret = ECKey()
            secret.generate()
            sha256_dict[sha256(secret.get_bytes())] = secret.get_bytes()
            hash256_dict[hash256(secret.get_bytes())] = secret.get_bytes()
            ripemd160_dict[hashlib.new(
                'ripemd160', secret.get_bytes()).digest()] = secret.get_bytes()
            hash160_dict[hash160(secret.get_bytes())] = secret.get_bytes()

        # Test (de)serialization of terminal expressions.

        # pk(key)
        pk_hex = self.get_pubkey_hex_list(1, keyhash_dict)[0]
        desc0 = "pk({})".format(pk_hex)
        self.test_miniscript_expr(desc0, "Konduesm")

        # pk_h(key)
        pkh = list(keyhash_dict)[0]
        desc1 = "pk_h({})".format(pkh.hex())
        self.test_miniscript_expr(desc1, "Knduesm")

        # older(delay)
        delay = 20
        desc2 = "older({})".format(delay)
        self.test_miniscript_expr(desc2, "Bzfm")

        # after(time)
        time = 21
        desc3 = "after({})".format(time)
        self.test_miniscript_expr(desc3, "Bzfm")

        # sha256(h)
        sha256_digest = list(sha256_dict)[0]
        desc5 = "sha256({})".format(sha256_digest.hex())
        self.test_miniscript_expr(desc5, "Bondum")

        # hash256(h)
        h256_digest = list(hash256_dict)[0]
        desc6 = "hash256({})".format(h256_digest.hex())
        self.test_miniscript_expr(desc6, "Bondum")

        # ripemd160(h)
        ripemd160_digest = list(ripemd160_dict)[0]
        desc7 = "ripemd160({})".format(ripemd160_digest.hex())
        self.test_miniscript_expr(desc7, "Bondum")

        # hash160(h)
        h160_digest = list(hash160_dict)[0]
        desc8 = "hash160({})".format(h160_digest.hex())
        self.test_miniscript_expr(desc8, "Bondum")

        # thresh_m(k, pk, pk, ...)
        k = 2
        pk_hex_list = self.get_pubkey_hex_list(3, keyhash_dict)
        desc9 = ("thresh_m({},{},{},{})").format(k, pk_hex_list[0],
                                                 pk_hex_list[1],
                                                 pk_hex_list[2])
        self.test_miniscript_expr(desc9, "Bnduesm")

        # Test (de)serialization and (dis)satisfaction
        # of composable expressions.

        # Append miniscript nodes for satisfaction tests.
        miniscript_list = []

        # c:pkh(key)
        pkh = list(keyhash_dict)[0]
        desc10 = "c:pk_h({})".format(pkh.hex())
        miniscript_list.append(self.test_miniscript_expr(desc10, "Bndemus"))

        # and_v(vc:pk_h(key), c:pk_h(key)) expression.
        pkh0 = list(keyhash_dict)[0]
        pkh1 = list(keyhash_dict)[1]
        desc11 = "and_v(vc:pk_h({}),c:pk_h({}))".format(pkh0.hex(), pkh1.hex())
        miniscript_list.append(self.test_miniscript_expr(desc11, "Bnfmus"))

        # and_v(vc:pk_h(key),older(delay))) expression.
        pkh0 = list(keyhash_dict)[0]
        delay = 10
        desc12 = "and_v(vc:pk_h({}),older({}))".format(
            pkh0.hex(), delay)
        miniscript_list.append(self.test_miniscript_expr(desc12, "Bnfms"))

        # or_b(c:pk(key),a:and_b(c:pk_h(key),sc:pk(key)))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        pkh0 = list(keyhash_dict)[0]
        desc13 = "or_b(c:pk({}),a:and_b(c:pk_h({}),sc:pk({})))".format(
            pk_hex_list[0], pkh0.hex(), pk_hex_list[1])
        miniscript_list.append(self.test_miniscript_expr(desc13, "Bdemus"))

        # or_b(c:pk(key),a:and_n(c:pk(key),c:pk_h(key)))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        pkh0 = list(keyhash_dict)[0]
        desc14 = "or_b(c:pk({}),a:and_n(c:pk({}),c:pk_h({})))".format(
            *pk_hex_list, pkh0.hex())
        miniscript_list.append(self.test_miniscript_expr(desc14, "Bdemus"))

        # or_b(c:pk(key),sc:pk(key))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc15 = "or_b(c:pk({}),sc:pk({}))".format(*pk_hex_list)
        miniscript_list.append(self.test_miniscript_expr(desc15, "Bdemus"))

        # or_d(c:pk_h(key),c:pk_h(key))
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc16 = "or_d(c:pk({}),c:pk({}))".format(*pk_hex_list)
        miniscript_list.append(self.test_miniscript_expr(desc16, "Bdemus"))

        # t:or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:hash160(h))))
        # and_v(or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:hash160(h)))),1)
        h160_digest = list(hash160_dict)[0]
        pk_hex_list = self.get_pubkey_hex_list(3, keyhash_dict)
        desc17 = ("t:or_c(c:pk({}),and_v(vc:pk({}),or_c(c:pk({})," +
                 "v:hash160({}))))").format(*pk_hex_list, h160_digest.hex())
        desc17_alt = ("and_v(or_c(c:pk({}),and_v(vc:pk({}),or_c(c:pk({})," +
                     "v:hash160({})))),1)").format(*pk_hex_list,
                                                   h160_digest.hex())
        miniscript_list.append(self.test_miniscript_expr(desc17, "Bufsm",
                                                         desc17_alt))

        # t:or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:sha256(h))))
        # and_v(or_c(c:pk(key),and_v(vc:pk(key),or_c(c:pk(key),v:sha256(h)))),1)
        sha256_digest = list(sha256_dict)[0]
        pk_hex_list = self.get_pubkey_hex_list(3, keyhash_dict)
        desc18 = ("t:or_c(c:pk({}),and_v(vc:pk({}),or_c(c:pk({})," +
                 "v:sha256({}))))").format(*pk_hex_list, sha256_digest.hex())
        desc18_alt = ("and_v(or_c(c:pk({}),and_v(vc:pk({})," +
                     "or_c(c:pk({}),v:sha256({})))),1)").format(
                         *pk_hex_list, sha256_digest.hex())
        miniscript_list.append(self.test_miniscript_expr(desc18, "Bufsm",
                                                         desc18_alt))

        # or_i(and_v(vc:pk_h(key_local),hash256(h)),older(20))
        h256_digest = list(hash256_dict)[0]
        pkh0 = list(keyhash_dict)[0]
        delay = 20
        desc19 = "or_i(and_v(vc:pk_h({}),hash256({})),older({}))".format(
            pkh0.hex(), h256_digest.hex(), delay)
        miniscript_list.append(self.test_miniscript_expr(desc19, "Bfm"))

        # andor(c:pk(key),older(25),c:pk(key))
        delay = 25
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc20 = "andor(c:pk({}),older({}),c:pk({}))".format(
            pk_hex_list[0], delay, pk_hex_list[1])
        miniscript_list.append(self.test_miniscript_expr(desc20, "Bdems"))

        # andor(c:pk(key),or_i(and_v(vc:pk_h(key),ripemd160(h)),older(35)),c:pk(key))
        older = 35
        pkh0 = list(keyhash_dict)[0]
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        ripemd160_digest = list(ripemd160_dict)[0]
        desc21 = ("andor(c:pk({}),or_i(and_v(vc:pk_h({}),ripemd160({}))," +
                  "older({})),c:pk({}))").format(
            pk_hex_list[0], pkh0.hex(), ripemd160_digest.hex(), older,
            pk_hex_list[1])
        miniscript_list.append(self.test_miniscript_expr(desc21, "Bdems"))

        # thresh(k,c:pk(key),sc:pk(key),sc:pk(key),sdv:after(30))
        n = 4
        after = 30
        for k in range(2, n):
            pk_hex_list = self.get_pubkey_hex_list(n-1, keyhash_dict)
            desc22 = ("thresh({},c:pk({}),sc:pk({}),sc:pk({})," +
                           "sdv:after({}))").format(k, *pk_hex_list, after)
            miniscript_list.append(self.test_miniscript_expr(
                desc22, "Bdmus"))

        # thresh(k,c:pk(key),sc:pk(key),sc:pk(key),sc:pk(key),sc:pk(key))
        n = 5
        for k in range(2, n):
            pk_hex_list = self.get_pubkey_hex_list(n, keyhash_dict)
            desc23 = ("thresh({},c:pk({}),sc:pk({}),sc:pk({}),sc:pk({})" +
                           ",sc:pk({}))").format(k, *pk_hex_list)
            miniscript_list.append(self.test_miniscript_expr(
                desc23, "Bdemus"))

        # or_d(thresh_m(1,key),or_b(thresh_m(3,key,key,key),su:after(50)))
        # or_d(thresh_m(1,key),or_b(thresh_m(3,key,key,key),s:or_i(after(50),0)))
        k1 = 1
        k2 = 2
        after = 30
        pk_hex_list = self.get_pubkey_hex_list(4, keyhash_dict)
        desc24 = ("or_d(thresh_m({},{}),or_b(thresh_m({},{},{},{})," +
                         "su:after({})))").format(k1, pk_hex_list[0], k2,
                                                  pk_hex_list[1],
                                                  pk_hex_list[2],
                                                  pk_hex_list[3], after)
        desc24_alt = ("or_d(thresh_m({},{}),or_b(thresh_m({},{},{},{})," +
                      "s:or_i(after({}),0)))").format(k1, pk_hex_list[0], k2,
                                                      pk_hex_list[1],
                                                      pk_hex_list[2],
                                                      pk_hex_list[3], after)
        miniscript_list.append(self.test_miniscript_expr(
            desc24, "Bdemu", desc24_alt))

        # uuj:and_v(v:thresh_m(2,key,key),after(10))
        # or_i(or_i(j:and_v(v:thresh_m(2,key,key),after(time)),0),0)
        k = 2
        delay = 40
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc25 = "uuj:and_v(v:thresh_m({},{},{}),after({}))".format(
            k, pk_hex_list[0], pk_hex_list[1], delay)
        desc25_alt = ("or_i(or_i(j:and_v(v:thresh_m({},{},{}),after({})),0)," +
                      "0)").format(k, pk_hex_list[0], pk_hex_list[1], delay)
        miniscript_list.append(self.test_miniscript_expr(
            desc25, "Bdsm", desc25_alt))

        # or_b(un:thresh_m(k,key,key),al:older(delay))
        # or_b(or_i(n:thresh_m(k,key,key),0),a:or_i(0,older(delay)))
        k = 2
        older = 45
        pk_hex_list = self.get_pubkey_hex_list(2, keyhash_dict)
        desc26 = "or_b(un:thresh_m({},{},{}),al:older({}))".format(
            k, pk_hex_list[0], pk_hex_list[1], older)
        desc26_alt = ("or_b(or_i(n:thresh_m({},{},{}),0)," +
                      "a:or_i(0,older({})))").format(k, pk_hex_list[0],
                                                     pk_hex_list[1],
                                                     older)
        miniscript_list.append(self.test_miniscript_expr(desc26, "Bdu",
                                                         desc26_alt))

        # Test Miniscript satisfaction of all test expressions.
        for ms in miniscript_list:
            self.test_miniscript_satisfaction(ms, keyhash_dict, sha256_dict,
                                              hash256_dict, ripemd160_dict,
                                              hash160_dict)


if __name__ == '__main__':
    MiniscriptTest().main()
