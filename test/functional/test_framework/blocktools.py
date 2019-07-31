#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Utilities for manipulating blocks and transactions."""

from .address import (
    key_to_p2sh_p2wpkh,
    key_to_p2wpkh,
    script_to_p2sh_p2wsh,
    script_to_p2wsh,
)
from .messages import (
    CBlock,
    COIN,
    COutPoint,
    CTransaction,
    CTxIn,
    CTxInWitness,
    CTxOut,
    FromHex,
    ToHex,
    bytes_to_hex_str,
    hash256,
    hex_str_to_bytes,
    ser_string,
    ser_uint256,
    sha256,
    uint256_from_str,
)
from .script import (
    CScript,
    OP_0,
    OP_1,
    OP_CHECKMULTISIG,
    OP_CHECKSIG,
    OP_RETURN,
    OP_TRUE,
    hash160,
)
from .util import assert_equal
from io import BytesIO

MAX_BLOCK_SIGOPS = 20000

# Genesis block time (regtest)
TIME_GENESIS_BLOCK = 1296688602

# From BIP141
WITNESS_COMMITMENT_HEADER = b"\xaa\x21\xa9\xed"


def create_block(hashprev, coinbase, ntime=None, *, version=1):
    """Create a block (with regtest difficulty)."""
    block = CBlock()
    block.nVersion = version
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

def get_witness_script(witness_root, witness_nonce):
    witness_commitment = uint256_from_str(hash256(ser_uint256(witness_root) + ser_uint256(witness_nonce)))
    output_data = WITNESS_COMMITMENT_HEADER + ser_uint256(witness_commitment)
    return CScript([OP_RETURN, output_data])

def add_witness_commitment(block, nonce=0):
    """Add a witness commitment to the block's coinbase transaction.

    According to BIP141, blocks with witness rules active must commit to the
    hash of all in-block transactions including witness."""
    # First calculate the merkle root of the block's
    # transactions, with witnesses.
    witness_nonce = nonce
    witness_root = block.calc_witness_merkle_root()
    # witness_nonce should go to coinbase witness.
    block.vtx[0].wit.vtxinwit = [CTxInWitness()]
    block.vtx[0].wit.vtxinwit[0].scriptWitness.stack = [ser_uint256(witness_nonce)]

    # witness commitment is the last OP_RETURN output in coinbase
    block.vtx[0].vout.append(CTxOut(0, get_witness_script(witness_root, witness_nonce)))
    block.vtx[0].rehash()
    block.hashMerkleRoot = block.calc_merkle_root()
    block.rehash()

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

def create_coinbase(height, pubkey=None):
    """Create a coinbase transaction, assuming no miner fees.

    If pubkey is passed in, the coinbase output will be a P2PK output;
    otherwise an anyone-can-spend output."""
    coinbase = CTransaction()
    coinbase.vin.append(CTxIn(COutPoint(0, 0xffffffff),
                        ser_string(serialize_script_num(height)), 0xffffffff))
    coinbaseoutput = CTxOut()
    coinbaseoutput.nValue = 50 * COIN
    halvings = int(height / 150)  # regtest
    coinbaseoutput.nValue >>= halvings
    if (pubkey is not None):
        coinbaseoutput.scriptPubKey = CScript([pubkey, OP_CHECKSIG])
    else:
        coinbaseoutput.scriptPubKey = CScript([OP_TRUE])
    coinbase.vout = [coinbaseoutput]
    coinbase.calc_sha256()
    return coinbase

def create_tx_with_script(prevtx, n, script_sig=b"", *, amount, script_pub_key=CScript()):
    """Return one-input, one-output transaction object
       spending the prevtx's n-th output with the given amount.

       Can optionally pass scriptPubKey and scriptSig, default is anyone-can-spend output.
    """
    tx = CTransaction()
    assert(n < len(prevtx.vout))
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

def witness_script(use_p2wsh, pubkey):
    """Create a scriptPubKey for a pay-to-witness TxOut.

    This is either a P2WPKH output for the given pubkey, or a P2WSH output of a
    1-of-1 multisig for the given pubkey. Returns the hex encoding of the
    scriptPubKey."""
    if not use_p2wsh:
        # P2WPKH instead
        pubkeyhash = hash160(hex_str_to_bytes(pubkey))
        pkscript = CScript([OP_0, pubkeyhash])
    else:
        # 1-of-1 multisig
        witness_program = CScript([OP_1, hex_str_to_bytes(pubkey), OP_1, OP_CHECKMULTISIG])
        scripthash = sha256(witness_program)
        pkscript = CScript([OP_0, scripthash])
    return bytes_to_hex_str(pkscript)

def create_witness_tx(node, use_p2wsh, utxo, pubkey, encode_p2sh, amount):
    """Return a transaction (in hex) that spends the given utxo to a segwit output.

    Optionally wrap the segwit output using P2SH."""
    if use_p2wsh:
        program = CScript([OP_1, hex_str_to_bytes(pubkey), OP_1, OP_CHECKMULTISIG])
        addr = script_to_p2sh_p2wsh(program) if encode_p2sh else script_to_p2wsh(program)
    else:
        addr = key_to_p2sh_p2wpkh(pubkey) if encode_p2sh else key_to_p2wpkh(pubkey)
    if not encode_p2sh:
        assert_equal(node.getaddressinfo(addr)['scriptPubKey'], witness_script(use_p2wsh, pubkey))
    return node.createrawtransaction([utxo], {addr: amount})

def send_to_witness(use_p2wsh, node, utxo, pubkey, encode_p2sh, amount, sign=True, insert_redeem_script=""):
    """Create a transaction spending a given utxo to a segwit output.

    The output corresponds to the given pubkey: use_p2wsh determines whether to
    use P2WPKH or P2WSH; encode_p2sh determines whether to wrap in P2SH.
    sign=True will have the given node sign the transaction.
    insert_redeem_script will be added to the scriptSig, if given."""
    tx_to_witness = create_witness_tx(node, use_p2wsh, utxo, pubkey, encode_p2sh, amount)
    if (sign):
        signed = node.signrawtransactionwithwallet(tx_to_witness)
        assert("errors" not in signed or len(["errors"]) == 0)
        return node.sendrawtransaction(signed["hex"])
    else:
        if (insert_redeem_script):
            tx = FromHex(CTransaction(), tx_to_witness)
            tx.vin[0].scriptSig += CScript([hex_str_to_bytes(insert_redeem_script)])
            tx_to_witness = ToHex(tx)

    return node.sendrawtransaction(tx_to_witness)
