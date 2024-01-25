#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Utilities for manipulating blocks and transactions."""

from binascii import a2b_hex
from decimal import Decimal
import io
import struct
import time
import unittest

from .messages import (
    CBlock,
    CCbTx,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    FromHex,
    uint256_to_string,
)
from .script import CScript, CScriptNum, CScriptOp, OP_TRUE, OP_CHECKSIG
from .util import assert_equal, hex_str_to_bytes
from io import BytesIO

MAX_BLOCK_SIGOPS = 20000

# Genesis block time (regtest)
TIME_GENESIS_BLOCK = 1417713337

# Coinbase transaction outputs can only be spent after this number of new blocks (network rule)
COINBASE_MATURITY = 100

NORMAL_GBT_REQUEST_PARAMS = {"rules": []} # type: ignore[var-annotated]

def create_block(hashprev=None, coinbase=None, ntime=None, *, version=None, tmpl=None, txlist=None, dip4_activated=False, v20_activated=False):
    """Create a block (with regtest difficulty)."""
    block = CBlock()
    if tmpl is None:
        tmpl = {}
    block.nVersion = version or tmpl.get('version') or 1
    block.nTime = ntime or tmpl.get('curtime') or int(time.time() + 600)
    block.hashPrevBlock = hashprev or int(tmpl['previousblockhash'], 0x10)
    if tmpl and not tmpl.get('bits') is None:
        block.nBits = struct.unpack('>I', a2b_hex(tmpl['bits']))[0]
    else:
        block.nBits = 0x207fffff  # difficulty retargeting is disabled in REGTEST chainparams
    if coinbase is None:
        coinbase = create_coinbase(height=tmpl['height'], dip4_activated=dip4_activated, v20_activated=v20_activated)
    block.vtx.append(coinbase)
    if txlist:
        for tx in txlist:
            if not hasattr(tx, 'calc_sha256'):
                txo = CTransaction()
                txo.deserialize(io.BytesIO(tx))
                tx = txo
            block.vtx.append(tx)
    block.hashMerkleRoot = block.calc_merkle_root()
    block.calc_sha256()
    return block

def create_block_with_mnpayments(mninfo, node, vtx=None, mn_payee=None, mn_amount=None):
    if vtx is None:
        vtx = []
    bt = node.getblocktemplate()
    height = bt['height']
    tip_hash = bt['previousblockhash']
    coinbasevalue = bt['coinbasevalue']

    assert len(bt['masternode']) <= 2
    if mn_payee is None:
        mn_payee = bt['masternode'][0]['payee']

    mn_operator_payee = None
    if len(bt['masternode']) == 2:
        mn_operator_payee = bt['masternode'][1]['payee']
    # we can't take the masternode payee amount from the template here as we might have additional fees in vtx

    # calculate fees that the block template included (we'll have to remove it from the coinbase as we won't
    # include the template's transactions
    bt_fees = 0
    for tx in bt['transactions']:
        bt_fees += tx['fee']

    new_fees = 0
    for tx in vtx:
        in_value = 0
        out_value = 0
        for txin in tx.vin:
            txout = node.gettxout(uint256_to_string(txin.prevout.hash), txin.prevout.n, False)
            in_value += int(txout['value'] * COIN)
        for txout in tx.vout:
            out_value += txout.nValue
        new_fees += in_value - out_value

    # fix fees
    coinbasevalue -= bt_fees
    coinbasevalue += new_fees

    operator_reward = 0
    if mn_operator_payee is not None:
        for mn in mninfo:
            if mn.rewards_address == mn_payee:
                operator_reward = mn.operator_reward
                break
        assert operator_reward > 0

    mn_operator_amount = 0
    if mn_amount is None:
        v20_info = node.getblockchaininfo()['softforks']['v20']
        mn_amount_total = get_masternode_payment(height, coinbasevalue, v20_info['active'])
        mn_operator_amount = mn_amount_total * operator_reward // 100
        mn_amount = mn_amount_total - mn_operator_amount
    miner_amount = coinbasevalue - mn_amount - mn_operator_amount

    miner_address = node.get_deterministic_priv_key().address
    outputs = {miner_address: str(Decimal(miner_amount) / COIN)}
    if mn_amount > 0:
        outputs[mn_payee] = str(Decimal(mn_amount) / COIN)
    if mn_operator_amount > 0:
        outputs[mn_operator_payee] = str(Decimal(mn_operator_amount) / COIN)

    coinbase = FromHex(CTransaction(), node.createrawtransaction([], outputs))
    coinbase.vin = create_coinbase(height).vin

    # We can't really use this one as it would result in invalid merkle roots for masternode lists
    if len(bt['coinbase_payload']) != 0:
        tip_block = node.getblock(tip_hash)
        cbtx = FromHex(CCbTx(version=1), bt['coinbase_payload'])
        if 'cbTx' in tip_block:
            cbtx.merkleRootMNList = int(tip_block['cbTx']['merkleRootMNList'], 16)
        else:
            cbtx.merkleRootMNList = 0
        coinbase.nVersion = 3
        coinbase.nType = 5 # CbTx
        coinbase.vExtraPayload = cbtx.serialize()

    coinbase.calc_sha256()

    block = create_block(int(tip_hash, 16), coinbase, ntime=bt['curtime'], version=bt['version'])
    block.vtx += vtx

    # Add quorum commitments from template
    for tx in bt['transactions']:
        tx2 = FromHex(CTransaction(), tx['data'])
        if tx2.nType == 6:
            block.vtx.append(tx2)

    block.hashMerkleRoot = block.calc_merkle_root()
    block.solve()
    return block

def script_BIP34_coinbase_height(height):
    if height <= 16:
        res = CScriptOp.encode_op_n(height)
        # Append dummy to increase scriptSig size above 2 (see bad-cb-length consensus rule)
        return CScript([res, OP_TRUE])
    return CScript([CScriptNum(height)])


def create_coinbase(height, pubkey=None, dip4_activated=False, v20_activated=False, nValue=500):
    """Create a coinbase transaction, assuming no miner fees.

    If pubkey is passed in, the coinbase output will be a P2PK output;
    otherwise an anyone-can-spend output."""
    coinbase = CTransaction()
    coinbase.vin.append(CTxIn(COutPoint(0, 0xffffffff), script_BIP34_coinbase_height(height), 0xffffffff))
    coinbaseoutput = CTxOut()
    coinbaseoutput.nValue = nValue * COIN
    if nValue == 500:
        halvings = int(height / 150)  # regtest
        coinbaseoutput.nValue >>= halvings
    if (pubkey is not None):
        coinbaseoutput.scriptPubKey = CScript([pubkey, OP_CHECKSIG])
    else:
        coinbaseoutput.scriptPubKey = CScript([OP_TRUE])
    coinbase.vout = [coinbaseoutput]
    if dip4_activated:
        coinbase.nVersion = 3
        coinbase.nType = 5
        cbtx_version = 3 if v20_activated else 2
        cbtx_payload = CCbTx(cbtx_version, height, 0, 0, 0)
        coinbase.vExtraPayload = cbtx_payload.serialize()
    coinbase.calc_sha256()
    return coinbase

def create_tx_with_script(prevtx, n, script_sig=b"", *, amount, script_pub_key=CScript()):
    """Return one-input, one-output transaction object
       spending the prevtx's n-th output with the given amount.

       Can optionally pass scriptPubKey and scriptSig, default is anyone-can-spend output.
    """
    tx = CTransaction()
    assert n < len(prevtx.vout)
    tx.vin.append(CTxIn(COutPoint(prevtx.sha256, n), script_sig, 0xffffffff))
    tx.vout.append(CTxOut(amount, script_pub_key))
    tx.calc_sha256()
    return tx

def create_transaction(node, txid, to_address, *, amount):
    """ Return signed transaction spending the first output of the
        input txid. Note that the node must be able to sign for the
        output that is being spent, and the node must not be running
        multiple wallets.
    """
    raw_tx = create_raw_transaction(node, txid, to_address, amount=amount)
    tx = CTransaction()
    tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx)))
    return tx

def create_raw_transaction(node, txid, to_address, *, amount):
    """ Return raw signed transaction spending the first output of the
        input txid. Note that the node must be able to sign for the
        output that is being spent, and the node must not be running
        multiple wallets.
    """
    rawtx = node.createrawtransaction(inputs=[{"txid": txid, "vout": 0}], outputs={to_address: amount})
    signresult = node.signrawtransactionwithwallet(rawtx)
    assert_equal(signresult["complete"], True)
    return signresult['hex']

def get_legacy_sigopcount_block(block, accurate=True):
    count = 0
    for tx in block.vtx:
        count += get_legacy_sigopcount_tx(tx, accurate)
    return count

def get_legacy_sigopcount_tx(tx, accurate=True):
    count = 0
    for i in tx.vout:
        count += i.scriptPubKey.GetSigOpCount(accurate)
    for j in tx.vin:
        # scriptSig might be of type bytes, so convert to CScript for the moment
        count += CScript(j.scriptSig).GetSigOpCount(accurate)
    return count

# Identical to GetMasternodePayment in C++ code
def get_masternode_payment(nHeight, blockValue, fV20Active):
    ret = int(blockValue / 5)

    nMNPIBlock = 350
    nMNPIPeriod = 10
    nReallocActivationHeight = 2500

    if nHeight > nMNPIBlock:
        ret += int(blockValue / 20)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 1):
        ret += int(blockValue / 20)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 2):
        ret += int(blockValue / 20)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 3):
        ret += int(blockValue / 40)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 4):
        ret += int(blockValue / 40)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 5):
        ret += int(blockValue / 40)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 6):
        ret += int(blockValue / 40)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 7):
        ret += int(blockValue / 40)
    if nHeight > nMNPIBlock+(nMNPIPeriod* 9):
        ret += int(blockValue / 40)

    if nHeight < nReallocActivationHeight:
        # Block Reward Realocation is not activated yet, nothing to do
        return ret

    nSuperblockCycle = 10
    # Actual realocation starts in the cycle next to one activation happens in
    nReallocStart = nReallocActivationHeight - nReallocActivationHeight % nSuperblockCycle + nSuperblockCycle

    if nHeight < nReallocStart:
        # Activated but we have to wait for the next cycle to start realocation, nothing to do
        return ret

    if fV20Active:
        # Once MNRewardReallocated activates, block reward is 80% of block subsidy (+ tx fees) since treasury is 20%
        # Since the MN reward needs to be equal to 60% of the block subsidy (according to the proposal), MN reward is set to 75% of the block reward.
        # Previous reallocation periods are dropped.
        return blockValue * 3 // 4

    # Periods used to reallocate the masternode reward from 50% to 60%
    vecPeriods = [
        513, # Period 1:  51.3%
        526, # Period 2:  52.6%
        533, # Period 3:  53.3%
        540, # Period 4:  54%
        546, # Period 5:  54.6%
        552, # Period 6:  55.2%
        557, # Period 7:  55.7%
        562, # Period 8:  56.2%
        567, # Period 9:  56.7%
        572, # Period 10: 57.2%
        577, # Period 11: 57.7%
        582, # Period 12: 58.2%
        585, # Period 13: 58.5%
        588, # Period 14: 58.8%
        591, # Period 15: 59.1%
        594, # Period 16: 59.4%
        597, # Period 17: 59.7%
        599, # Period 18: 59.9%
        600  # Period 19: 60%
    ]

    nReallocCycle = nSuperblockCycle * 3
    nCurrentPeriod = min(int((nHeight - nReallocStart) / nReallocCycle), len(vecPeriods) - 1)

    return int(blockValue * vecPeriods[nCurrentPeriod] / 1000)

class TestFrameworkBlockTools(unittest.TestCase):
    def test_create_coinbase(self):
        height = 20
        coinbase_tx = create_coinbase(height=height)
        assert_equal(CScriptNum.decode(coinbase_tx.vin[0].scriptSig), height)
