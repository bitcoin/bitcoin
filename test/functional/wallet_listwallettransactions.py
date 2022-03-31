#!/usr/bin/env python3
"""Test the listwallettransactions API for MWEB transactions."""
from decimal import Decimal
from io import BytesIO

from test_framework.messages import COIN, CTransaction
from test_framework.test_framework import BitcoinTestFramework
from test_framework.ltc_util import setup_mweb_chain
from test_framework.util import (
    assert_array_result,
    assert_equal,
    hex_str_to_bytes,
)

def tx_from_hex(hexstring):
    tx = CTransaction()
    f = BytesIO(hex_str_to_bytes(hexstring))
    tx.deserialize(f)
    return tx

class ListWalletTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3
        self.extra_args = [['-whitelist=noban@127.0.0.1'],['-whitelist=noban@127.0.0.1'],[]]  # immediate tx relay
        
    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node0 = self.nodes[0]
        node1 = self.nodes[1]
        node2 = self.nodes[2]

        # Setup MWEB chain using node0 as miner
        setup_mweb_chain(node0)
        self.sync_all()

        # Simple send, 0 to 1:
        txid = node0.sendtoaddress(node1.getnewaddress(), 0.1)
        self.sync_all()
        assert_array_result(node0.listwallettransactions(txid),
                            {"txid": txid},
                            {"type": "SendToAddress", "amount": Decimal("-0.1"), "confirmations": 0})
        assert_array_result(node1.listwallettransactions(),
                            {"txid": txid},
                            {"type": "RecvWithAddress", "amount": Decimal("0.1"), "confirmations": 0})

        # mine a block, confirmations should change:
        blockhash = node0.generate(1)[0]
        blockheight = node0.getblockheader(blockhash)['height']
        self.sync_all()
        assert_array_result(node0.listwallettransactions(),
                            {"txid": txid},
                            {"type": "SendToAddress", "amount": Decimal("-0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})
        assert_array_result(node1.listwallettransactions(),
                            {"txid": txid},
                            {"type": "RecvWithAddress", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        # send-to-self:
        txid = node0.sendtoaddress(node0.getnewaddress(), 0.2)
        assert_array_result(node0.listwallettransactions(),
                            {"txid": txid, "type": "SendToSelf"},
                            {"amount": Decimal("0.2")})

        # pegin to self
        node0_mweb_addr = node0.getnewaddress(address_type='mweb') 
        txid = node0.sendtoaddress(node0_mweb_addr, 0.4)
        assert_array_result(node0.listwallettransactions(),
                            {"txid": txid, "type": "SendToSelf"},
                            {"amount": Decimal("0.4")})
        
        # mine a block, confirmations should change:
        blockhash = node0.generate(1)[0]
        blockheight = node0.getblockheader(blockhash)['height']
        self.sync_all()

        assert_array_result(node0.listwallettransactions(),
                            {"txid": txid, "type": "SendToSelf"},
                            {"amount": Decimal("0.4"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        # pegin from 0 to 1
        node1_mweb_addr = node1.getnewaddress(address_type='mweb')
        txid = node0.sendtoaddress(node1_mweb_addr, 5)
        self.sync_all()
        assert_array_result(node0.listwallettransactions(txid),
                            {"txid": txid},
                            {"type": "SendToAddress", "amount": Decimal("-5.0"), "confirmations": 0, "address": node1_mweb_addr})
        assert_array_result(node1.listwallettransactions(),
                            {"address": node1_mweb_addr},
                            {"type": "RecvWithAddress", "amount": Decimal("5.0"), "confirmations": 0})
        
        # mine a block, confirmations should change:
        blockhash = node0.generate(1)[0]
        blockheight = node0.getblockheader(blockhash)['height']
        self.sync_all()
        assert_array_result(node0.listwallettransactions(txid),
                            {"txid": txid},
                            {"type": "SendToAddress", "amount": Decimal("-5.0"), "confirmations": 1, "blockheight": blockheight, "address": node1_mweb_addr})
        assert_array_result(node1.listwallettransactions(),
                            {"address": node1_mweb_addr},
                            {"type": "RecvWithAddress", "amount": Decimal("5.0"), "confirmations": 1, "blockheight": blockheight})

        
        # pegout from 1 to 0 (node0 online)
        node0_addr = node0.getnewaddress()
        txid = node1.sendtoaddress(node0_addr, 3)
        self.sync_all()
        assert_array_result(node1.listwallettransactions(),
                            {"address": node0_addr},
                            {"txid": txid, "type": "SendToAddress", "amount": Decimal("-3.0"), "confirmations": 0})
        assert_array_result(node0.listwallettransactions(),
                            {"address": node0_addr},
                            {"txid": txid, "type": "RecvWithAddress", "amount": Decimal("3.0"), "confirmations": 0})

        # mine a block, confirmations should change:
        blockhash = node0.generate(1)[0]
        blockheight = node0.getblockheader(blockhash)['height']
        self.sync_all()
        assert_array_result(node1.listwallettransactions(),
                            {"address": node0_addr},
                            {"txid": txid, "type": "SendToAddress", "amount": Decimal("-3.0"), "confirmations": 1, "blockheight": blockheight})
        assert_array_result(node0.listwallettransactions(),
                            {"address": node0_addr},
                            {"type": "RecvWithAddress", "amount": Decimal("3.0"), "confirmations": 1, "blockheight": blockheight})

        # pegout from 1 to 2 and mine a block while node2 is offline
        # node2 won't see the original MWEB transaction; just the pegout kernel and hogex output.
        node2_addr = node2.getnewaddress()
        self.stop_node(2)
        txid = node1.sendtoaddress(node2_addr, 1)
        blockhash = node1.generate(1)[0]
        blockheight = node1.getblockheader(blockhash)['height']
        hogex_txid = node1.getblock(blockhash)['tx'][-1]
        self.start_node(2)
        self.connect_nodes(2, 0)
        self.connect_nodes(2, 1)
        self.sync_all()

        assert_array_result(node1.listwallettransactions(),
                            {"address": node2_addr},
                            {"txid": txid, "type": "SendToAddress", "amount": Decimal("-1.0"), "confirmations": 1, "blockheight": blockheight})
        assert_array_result(node2.listwallettransactions(),
                            {"address": node2_addr},
                            {"txid": hogex_txid, "type": "RecvWithAddress", "amount": Decimal("1.0"), "confirmations": 1, "blockheight": blockheight})

        # TODO: Reorg and ensure hogex is marked as not accepted

if __name__ == '__main__':
    ListWalletTransactionsTest().main()
