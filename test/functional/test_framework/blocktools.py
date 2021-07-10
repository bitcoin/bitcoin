#!/usr/bin/env python3
# Copyright (c) 2015-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Utilities for manipulating blocks and transactions."""

from .messages import (
    CBlock,
    CCbTx,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxOut,
    ser_string,
)
from .script import CScript, OP_TRUE, OP_CHECKSIG
from .util import assert_equal, hex_str_to_bytes
from io import BytesIO

def create_block(hashprev, coinbase, ntime=None):
    """Create a block (with regtest difficulty)."""
    block = CBlock()
    if ntime is None:
        import time
        block.nTime = int(time.time() + 600)
    else:
        block.nTime = ntime
    block.hashPrevBlock = hashprev
    block.nBits = 0x207fffff  # difficulty retargeting is disabled in REGTEST chainparams
    block.vtx.append(coinbase)
    block.hashMerkleRoot = block.calc_merkle_root()
    block.calc_sha256()
    return block

def serialize_script_num(value):
    r = bytearray(0)
    if value == 0:
        return r
    neg = value < 0
    absvalue = -value if neg else value
    while (absvalue):
        r.append(int(absvalue & 0xff))
        absvalue >>= 8
    if r[-1] & 0x80:
        r.append(0x80 if neg else 0)
    elif neg:
        r[-1] |= 0x80
    return r

def create_coinbase(height, pubkey=None, dip4_activated=False):
    """Create a coinbase transaction, assuming no miner fees.

    If pubkey is passed in, the coinbase output will be a P2PK output;
    otherwise an anyone-can-spend output."""
    coinbase = CTransaction()
    coinbase.vin.append(CTxIn(COutPoint(0, 0xffffffff),
                        ser_string(serialize_script_num(height)), 0xffffffff))
    coinbaseoutput = CTxOut()
    coinbaseoutput.nValue = 500 * COIN
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
        cbtx_payload = CCbTx(2, height, 0, 0)
        coinbase.vExtraPayload = cbtx_payload.serialize()
    coinbase.calc_sha256()
    return coinbase

def create_tx_with_script(prevtx, n, script_sig=b"", amount=1, script_pub_key=CScript()):
    """Return one-input, one-output transaction object
       spending the prevtx's n-th output with the given amount.

       Can optionally pass scriptPubKey and scriptSig, default is anyone-can-spend ouput.
    """
    tx = CTransaction()
    assert(n < len(prevtx.vout))
    tx.vin.append(CTxIn(COutPoint(prevtx.sha256, n), script_sig, 0xffffffff))
    tx.vout.append(CTxOut(amount, script_pub_key))
    tx.calc_sha256()
    return tx

def create_transaction(node, txid, to_address, amount):
    """ Return signed transaction spending the first output of the
        input txid. Note that the node must be able to sign for the
        output that is being spent, and the node must not be running
        multiple wallets.
    """
    raw_tx = create_raw_transaction(node, txid, to_address, amount)
    tx = CTransaction()
    tx.deserialize(BytesIO(hex_str_to_bytes(raw_tx)))
    return tx

def create_raw_transaction(node, txid, to_address, amount):
    """ Return raw signed transaction spending the first output of the
        input txid. Note that the node must be able to sign for the
        output that is being spent, and the node must not be running
        multiple wallets.
    """
    inputs = [{"txid": txid, "vout": 0}]
    outputs = {to_address: amount}
    rawtx = node.createrawtransaction(inputs, outputs)
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
def get_masternode_payment(nHeight, blockValue, nReallocActivationHeight):
    ret = int(blockValue / 5)

    nMNPIBlock = 350
    nMNPIPeriod = 10

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
