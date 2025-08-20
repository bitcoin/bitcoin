#!/usr/bin/env python3
# Copyright (c) 2016-2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test NULLDUMMY softfork.

Connect to a single node.
Generate 2 blocks (save the coinbases for later).
Generate COINBASE_MATURITY (CB) more blocks to ensure the coinbases are mature.
[Policy/Consensus] Check that NULLDUMMY compliant transactions are accepted in block CB + 3.
[Policy] Check that non-NULLDUMMY transactions are rejected before activation.
[Consensus] Check that the new NULLDUMMY rules are not enforced on block CB + 4.
[Policy/Consensus] Check that the new NULLDUMMY rules are enforced on block CB + 5.
"""

from test_framework.blocktools import (
    COINBASE_MATURITY,
    NORMAL_GBT_REQUEST_PARAMS,
    create_block,
)
from test_framework.messages import (
    CTransaction,
    tx_from_hex,
)
from test_framework.script import (
    OP_0,
    OP_TRUE,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import getnewdestination
from test_framework.key import ECKey
from test_framework.wallet_util import bytes_to_wif

NULLDUMMY_ERROR = "non-mandatory-script-verify-flag (Dummy CHECKMULTISIG argument must be zero)"


def invalidate_nulldummy_tx(tx):
    """Transform a NULLDUMMY compliant tx (i.e. scriptSig starts with OP_0)
    to be non-NULLDUMMY compliant by replacing the dummy with OP_TRUE"""
    assert_equal(tx.vin[0].scriptSig[0], OP_0)
    tx.vin[0].scriptSig = bytes([OP_TRUE]) + tx.vin[0].scriptSig[1:]
    tx.rehash()

class NULLDUMMYTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.extra_args = [[
            '-whitelist=127.0.0.1',
            '-dip3params=105:105',
            f'-testactivationheight=bip147@{COINBASE_MATURITY + 5}',
            '-par=1',  # Use only one script thread to get the exact reject reason for testing
        ]]

    def create_transaction(self, *, txid, input_details=None, addr, amount, privkey):
        input = {"txid": txid, "vout": 0}
        output = {addr: amount}
        rawtx = self.nodes[0].createrawtransaction([input], output)
        # Details only needed for scripthash or witness spends
        input = None if not input_details else [{**input, **input_details}]
        signedtx = self.nodes[0].signrawtransactionwithkey(rawtx, [privkey], input)
        return tx_from_hex(signedtx["hex"])

    def run_test(self):
        eckey = ECKey()
        eckey.generate()
        self.privkey = bytes_to_wif(eckey.get_bytes())
        self.pubkey = eckey.get_pubkey().get_bytes().hex()
        cms = self.nodes[0].createmultisig(1, [self.pubkey])
        self.ms_address = cms["address"]
        ms_unlock_details = {"scriptPubKey": self.nodes[0].validateaddress(self.ms_address)["scriptPubKey"],
                             "redeemScript": cms["redeemScript"]}

        self.coinbase_blocks = self.generate(self.nodes[0], 2)  # block height = 2
        coinbase_txid = []
        for i in self.coinbase_blocks:
            coinbase_txid.append(self.nodes[0].getblock(i)['tx'][0])
        self.generate(self.nodes[0], COINBASE_MATURITY)  # block height = COINBASE_MATURITY + 2
        self.lastblockhash = self.nodes[0].getbestblockhash()
        self.lastblockheight = COINBASE_MATURITY + 2
        self.lastblocktime = self.mocktime + self.lastblockheight

        self.log.info(f"Test 1: NULLDUMMY compliant base transactions should be accepted to mempool and mined before activation [{COINBASE_MATURITY + 3}]")
        test1txs = [self.create_transaction(txid=coinbase_txid[0], addr=self.ms_address, amount=49,
                                            privkey=self.nodes[0].get_deterministic_priv_key().key)]
        txid1 = self.nodes[0].sendrawtransaction(test1txs[0].serialize().hex(), 0)
        test1txs.append(self.create_transaction(txid=txid1, input_details=ms_unlock_details,
                                                addr=self.ms_address, amount=48,
                                                privkey=self.privkey))
        txid2 = self.nodes[0].sendrawtransaction(test1txs[1].serialize().hex(), 0)
        self.block_submit(self.nodes[0], test1txs, accept=True)

        self.log.info("Test 2: Non-NULLDUMMY base multisig transaction should not be accepted to mempool before activation")
        test2tx = self.create_transaction(txid=txid2, input_details=ms_unlock_details,
                                          addr=self.ms_address, amount=47,
                                          privkey=self.privkey)
        invalidate_nulldummy_tx(test2tx)
        assert_raises_rpc_error(-26, NULLDUMMY_ERROR, self.nodes[0].sendrawtransaction, test2tx.serialize().hex(), 0)

        self.log.info(f"Test 3: Non-NULLDUMMY base transactions should be accepted in a block before activation [{COINBASE_MATURITY + 4}]")
        self.block_submit(self.nodes[0], [test2tx], accept=True)

        self.log.info("Test 4: Non-NULLDUMMY base multisig transaction is invalid after activation")
        test4tx = self.create_transaction(txid=test2tx.hash, input_details=ms_unlock_details,
                                          addr=getnewdestination()[2], amount=46,
                                          privkey=self.privkey)
        test6txs = [CTransaction(test4tx)]
        invalidate_nulldummy_tx(test4tx)
        assert_raises_rpc_error(-26, NULLDUMMY_ERROR, self.nodes[0].sendrawtransaction, test4tx.serialize().hex(), 0)
        self.block_submit(self.nodes[0], [test4tx], accept=False)

        self.log.info(f"Test 6: NULLDUMMY compliant base/witness transactions should be accepted to mempool and in block after activation [{COINBASE_MATURITY + 5}]")
        for i in test6txs:
            self.nodes[0].sendrawtransaction(i.serialize().hex(), 0)
        self.block_submit(self.nodes[0], test6txs, accept=True)


    def block_submit(self, node, txs, *, accept=False):
        dip4_activated = self.lastblockheight + 1 >= COINBASE_MATURITY + 5
        tmpl = node.getblocktemplate(NORMAL_GBT_REQUEST_PARAMS)
        assert_equal(tmpl['previousblockhash'], self.lastblockhash)
        assert_equal(tmpl['height'], self.lastblockheight + 1)
        block = create_block(tmpl=tmpl, ntime=self.lastblocktime + 1, txlist=txs, dip4_activated=dip4_activated)
        block.solve()
        assert_equal(None if accept else NULLDUMMY_ERROR, node.submitblock(block.serialize().hex()))
        if accept:
            assert_equal(node.getbestblockhash(), block.hash)
            self.lastblockhash = block.hash
            self.lastblocktime += 1
            self.lastblockheight += 1
        else:
            assert_equal(node.getbestblockhash(), self.lastblockhash)


if __name__ == '__main__':
    NULLDUMMYTest().main()
