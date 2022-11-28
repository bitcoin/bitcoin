#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://www.opensource.org/licenses/mit-license.php.
"""Test output type mixing during coin selection

A wallet may have different types of UTXOs to choose from during coin selection,
where output type is one of the following:
    - BECH32M
    - BECH32
    - P2SH-SEGWIT
    - LEGACY

This test verifies that mixing different output types is avoided unless
absolutely necessary. Both wallets start with zero funds. Alice mines
enough blocks to have spendable coinbase outputs. Alice sends three
random value payments which sum to 10SYS for each output type to Bob,
for a total of 40SYS in Bob's wallet.

Bob then sends random valued payments back to Alice, some of which need
unconfirmed change, and we verify that none of these payments contain mixed
inputs. Finally, Bob sends the remainder of his funds, which requires mixing.

The payment values are random, but chosen such that they sum up to a specified
total. This ensures we are not relying on specific values for the UTXOs,
but still know when to expect mixing due to the wallet being close to empty.

"""

import random
from test_framework.test_framework import SyscoinTestFramework
from test_framework.blocktools import COINBASE_MATURITY

ADDRESS_TYPES = [
    "bech32m",
    "bech32",
    "p2sh-segwit",
    "legacy",
]


def is_bech32_address(node, addr):
    """Check if an address contains a bech32 output."""
    addr_info = node.getaddressinfo(addr)
    return addr_info['desc'].startswith('wpkh(')


def is_bech32m_address(node, addr):
    """Check if an address contains a bech32m output."""
    addr_info = node.getaddressinfo(addr)
    return addr_info['desc'].startswith('tr(')


def is_p2sh_segwit_address(node, addr):
    """Check if an address contains a P2SH-Segwit output.
       Note: this function does not actually determine the type
       of P2SH output, but is sufficient for this test in that
       we are only generating P2SH-Segwit outputs.
    """
    addr_info = node.getaddressinfo(addr)
    return addr_info['desc'].startswith('sh(wpkh(')


def is_legacy_address(node, addr):
    """Check if an address contains a legacy output."""
    addr_info = node.getaddressinfo(addr)
    return addr_info['desc'].startswith('pkh(')


def is_same_type(node, tx):
    """Check that all inputs are of the same OutputType"""
    vins = node.getrawtransaction(tx, True)['vin']
    inputs = []
    for vin in vins:
        prev_tx, n = vin['txid'], vin['vout']
        inputs.append(
            node.getrawtransaction(
                prev_tx,
                True,
            )['vout'][n]['scriptPubKey']['address']
        )
    has_legacy = False
    has_p2sh = False
    has_bech32 = False
    has_bech32m = False

    for addr in inputs:
        if is_legacy_address(node, addr):
            has_legacy = True
        if is_p2sh_segwit_address(node, addr):
            has_p2sh = True
        if is_bech32_address(node, addr):
            has_bech32 = True
        if is_bech32m_address(node, addr):
            has_bech32m = True

    return (sum([has_legacy, has_p2sh, has_bech32, has_bech32m]) == 1)


def generate_payment_values(n, m):
    """Return a randomly chosen list of n positive integers summing to m.
    Each such list is equally likely to occur."""

    dividers = sorted(random.sample(range(1, m), n - 1))
    return [a - b for a, b in zip(dividers + [m], [0] + dividers)]


class AddressInputTypeGrouping(SyscoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, legacy=False)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [
            [
                "-addresstype=bech32",
                "-whitelist=noban@127.0.0.1",
                "-txindex",
            ],
            [
                "-addresstype=p2sh-segwit",
                "-whitelist=noban@127.0.0.1",
                "-txindex",
            ],
        ]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_sqlite()

    def make_payment(self, A, B, v, addr_type):
        fee_rate = random.randint(1, 20)
        self.log.debug(f"Making payment of {v} SYS at fee_rate {fee_rate}")
        tx = B.sendtoaddress(
            address=A.getnewaddress(address_type=addr_type),
            amount=v,
            fee_rate=fee_rate,
        )
        return tx

    def run_test(self):

        # alias self.nodes[i] to A, B for readability
        A, B = self.nodes[0], self.nodes[1]
        self.generate(A, COINBASE_MATURITY + 5)

        self.log.info("Creating mixed UTXOs in B's wallet")
        for v in generate_payment_values(3, 10):
            self.log.debug(f"Making payment of {v} SYS to legacy")
            A.sendtoaddress(B.getnewaddress(address_type="legacy"), v)

        for v in generate_payment_values(3, 10):
            self.log.debug(f"Making payment of {v} SYS to p2sh")
            A.sendtoaddress(B.getnewaddress(address_type="p2sh-segwit"), v)

        for v in generate_payment_values(3, 10):
            self.log.debug(f"Making payment of {v} SYS to bech32")
            A.sendtoaddress(B.getnewaddress(address_type="bech32"), v)

        for v in generate_payment_values(3, 10):
            self.log.debug(f"Making payment of {v} SYS to bech32m")
            A.sendtoaddress(B.getnewaddress(address_type="bech32m"), v)

        self.generate(A, 1)

        self.log.info("Sending payments from B to A")
        for v in generate_payment_values(5, 9):
            tx = self.make_payment(
                A, B, v, random.choice(ADDRESS_TYPES)
            )
            self.generate(A, 1)
            assert is_same_type(B, tx)

        tx = self.make_payment(A, B, 30.99, random.choice(ADDRESS_TYPES))
        assert not is_same_type(B, tx)


if __name__ == '__main__':
    AddressInputTypeGrouping().main()
