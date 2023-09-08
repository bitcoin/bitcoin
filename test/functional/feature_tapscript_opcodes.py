#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

#
# Test for taproot sighash algorithm with pegins and issuances

from random import randint
from test_framework.util import assert_raises_rpc_error, satoshi_round
from test_framework.key import ECKey, compute_xonly_pubkey, generate_privkey, sign_schnorr
from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxInWitness, CTxOut, ser_uint256, sha256, tx_from_hex
from test_framework.test_framework import BitcoinTestFramework
from test_framework.script import CScript, CScriptNum, CScriptOp, OP_0, OP_1, OP_2, OP_ADD64, OP_SUB64, OP_MUL64, OP_DIV64, OP_LESSTHAN64, OP_LESSTHANOREQUAL64, OP_GREATERTHAN64, OP_GREATERTHANOREQUAL64, OP_NEG64, OP_AND, OP_DROP, OP_DUP, OP_EQUAL, OP_EQUALVERIFY, OP_FALSE, OP_FROMALTSTACK, OP_GREATERTHAN64, OP_GREATERTHANOREQUAL64, OP_SIZE, OP_SUB64, OP_SWAP, OP_TOALTSTACK, OP_VERIFY, OP_XOR, taproot_construct, SIGHASH_DEFAULT, SIGHASH_ALL, SIGHASH_NONE, SIGHASH_SINGLE, SIGHASH_ANYONECANPAY

import os

VALID_SIGHASHES_ECDSA = [
    SIGHASH_ALL,
    SIGHASH_NONE,
    SIGHASH_SINGLE,
    SIGHASH_ANYONECANPAY + SIGHASH_ALL,
    SIGHASH_ANYONECANPAY + SIGHASH_NONE,
    SIGHASH_ANYONECANPAY + SIGHASH_SINGLE
]

VALID_SIGHASHES_TAPROOT = [SIGHASH_DEFAULT] + VALID_SIGHASHES_ECDSA

class TapHashPeginTest(BitcoinTestFramework):

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.extra_args = [
            ["-initialfreecoins=2100000000000000",
             "-anyonecanspendaremine=1",
            "-blindedaddresses=1",
            "-validatepegin=0",
            "-con_parent_chain_signblockscript=51",
            "-parentscriptprefix=75",
            "-parent_bech32_hrp=ert",
            "-minrelaytxfee=0",
            "-maxtxfee=100.0",
        ]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_network(self, split=False):
        self.setup_nodes()


    def get_utxo(self, fund_tx, idx):
        spent = None
        # Coin selection
        for utxo in self.nodes[0].listunspent():
            if utxo["txid"] == ser_uint256(fund_tx.vin[idx].prevout.hash)[::-1].hex() and utxo["vout"] == fund_tx.vin[idx].prevout.n:
                spent = utxo

        assert(spent is not None)
        assert(len(fund_tx.vin) == 2)
        return spent

    def create_taproot_utxo(self, scripts = None, blind = False):
        # modify the transaction to add one output that should spend previous taproot
        # Create a taproot prevout
        addr = self.nodes[0].getnewaddress()

        sec = generate_privkey()
        pub = compute_xonly_pubkey(sec)[0]
        tap = taproot_construct(pub, scripts)
        spk = tap.scriptPubKey

        # No need to test blinding in this unit test
        unconf_addr = self.nodes[0].getaddressinfo(addr)['unconfidential']

        raw_tx = self.nodes[0].createrawtransaction([], [{unconf_addr: 1.2}])
        # edit spk directly, no way to get new address.
        # would need to implement bech32m in python
        tx = tx_from_hex(raw_tx)
        tx.vout[0].scriptPubKey = spk
        tx.vout[0].nValue = CTxOutValue(12*10**7)
        raw_hex = tx.serialize().hex()

        fund_tx = self.nodes[0].fundrawtransaction(raw_hex, False, )["hex"]
        fund_tx = tx_from_hex(fund_tx)

        # Createrawtransaction might rearrage txouts
        prev_vout = None
        for i, out in enumerate(fund_tx.vout):
            if spk == out.scriptPubKey:
                prev_vout = i

        tx = self.nodes[0].blindrawtransaction(fund_tx.serialize().hex())
        signed_raw_tx = self.nodes[0].signrawtransactionwithwallet(tx)
        self.nodes[0].sendrawtransaction(signed_raw_tx['hex'])
        tx = tx_from_hex(signed_raw_tx['hex'])
        tx.rehash()
        self.nodes[0].generate(1)
        last_blk = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        assert(tx.hash in last_blk['tx'])

        return tx, prev_vout, spk, sec, pub, tap

    def tapscript_satisfy_test(self, script, inputs = None, add_issuance = False,
       add_pegin = False, fail=None, add_prevout=False, add_asset=False,
       add_value=False, add_spk = False, seq = 0, add_out_spk = None,
       add_out_asset = None, add_out_value = None, add_out_nonce = None,
       ver = 2, locktime = 0, add_num_outputs=False, add_weight=False, blind=False):
        if inputs is None:
            inputs = []
        # Create a taproot utxo
        scripts = [("s0", script)]
        prev_tx, prev_vout, spk, sec, pub, tap = self.create_taproot_utxo(scripts)

        if add_pegin:
            fund_info = self.nodes[0].getpeginaddress()
            peg_id = self.nodes[0].sendtoaddress(fund_info["mainchain_address"], 1)
            raw_peg_tx = self.nodes[0].gettransaction(peg_id)["hex"]
            peg_txid = self.nodes[0].sendrawtransaction(raw_peg_tx)
            self.nodes[0].generate(101)
            peg_prf = self.nodes[0].gettxoutproof([peg_txid])
            claim_script = fund_info["claim_script"]

            raw_claim = self.nodes[0].createrawpegin(raw_peg_tx, peg_prf, claim_script)
            tx = tx_from_hex(raw_claim['hex'])
        else:
            tx = CTransaction()

        tx.nVersion = ver
        tx.nLockTime = locktime
        # Spend the pegin and taproot tx together
        in_total = prev_tx.vout[prev_vout].nValue.getAmount()
        fees = 1000
        tap_in_pos = 0

        if blind:
             # Add an unrelated output
            key = ECKey()
            key.generate()
            tx.vout.append(CTxOut(nValue = CTxOutValue(10000), scriptPubKey = spk, nNonce=CTxOutNonce(key.get_pubkey().get_bytes())))

            tx_hex = self.nodes[0].fundrawtransaction(tx.serialize().hex())
            tx = tx_from_hex(tx_hex['hex'])

        tx.vin.append(CTxIn(COutPoint(prev_tx.sha256, prev_vout), nSequence=seq))
        tx.vout.append(CTxOut(nValue = CTxOutValue(in_total - fees), scriptPubKey = spk)) # send back to self
        tx.vout.append(CTxOut(CTxOutValue(fees)))

        if add_issuance:
            blind_addr = self.nodes[0].getnewaddress()
            issue_addr = self.nodes[0].validateaddress(blind_addr)['unconfidential']
            # Issuances only require one fee output and that output must the last
            # one. However the way, the current code is structured, it is not possible
            # to this in a super clean without special casing.
            if add_pegin:
                tx.vout.pop()
                tx.vout.pop()
                tx.vout.insert(0, CTxOut(nValue = CTxOutValue(in_total), scriptPubKey = spk)) # send back to self)
            issued_tx = self.nodes[0].rawissueasset(tx.serialize().hex(), [{"asset_amount":2, "asset_address":issue_addr, "blind":False}])[0]["hex"]
            tx = tx_from_hex(issued_tx)
        # Sign inputs
        if add_pegin:
            signed = self.nodes[0].signrawtransactionwithwallet(tx.serialize().hex())
            tx = tx_from_hex(signed['hex'])
            tap_in_pos += 1
        else:
            # Need to create empty witness when not deserializing from rpc
            tx.wit.vtxinwit.append(CTxInWitness())

        if blind:
            tx.vin[0], tx.vin[1] = tx.vin[1], tx.vin[0]
            utxo = self.get_utxo(tx, 1)
            zero_str = "0"*64
            blinded_raw = self.nodes[0].rawblindrawtransaction(tx.serialize().hex(), [zero_str, utxo["amountblinder"]], [1.2, utxo['amount']], [utxo['asset'], utxo['asset']], [zero_str, utxo['assetblinder']])
            tx = tx_from_hex(blinded_raw)
            signed_raw_tx = self.nodes[0].signrawtransactionwithwallet(tx.serialize().hex())
            tx = tx_from_hex(signed_raw_tx['hex'])


        suffix_annex = []
        control_block = bytes([tap.leaves["s0"].version + tap.negflag]) + tap.internal_pubkey + tap.leaves["s0"].merklebranch
        # Add the prevout to the top of inputs. The witness script will check for equality.
        if add_prevout:
            inputs = [prev_vout.to_bytes(4, 'little'), ser_uint256(prev_tx.sha256)]
        if add_asset:
            assert blind # only used with blinding in testing
            utxo = self.nodes[0].gettxout(ser_uint256(tx.vin[1].prevout.hash)[::-1].hex(), tx.vin[1].prevout.n)
            if "assetcommitment" in utxo:
                asset = bytes.fromhex(utxo["assetcommitment"])
            else:
                asset = b"\x01" + bytes.fromhex(utxo["asset"])[::-1]
            inputs = [asset[0:1], asset[1:33]]
        if add_value:
            utxo = self.nodes[0].gettxout(ser_uint256(tx.vin[1].prevout.hash)[::-1].hex(), tx.vin[1].prevout.n)
            if "valuecommitment" in utxo:
                value = bytes.fromhex(utxo["valuecommitment"])
                inputs = [value[0:1], value[1:33]]
            else:
                value = b"\x01" + int(satoshi_round(utxo["value"])*COIN).to_bytes(8, 'little')
                inputs = [value[0:1], value[1:9]]
        if add_spk:
            ver = CScriptOp.decode_op_n(int.from_bytes(spk[0:1], 'little'))
            inputs = [CScriptNum.encode(CScriptNum(ver))[1:], spk[2:len(spk)]] # always segwit

        # Add witness for outputs
        if add_out_asset is not None:
            asset = tx.vout[add_out_asset].nAsset.vchCommitment
            inputs = [asset[0:1], asset[1:33]]
        if add_out_value is not None:
            value = tx.vout[add_out_value].nValue.vchCommitment
            if len(value) == 9:
                inputs = [value[0:1], value[1:9][::-1]]
            else:
                inputs = [value[0:1], value[1:33]]
        if add_out_nonce is not None:
            nonce = tx.vout[add_out_nonce].nNonce.vchCommitment
            if len(nonce) == 1:
                inputs = [b'']
            else:
                inputs = [nonce]
        if add_out_spk is not None:
            out_spk  = tx.vout[add_out_spk].scriptPubKey
            if len(out_spk) == 0:
                # Python upstream encoding CScriptNum interesting behaviour where it also encodes the length
                # This assumes the implicit wallet behaviour of using segwit outputs.
                # This is useful while sending scripts, but not while using CScriptNums in constructing scripts
                inputs = [CScriptNum.encode(CScriptNum(-1))[1:], sha256(out_spk)]
            else:
                ver = CScriptOp.decode_op_n(int.from_bytes(out_spk[0:1], 'little'))
                inputs = [CScriptNum.encode(CScriptNum(ver))[1:], out_spk[2:len(out_spk)]] # always segwit
        if add_num_outputs:
            num_outs = len(tx.vout)
            inputs = [CScriptNum.encode(CScriptNum(num_outs))[1:]]
        if add_weight:
            # Add a dummy input and check the overall weight
            inputs = [int(5).to_bytes(8, 'little')]
            wit = inputs + [bytes(tap.leaves["s0"].script), control_block] + suffix_annex
            tx.wit.vtxinwit[tap_in_pos].scriptWitness.stack = wit

            exp_weight = self.nodes[0].decoderawtransaction(tx.serialize().hex())["weight"]
            inputs = [exp_weight.to_bytes(8, 'little')]
        wit = inputs + [bytes(tap.leaves["s0"].script), control_block] + suffix_annex
        tx.wit.vtxinwit[tap_in_pos].scriptWitness.stack = wit

        if fail:
            assert_raises_rpc_error(-26, fail, self.nodes[0].sendrawtransaction, tx.serialize().hex())
            return


        self.nodes[0].sendrawtransaction(hexstring = tx.serialize().hex())
        self.nodes[0].generate(1)
        last_blk = self.nodes[0].getblock(self.nodes[0].getbestblockhash())
        tx.rehash()
        assert(tx.hash in last_blk['tx'])


    def run_test(self):
        self.nodes[0].generate(101)
        self.wait_until(lambda: self.nodes[0].getblockcount() == 101, timeout=5)
        # Test whether the above test framework is working
        self.log.info("Test simple op_1")
        self.tapscript_satisfy_test(CScript([OP_1]))


        # short handle to convert int to 8 byte LE
        def le8(x, signed=True):
            return int(x).to_bytes(8, 'little', signed=signed)

        def check_add(a, b, c, fail=None):
            self.tapscript_satisfy_test(CScript([OP_ADD64, OP_VERIFY, le8(c), OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_sub(a, b, c, fail=None):
            self.tapscript_satisfy_test(CScript([OP_SUB64, OP_VERIFY, le8(c), OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_mul(a, b, c, fail=None):
            self.tapscript_satisfy_test(CScript([OP_MUL64, OP_VERIFY, le8(c), OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_div(a, b, q, r, fail=None):
            self.tapscript_satisfy_test(CScript([OP_DIV64, OP_VERIFY, le8(q), OP_EQUALVERIFY, le8(r), OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_le(a, b, res, fail=None):
            self.tapscript_satisfy_test(CScript([OP_LESSTHAN64, res, OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_leq(a, b, res, fail=None):
            self.tapscript_satisfy_test(CScript([OP_LESSTHANOREQUAL64, res, OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_ge(a, b, res, fail=None):
            self.tapscript_satisfy_test(CScript([OP_GREATERTHAN64, res, OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_geq(a, b, res, fail=None):
            self.tapscript_satisfy_test(CScript([OP_GREATERTHANOREQUAL64, res, OP_EQUAL]), inputs = [le8(a), le8(b)], fail=fail)

        def check_neg(a, res, fail=None):
            self.tapscript_satisfy_test(CScript([OP_NEG64, OP_VERIFY, le8(res), OP_EQUAL]), inputs = [le8(a)], fail=fail)
        # Arithematic opcodes
        self.log.info("Check Arithmetic opcodes")
        check_add(5, 5, 10)
        check_add(-5, -5, -10)
        check_add(-5, 5, 0)
        check_add(14231, -123213, 14231 - 123213)
        check_add(2**63 - 1, 5, 4, fail="Script failed an OP_VERIFY operation")#overflow
        check_add(5, 2**63 - 1, 4, fail="Script failed an OP_VERIFY operation")#overflow
        check_add(-5, -2**63 + 1, -4, fail="Script failed an OP_VERIFY operation")#overflow

        # Subtraction
        check_sub(5, 6, -1)
        check_sub(-5, 6, -11)
        check_sub(-5, -5, 0)
        check_sub(14231, -123213, 14231 + 123213)
        check_sub(2**63 - 1, 4, 2**63 - 5)
        check_sub(-5, 2**63 - 1, -4, fail="Script failed an OP_VERIFY operation")#overflow
        check_sub(2**63 - 1, -4, 5, fail="Script failed an OP_VERIFY operation")#overflow

        # Multiplication
        check_mul(5, 6, 30)
        check_mul(-5, 6, -30)
        check_mul(-5, 0, 0)
        check_mul(-5, -6, 30)
        check_mul(14231, -123213, -14231 * 123213)
        check_mul(2**32, 2**31 - 1, 2**63 - 2**32)
        check_mul(2**32, 2**31, 0, fail="Script failed an OP_VERIFY operation")#overflow
        check_mul(-2**32, 2**31, -2**63)# no  overflow
        check_mul(-2**32, -2**32, 0, fail="Script failed an OP_VERIFY operation")#overflow

        # Division
        check_div(5, 6, 0, 5)
        check_div(4, 2, 2, 0)
        check_div(-5, 6, -1, 1) # r must be in 0<=r<a
        check_div(5, -6, 0, 5) # r must be in 0<=r<a
        check_div(-5, 0, 0, 0, fail="Script failed an OP_VERIFY operation") # only fails if b = 0
        check_div(-2**63, -1, 0, 0, fail="Script failed an OP_VERIFY operation") # failure on -2**63/-1
        check_div(6213123213513, 621, 6213123213513//621, 6213123213513 % 621)
        check_div(2**62, 2**31, 2**31, 0)

        # Less than test
        # Comparison tests
        # Less than
        check_le(5, 6, 1)
        check_le(5, 5, 0)
        check_le(6, 5, 0)

        # Less than equal
        check_leq(5, 6, 1)
        check_leq(5, 5, 1)
        check_leq(6, 5, 0)

        # Greater than
        check_ge(5, 6, 0)
        check_ge(5, 5, 0)
        check_ge(6, 5, 1)

        # Greater than equal
        check_geq(5, 6, 0)
        check_geq(5, 5, 1)
        check_geq(6, 5, 1)

        # equal
        check_neg(5, -5)
        check_neg(-5, 5)
        check_neg(5, 4, fail="Script evaluated without error but finished with a false/empty top stack element")
        check_neg(-2**63, 0, fail="Script failed an OP_VERIFY operation")
        check_neg(2**63-1, -2**63 + 1)

        # Test boolean operations
        for _ in range(5):
            a = randint(-2**63, 2**63 -1)
            b = randint(-2**63, 2**63 -1)
            self.tapscript_satisfy_test(CScript([OP_AND, le8(a & b), OP_EQUAL]), inputs = [le8(a), le8(b)])
            self.tapscript_satisfy_test(CScript([OP_OR, le8(a | b), OP_EQUAL]), inputs = [le8(a), le8(b)])
            self.tapscript_satisfy_test(CScript([OP_INVERT, le8(~a), OP_EQUAL]), inputs = [le8(a)])
            self.tapscript_satisfy_test(CScript([OP_XOR, le8(a ^ b), OP_EQUAL]), inputs = [le8(a), le8(b)])

        # Finally, test conversion opcodes
        self.log.info("Check conversion opcodes")
        for n in [-2**31 + 1, -1, 0, 1, 2**31 - 1]:
            self.tapscript_satisfy_test(CScript([le8(n), OP_LE64TOSCRIPTNUM, n, OP_EQUAL]))
            self.tapscript_satisfy_test(CScript([n, OP_SCRIPTNUMTOLE64, le8(n), OP_EQUAL]))
            self.tapscript_satisfy_test(CScript([n, OP_DUP, OP_SCRIPTNUMTOLE64, OP_LE64TOSCRIPTNUM, OP_EQUAL]))
            self.tapscript_satisfy_test(CScript([le8(n), OP_DUP, OP_LE64TOSCRIPTNUM, OP_SCRIPTNUMTOLE64, OP_EQUAL]))
            self.tapscript_satisfy_test(CScript([n.to_bytes(4, 'little', signed=True), OP_LE32TOLE64, le8(n), OP_EQUAL]), fail= "Script evaluated without error but finished with a false/empty top stack element" if (n < 0) else None)

        # Non 8 byte inputs
        self.tapscript_satisfy_test(CScript([OP_ADD64, OP_VERIFY, le8(9), OP_EQUAL]), inputs = [le8(0), int(9).to_bytes(7, 'little')], fail="Arithmetic opcodes expect 8 bytes operands")
        # Overflow inputs
        self.tapscript_satisfy_test(CScript([le8(2**31), OP_LE64TOSCRIPTNUM, 2**31, OP_EQUAL]), fail = "Arithmetic opcode error")


        self.log.info("Check Crypto opcodes")

        res = bytes.fromhex("03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7")
        scalar = bytes.fromhex("000000000000000000000000000000000000000000000000000000000000000a")
        g = bytes.fromhex("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")
        self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [res, g, scalar])

        res = bytes.fromhex("032c0158d0f6df4881e99e65fbea21f27321d817f79ad39e08eaf4f16f1419bb0c")
        scalar = bytes.fromhex("e0f47c124f228b97bbdc0e4398aac9788869b9fbbc193d5323fdad9570609de6")
        g = bytes.fromhex("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")
        self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [res, g, scalar])

        # test that other random values fail correctly
        res = bytes.fromhex("032c0158d0f6df4881e99e65fbea21f27321d817f79ad39e08eaf4f16f1419bb0c")
        scalar = bytes.fromhex("e0f47c124f228b97bbdc0e4398aac9788869b9fbbc193d5323fdad9570609de6")
        g = bytes.fromhex("0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")# y coordinate mutated
        self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [res, g, scalar], fail="Public key is neither compressed or uncompressed")

        invalid_full_pks_33 = [
            "0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81700" # invalid point on curve
            ]
        invalid_full_pks_non33 = [
            "0979be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798", # 33 byte without 0x02,0x03
            "0462b31419b87d5e095e9d7532f92aea39c39735253a48fbdaff48441d5cf706f9a7a946ce481411e4034143e0acad4e79aecb2a8212ee5cca26a7cc5fe5b45881", # compressed
            "62b31419b87d5e095e9d7532f92aea39c39735253a48fbdaff48441d5cf706f9", # x-only key
            "0262b31419b87d5e095e9d7532f92aea39c39735253a48fbdaff48441d5cf706f9f9f9" # 34 byte key
        ]
        invalid_tweaks = [
            "e07c124f228b97bbdc0e4398aac9788869b9fbbc193d5323fdad9570609de6", # non 32 byte hash
            "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364143" # order + 2
            ]
        invalid_xonly_keys = [
            "0379be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798", # 33 byte valid key
            "79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81700", # invalid point on curve
            "0462b31419b87d5e095e9d7532f92aea39c39735253a48fbdaff48441d5cf706f9a7a946ce481411e4034143e0acad4e79aecb2a8212ee5cca26a7cc5fe5b45881", # compressed
            "62b31419b87d5e095e9d7532f92aea39c39735253a48fbdaff48441d5cf706" # 31 byte x-only key
        ]

        valid_res = bytes.fromhex("032c0158d0f6df4881e99e65fbea21f27321d817f79ad39e08eaf4f16f1419bb0c")
        valid_scalar = bytes.fromhex("e0f47c124f228b97bbdc0e4398aac9788869b9fbbc193d5323fdad9570609de6")
        valid_g = bytes.fromhex("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")
        valid_xonly_key = bytes.fromhex("79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798")

        for invalid_tweak in invalid_tweaks:
            self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [valid_res, valid_g, bytes.fromhex(invalid_tweak)], fail="EC scalar mult verify fail")

        self.tapscript_satisfy_test(CScript([OP_TWEAKVERIFY, OP_1]), inputs = [valid_res, bytes.fromhex(invalid_tweaks[1]), valid_xonly_key], fail="EC scalar mult verify fail")
        self.tapscript_satisfy_test(CScript([OP_TWEAKVERIFY, OP_1]), inputs = [valid_res, bytes.fromhex(invalid_tweaks[0]), valid_xonly_key], fail="Public key is neither compressed or uncompressed")

        for invalid_full_pk in invalid_full_pks_33:
            self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [bytes.fromhex(invalid_full_pk), valid_g, valid_scalar], fail="EC scalar mult verify fail")
            self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [valid_res, bytes.fromhex(invalid_full_pk), valid_scalar], fail="EC scalar mult verify fail")

        for invalid_full_pk in invalid_full_pks_non33:
            self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [bytes.fromhex(invalid_full_pk), valid_g, valid_scalar], fail="Public key is neither compressed or uncompressed")
            self.tapscript_satisfy_test(CScript([OP_ECMULSCALARVERIFY, OP_1]), inputs = [valid_res, bytes.fromhex(invalid_full_pk), valid_scalar], fail="Public key is neither compressed or uncompressed")

        self.tapscript_satisfy_test(CScript([OP_TWEAKVERIFY, OP_1]), inputs = [valid_res, valid_scalar, bytes.fromhex(invalid_xonly_keys[0])], fail="Public key is neither compressed or uncompressed")
        self.tapscript_satisfy_test(CScript([OP_TWEAKVERIFY, OP_1]), inputs = [valid_res, valid_scalar, bytes.fromhex(invalid_xonly_keys[1])], fail="EC scalar mult verify fail")
        self.tapscript_satisfy_test(CScript([OP_TWEAKVERIFY, OP_1]), inputs = [valid_res, valid_scalar, bytes.fromhex(invalid_xonly_keys[2])], fail="Public key is neither compressed or uncompressed")
        self.tapscript_satisfy_test(CScript([OP_TWEAKVERIFY, OP_1]), inputs = [valid_res, valid_scalar, bytes.fromhex(invalid_xonly_keys[3])], fail="Public key is neither compressed or uncompressed")

        # Checksigfromstack tests
        # 32 byte msg
        def csfs_test(msg, mutute_sig = False, fail=None):
            sec = generate_privkey()
            msg = bytes.fromhex(msg)
            pub = compute_xonly_pubkey(sec)[0]
            sig = sign_schnorr(sec, msg)
            if mutute_sig:
                new_sig = sig[:63]
                new_sig += (sig[63] ^ 0xff).to_bytes(1, 'little') # flip the last bit
                sig = new_sig
            self.tapscript_satisfy_test(CScript([msg, pub, OP_CHECKSIGFROMSTACK]), inputs = [sig], fail=fail)

        csfs_test("e168c349d0d2499caf3a6d71734c743d517f94f8571fa52c04285b68deec1936")
        csfs_test("Hello World!".encode('utf-8').hex())
        csfs_test("Hello World!".encode('utf-8').hex(), fail="Invalid Schnorr signature", mutute_sig=True)
        msg = bytes.fromhex("e168c349d0d2499caf3a6d71734c743d517f94f8571fa52c04285b68deec1936")
        pub = bytes.fromhex("3f67e97da0df6931189cfb0072447da22707897bd5de04936a277ed7e00b35b3")
        self.tapscript_satisfy_test(CScript([OP_0, msg, pub, OP_CHECKSIGFROMSTACK, OP_NOT]), inputs = [])
        short_sig = bytes.fromhex("0102")
        self.tapscript_satisfy_test(CScript([short_sig, msg, pub, OP_CHECKSIGFROMSTACK, OP_NOT]), inputs = [], fail="Invalid Schnorr signature size")

        short_pub = bytes.fromhex("67e97da0df6931189cfb0072447da22707897bd5de04936a277ed7e00b35b3")
        long_pub = bytes.fromhex("67e97da0df6931189cfb0072447da22707897bd5de04936a277ed7e00b35b30908")
        sig = bytes.fromhex("3f67e97da0df6931189cfb0072447da22707897bd5de04936a277ed7e00b35b33f67e97da0df6931189cfb0072447da22707897bd5de04936a277ed7e00b35b3")
        self.tapscript_satisfy_test(CScript([msg, short_pub, OP_CHECKSIGFROMSTACK]), inputs = [sig], fail="Public key version reserved for soft-fork upgrades")
        self.tapscript_satisfy_test(CScript([msg, long_pub, OP_CHECKSIGFROMSTACK]), inputs = [sig], fail="Public key version reserved for soft-fork upgrades")

if __name__ == '__main__':
    TapHashPeginTest().main()
