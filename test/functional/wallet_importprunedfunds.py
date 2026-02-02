#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the importprunedfunds and removeprunedfunds RPCs."""
from decimal import Decimal

from test_framework.address import key_to_p2wpkh
from test_framework.blocktools import COINBASE_MATURITY
from test_framework.messages import (
    CMerkleBlock,
    from_hex,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises_rpc_error,
    wallet_importprivkey,
)
from test_framework.wallet_util import generate_keypair


class ImportPrunedFundsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Mining blocks...")
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)

        # address
        address1 = self.nodes[0].getnewaddress()
        # pubkey
        address2 = self.nodes[0].getnewaddress()
        # privkey
        address3_privkey, address3_pubkey = generate_keypair(wif=True)
        address3 = key_to_p2wpkh(address3_pubkey)
        wallet_importprivkey(self.nodes[0], address3_privkey, "now")

        # Check only one address
        address_info = self.nodes[0].getaddressinfo(address1)
        assert_equal(address_info['ismine'], True)

        self.sync_all()

        # Node 1 sync test
        assert_equal(self.nodes[1].getblockcount(), COINBASE_MATURITY + 1)

        # Address Test - before import
        address_info = self.nodes[1].getaddressinfo(address1)
        assert_equal(address_info['ismine'], False)

        address_info = self.nodes[1].getaddressinfo(address2)
        assert_equal(address_info['ismine'], False)

        address_info = self.nodes[1].getaddressinfo(address3)
        assert_equal(address_info['ismine'], False)

        # Send funds to self
        txnid1 = self.nodes[0].sendtoaddress(address1, 0.1)
        self.generate(self.nodes[0], 1)
        rawtxn1 = self.nodes[0].gettransaction(txnid1)['hex']
        proof1 = self.nodes[0].gettxoutproof([txnid1])

        txnid2 = self.nodes[0].sendtoaddress(address2, 0.05)
        self.generate(self.nodes[0], 1)
        rawtxn2 = self.nodes[0].gettransaction(txnid2)['hex']
        proof2 = self.nodes[0].gettxoutproof([txnid2])

        txnid3 = self.nodes[0].sendtoaddress(address3, 0.025)
        self.generate(self.nodes[0], 1)
        rawtxn3 = self.nodes[0].gettransaction(txnid3)['hex']
        proof3 = self.nodes[0].gettxoutproof([txnid3])

        self.sync_all()

        # Import with no affiliated address
        assert_raises_rpc_error(-5, "No addresses", self.nodes[1].importprunedfunds, rawtxn1, proof1)

        balance1 = self.nodes[1].getbalance()
        assert_equal(balance1, Decimal(0))

        # Import with affiliated address with no rescan
        self.nodes[1].createwallet('wwatch', disable_private_keys=True)
        wwatch = self.nodes[1].get_wallet_rpc('wwatch')
        wwatch.importdescriptors([{"desc": self.nodes[0].getaddressinfo(address2)["desc"], "timestamp": "now"}])
        wwatch.importprunedfunds(rawtransaction=rawtxn2, txoutproof=proof2)
        assert [tx for tx in wwatch.listtransactions() if tx['txid'] == txnid2]

        # Import with private key with no rescan
        w1 = self.nodes[1].get_wallet_rpc(self.default_wallet_name)
        wallet_importprivkey(w1, address3_privkey, "now")
        w1.importprunedfunds(rawtxn3, proof3)
        assert [tx for tx in w1.listtransactions() if tx['txid'] == txnid3]
        balance3 = w1.getbalance()
        assert_equal(balance3, Decimal('0.025'))

        # Addresses Test - after import
        address_info = w1.getaddressinfo(address1)
        assert_equal(address_info['ismine'], False)
        address_info = wwatch.getaddressinfo(address2)
        assert_equal(address_info['ismine'], True)
        address_info = w1.getaddressinfo(address3)
        assert_equal(address_info['ismine'], True)

        # Remove transactions
        assert_raises_rpc_error(-4, f'Transaction {txnid1} does not belong to this wallet', w1.removeprunedfunds, txnid1)
        assert not [tx for tx in w1.listtransactions() if tx['txid'] == txnid1]

        wwatch.removeprunedfunds(txnid2)
        assert not [tx for tx in wwatch.listtransactions() if tx['txid'] == txnid2]

        w1.removeprunedfunds(txnid3)
        assert not [tx for tx in w1.listtransactions() if tx['txid'] == txnid3]

        # Check various RPC parameter validation errors
        assert_raises_rpc_error(-22, "TX decode failed", w1.importprunedfunds, b'invalid tx'.hex(), proof1)
        assert_raises_rpc_error(-5, "Transaction given doesn't exist in proof", w1.importprunedfunds, rawtxn2, proof1)

        mb = from_hex(CMerkleBlock(), proof1)
        mb.header.hashMerkleRoot = 0xdeadbeef  # cause mismatch between merkle root and merkle block
        assert_raises_rpc_error(-5, "Something wrong with merkleblock", w1.importprunedfunds, rawtxn1, mb.serialize().hex())

        mb = from_hex(CMerkleBlock(), proof1)
        mb.header.nTime += 1  # modify arbitrary block header field to change block hash
        assert_raises_rpc_error(-5, "Block not found in chain", w1.importprunedfunds, rawtxn1, mb.serialize().hex())

        self.log.info("Test removeprunedfunds with conflicting transactions")
        node = self.nodes[0]

        # Create a transaction
        utxo = node.listunspent()[0]
        addr = node.getnewaddress()
        tx1_id = node.send(outputs=[{addr: 1}], inputs=[utxo])["txid"]
        tx1_fee = node.gettransaction(tx1_id)["fee"]

        # Create a conflicting tx with a larger fee (tx1_fee is negative)
        output_value = utxo["amount"] + tx1_fee - Decimal("0.00001")
        raw_tx2 = node.createrawtransaction(inputs=[utxo], outputs=[{addr: output_value}])
        signed_tx2 = node.signrawtransactionwithwallet(raw_tx2)
        tx2_id = node.sendrawtransaction(signed_tx2["hex"])
        assert_not_equal(tx2_id, tx1_id)

        # Both txs should be in the wallet, tx2 replaced tx1 in mempool
        assert tx1_id in [tx["txid"] for tx in node.listtransactions()]
        assert tx2_id in [tx["txid"] for tx in node.listtransactions()]

        # Remove the replaced tx from wallet
        node.removeprunedfunds(tx1_id)

        # The UTXO should still be considered spent (by tx2)
        available_utxos = [u["txid"] for u in node.listunspent(minconf=0)]
        assert utxo["txid"] not in available_utxos, "UTXO should still be spent by conflicting tx"

        self.log.info("Test importing a spending transaction where no outputs are mine")
        # Fund a specific address
        spend_addr = node.getnewaddress()
        node.sendtoaddress(spend_addr, 0.5)
        self.generate(node, 1)
        
        # Create a tx that spends from spend_addr to an external address (node 1)
        # Use subtractfeefromamount to ensure no change output
        w1 = self.nodes[1].get_wallet_rpc(self.default_wallet_name)
        external_addr = w1.getnewaddress()
        
        # Let's create a new wallet for this test to isolate inputs
        node.createwallet("spender")
        w_spender = node.get_wallet_rpc("spender")
        
        # Just use the first wallet found that isn't 'spender'
        default_wallet_name = [w for w in node.listwallets() if w != "spender"][0]
        w_default = node.get_wallet_rpc(default_wallet_name)
        
        privkey_s, pubkey_s = generate_keypair(wif=True)
        spend_addr_s = key_to_p2wpkh(pubkey_s)
        
        # Import to default wallet
        wallet_importprivkey(w_default, privkey_s, "now")
        
        # Fund it
        w_default.sendtoaddress(spend_addr_s, 0.5)
        self.generate(node, 1)
        
        # Import to spender wallet
        wallet_importprivkey(w_spender, privkey_s, "now")
        
        # Now w_spender has the key and balance (0.5)
        # Send ALL funds to external address
        tx_spend_id = w_spender.sendtoaddress(address=external_addr, amount=0.5, subtractfeefromamount=True)
        self.generate(node, 1)
        
        # Get proofs
        tx_spend_hex = w_spender.gettransaction(tx_spend_id)['hex']
        tx_spend_proof = w_spender.gettxoutproof([tx_spend_id])
        
        # Remove the spending transaction
        w_spender.removeprunedfunds(tx_spend_id)
        
        # Verify it's gone
        assert tx_spend_id not in [tx['txid'] for tx in w_spender.listtransactions()]
        
        # Try to import it back. 
        w_spender.importprunedfunds(tx_spend_hex, tx_spend_proof)
        
        # Verify it is back
        assert tx_spend_id in [tx['txid'] for tx in w_spender.listtransactions()]


if __name__ == '__main__':
    ImportPrunedFundsTest(__file__).main()
