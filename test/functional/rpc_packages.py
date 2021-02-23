#!/usr/bin/env python3
# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""RPCs that handle raw transaction packages."""

from decimal import Decimal
from io import BytesIO

from test_framework.address import ADDRESS_BCRT1_P2WSH_OP_TRUE
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    BIP125_SEQUENCE_NUMBER,
    COIN,
    CTransaction,
    CTxInWitness,
)
from test_framework.script import (
    CScript,
    OP_TRUE,
)
from test_framework.util import (
    assert_equal,
    hex_str_to_bytes,
)

class RPCPackagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        self.log.info("Generate blocks to create UTXOs")
        node = self.nodes[0]
        self.privkeys = [node.get_deterministic_priv_key().key]
        self.address = node.get_deterministic_priv_key().address
        self.coins = []
        # The last 100 coinbase transactions are premature
        for b in node.generatetoaddress(120, self.address)[:20]:
            coinbase = node.getblock(blockhash=b, verbosity=2)["tx"][0]
            self.coins.append({
                "txid": coinbase["txid"],
                "amount": coinbase["vout"][0]["value"],
                "scriptPubKey": coinbase["vout"][0]["scriptPubKey"],
            })

        # Create some transactions that can be reused throughout the test. Never submit these to mempool.
        self.independent_txns_hex = []
        self.independent_txns_testres = []
        for _ in range(3):
            coin = self.coins.pop()
            rawtx = node.createrawtransaction([{"txid" : coin["txid"], "vout" : 0}],
                {self.address : coin["amount"] - Decimal("0.0001")})
            signedtx = node.signrawtransactionwithkey(hexstring=rawtx, privkeys=self.privkeys)
            assert signedtx["complete"]
            testres = node.testmempoolaccept([signedtx["hex"]])
            assert testres[0]["allowed"]
            self.independent_txns_hex.append(signedtx["hex"])
            # testmempoolaccept returns a list of length one, avoid creating a 2D list
            self.independent_txns_testres.append(testres[0])

        self.test_independent()
        self.test_chain()
        self.test_multiple_children()
        self.test_multiple_parents()
        self.test_conflicting()
        self.test_rbf()

    def chain_transaction(self, parent_txid, value, n=0, parent_locking_script=None):
        """Build a transaction that spends parent_txid.vout[n] and produces one output with amount=value.
        Return tuple (CTransaction object, raw hex, scriptPubKey of the output created).
        """
        node = self.nodes[0]
        inputs = [{"txid" : parent_txid, "vout" : n}]
        outputs = {self.address : value}
        rawtx = node.createrawtransaction(inputs, outputs)
        prevtxs = [{
            "txid": parent_txid,
            "vout": n,
            "scriptPubKey": parent_locking_script,
            "amount": value + Decimal("0.0001"),
        }] if parent_locking_script else None
        signedtx = node.signrawtransactionwithkey(hexstring=rawtx, privkeys=self.privkeys, prevtxs=prevtxs)
        tx = CTransaction()
        assert signedtx["complete"]
        tx.deserialize(BytesIO(hex_str_to_bytes(signedtx["hex"])))
        return (tx, signedtx["hex"], tx.vout[0].scriptPubKey.hex())

    def test_independent(self):
        self.log.info("Test multiple independent transactions in a package")
        node = self.nodes[0]
        assert_equal(self.independent_txns_testres, node.testmempoolaccept(rawtxs=self.independent_txns_hex))

        self.log.info("Test a valid package with garbage inserted")
        garbage_tx = node.createrawtransaction([{"txid": "00" * 32, "vout": 5}], {self.address: 1})
        tx = CTransaction()
        tx.deserialize(BytesIO(hex_str_to_bytes(garbage_tx)))
        testres_bad = node.testmempoolaccept(self.independent_txns_hex + [garbage_tx])
        testres_independent_ids = [{"txid": res["txid"], "wtxid": res["wtxid"]} for res in self.independent_txns_testres]
        assert_equal(testres_bad, testres_independent_ids + [
            {"txid": tx.rehash(), "wtxid": tx.getwtxid(), "allowed": False, "reject-reason": "missing-inputs"}
        ])

        self.log.info("Check testmempoolaccept tells us when some transactions completed validation successfully")
        coin = self.coins.pop()
        tx_bad_sig_hex = node.createrawtransaction([{"txid" : coin["txid"], "vout" : 0}],
                                           {self.address : coin["amount"] - Decimal("0.0001")})
        tx_bad_sig = CTransaction()
        tx_bad_sig.deserialize(BytesIO(hex_str_to_bytes(tx_bad_sig_hex)))
        testres_bad_sig = node.testmempoolaccept(self.independent_txns_hex + [tx_bad_sig_hex])
        assert_equal(testres_bad_sig, self.independent_txns_testres + [{
            "txid": tx_bad_sig.rehash(),
            "wtxid": tx_bad_sig.getwtxid(), "allowed": False,
            "reject-reason": "mandatory-script-verify-flag-failed (Operation not valid with the current stack size)"
        }])

        self.log.info("Check testmempoolaccept reports txns in packages that exceed max feerate")
        coin = self.coins.pop()
        tx_high_fee_raw = node.createrawtransaction([{"txid" : coin["txid"], "vout" : 0}],
                                           {self.address : coin["amount"] - Decimal("0.999")})
        tx_high_fee_signed = node.signrawtransactionwithkey(hexstring=tx_high_fee_raw, privkeys=self.privkeys)
        assert tx_high_fee_signed["complete"]
        tx_high_fee = CTransaction()
        tx_high_fee.deserialize(BytesIO(hex_str_to_bytes(tx_high_fee_signed["hex"])))
        testres_high_fee = node.testmempoolaccept([tx_high_fee_signed["hex"]])
        assert_equal(testres_high_fee, [
            {"txid": tx_high_fee.rehash(), "wtxid": tx_high_fee.getwtxid(), "allowed": False, "reject-reason": "max-fee-exceeded"}
        ])
        testres_package_high_fee = node.testmempoolaccept(self.independent_txns_hex + [tx_high_fee_signed["hex"]])
        assert_equal(testres_package_high_fee, self.independent_txns_testres + testres_high_fee)

    def test_chain(self):
        node = self.nodes[0]
        first_coin = self.coins.pop()

        self.log.info("Create a chain of 25 transactions")
        parent_locking_script = None
        txid = first_coin["txid"]
        chain_hex = []
        chain_txns = []
        value = first_coin["amount"]

        for _ in range(25):
            value -= Decimal("0.0001") # Deduct reasonable fee
            (tx, txhex, parent_locking_script) = self.chain_transaction(txid, value, 0, parent_locking_script)
            txid = tx.rehash()
            chain_hex.append(txhex)
            chain_txns.append(tx)

        self.log.info("Check that testmempoolaccept requires packages to be sorted by dependency")
        testres_multiple_unsorted = node.testmempoolaccept(rawtxs=chain_hex[::-1])
        assert_equal(testres_multiple_unsorted, [{"txid": chain_txns[-1].rehash(), "wtxid": chain_txns[-1].getwtxid(), "allowed": False, "reject-reason": "missing-inputs"}]
                                              + [{"txid": tx.rehash(), "wtxid": tx.getwtxid()} for tx in chain_txns[::-1]][1:])

        self.log.info("Testmempoolaccept with entire package")
        testres_multiple = node.testmempoolaccept(rawtxs=chain_hex)

        testres_single = []
        self.log.info("Test accept and then submit each one individually, which should be identical to package testaccept")
        for rawtx in chain_hex:
            testres = node.testmempoolaccept([rawtx])
            testres_single.append(testres[0])
            # Submit the transaction now so its child should have no problem validating
            node.sendrawtransaction(rawtx)
        assert_equal(testres_single, testres_multiple)

        # Clean up by clearing the mempool
        node.generate(1)

    def test_multiple_children(self):
        node = self.nodes[0]

        self.log.info("Create a package in which a transaction has two children within the package")
        first_coin = self.coins.pop()
        value = (first_coin["amount"] - Decimal("0.0002")) / 2 # Deduct reasonable fee and make 2 outputs
        inputs = [{"txid" : first_coin["txid"], "vout" : 0}]
        outputs = [{self.address : value}, {ADDRESS_BCRT1_P2WSH_OP_TRUE : value}]
        rawtx = node.createrawtransaction(inputs, outputs)

        parent_signed = node.signrawtransactionwithkey(hexstring=rawtx, privkeys=self.privkeys)
        parent_tx = CTransaction()
        assert parent_signed["complete"]
        parent_tx.deserialize(BytesIO(hex_str_to_bytes(parent_signed["hex"])))
        parent_txid = parent_tx.rehash()
        assert node.testmempoolaccept([parent_signed["hex"]])[0]["allowed"]

        parent_locking_script_a = parent_tx.vout[0].scriptPubKey.hex()
        child_value = value - Decimal("0.0001")

        # Child A
        (_, tx_child_a_hex, _) = self.chain_transaction(parent_txid, child_value, 0, parent_locking_script_a)
        assert not node.testmempoolaccept([tx_child_a_hex])[0]["allowed"]

        # Child B
        rawtx_b = node.createrawtransaction([{"txid" : parent_txid, "vout" : 1}], {self.address : child_value})
        tx_child_b = CTransaction()
        tx_child_b.deserialize(BytesIO(hex_str_to_bytes(rawtx_b)))
        tx_child_b.wit.vtxinwit = [CTxInWitness()]
        tx_child_b.wit.vtxinwit[0].scriptWitness.stack = [CScript([OP_TRUE])]
        tx_child_b_hex = tx_child_b.serialize().hex()
        assert not node.testmempoolaccept([tx_child_b_hex])[0]["allowed"]

        self.log.info("Testmempoolaccept with entire package, should work with children in either order")
        testres_multiple_ab = node.testmempoolaccept(rawtxs=[parent_signed["hex"], tx_child_a_hex, tx_child_b_hex])
        testres_multiple_ba = node.testmempoolaccept(rawtxs=[parent_signed["hex"], tx_child_b_hex, tx_child_a_hex])
        assert all([testres["allowed"] for testres in testres_multiple_ab + testres_multiple_ba])

        testres_single = []
        self.log.info("Test accept and then submit each one individually, which should be identical to package testaccept")
        for rawtx in [parent_signed["hex"], tx_child_a_hex, tx_child_b_hex]:
            testres = node.testmempoolaccept([rawtx])
            testres_single.append(testres[0])
            # Submit the transaction now so its child should have no problem validating
            node.sendrawtransaction(rawtx)
        assert_equal(testres_single, testres_multiple_ab)

    def test_multiple_parents(self):
        node = self.nodes[0]

        self.log.info("Create a package in which a transaction has two parents within the package")
        # Parent A
        parent_a_coin = self.coins.pop()
        parent_a_value = parent_a_coin["amount"] - Decimal("0.0001")
        (tx_parent_a, hex_parent_a, parent_locking_script_a) = self.chain_transaction(parent_a_coin["txid"], parent_a_value)

        # Parent B
        parent_b_coin = self.coins.pop()
        parent_b_value = parent_b_coin["amount"] - Decimal("0.0001")
        (tx_parent_b, hex_parent_b, parent_locking_script_b) = self.chain_transaction(parent_b_coin["txid"], parent_b_value)

        # Child
        inputs = [{"txid" : tx_parent_a.rehash(), "vout" : 0}, {"txid" : tx_parent_b.rehash(), "vout" : 0}]
        outputs = {self.address : parent_a_value + parent_b_value - Decimal("0.0001")}
        rawtx_child = node.createrawtransaction(inputs, outputs)
        prevtxs = [{"txid": tx_parent_a.rehash(), "vout": 0, "scriptPubKey": parent_locking_script_a, "amount": parent_a_value},
                   {"txid": tx_parent_b.rehash(), "vout": 0, "scriptPubKey": parent_locking_script_b, "amount": parent_b_value}]
        signedtx_child = node.signrawtransactionwithkey(hexstring=rawtx_child, privkeys=self.privkeys, prevtxs=prevtxs)
        assert signedtx_child["complete"]
        assert not node.testmempoolaccept([signedtx_child["hex"]])[0]["allowed"]

        self.log.info("Testmempoolaccept with entire package, should work with parents in either order")
        testres_multiple_ab = node.testmempoolaccept(rawtxs=[hex_parent_a, hex_parent_b, signedtx_child["hex"]])
        testres_multiple_ba = node.testmempoolaccept(rawtxs=[hex_parent_b, hex_parent_a, signedtx_child["hex"]])
        assert all([testres["allowed"] for testres in testres_multiple_ab + testres_multiple_ba])

        testres_single = []
        self.log.info("Test accept and then submit each one individually, which should be identical to package testaccept")
        for rawtx in [hex_parent_a, hex_parent_b, signedtx_child["hex"]]:
            testres = node.testmempoolaccept([rawtx])
            testres_single.append(testres[0])
            # Submit the transaction now so its child should have no problem validating
            node.sendrawtransaction(rawtx)
        assert_equal(testres_single, testres_multiple_ab)

    def test_conflicting(self):
        node = self.nodes[0]
        prevtx = self.coins.pop()
        inputs = [{"txid" : prevtx["txid"], "vout" : 0}]
        output1 = {node.get_deterministic_priv_key().address: 50 - 0.00125}
        output2 = {ADDRESS_BCRT1_P2WSH_OP_TRUE: 50 - 0.00125}

        # tx1 and tx2 share the same inputs
        rawtx1 = node.createrawtransaction(inputs, output1)
        rawtx2 = node.createrawtransaction(inputs, output2)
        signedtx1 = node.signrawtransactionwithkey(hexstring=rawtx1, privkeys=self.privkeys)
        signedtx2 = node.signrawtransactionwithkey(hexstring=rawtx2, privkeys=self.privkeys)
        tx1 = CTransaction()
        tx1.deserialize(BytesIO(hex_str_to_bytes(signedtx1["hex"])))
        tx2 = CTransaction()
        tx2.deserialize(BytesIO(hex_str_to_bytes(signedtx2["hex"])))
        assert signedtx1["complete"]
        assert signedtx2["complete"]

        # Ensure tx1 and tx2 are valid by themselves
        assert node.testmempoolaccept([signedtx1["hex"]])[0]["allowed"]
        assert node.testmempoolaccept([signedtx2["hex"]])[0]["allowed"]

        self.log.info("Test duplicate transactions in the same package")
        testres = node.testmempoolaccept([signedtx1["hex"], signedtx1["hex"]])
        assert_equal(testres, [
            {"txid": tx1.rehash(), "wtxid": tx1.getwtxid()},
            {"txid": tx1.rehash(), "wtxid": tx1.getwtxid(), "allowed": False, "reject-reason": "conflict-in-package"}
        ])

        self.log.info("Test conflicting transactions in the same package")
        testres = node.testmempoolaccept([signedtx1["hex"], signedtx2["hex"]])
        assert_equal(testres, [
            {"txid": tx1.rehash(), "wtxid": tx1.getwtxid()},
            {"txid": tx2.rehash(), "wtxid": tx2.getwtxid(), "allowed": False, "reject-reason": "conflict-in-package"}
        ])

    def test_rbf(self):
        node = self.nodes[0]
        coin = self.coins.pop()
        inputs = [{"txid" : coin["txid"], "vout" : 0, "sequence": BIP125_SEQUENCE_NUMBER}]
        fee = Decimal('0.00125000')
        output = {node.get_deterministic_priv_key().address: 50 - fee}
        raw_replaceable_tx = node.createrawtransaction(inputs, output)
        signed_replaceable_tx = node.signrawtransactionwithkey(hexstring=raw_replaceable_tx, privkeys=self.privkeys)
        testres_replaceable = node.testmempoolaccept([signed_replaceable_tx["hex"]])
        replaceable_tx = CTransaction()
        replaceable_tx.deserialize(BytesIO(hex_str_to_bytes(signed_replaceable_tx["hex"])))
        assert_equal(testres_replaceable, [
            {"txid": replaceable_tx.rehash(), "wtxid": replaceable_tx.getwtxid(),
            "allowed": True, "vsize": replaceable_tx.get_vsize(), "fees": { "base": fee }}
        ])

        # Replacement transaction is identical except has double the fee
        replacement_tx = CTransaction()
        replacement_tx.deserialize(BytesIO(hex_str_to_bytes(signed_replaceable_tx["hex"])))
        replacement_tx.vout[0].nValue -= int(fee * COIN)  # Doubled fee
        signed_replacement_tx = node.signrawtransactionwithkey(replacement_tx.serialize().hex(), self.privkeys)
        replacement_tx.deserialize(BytesIO(hex_str_to_bytes(signed_replacement_tx["hex"])))

        self.log.info("Test that transactions within a package cannot replace each other")
        testres_rbf_conflicting = node.testmempoolaccept([signed_replaceable_tx["hex"], signed_replacement_tx["hex"]])
        assert_equal(testres_rbf_conflicting, [
            {"txid": replaceable_tx.rehash(), "wtxid": replaceable_tx.getwtxid()},
            {"txid": replacement_tx.rehash(), "wtxid": replacement_tx.getwtxid(),
            "allowed": False, "reject-reason": "conflict-in-package"}
        ])

        self.log.info("Test a package with a transaction replacing a mempool transaction")
        node.sendrawtransaction(signed_replaceable_tx["hex"])
        testres_rbf = node.testmempoolaccept(self.independent_txns_hex + [signed_replacement_tx["hex"]])
        testres_replacement = testres_replaceable[0]
        testres_replacement["txid"] = replacement_tx.rehash()
        testres_replacement["wtxid"] = replacement_tx.getwtxid()
        testres_replacement["fees"]["base"] = Decimal(str(2 * fee))
        assert_equal(testres_rbf, self.independent_txns_testres + [testres_replacement])


if __name__ == "__main__":
    RPCPackagesTest().main()
