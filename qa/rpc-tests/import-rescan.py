#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (start_nodes, connect_nodes, sync_blocks, assert_equal)
from decimal import Decimal

import collections
import enum
import itertools
import functools

Call = enum.Enum("Call", "single multi")
Data = enum.Enum("Data", "address pub priv")
ImportNode = collections.namedtuple("ImportNode", "rescan")


def call_import_rpc(call, data, address, scriptPubKey, pubkey, key, label, node, rescan):
    """Helper that calls a wallet import RPC on a bitcoin node."""
    watchonly = data != Data.priv
    if call == Call.single:
        if data == Data.address:
            response = node.importaddress(address, label, rescan)
        elif data == Data.pub:
            response = node.importpubkey(pubkey, label, rescan)
        elif data == Data.priv:
            response = node.importprivkey(key, label, rescan)
        assert_equal(response, None)
    elif call == Call.multi:
        response = node.importmulti([{
            "scriptPubKey": {
                "address": address
            },
            "pubkeys": [pubkey] if data == Data.pub else [],
            "keys": [key] if data == Data.priv else [],
            "label": label,
            "watchonly": watchonly
        }], {"rescan": rescan})
        assert_equal(response, [{"success": True}])
    return watchonly


# List of RPCs that import a wallet key or address in various ways.
IMPORT_RPCS = [functools.partial(call_import_rpc, call, data) for call, data in itertools.product(Call, Data)]

# List of bitcoind nodes that will import keys.
IMPORT_NODES = [
    ImportNode(rescan=True),
    ImportNode(rescan=False),
]


class ImportRescanTest(BitcoinTestFramework):
    def __init__(self):
        super().__init__()
        self.num_nodes = 1 + len(IMPORT_NODES)

    def setup_network(self):
        extra_args = [["-debug=1"] for _ in range(self.num_nodes)]
        self.nodes = start_nodes(self.num_nodes, self.options.tmpdir, extra_args)
        for i in range(1, self.num_nodes):
            connect_nodes(self.nodes[i], 0)

    def run_test(self):
        # Create one transaction on node 0 with a unique amount and label for
        # each possible type of wallet import RPC.
        import_rpc_variants = []
        for i, import_rpc in enumerate(IMPORT_RPCS):
            label = "label{}".format(i)
            addr = self.nodes[0].validateaddress(self.nodes[0].getnewaddress(label))
            key = self.nodes[0].dumpprivkey(addr["address"])
            amount = 24.9375 - i * .0625
            txid = self.nodes[0].sendtoaddress(addr["address"], amount)
            import_rpc = functools.partial(import_rpc, addr["address"], addr["scriptPubKey"], addr["pubkey"], key,
                                           label)
            import_rpc_variants.append((import_rpc, label, amount, txid, addr))

        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        sync_blocks(self.nodes)

        # For each importing node and variation of wallet import RPC, invoke
        # the RPC and check the results from getbalance and listtransactions.
        for node, import_node in zip(self.nodes[1:], IMPORT_NODES):
            for import_rpc, label, amount, txid, addr in import_rpc_variants:
                watchonly = import_rpc(node, import_node.rescan)

                balance = node.getbalance(label, 0, True)
                if import_node.rescan:
                    assert_equal(balance, amount)
                else:
                    assert_equal(balance, 0)

                txs = node.listtransactions(label, 10000, 0, True)
                if import_node.rescan:
                    assert_equal(len(txs), 1)
                    assert_equal(txs[0]["account"], label)
                    assert_equal(txs[0]["address"], addr["address"])
                    assert_equal(txs[0]["amount"], amount)
                    assert_equal(txs[0]["category"], "receive")
                    assert_equal(txs[0]["label"], label)
                    assert_equal(txs[0]["txid"], txid)
                    assert_equal(txs[0]["confirmations"], 1)
                    assert_equal("trusted" not in txs[0], True)
                    if watchonly:
                        assert_equal(txs[0]["involvesWatchonly"], True)
                    else:
                        assert_equal("involvesWatchonly" not in txs[0], True)
                else:
                    assert_equal(len(txs), 0)

        # Create spends for all the imported addresses.
        spend_txids = []
        fee = self.nodes[0].getnetworkinfo()["relayfee"]
        for import_rpc, label, amount, txid, addr in import_rpc_variants:
            raw_tx = self.nodes[0].getrawtransaction(txid)
            decoded_tx = self.nodes[0].decoderawtransaction(raw_tx)
            input_vout = next(out["n"] for out in decoded_tx["vout"]
                              if out["scriptPubKey"]["addresses"] == [addr["address"]])
            inputs = [{"txid": txid, "vout": input_vout}]
            outputs = {self.nodes[0].getnewaddress(): Decimal(amount) - fee}
            raw_spend_tx = self.nodes[0].createrawtransaction(inputs, outputs)
            signed_spend_tx = self.nodes[0].signrawtransaction(raw_spend_tx)
            spend_txid = self.nodes[0].sendrawtransaction(signed_spend_tx["hex"])
            spend_txids.append(spend_txid)

        self.nodes[0].generate(1)
        assert_equal(self.nodes[0].getrawmempool(), [])
        sync_blocks(self.nodes)

        # Check the results from getbalance and listtransactions after the spends.
        for node, import_node in zip(self.nodes[1:], IMPORT_NODES):
            txs = node.listtransactions("*", 10000, 0, True)
            for (import_rpc, label, amount, txid, addr), spend_txid in zip(import_rpc_variants, spend_txids):
                balance = node.getbalance(label, 0, True)
                spend_tx = [tx for tx in txs if tx["txid"] == spend_txid]
                if import_node.rescan:
                    assert_equal(balance, amount)
                    assert_equal(len(spend_tx), 1)
                    assert_equal(spend_tx[0]["account"], "")
                    assert_equal(spend_tx[0]["amount"] + spend_tx[0]["fee"], -amount)
                    assert_equal(spend_tx[0]["category"], "send")
                    assert_equal("label" not in spend_tx[0], True)
                    assert_equal(spend_tx[0]["confirmations"], 1)
                    assert_equal("trusted" not in spend_tx[0], True)
                    assert_equal("involvesWatchonly" not in txs[0], True)
                else:
                    assert_equal(balance, 0)
                    assert_equal(spend_tx, [])


if __name__ == "__main__":
    ImportRescanTest().main()
