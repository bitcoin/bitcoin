#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test sigop limit mempool policy (`-bytespersigop` parameter)"""
from math import ceil

from test_framework.messages import (
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    WITNESS_SCALE_FACTOR,
)
from test_framework.script import (
    CScript,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_ENDIF,
    OP_FALSE,
    OP_IF,
    OP_RETURN,
    OP_TRUE,
)
from test_framework.script_util import (
    script_to_p2wsh_script,
)
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than_or_equal,
)
from test_framework.wallet import MiniWallet


DEFAULT_BYTES_PER_SIGOP = 20  # default setting


class BytesPerSigOpTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # allow large datacarrier output to pad transactions
        self.extra_args = [['-datacarriersize=100000']]

    def create_p2wsh_spending_tx(self, witness_script, output_script):
        """Create a 1-input-1-output P2WSH spending transaction with only the
           witness script in the witness stack and the given output script."""
        # create P2WSH address and fund it via MiniWallet first
        txid, vout = self.wallet.send_to(
            from_node=self.nodes[0],
            scriptPubKey=script_to_p2wsh_script(witness_script),
            amount=1000000,
        )

        # create spending transaction
        tx = CTransaction()
        tx.vin = [CTxIn(COutPoint(int(txid, 16), vout))]
        tx.wit.vtxinwit = [CTxInWitness()]
        tx.wit.vtxinwit[0].scriptWitness.stack = [bytes(witness_script)]
        tx.vout = [CTxOut(500000, output_script)]
        return tx

    def test_sigops_limit(self, bytes_per_sigop, num_sigops):
        sigop_equivalent_vsize = ceil(num_sigops * bytes_per_sigop / WITNESS_SCALE_FACTOR)
        self.log.info(f"- {num_sigops} sigops (equivalent size of {sigop_equivalent_vsize} vbytes)")

        # create a template tx with the specified sigop cost in the witness script
        # (note that the sigops count even though being in a branch that's not executed)
        num_multisigops = num_sigops // 20
        num_singlesigops = num_sigops % 20
        witness_script = CScript(
            [OP_FALSE, OP_IF] +
            [OP_CHECKMULTISIG]*num_multisigops +
            [OP_CHECKSIG]*num_singlesigops +
            [OP_ENDIF, OP_TRUE]
        )
        # use a 256-byte data-push as lower bound in the output script, in order
        # to avoid having to compensate for tx size changes caused by varying
        # length serialization sizes (both for scriptPubKey and data-push lengths)
        tx = self.create_p2wsh_spending_tx(witness_script, CScript([OP_RETURN, b'X'*256]))

        # bump the tx to reach the sigop-limit equivalent size by padding the datacarrier output
        assert_greater_than_or_equal(sigop_equivalent_vsize, tx.get_vsize())
        vsize_to_pad = sigop_equivalent_vsize - tx.get_vsize()
        tx.vout[0].scriptPubKey = CScript([OP_RETURN, b'X'*(256+vsize_to_pad)])
        assert_equal(sigop_equivalent_vsize, tx.get_vsize())

        res = self.nodes[0].testmempoolaccept([tx.serialize().hex()])[0]
        assert_equal(res['allowed'], True)
        assert_equal(res['vsize'], sigop_equivalent_vsize)

        # increase the tx's vsize to be right above the sigop-limit equivalent size
        # => tx's vsize in mempool should also grow accordingly
        tx.vout[0].scriptPubKey = CScript([OP_RETURN, b'X'*(256+vsize_to_pad+1)])
        res = self.nodes[0].testmempoolaccept([tx.serialize().hex()])[0]
        assert_equal(res['allowed'], True)
        assert_equal(res['vsize'], sigop_equivalent_vsize+1)

        # decrease the tx's vsize to be right below the sigop-limit equivalent size
        # => tx's vsize in mempool should stick at the sigop-limit equivalent
        # bytes level, as it is higher than the tx's serialized vsize
        # (the maximum of both is taken)
        tx.vout[0].scriptPubKey = CScript([OP_RETURN, b'X'*(256+vsize_to_pad-1)])
        res = self.nodes[0].testmempoolaccept([tx.serialize().hex()])[0]
        assert_equal(res['allowed'], True)
        assert_equal(res['vsize'], sigop_equivalent_vsize)

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        for bytes_per_sigop in (DEFAULT_BYTES_PER_SIGOP, 43, 81, 165, 327, 649, 1072):
            if bytes_per_sigop == DEFAULT_BYTES_PER_SIGOP:
                self.log.info(f"Test default sigops limit setting ({bytes_per_sigop} bytes per sigop)...")
            else:
                bytespersigop_parameter = f"-bytespersigop={bytes_per_sigop}"
                self.log.info(f"Test sigops limit setting {bytespersigop_parameter}...")
                self.restart_node(0, extra_args=[bytespersigop_parameter] + self.extra_args[0])

            for num_sigops in (69, 101, 142, 183, 222):
                self.test_sigops_limit(bytes_per_sigop, num_sigops)


if __name__ == '__main__':
    BytesPerSigOpTest().main()
