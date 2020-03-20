#!/usr/bin/env python3

# Copyright (C) 2020 The python-bitcoinlib developers
#
# This file is part of python-bitcoinlib.
#
# It is subject to the license terms in the LICENSE file found in the top-level
# directory of this distribution.
#
# No part of python-bitcoinlib, including this file, may be copied, modified,
# propagated, or distributed except according to the terms contained in the
# LICENSE file.

"""Low-level example of how to spend a P2WPKH output."""

import sys
if sys.version_info.major < 3:
    sys.stderr.write("Sorry, Python 3.x required by this example.\n")
    sys.exit(1)

import hashlib

from bitcoin import SelectParams
from bitcoin.core import b2x, b2lx, lx, COIN, COutPoint, CTxOut, CTxIn, CTxInWitness, CTxWitness, CScriptWitness, CMutableTransaction, Hash160
from bitcoin.core.script import CScript, OP_0, SignatureHash, SIGHASH_ALL, SIGVERSION_WITNESS_V0
from bitcoin.wallet import CBitcoinSecret, P2WPKHBitcoinAddress
from bitcoin.rpc import Proxy

SelectParams("regtest")
connection = Proxy()

if connection._call("getblockchaininfo")["chain"] != "regtest":
    sys.stderr.write("This example is intended for regtest only.\n")
    sys.exit(1)


# Create the (in)famous correct brainwallet secret key.
h = hashlib.sha256(b'correct horse battery staple').digest()
seckey = CBitcoinSecret.from_secret_bytes(h)

# Create an address from that private key.
public_key = seckey.pub
scriptPubKey = CScript([OP_0, Hash160(public_key)])
address = P2WPKHBitcoinAddress.from_scriptPubKey(scriptPubKey)

# Give the private key to bitcoind (for ismine, listunspent, etc).
connection._call("importprivkey", str(seckey))

# Check if there's any funds available.
unspentness = lambda: connection._call("listunspent", 6, 9999, [str(address)], True, {"minimumAmount": 1.0})
unspents = unspentness()
while len(unspents) == 0:
    # mine some funds into the address
    connection._call("generatetoaddress", 110, str(address))
    unspents = unspentness()

# Choose the first UTXO, let's spend it!
unspent_utxo_details = unspents[0]
txid = unspent_utxo_details["txid"]
vout = unspent_utxo_details["vout"]
amount = int(float(unspent_utxo_details["amount"]) * COIN)

# Calculate an amount for the upcoming new UTXO. Set a high fee to bypass
# bitcoind minfee setting.
amount_less_fee = int(amount - (0.01 * COIN))

# Create a destination to send the coins.
destination_address = connection._call("getnewaddress", "python-bitcoinlib-example", "bech32")
destination_address = P2WPKHBitcoinAddress(destination_address)
target_scriptPubKey = destination_address.to_scriptPubKey()

# Create the unsigned transaction.
txin = CTxIn(COutPoint(lx(txid), vout))
txout = CTxOut(amount_less_fee, target_scriptPubKey)
tx = CMutableTransaction([txin], [txout])

# Specify which transaction input is going to be signed for.
txin_index = 0

# When signing a P2WPKH transaction, use an "implicit" script that isn't
# specified in the scriptPubKey or the witness.
redeem_script = address.to_redeemScript()

# Calculate the signature hash for the transaction. This is then signed by the
# private key that controls the UTXO being spent here at this txin_index.
sighash = SignatureHash(redeem_script, tx, txin_index, SIGHASH_ALL, amount=amount, sigversion=SIGVERSION_WITNESS_V0)
signature = seckey.sign(sighash) + bytes([SIGHASH_ALL])

# Construct a witness for this transaction input. The public key is given in
# the witness so that the appropriate redeem_script can be calculated by
# anyone. The original scriptPubKey had only the Hash160 hash of the public
# key, not the public key itself, and the redeem script can be entirely
# re-constructed (from implicit template) if given just the public key. So the
# public key is added to the witness. This is P2WPKH in bip141.
witness = [signature, public_key]

# Aggregate all of the witnesses together, and then assign them to the
# transaction object.
ctxinwitnesses = [CTxInWitness(CScriptWitness(witness))]
tx.wit = CTxWitness(ctxinwitnesses)

# Broadcast the transaction to the regtest network.
spend_txid = connection.sendrawtransaction(tx)

# Done! Print the transaction to standard output. Show the transaction
# serialization in hex (instead of bytes), and render the txid.
print("serialized transaction: {}".format(b2x(tx.serialize())))
print("txid: {}".format(b2lx(spend_txid)))
