#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test OP_CHECKCONTRACTVERIFY
"""

from dataclasses import dataclass
import hashlib
from typing import Any, Dict, List, Optional, Tuple, Union

from test_framework import script, key
from test_framework.test_framework import BitcoinTestFramework, TestNode
from test_framework.p2p import P2PInterface
from test_framework.wallet import MiniWallet, MiniWalletMode
from test_framework.script import (
    CScript,
    OP_CHECKCONTRACTVERIFY,
    OP_RETURN,
    OP_TRUE,
    TaprootInfo,
)
from test_framework.messages import CTransaction, COutPoint, CTxInWitness, CTxOut, CTxIn
from test_framework.util import assert_equal, assert_raises_rpc_error

# Modes for OP_CHECKCONTRACTVERIFY
CCV_MODE_CHECK_INPUT: int = -1
CCV_MODE_CHECK_OUTPUT: int = 0
CCV_MODE_CHECK_OUTPUT_IGNORE_AMOUNT: int = 1
CCV_MODE_CHECK_OUTPUT_DEDUCT_AMOUNT: int = 2


# x-only public key with provably unknown private key defined in BIP-341
NUMS_KEY: bytes = bytes.fromhex(key.H_POINT)


# ======= Utility Classes & Functions =======

TapLeaf = Tuple[str, CScript]
TapTree = Union[TapLeaf, List["TapTree"]]


def flatten_scripts(tree: TapTree) -> List[TapLeaf]:
    result = []
    if isinstance(tree, list):
        assert len(tree) == 2
        result.extend(flatten_scripts(tree[0]))
        result.extend(flatten_scripts(tree[1]))
    else:
        assert isinstance(tree, tuple) and len(tree) == 2
        result.append(tree)
    return result


class P2TR:
    """
    A class representing a Pay-to-Taproot script.
    """

    def __init__(self, internal_pubkey: bytes, scripts: TapTree):
        assert len(internal_pubkey) == 32

        self.internal_pubkey = internal_pubkey
        self.scripts = scripts
        self.tr_info = script.taproot_construct(internal_pubkey, [scripts])

    def get_script(self, clause_name: str):
        for name, clause_script in flatten_scripts(self.scripts):
            if name == clause_name:
                return clause_script
        raise ValueError(f"Clause {clause_name} not found")

    def get_tr_info(self) -> TaprootInfo:
        return self.tr_info

    def get_tx_out(self, value: int) -> CTxOut:
        return CTxOut(
            nValue=value,
            scriptPubKey=self.get_tr_info().scriptPubKey
        )


class AugmentedP2TR:
    """
    An abstract class representing a Pay-to-Taproot script with some embedded data.
    While the exact script can only be produced once the embedded data is known,
    the scripts and the "naked internal key" are decided in advance.
    """

    def __init__(self, naked_internal_pubkey: bytes):
        assert len(naked_internal_pubkey) == 32

        self.naked_internal_pubkey = naked_internal_pubkey

    def get_scripts(self) -> TapTree:
        raise NotImplementedError("This must be implemented in subclasses")

    def get_script(self, clause_name: str):
        for name, clause_script in flatten_scripts(self.get_scripts()):
            if name == clause_name:
                return clause_script
        raise ValueError(f"Clause {clause_name} not found")

    def get_taptree(self) -> bytes:
        # use dummy data, since it doesn't affect the merkle root
        return self.get_tr_info(b'').merkle_root

    def get_tr_info(self, data: bytes) -> TaprootInfo:
        if len(data) == 0:
            internal_pubkey = self.naked_internal_pubkey
        else:
            data_hash = hashlib.sha256(
                self.naked_internal_pubkey + data).digest()

            internal_pubkey, _ = key.tweak_add_pubkey(
                self.naked_internal_pubkey, data_hash)
        return script.taproot_construct(internal_pubkey, [self.get_scripts()])

    def get_tx_out(self, value: int, data: bytes) -> CTxOut:
        return CTxOut(nValue=value, scriptPubKey=self.get_tr_info(data).scriptPubKey)


class PrivkeyPlaceholder:
    """
    A placeholder for a witness element that will be replaced with the
    corresponding Schnorr signature.
    """

    def __init__(self, privkey: key.ECKey):
        self.privkey = privkey


@dataclass
class CcvInput:
    txid: str
    vout_index: int
    amount: int
    contract: Union[P2TR, AugmentedP2TR]
    data: Optional[bytes]
    leaf_name: str

    # excluding the control block and the script
    wit_stack: List[Union[bytes, PrivkeyPlaceholder]]

    nSequence: int = 0


def create_tx(
    inputs: List[CcvInput],
    outputs: List[CTxOut],
    *,
    version: int = 2,
    nLockTime: int = 0
) -> CTransaction:
    """
    Creates a transaction with the given inputs and outputs.
    Inputs are spending UTXOs that might or might not have data
    embedded with OP_CHECKCONTRACTVERIFY.

    The witness is constructed by replacing each PrivkeyPlaceholder with a signature
    constructed with its private key (using SIGHASH_DEFAULT), and then appending the
    control block and the leaf script, per BIP-341.
    """
    tx = CTransaction()
    tx.version = version
    tx.nLockTime = nLockTime
    tx.vin = []
    tx.wit.vtxinwit = []

    in_txouts = []

    for inp in inputs:
        txin = CTxIn(COutPoint(int(inp.txid, 16), inp.vout_index),
                     nSequence=inp.nSequence)
        tx.vin.append(txin)

        # Retrieve leaf script & control block
        if isinstance(inp.contract, AugmentedP2TR):
            assert inp.data is not None
            tr_info = inp.contract.get_tr_info(inp.data)
        else:
            assert isinstance(inp.contract, P2TR) and inp.data is None
            tr_info = inp.contract.get_tr_info()

        in_txouts.append(
            CTxOut(nValue=inp.amount, scriptPubKey=tr_info.scriptPubKey))

        leaf_script = tr_info.leaves[inp.leaf_name].script
        control_block = tr_info.controlblock_for_script_spend(inp.leaf_name)
        wit_stack = inp.wit_stack.copy()
        wit_stack.extend([leaf_script, control_block])

        wit = CTxInWitness()
        wit.scriptWitness.stack = wit_stack
        tx.wit.vtxinwit.append(wit)

    tx.vout = outputs

    # For each input, if any witness stack element is a PrivkeyPlaceholder, replace with the
    # actual signature. This assumes signing with SIGHASH_DEFAULT.
    for i in range(len(tx.vin)):
        for j in range(len(tx.wit.vtxinwit[i].scriptWitness.stack)):
            if isinstance(tx.wit.vtxinwit[i].scriptWitness.stack[j], PrivkeyPlaceholder):
                contract = inputs[i].contract
                clause = inputs[i].leaf_name
                privkey: key.ECKey = tx.wit.vtxinwit[i].scriptWitness.stack[j].privkey
                sigmsg = script.TaprootSignatureHash(
                    tx, in_txouts, input_index=i, hash_type=0,
                    scriptpath=True, leaf_script=contract.get_script(clause),
                )
                sig = key.sign_schnorr(privkey.get_bytes(), sigmsg)
                tx.wit.vtxinwit[i].scriptWitness.stack[j] = sig

    return tx


# ======= Contract definitions =======


class EmbedData(P2TR):
    """
    An output that can only be spent to a `CompareWithEmbeddedData` output, with
    its embedded data passed as the witness.
    """

    def __init__(self, ignore_amount: bool = False):
        super().__init__(
            NUMS_KEY,
            (
                "forced",
                CScript([
                    # witness: <data>
                    0,  # index
                    0,  # use NUMS as the naked pubkey
                    CompareWithEmbeddedData().get_taptree(),  # output Merkle tree
                    CCV_MODE_CHECK_OUTPUT_IGNORE_AMOUNT if ignore_amount else 0,  # mode
                    OP_CHECKCONTRACTVERIFY,
                    OP_TRUE
                ])
            )
        )


class CompareWithEmbeddedData(AugmentedP2TR):
    """
    An output that can only be spent by passing the embedded data in the witness.
    """

    def __init__(self):
        super().__init__(NUMS_KEY)

    def get_scripts(self) -> TapTree:
        return (
            "check_data",
            CScript([
                # witness: <data>
                -1,  # index: check current input
                0,   # use NUMS as the naked pubkey
                -1,  # use taptree of the current input
                CCV_MODE_CHECK_INPUT,  # check input
                OP_CHECKCONTRACTVERIFY,
                OP_TRUE
            ])
        )


class SendToSelf(P2TR):
    """
    A utxo that can only be spent by sending the entire amount to the same script.
    The output index must match the input index.
    """

    def __init__(self):
        super().__init__(
            NUMS_KEY,
            ("send_to_self", CScript([
                # witness: <>
                0,   # no data tweaking
                -1,  # index: check current output
                -1,  # use internal key of the current input
                -1,  # use taptree of the current input
                CCV_MODE_CHECK_OUTPUT,  # all the amount must go to this output
                OP_CHECKCONTRACTVERIFY,
                OP_TRUE
            ]))
        )


class SplitFunds(P2TR):
    """
    An output that can only be spent by sending part of the fund to itself,
    and all the remaining funds to the P2TR address with NUMS pubkey.
    """

    def __init__(self):
        super().__init__(
            NUMS_KEY,
            (
                "split", CScript([
                    # witness: <>
                    0,   # no data tweaking
                    0,   # index
                    -1,  # use internal key of the current input
                    -1,  # use taptree of the current input
                    # check output, deduct amount from input
                    CCV_MODE_CHECK_OUTPUT_DEDUCT_AMOUNT,
                    OP_CHECKCONTRACTVERIFY,

                    0,   # no data tweaking
                    1,   # index
                    0,   # NUMS pubkey
                    0,   # no taptweak
                    # check output, all remaining amount must go to this output
                    CCV_MODE_CHECK_OUTPUT,
                    OP_CHECKCONTRACTVERIFY,

                    OP_TRUE
                ])
            )
        )


def ccv(mode=0, index=0, data=0, pk=0, tree=0):
    return CScript([
        data,
        index,
        pk,
        tree,
        mode,
        OP_CHECKCONTRACTVERIFY,
    ])


class TestCCVSuccess(P2TR):
    """
    A utxo with hardcoded parameters to test the OP_SUCCESS behavior for the mode.
    """

    def __init__(self, **kwargs):
        super().__init__(NUMS_KEY, [
            ("spend", CScript([
                *ccv(**kwargs),
                OP_RETURN  # make sure that the script execution fails if it reaches here
            ]))
        ])


class TestCCVFail(P2TR):
    """
    A utxo with hardcoded parameters to test the failure cases for invalid parameters.
    """

    def __init__(self, **kwargs):
        super().__init__(NUMS_KEY, [
            ("spend", CScript([
                *ccv(**kwargs),
                OP_TRUE  # make sure that the script execution succeeds if it reaches here
            ]))
        ])


# ======= Tests =======


class CheckContractVerifyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [
            [
                # Use only one script thread to get the exact reject reason for testing
                "-par=1",
                # TODO: figure out changes to standardness rules
                "-acceptnonstdtxn=1",
                # TODO: remove when package relay submission becomes a thing.
                "-minrelaytxfee=0",
                "-blockmintxfee=0",
            ]
        ]
        self.setup_clean_chain = True

    def run_test(self):
        wallet = MiniWallet(self.nodes[0], mode=MiniWalletMode.RAW_OP_TRUE)
        node = self.nodes[0]
        node.add_p2p_connection(P2PInterface())

        # Generate some matured UTXOs to spend into vaults.
        self.generate(wallet, 200)

        self.test_ccv(node, wallet, data=b'\x42'*32, ignore_amount=False)
        self.test_ccv(node, wallet, data=b'', ignore_amount=True)
        self.test_ccv(node, wallet, data=b'\x42'*32, ignore_amount=True)
        self.test_many_to_one(node, wallet)
        self.test_send_to_self(node, wallet)
        self.test_deduct_amount(node, wallet)
        self.test_undefined_modes_opsuccess(node, wallet)
        self.test_invalid_parameters(node, wallet)

    def test_ccv(
        self,
        node: TestNode,
        wallet: MiniWallet,
        data,
        ignore_amount: bool
    ):
        assert_equal(node.getmempoolinfo()["size"], 0)

        #            tx1      tx2
        # MiniWallet ==> S ==(data)==> T ==> OP_TRUE
        #    S: tr(NUMS, CheckOutputContract(T, data))
        #    T: tr(NUMS×data, CheckInputContract(data == 0x424242...))

        T = CompareWithEmbeddedData()
        S = EmbedData(ignore_amount=ignore_amount)

        # Create UTXO for S

        # If ignoring amount, the contract value (if any) can be used to pay for fees
        # otherwise, we put 0 fees for the sake of the tests.
        # In practice, package relay would likely be used to manage fees, when ready.
        amount_sats = 100000
        fees = 1000 if ignore_amount else 0

        res = wallet.send_to(
            from_node=node,
            scriptPubKey=S.get_tr_info().scriptPubKey,
            amount=amount_sats
        )
        tx1_txid = res['txid']
        tx1_n = res['sent_vout']

        # Create UTXO for T

        tx2 = create_tx(
            inputs=[CcvInput(tx1_txid, tx1_n, amount_sats,
                             S, None, "forced", [data])],
            outputs=[T.get_tx_out(amount_sats - fees, data)]
        )

        if not ignore_amount:
            # broadcast with insufficient output amount; this should fail
            tx2.vout[0].nValue -= 1
            self.assert_broadcast_tx(
                tx2, err_msg='Incorrect amount for OP_CHECKCONTRACTVERIFY')
            tx2.vout[0].nValue += 1

        tx2_txid = self.assert_broadcast_tx(tx2, mine_all=True)

        # create an output for the final spend
        dest_ctr = P2TR(NUMS_KEY, [("true", CScript([OP_TRUE]))])

        # try to spend with the wrong data
        tx3_wrongdata = create_tx(
            inputs=[CcvInput(tx2_txid, 0, amount_sats - fees, T,
                             data, "check_data", [b'\x43'*32])],
            outputs=[dest_ctr.get_tx_out(amount_sats - 2 * fees)]
        )

        self.assert_broadcast_tx(
            tx3_wrongdata, err_msg="Mismatching contract data or program")

        # Broadcasting with correct data succeeds
        tx3 = create_tx(
            inputs=[CcvInput(tx2_txid, 0, amount_sats - fees,
                             T, data, "check_data", [data])],
            outputs=[dest_ctr.get_tx_out(amount_sats - 2 * fees)]
        )
        self.assert_broadcast_tx(tx3, mine_all=True)

    def test_many_to_one(
        self,
        node: TestNode,
        wallet: MiniWallet
    ):
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Creates 3 utxos with different amounts and all with the same EmbedData script.
        # Spending them together to a single output, the total amount must be preserved.

        #               tx1,tx2,tx3       tx4
        # MiniWallet ==> S1, S2, S3 ==> T ==> OP_TRUE
        #   S1: tr(NUMS, CheckOutputContract(T, data))
        #   S2: tr(NUMS, CheckOutputContract(T, data))
        #   S3: tr(NUMS, CheckOutputContract(T, data))
        #   T: tr(NUMS×data, CheckInputContract(data == 0x424242...))

        data = b'\x42'*32
        amounts_sats: List[int] = []

        T = CompareWithEmbeddedData()
        S = EmbedData()

        tx_ids_and_n: List[Tuple[str, int]] = []
        inputs = []
        for i in range(3):
            # Create UTXO for S[i]
            amount_sats = 100000 * (i + 1)
            amounts_sats.append(amount_sats)

            res = wallet.send_to(
                from_node=node,
                scriptPubKey=S.get_tr_info().scriptPubKey,
                amount=amount_sats
            )
            tx_ids_and_n.append((res['txid'], res['sent_vout']))
            inputs.append(CcvInput(
                res['txid'], res['sent_vout'], amount_sats, S, None, "forced", [data]
            ))

        # Create UTXO for T
        outputs = [T.get_tx_out(sum(amounts_sats), data)]
        tx4 = create_tx(inputs, outputs)

        # broadcast with insufficient output amount; this should fail
        tx4.vout[0].nValue -= 1
        self.assert_broadcast_tx(
            tx4, err_msg='Incorrect amount for OP_CHECKCONTRACTVERIFY')
        tx4.vout[0].nValue += 1

        # correct amount succeeds
        self.assert_broadcast_tx(tx4, mine_all=True)

    def test_send_to_self(
        self,
        node: TestNode,
        wallet: MiniWallet
    ):
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Creates a utxo with the SendToSelf contract, and verifies that:
        # - sending to a different scriptPubKey fails;
        # - sending to an output with the same scriptPubKey works.

        amount_sats = 10000

        C = SendToSelf()

        res = wallet.send_to(
            from_node=node,
            scriptPubKey=C.get_tr_info().scriptPubKey,
            amount=amount_sats
        )
        tx_id, n = (res['txid'], res['sent_vout'])

        # Create UTXO for C
        tx2 = create_tx(
            inputs=[CcvInput(tx_id, n, amount_sats, C,
                             None, "send_to_self", [])],
            outputs=[C.get_tx_out(amount_sats)]
        )

        # broadcast with insufficient output amount; this should fail
        tx2.vout[0].nValue -= 1
        self.assert_broadcast_tx(
            tx2, err_msg='Incorrect amount for OP_CHECKCONTRACTVERIFY')
        tx2.vout[0].nValue += 1

        # broadcast with incorrect output script; this should fail
        correct_script = tx2.vout[0].scriptPubKey
        tx2.vout[0].scriptPubKey = correct_script[:-1] + \
            bytes([correct_script[-1] ^ 1])
        self.assert_broadcast_tx(
            tx2, err_msg="Mismatching contract data or program")
        tx2.vout[0].scriptPubKey = correct_script

        # correct amount succeeds
        self.assert_broadcast_tx(tx2, mine_all=True)

    def test_deduct_amount(
        self,
        node: TestNode,
        wallet: MiniWallet
    ):
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Tests the behavior of the CCV_MODE_CHECK_OUTPUT_DEDUCT_AMOUNT mode

        #            tx1              tx2
        # MiniWallet ==> SplitFunds() ==> SplitFunds(), P2TR(NUMS)

        S = SplitFunds()

        amount_sats = 10000
        recover_amount_sats = 3000  # the amount that goes back to the same script

        res = wallet.send_to(
            from_node=node,
            scriptPubKey=S.get_tr_info().scriptPubKey,
            amount=amount_sats
        )
        tx1_txid, tx1_n = (res['txid'], res['sent_vout'])

        # Create UTXO for S
        tx2 = create_tx(
            inputs=[CcvInput(tx1_txid, tx1_n, amount_sats,
                             S, None, "split", [])],
            outputs=[
                S.get_tx_out(recover_amount_sats),
                CTxOut(
                    nValue=amount_sats - recover_amount_sats - 1,  # 1 sat short, this must fail
                    scriptPubKey=b'\x51\x20' + NUMS_KEY
                )
            ]
        )

        self.assert_broadcast_tx(
            tx2, err_msg='Incorrect amount for OP_CHECKCONTRACTVERIFY')

        tx2.vout[1].nValue += 1  # correct amount

        self.assert_broadcast_tx(tx2, mine_all=True)

    def test_undefined_modes_opsuccess(
        self,
        node: TestNode,
        wallet: MiniWallet
    ):
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Tests that undefined modes immediately terminate the script successfully.

        testcases = [
            {"mode": -2},
            {"mode": 3},
        ]

        amount_sats = 10000
        fees = 1000

        for testcase in testcases:
            C = TestCCVSuccess(**testcase)

            res = wallet.send_to(
                from_node=node,
                scriptPubKey=C.get_tr_info().scriptPubKey,
                amount=amount_sats
            )
            tx_id, n = (res['txid'], res['sent_vout'])

            # Create UTXO for C
            tx2 = create_tx(
                inputs=[CcvInput(tx_id, n, amount_sats, C, None, "spend", [])],
                outputs=[CTxOut(
                    nValue=amount_sats - fees,
                    scriptPubKey=P2TR(
                        NUMS_KEY, [("true", CScript([OP_TRUE]))]).tr_info.scriptPubKey
                )]
            )

            # it should succeed
            self.assert_broadcast_tx(tx2, mine_all=True)

    def test_invalid_parameters(
        self,
        node: TestNode,
        wallet: MiniWallet
    ):
        assert_equal(node.getmempoolinfo()["size"], 0)

        # Tests that invalid parameters, that cause the Script validation to fail

        testcases: List[Dict[str, Any]] = [
            {"pk": -2},
            {"pk": 1},
            {"pk": b'\x42'*31},
            {"pk": b'\x42'*33},
            {"pk": b'\x42'*64},
            {"tree": -2},
            {"tree": 1},
            {"tree": b'\x42'*31},
            {"tree": b'\x42'*33},
        ]

        amount_sats = 10000
        fees = 1000

        for testcase in testcases:
            C = TestCCVFail(**testcase)

            res = wallet.send_to(
                from_node=node,
                scriptPubKey=C.get_tr_info().scriptPubKey,
                amount=amount_sats
            )
            tx_id, n = (res['txid'], res['sent_vout'])

            # Create UTXO for C
            tx2 = create_tx(
                inputs=[CcvInput(tx_id, n, amount_sats, C, None, "spend", [])],
                outputs=[CTxOut(
                    nValue=amount_sats - fees,
                    scriptPubKey=P2TR(
                        NUMS_KEY, [("true", CScript([OP_TRUE]))]).tr_info.scriptPubKey
                )]
            )

            # it should fail
            self.assert_broadcast_tx(
                tx2, err_msg='Invalid arguments for OP_CHECKCONTRACTVERIFY')

    # taken from OP_VAULT PR's functional test

    def assert_broadcast_tx(
        self,
        tx: CTransaction,
        mine_all: bool = False,
        err_msg: Optional[str] = None
    ) -> str:
        """
        Broadcast a transaction and facilitate various assertions about how the
        broadcast went.
        """
        node = self.nodes[0]
        txhex = tx.serialize().hex()
        txid = tx.rehash()

        if not err_msg:
            assert_equal(node.sendrawtransaction(txhex), txid)
        else:
            assert_raises_rpc_error(-26, err_msg,
                                    node.sendrawtransaction, txhex)

        if mine_all:
            self.generate(node, 1)
            assert_equal(node.getmempoolinfo()["size"], 0)

        return txid


if __name__ == "__main__":
    CheckContractVerifyTest(__file__).main()
