#!/usr/bin/env python3
# Copyright (c) 2016-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the bumpfee RPC.

Verifies that the bumpfee RPC creates replacement transactions successfully when
its preconditions are met, and returns appropriate errors in other cases.

This module consists of around a dozen individual test cases implemented in the
top-level functions named as test_<test_case_description>. The test functions
can be disabled or reordered if needed for debugging. If new test cases are
added in the future, they should try to follow the same convention and not
make assumptions about execution order.
"""
from decimal import Decimal
import io

from test_framework.blocktools import add_witness_commitment, create_block, create_coinbase, send_to_witness
from test_framework.messages import BIP125_SEQUENCE_NUMBER, CTransaction
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    assert_raises_rpc_error,
    connect_nodes,
    hex_str_to_bytes,
)

WALLET_PASSPHRASE = "test"
WALLET_PASSPHRASE_TIMEOUT = 3600

class BumpFeeTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True
        self.extra_args = [[
            "-walletrbf={}".format(i),
            "-mintxfee=0.00002",
            "-deprecatedrpc=totalFee",
        ] for i in range(self.num_nodes)]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # Encrypt wallet for test_locked_wallet_fails test
        self.nodes[1].encryptwallet(WALLET_PASSPHRASE)
        self.nodes[1].walletpassphrase(WALLET_PASSPHRASE, WALLET_PASSPHRASE_TIMEOUT)

        connect_nodes(self.nodes[0], 1)
        self.sync_all()

        peer_node, rbf_node = self.nodes
        rbf_node_address = rbf_node.getnewaddress()

        # fund rbf node with 10 coins of 0.001 btc (100,000 satoshis)
        self.log.info("Mining blocks...")
        peer_node.generate(110)
        self.sync_all()
        for i in range(25):
            peer_node.sendtoaddress(rbf_node_address, 0.001)
        self.sync_all()
        peer_node.generate(1)
        self.sync_all()
        assert_equal(rbf_node.getbalance(), Decimal("0.025"))

        self.log.info("Running tests")
        dest_address = peer_node.getnewaddress()
        test_simple_bumpfee_succeeds(self, "default", rbf_node, peer_node, dest_address)
        test_simple_bumpfee_succeeds(self, "fee_rate", rbf_node, peer_node, dest_address)
        test_feerate_args(self, rbf_node, peer_node, dest_address)
        test_segwit_bumpfee_succeeds(rbf_node, dest_address)
        test_nonrbf_bumpfee_fails(peer_node, dest_address)
        test_notmine_bumpfee_fails(rbf_node, peer_node, dest_address)
        test_bumpfee_with_descendant_fails(rbf_node, rbf_node_address, dest_address)
        test_small_output_fails(rbf_node, dest_address)
        test_dust_to_fee(rbf_node, dest_address)
        test_settxfee(rbf_node, dest_address)
        test_rebumping(rbf_node, dest_address)
        test_rebumping_not_replaceable(rbf_node, dest_address)
        test_unconfirmed_not_spendable(rbf_node, rbf_node_address)
        test_bumpfee_metadata(rbf_node, dest_address)
        test_locked_wallet_fails(rbf_node, dest_address)
        test_change_script_match(rbf_node, dest_address)
        test_maxtxfee_fails(self, rbf_node, dest_address)
        # These tests wipe out a number of utxos that are expected in other tests
        test_small_output_with_feerate_succeeds(rbf_node, dest_address)
        test_no_more_inputs_fails(rbf_node, dest_address)
        self.log.info("Success")


def test_simple_bumpfee_succeeds(self, mode, rbf_node, peer_node, dest_address):
    rbfid = spend_one_input(rbf_node, dest_address)
    rbftx = rbf_node.gettransaction(rbfid)
    self.sync_mempools((rbf_node, peer_node))
    assert rbfid in rbf_node.getrawmempool() and rbfid in peer_node.getrawmempool()
    if mode == "fee_rate":
        bumped_tx = rbf_node.bumpfee(rbfid, {"fee_rate":0.0015})
    else:
        bumped_tx = rbf_node.bumpfee(rbfid)
    assert_equal(bumped_tx["errors"], [])
    assert bumped_tx["fee"] > -rbftx["fee"]
    assert_equal(bumped_tx["origfee"], -rbftx["fee"])
    # check that bumped_tx propagates, original tx was evicted and has a wallet conflict
    self.sync_mempools((rbf_node, peer_node))
    assert bumped_tx["txid"] in rbf_node.getrawmempool()
    assert bumped_tx["txid"] in peer_node.getrawmempool()
    assert rbfid not in rbf_node.getrawmempool()
    assert rbfid not in peer_node.getrawmempool()
    oldwtx = rbf_node.gettransaction(rbfid)
    assert len(oldwtx["walletconflicts"]) > 0
    # check wallet transaction replaces and replaced_by values
    bumpedwtx = rbf_node.gettransaction(bumped_tx["txid"])
    assert_equal(oldwtx["replaced_by_txid"], bumped_tx["txid"])
    assert_equal(bumpedwtx["replaces_txid"], rbfid)

def test_feerate_args(self, rbf_node, peer_node, dest_address):
    rbfid = spend_one_input(rbf_node, dest_address)
    self.sync_mempools((rbf_node, peer_node))
    assert rbfid in rbf_node.getrawmempool() and rbfid in peer_node.getrawmempool()

    assert_raises_rpc_error(-8, "confTarget can't be set with totalFee or fee_rate. Please provide either a confirmation target in blocks for automatic fee estimation, or an explicit fee rate.", rbf_node.bumpfee, rbfid, {"fee_rate":0.00001, "confTarget":1})
    assert_raises_rpc_error(-8, "confTarget can't be set with totalFee or fee_rate. Please provide either a confirmation target in blocks for automatic fee estimation, or an explicit fee rate.", rbf_node.bumpfee, rbfid, {"totalFee":0.00001, "confTarget":1})
    assert_raises_rpc_error(-8, "fee_rate can't be set along with totalFee.", rbf_node.bumpfee, rbfid, {"fee_rate":0.00001, "totalFee":0.001})

    # Bumping to just above minrelay should fail to increase total fee enough, at least
    assert_raises_rpc_error(-8, "Insufficient total fee", rbf_node.bumpfee, rbfid, {"fee_rate":0.00001000})

    assert_raises_rpc_error(-3, "Amount out of range", rbf_node.bumpfee, rbfid, {"fee_rate":-1})

    assert_raises_rpc_error(-4, "is too high (cannot be higher than", rbf_node.bumpfee, rbfid, {"fee_rate":1})


def test_segwit_bumpfee_succeeds(rbf_node, dest_address):
    # Create a transaction with segwit output, then create an RBF transaction
    # which spends it, and make sure bumpfee can be called on it.

    segwit_in = next(u for u in rbf_node.listunspent() if u["amount"] == Decimal("0.001"))
    segwit_out = rbf_node.getaddressinfo(rbf_node.getnewaddress(address_type='p2sh-segwit'))
    segwitid = send_to_witness(
        use_p2wsh=False,
        node=rbf_node,
        utxo=segwit_in,
        pubkey=segwit_out["pubkey"],
        encode_p2sh=False,
        amount=Decimal("0.0009"),
        sign=True)

    rbfraw = rbf_node.createrawtransaction([{
        'txid': segwitid,
        'vout': 0,
        "sequence": BIP125_SEQUENCE_NUMBER
    }], {dest_address: Decimal("0.0005"),
         rbf_node.getrawchangeaddress(): Decimal("0.0003")})
    rbfsigned = rbf_node.signrawtransactionwithwallet(rbfraw)
    rbfid = rbf_node.sendrawtransaction(rbfsigned["hex"])
    assert rbfid in rbf_node.getrawmempool()

    bumped_tx = rbf_node.bumpfee(rbfid)
    assert bumped_tx["txid"] in rbf_node.getrawmempool()
    assert rbfid not in rbf_node.getrawmempool()


def test_nonrbf_bumpfee_fails(peer_node, dest_address):
    # cannot replace a non RBF transaction (from node which did not enable RBF)
    not_rbfid = peer_node.sendtoaddress(dest_address, Decimal("0.00090000"))
    assert_raises_rpc_error(-4, "not BIP 125 replaceable", peer_node.bumpfee, not_rbfid)


def test_notmine_bumpfee_fails(rbf_node, peer_node, dest_address):
    # cannot bump fee unless the tx has only inputs that we own.
    # here, the rbftx has a peer_node coin and then adds a rbf_node input
    # Note that this test depends upon the RPC code checking input ownership prior to change outputs
    # (since it can't use fundrawtransaction, it lacks a proper change output)
    utxos = [node.listunspent()[-1] for node in (rbf_node, peer_node)]
    inputs = [{
        "txid": utxo["txid"],
        "vout": utxo["vout"],
        "address": utxo["address"],
        "sequence": BIP125_SEQUENCE_NUMBER
    } for utxo in utxos]
    output_val = sum(utxo["amount"] for utxo in utxos) - Decimal("0.001")
    rawtx = rbf_node.createrawtransaction(inputs, {dest_address: output_val})
    signedtx = rbf_node.signrawtransactionwithwallet(rawtx)
    signedtx = peer_node.signrawtransactionwithwallet(signedtx["hex"])
    rbfid = rbf_node.sendrawtransaction(signedtx["hex"])
    assert_raises_rpc_error(-4, "Transaction contains inputs that don't belong to this wallet",
                            rbf_node.bumpfee, rbfid)


def test_bumpfee_with_descendant_fails(rbf_node, rbf_node_address, dest_address):
    # cannot bump fee if the transaction has a descendant
    # parent is send-to-self, so we don't have to check which output is change when creating the child tx
    parent_id = spend_one_input(rbf_node, rbf_node_address)
    tx = rbf_node.createrawtransaction([{"txid": parent_id, "vout": 0}], {dest_address: 0.00020000})
    tx = rbf_node.signrawtransactionwithwallet(tx)
    rbf_node.sendrawtransaction(tx["hex"])
    assert_raises_rpc_error(-8, "Transaction has descendants in the wallet", rbf_node.bumpfee, parent_id)

def test_small_output_fails(rbf_node, dest_address):
    # cannot bump fee with a too-small output
    rbfid = spend_one_input(rbf_node, dest_address)
    rbf_node.bumpfee(rbfid, {"totalFee": 50000})

    rbfid = spend_one_input(rbf_node, dest_address)
    assert_raises_rpc_error(-4, "Change output is too small", rbf_node.bumpfee, rbfid, {"totalFee": 50001})

def test_small_output_with_feerate_succeeds(rbf_node, dest_address):

    # Make sure additional inputs exist
    rbf_node.generatetoaddress(101, rbf_node.getnewaddress())
    rbfid = spend_one_input(rbf_node, dest_address)
    original_input_list = rbf_node.getrawtransaction(rbfid, 1)["vin"]
    assert_equal(len(original_input_list), 1)
    original_txin = original_input_list[0]
    # Keep bumping until we out-spend change output
    tx_fee = 0
    while tx_fee < Decimal("0.0005"):
        new_input_list = rbf_node.getrawtransaction(rbfid, 1)["vin"]
        new_item = list(new_input_list)[0]
        assert_equal(len(original_input_list), 1)
        assert_equal(original_txin["txid"], new_item["txid"])
        assert_equal(original_txin["vout"], new_item["vout"])
        rbfid_new_details = rbf_node.bumpfee(rbfid)
        rbfid_new = rbfid_new_details["txid"]
        raw_pool = rbf_node.getrawmempool()
        assert rbfid not in raw_pool
        assert rbfid_new in raw_pool
        rbfid = rbfid_new
        tx_fee = rbfid_new_details["origfee"]

    # input(s) have been added
    final_input_list = rbf_node.getrawtransaction(rbfid, 1)["vin"]
    assert_greater_than(len(final_input_list), 1)
    # Original input is in final set
    assert [txin for txin in final_input_list
            if txin["txid"] == original_txin["txid"]
            and txin["vout"] == original_txin["vout"]]

    rbf_node.generatetoaddress(1, rbf_node.getnewaddress())
    assert_equal(rbf_node.gettransaction(rbfid)["confirmations"], 1)

def test_dust_to_fee(rbf_node, dest_address):
    # check that if output is reduced to dust, it will be converted to fee
    # the bumped tx sets fee=49,900, but it converts to 50,000
    rbfid = spend_one_input(rbf_node, dest_address)
    fulltx = rbf_node.getrawtransaction(rbfid, 1)
    # (32-byte p2sh-pwpkh output size + 148 p2pkh spend estimate) * 10k(discard_rate) / 1000 = 1800
    # P2SH outputs are slightly "over-discarding" due to the IsDust calculation assuming it will
    # be spent as a P2PKH.
    bumped_tx = rbf_node.bumpfee(rbfid, {"totalFee": 50000 - 1800})
    full_bumped_tx = rbf_node.getrawtransaction(bumped_tx["txid"], 1)
    assert_equal(bumped_tx["fee"], Decimal("0.00050000"))
    assert_equal(len(fulltx["vout"]), 2)
    assert_equal(len(full_bumped_tx["vout"]), 1)  # change output is eliminated


def test_settxfee(rbf_node, dest_address):
    assert_raises_rpc_error(-8, "txfee cannot be less than min relay tx fee", rbf_node.settxfee, Decimal('0.000005'))
    assert_raises_rpc_error(-8, "txfee cannot be less than wallet min fee", rbf_node.settxfee, Decimal('0.000015'))
    # check that bumpfee reacts correctly to the use of settxfee (paytxfee)
    rbfid = spend_one_input(rbf_node, dest_address)
    requested_feerate = Decimal("0.00025000")
    rbf_node.settxfee(requested_feerate)
    bumped_tx = rbf_node.bumpfee(rbfid)
    actual_feerate = bumped_tx["fee"] * 1000 / rbf_node.getrawtransaction(bumped_tx["txid"], True)["vsize"]
    # Assert that the difference between the requested feerate and the actual
    # feerate of the bumped transaction is small.
    assert_greater_than(Decimal("0.00001000"), abs(requested_feerate - actual_feerate))
    rbf_node.settxfee(Decimal("0.00000000"))  # unset paytxfee


def test_maxtxfee_fails(test, rbf_node, dest_address):
    test.restart_node(1, ['-maxtxfee=0.00003'] + test.extra_args[1])
    rbf_node.walletpassphrase(WALLET_PASSPHRASE, WALLET_PASSPHRASE_TIMEOUT)
    rbfid = spend_one_input(rbf_node, dest_address)
    assert_raises_rpc_error(-4, "Unable to create transaction: Fee exceeds maximum configured by -maxtxfee", rbf_node.bumpfee, rbfid)
    test.restart_node(1, test.extra_args[1])
    rbf_node.walletpassphrase(WALLET_PASSPHRASE, WALLET_PASSPHRASE_TIMEOUT)


def test_rebumping(rbf_node, dest_address):
    # check that re-bumping the original tx fails, but bumping the bumper succeeds
    rbfid = spend_one_input(rbf_node, dest_address)
    bumped = rbf_node.bumpfee(rbfid, {"totalFee": 2000})
    assert_raises_rpc_error(-4, "already bumped", rbf_node.bumpfee, rbfid, {"totalFee": 3000})
    rbf_node.bumpfee(bumped["txid"], {"totalFee": 3000})


def test_rebumping_not_replaceable(rbf_node, dest_address):
    # check that re-bumping a non-replaceable bump tx fails
    rbfid = spend_one_input(rbf_node, dest_address)
    bumped = rbf_node.bumpfee(rbfid, {"totalFee": 10000, "replaceable": False})
    assert_raises_rpc_error(-4, "Transaction is not BIP 125 replaceable", rbf_node.bumpfee, bumped["txid"],
                            {"totalFee": 20000})


def test_unconfirmed_not_spendable(rbf_node, rbf_node_address):
    # check that unconfirmed outputs from bumped transactions are not spendable
    rbfid = spend_one_input(rbf_node, rbf_node_address)
    rbftx = rbf_node.gettransaction(rbfid)["hex"]
    assert rbfid in rbf_node.getrawmempool()
    bumpid = rbf_node.bumpfee(rbfid)["txid"]
    assert bumpid in rbf_node.getrawmempool()
    assert rbfid not in rbf_node.getrawmempool()

    # check that outputs from the bump transaction are not spendable
    # due to the replaces_txid check in CWallet::AvailableCoins
    assert_equal([t for t in rbf_node.listunspent(minconf=0, include_unsafe=False) if t["txid"] == bumpid], [])

    # submit a block with the rbf tx to clear the bump tx out of the mempool,
    # then invalidate the block so the rbf tx will be put back in the mempool.
    # This makes it possible to check whether the rbf tx outputs are
    # spendable before the rbf tx is confirmed.
    block = submit_block_with_tx(rbf_node, rbftx)
    # Can not abandon conflicted tx
    assert_raises_rpc_error(-5, 'Transaction not eligible for abandonment', lambda: rbf_node.abandontransaction(txid=bumpid))
    rbf_node.invalidateblock(block.hash)
    # Call abandon to make sure the wallet doesn't attempt to resubmit
    # the bump tx and hope the wallet does not rebroadcast before we call.
    rbf_node.abandontransaction(bumpid)
    assert bumpid not in rbf_node.getrawmempool()
    assert rbfid in rbf_node.getrawmempool()

    # check that outputs from the rbf tx are not spendable before the
    # transaction is confirmed, due to the replaced_by_txid check in
    # CWallet::AvailableCoins
    assert_equal([t for t in rbf_node.listunspent(minconf=0, include_unsafe=False) if t["txid"] == rbfid], [])

    # check that the main output from the rbf tx is spendable after confirmed
    rbf_node.generate(1)
    assert_equal(
        sum(1 for t in rbf_node.listunspent(minconf=0, include_unsafe=False)
            if t["txid"] == rbfid and t["address"] == rbf_node_address and t["spendable"]), 1)


def test_bumpfee_metadata(rbf_node, dest_address):
    assert(rbf_node.getbalance() < 49)
    rbf_node.generatetoaddress(101, rbf_node.getnewaddress())
    rbfid = rbf_node.sendtoaddress(dest_address, 49, "comment value", "to value")
    bumped_tx = rbf_node.bumpfee(rbfid)
    bumped_wtx = rbf_node.gettransaction(bumped_tx["txid"])
    assert_equal(bumped_wtx["comment"], "comment value")
    assert_equal(bumped_wtx["to"], "to value")


def test_locked_wallet_fails(rbf_node, dest_address):
    rbfid = spend_one_input(rbf_node, dest_address)
    rbf_node.walletlock()
    assert_raises_rpc_error(-13, "Please enter the wallet passphrase with walletpassphrase first.",
                            rbf_node.bumpfee, rbfid)
    rbf_node.walletpassphrase(WALLET_PASSPHRASE, WALLET_PASSPHRASE_TIMEOUT)

def test_change_script_match(rbf_node, dest_address):
    """Test that the same change addresses is used for the replacement transaction when possible."""
    def get_change_address(tx):
        tx_details = rbf_node.getrawtransaction(tx, 1)
        txout_addresses = [txout['scriptPubKey']['addresses'][0] for txout in tx_details["vout"]]
        return [address for address in txout_addresses if rbf_node.getaddressinfo(address)["ischange"]]

    # Check that there is only one change output
    rbfid = spend_one_input(rbf_node, dest_address)
    change_addresses = get_change_address(rbfid)
    assert_equal(len(change_addresses), 1)

    # Now find that address in each subsequent tx, and no other change
    bumped_total_tx = rbf_node.bumpfee(rbfid, {"totalFee": 2000})
    assert_equal(change_addresses, get_change_address(bumped_total_tx['txid']))
    bumped_rate_tx = rbf_node.bumpfee(bumped_total_tx["txid"])
    assert_equal(change_addresses, get_change_address(bumped_rate_tx['txid']))

def spend_one_input(node, dest_address, change_size=Decimal("0.00049000")):
    tx_input = dict(
        sequence=BIP125_SEQUENCE_NUMBER, **next(u for u in node.listunspent() if u["amount"] == Decimal("0.00100000")))
    destinations = {dest_address: Decimal("0.00050000")}
    if change_size > 0:
        destinations[node.getrawchangeaddress()] = change_size
    rawtx = node.createrawtransaction([tx_input], destinations)
    signedtx = node.signrawtransactionwithwallet(rawtx)
    txid = node.sendrawtransaction(signedtx["hex"])
    return txid

def submit_block_with_tx(node, tx):
    ctx = CTransaction()
    ctx.deserialize(io.BytesIO(hex_str_to_bytes(tx)))

    tip = node.getbestblockhash()
    height = node.getblockcount() + 1
    block_time = node.getblockheader(tip)["mediantime"] + 1
    block = create_block(int(tip, 16), create_coinbase(height), block_time)
    block.vtx.append(ctx)
    block.rehash()
    block.hashMerkleRoot = block.calc_merkle_root()
    add_witness_commitment(block)
    block.solve()
    node.submitblock(block.serialize().hex())
    return block

def test_no_more_inputs_fails(rbf_node, dest_address):
    # feerate rbf requires confirmed outputs when change output doesn't exist or is insufficient
    rbf_node.generatetoaddress(1, dest_address)
    # spend all funds, no change output
    rbfid = rbf_node.sendtoaddress(rbf_node.getnewaddress(), rbf_node.getbalance(), "", "", True)
    assert_raises_rpc_error(-4, "Unable to create transaction: Insufficient funds", rbf_node.bumpfee, rbfid)

if __name__ == "__main__":
    BumpFeeTest().main()
