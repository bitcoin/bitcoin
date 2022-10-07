#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Litecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Random assortment of utility functions"""

import os

from test_framework.messages import COIN, COutPoint, CTransaction, CTxIn, CTxOut, FromHex, MWEBHeader
from test_framework.util import get_datadir_path, initialize_datadir, satoshi_round
from test_framework.script_util import DUMMY_P2WPKH_SCRIPT, hogaddr_script
from test_framework.test_node import TestNode

FIRST_MWEB_HEIGHT = 432 # Height of the first block to contain an MWEB if using default regtest params


"""Create a txout with a given amount and scriptPubKey

Mines coins as needed.

confirmed - txouts created will be confirmed in the blockchain;
            unconfirmed otherwise.

Returns the 'COutPoint' of the UTXO
"""
def make_utxo(node, amount, confirmed=True, scriptPubKey=DUMMY_P2WPKH_SCRIPT):
    fee = 1*COIN
    while node.getbalance() < satoshi_round((amount + fee)/COIN):
        node.generate(100)

    new_addr = node.getnewaddress()
    txid = node.sendtoaddress(new_addr, satoshi_round((amount+fee)/COIN))
    tx1 = node.getrawtransaction(txid, 1)
    txid = int(txid, 16)
    i = None

    for i, txout in enumerate(tx1['vout']):
        if txout['scriptPubKey']['addresses'] == [new_addr]:
            break
    assert i is not None

    tx2 = CTransaction()
    tx2.vin = [CTxIn(COutPoint(txid, i))]
    tx2.vout = [CTxOut(amount, scriptPubKey)]
    tx2.rehash()

    signed_tx = node.signrawtransactionwithwallet(tx2.serialize().hex())

    txid = node.sendrawtransaction(signed_tx['hex'], 0)

    # If requested, ensure txouts are confirmed.
    if confirmed:
        mempool_size = len(node.getrawmempool())
        while mempool_size > 0:
            node.generate(1)
            new_size = len(node.getrawmempool())
            # Error out if we have something stuck in the mempool, as this
            # would likely be a bug.
            assert new_size < mempool_size
            mempool_size = new_size

    return COutPoint(int(txid, 16), 0)


"""Generates all pre-MWEB blocks, pegs 1 coin into the MWEB,
then mines the first MWEB block which includes that pegin.

mining_node - The node to use to generate blocks
"""
def setup_mweb_chain(mining_node):
    # Create all pre-MWEB blocks
    mining_node.generate(FIRST_MWEB_HEIGHT - 1)

    # Pegin some coins
    mining_node.sendtoaddress(mining_node.getnewaddress(address_type='mweb'), 1)

    # Create some blocks - activate MWEB
    mining_node.generate(1)


"""Retrieves the HogEx transaction for the block.

node - The node to use to lookup the transaction.
block_hash - The block to retrieve the HogEx for. If not provided, the best block hash will be used.

Returns the HogEx as a 'CTransaction'
"""
def get_hogex_tx(node, block_hash = None):
    block_hash = block_hash or node.getbestblockhash()
    hogex_tx_id = node.getblock(block_hash)['tx'][-1]
    hogex_tx = FromHex(CTransaction(), node.getrawtransaction(hogex_tx_id, False, block_hash)) # TODO: Should validate that the tx is marked as a hogex tx
    hogex_tx.rehash()
    return hogex_tx
    

"""Retrieves the HogAddr for a block.

node - The node to use to lookup the HogAddr.
block_hash - The block to retrieve the HogAddr for. If not provided, the best block hash will be used.

Returns the HogAddr as a 'CTxOut'
"""
def get_hog_addr_txout(node, block_hash = None):
    return get_hogex_tx(node, block_hash).vout[0]
    

"""Retrieves the MWEB header for a block.

node - The node to use to lookup the MWEB header.
block_hash - The block to retrieve the MWEB header for. If not provided, the best block hash will be used.

Returns the MWEB header as an 'MWEBHeader' or None if the block doesn't contain MWEB data.
"""
def get_mweb_header(node, block_hash = None):
    block_hash = block_hash or node.getbestblockhash()
    best_block = node.getblock(block_hash, 2)
    if not 'mweb' in best_block:
        return None

    mweb_header = MWEBHeader()
    mweb_header.from_json(best_block['mweb'])
    return mweb_header

    
"""Creates a HogEx transaction that commits to the provided MWEB hash.
# TODO: In the future, this should support passing in pegins and pegouts.

node - The node to use to lookup the latest block.
mweb_hash - The hash of the MWEB to commit to.

Returns the built HogEx transaction as a 'CTransaction'
"""
def create_hogex(node, mweb_hash):
    hogex_tx = get_hogex_tx(node)

    tx = CTransaction()
    tx.vin = [CTxIn(COutPoint(hogex_tx.sha256, 0))]
    tx.vout = [CTxOut(hogex_tx.vout[0].nValue, hogaddr_script(mweb_hash.to_byte_arr()))]
    tx.hogex = True
    tx.rehash()
    return tx


""" Create a non-HD wallet from a temporary v15.1.0 node.

Returns the path of the wallet.dat.
"""
def create_non_hd_wallet(chain, options):    
    version = 150100
    bin_dir = os.path.join(options.previous_releases_path, 'v0.15.1', 'bin')
    initialize_datadir(options.tmpdir, 10, chain)
    data_dir = get_datadir_path(options.tmpdir, 10)

    # adjust conf for pre 17
    conf_file = os.path.join(data_dir, 'litecoin.conf')
    with open(conf_file, 'r', encoding='utf8') as conf:
        conf_data = conf.read()
    with open(conf_file, 'w', encoding='utf8') as conf:
        conf.write(conf_data.replace('[regtest]', ''))

    v15_node = TestNode(
        i=10,
        datadir=data_dir,
        chain=chain,
        rpchost=None,
        timewait=60,
        timeout_factor=1.0,
        bitcoind=os.path.join(bin_dir, 'litecoind'),
        bitcoin_cli=os.path.join(bin_dir, 'litecoin-cli'),
        version=version,
        coverage_dir=None,
        cwd=options.tmpdir,
        extra_args=["-usehd=0"],
    )
    v15_node.start()
    v15_node.wait_for_cookie_credentials()  # ensure cookie file is available to avoid race condition
    v15_node.wait_for_rpc_connection()
    v15_node.stop_node(wait=0)
    v15_node.wait_until_stopped()
    
    return os.path.join(v15_node.datadir, chain, "wallet.dat")