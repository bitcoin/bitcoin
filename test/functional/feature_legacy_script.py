#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test script interpreter edge cases in bare, P2SH and P2WSH scripts."""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.script import (
    CScript,
    OP_0,
    OP_1,
    OP_16,
    OP_1ADD,
    OP_2,
    OP_CHECKSEQUENCEVERIFY,
    OP_DEPTH,
    OP_DROP,
    OP_EQUAL,
    OP_EQUALVERIFY,
    OP_IFDUP,
    OP_NOP,
    OP_ROLL,
)
from test_framework.spenders import (
    ERR_EQUALVERIFY,
    ERR_NEGATIVE_LOCKTIME,
    ERR_SCRIPT_NUM,
    ERR_SIG_PUSHONLY,
    ERR_STACK_SIZE,
    SpenderTestMixin,
    add_spender,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import NodeSigner


def spenders_legacy():
    """Return a list of Spenders for script edge cases that don't involve Taproot."""

    spenders = []

    for p2sh in [False, True]:
        for witv0 in [False, True]:
            standard = p2sh or witv0
            # OP_CHECKSEQUENCEVERIFY fails for every negative operand, not just -1. The two's complement form of -2 has
            # the disable flag (1 << 31) set, but the negative check comes first. A positive operand with the disable
            # flag set is treated as a NOP.
            add_spender(spenders, "legacy/csv-negative", p2sh=p2sh, witv0=witv0, standard=standard, script=CScript([OP_CHECKSEQUENCEVERIFY]), inputs=[b'\x00\x00\x00\x80\x00'], failure={"inputs": [b'\x82']}, **ERR_NEGATIVE_LOCKTIME)
            # Numeric operands are limited to 4 bytes even when their value would fit in fewer once minimally encoded.
            add_spender(spenders, "legacy/num-nonminimal-size", p2sh=p2sh, witv0=witv0, standard=False, script=CScript([OP_1ADD, OP_DROP, OP_1]), inputs=[b'\xff\x00\x00\x00'], failure={"inputs": [b'\xff\x00\x00\x00\x00']}, **ERR_SCRIPT_NUM)
            # OP_IFDUP does not duplicate a false value, including negative zero and non-minimally encoded zero.
            for zero in [b'\x80', b'\x00\x00', b'\x00\x80']:
                add_spender(spenders, "legacy/ifdup-zero", p2sh=p2sh, witv0=witv0, standard=standard, script=CScript([OP_IFDUP, OP_DEPTH, OP_1, OP_EQUALVERIFY, OP_DROP, OP_1]), inputs=[zero], failure={"inputs": [b'\xaa']}, **ERR_EQUALVERIFY)
            # OP_ROLL moves the element at the given depth, even if elements above and below it have the same value.
            add_spender(spenders, "legacy/roll-duplicate", p2sh=p2sh, witv0=witv0, standard=standard, script=CScript([OP_2, OP_ROLL, b'\xaa', OP_EQUALVERIFY, b'\xaa', OP_EQUALVERIFY, b'\xcc', OP_EQUALVERIFY, b'\xbb', OP_EQUALVERIFY, b'\xaa', OP_EQUAL]), inputs=[b'\xaa', b'\xbb', b'\xaa', b'\xcc', b'\xaa'], failure={"inputs": [b'\xaa', b'\xbb', b'\xcc', b'\xaa', b'\xaa']}, **ERR_EQUALVERIFY)

    # A P2SH scriptSig may contain OP_16, the highest opcode that counts as a push, but not OP_NOP, the next one up.
    add_spender(spenders, "legacy/p2sh-pushonly", p2sh=True, script=CScript([OP_16, OP_EQUAL]), inputs=[CScript([OP_16])], failure={"inputs": [CScript([OP_16, OP_NOP])]}, **ERR_SIG_PUSHONLY)
    # The P2SH scriptPubKey runs on the stack the scriptSig leaves, and its 20-byte push counts towards the stack size
    # limit. A scriptSig that leaves 999 items is valid, but one that leaves 1000 is not.
    add_spender(spenders, "legacy/p2sh-stack-size", p2sh=True, standard=False, script=CScript([OP_1]), inputs=[CScript([OP_0] * 998)], failure={"inputs": [CScript([OP_0] * 999)]}, **ERR_STACK_SIZE)

    return spenders


class LegacyScriptTest(SpenderTestMixin, BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True

    def run_test(self):
        node = self.nodes[0]
        self.nodesigner = NodeSigner(node)
        self.generatetoaddress(node, COINBASE_MATURITY + 1, self.nodesigner.getnewaddress()[2])

        # Run the spenders once in isolation, so that the standardness of each one is checked, and once combined
        # with others.
        spenders = spenders_legacy()
        self.test_spenders(node, spenders, input_counts=[1])
        self.test_spenders(node, spenders, input_counts=[2, 3])


if __name__ == '__main__':
    LegacyScriptTest(__file__).main()
