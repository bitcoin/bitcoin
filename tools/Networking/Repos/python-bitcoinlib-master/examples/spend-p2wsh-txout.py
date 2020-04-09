#!/usr/bin/env python3

# Copyright (C) 2014 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

"""Low-level example of how to spend a P2WSH/BIP141 txout"""

import sys
if sys.version_info.major < 3:
    sys.stderr.write('Sorry, Python 3.x required by this example.\n')
    sys.exit(1)

import hashlib

from bitcoin import SelectParams
from bitcoin.core import b2x, lx, COIN, COutPoint, CMutableTxOut, CMutableTxIn, CMutableTransaction, CTxInWitness, CTxWitness
from bitcoin.core.script import CScript, CScriptWitness, OP_0, OP_CHECKSIG, SignatureHash, SIGHASH_ALL, SIGVERSION_WITNESS_V0
from bitcoin.wallet import CBitcoinSecret, CBitcoinAddress, P2WSHBitcoinAddress

SelectParams('testnet')

# Create the (in)famous correct brainwallet secret key.
h = hashlib.sha256(b'correct horse battery staple').digest()
seckey = CBitcoinSecret.from_secret_bytes(h)

# Create a witnessScript and corresponding redeemScript. Similar to a scriptPubKey
# the redeemScript must be satisfied for the funds to be spent.
txin_witnessScript = CScript([seckey.pub, OP_CHECKSIG])
txin_scriptHash = hashlib.sha256(txin_witnessScript).digest()
txin_redeemScript = CScript([OP_0, txin_scriptHash])


# Convert the P2WSH scriptPubKey to a base58 Bitcoin address and print it.
# You'll need to send some funds to it to create a txout to spend.
txin_p2wsh_address = P2WSHBitcoinAddress.from_scriptPubKey(txin_redeemScript)
print('Pay to:', str(txin_p2wsh_address))

# Same as the txid:vout the createrawtransaction RPC call requires
# lx() takes *little-endian* hex and converts it to bytes; in Bitcoin
# transaction hashes are shown little-endian rather than the usual big-endian.
txid = lx('ace9dc7c987a52266e38fe8544c2d12182401341c98d151f4b394cf69aa5c3e5')
vout = 0

# Specify the amount send to your P2WSH address.
amount = int(0.0001 * COIN)

# Calculate an amount for the upcoming new UTXO. Set a high fee (5%) to bypass
# bitcoind minfee setting.
amount_less_fee = amount * 0.95

# Create the txin structure, which includes the outpoint. The scriptSig
# defaults to being empty as is necessary for spending a P2WSH output.
txin = CMutableTxIn(COutPoint(txid, vout))


# Specify a destination address and create the txout.
destination_address = CBitcoinAddress(
    '2NGZrVvZG92qGYqzTLjCAewvPZ7JE8S8VxE').to_scriptPubKey()
txout = CMutableTxOut(amount_less_fee, destination_address)

# Create the unsigned transaction.
tx = CMutableTransaction([txin], [txout])

# Calculate the signature hash for that transaction. Note how the script we use
# is the witnessScript, not the redeemScript.
sighash = SignatureHash(script=txin_witnessScript, txTo=tx, inIdx=0,
                        hashtype=SIGHASH_ALL, amount=amount, sigversion=SIGVERSION_WITNESS_V0)

# Now sign it. We have to append the type of signature we want to the end, in
# this case the usual SIGHASH_ALL.
sig = seckey.sign(sighash) + bytes([SIGHASH_ALL])


# # Construct a witness for this P2WSH transaction and add to tx.
witness = CScriptWitness([sig, txin_witnessScript])
tx.wit = CTxWitness([CTxInWitness(witness)])

# TODO: upgrade VerifyScript to support Segregated Witness and place verify the witness program here.

# Done! Print the transaction to standard output with the bytes-to-hex
# function.
print(b2x(tx.serialize()))

