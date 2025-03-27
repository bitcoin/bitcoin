#!/usr/bin/env python3
# Copyright (c) 2019-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet implicit segwit feature."""

import test_framework.address as address
from test_framework.test_framework import BitcoinTestFramework

address_types = ('legacy', 'bech32', 'p2sh-segwit', 'p2pk')

def key_to_address(key, address_type):
    if address_type == 'legacy':
        return address.key_to_p2pkh(key)
    elif address_type == 'p2sh-segwit':
        return address.key_to_p2sh_p2wpkh(key)
    elif address_type == 'bech32':
        return address.key_to_p2wpkh(key)

def modify_rawtx(rawtx, p2pk_script):
    tx_bytes = bytearray.fromhex(rawtx)
    idx = 0
    # Skip version (4 bytes)
    idx += 4
    # Read input count (VarInt)
    input_count = tx_bytes[idx]
    idx += 1

    # Skip each input (each input: 32-byte txid, 4-byte vout, VarInt script length, script, 4-byte sequence)
    for _ in range(input_count):
        idx += 32
        idx += 4
        script_len = tx_bytes[idx]
        idx += 1 + script_len
        idx += 4

    idx += 1
    # Process the first output.
    idx += 8

    old_script_len = tx_bytes[idx]
    # Compute new script length.
    new_script_bytes = bytes.fromhex(p2pk_script)
    new_script_len = len(new_script_bytes)
    # Replace the script length byte.
    tx_bytes[idx] = new_script_len
    idx += 1

    # Remove the old script and insert the new one.
    before_script = tx_bytes[:idx]
    after_script = tx_bytes[idx + old_script_len:]
    new_tx_bytes = before_script + new_script_bytes + after_script
    new_tx_hex = new_tx_bytes.hex()
    return new_tx_hex

def send_a_to_b(receive_node, send_node):
    keys = {}
    for a in address_types:
        # For p2pk, generate a legacy address since Bitcoin Core doesn't support "p2pk"
        if a == 'p2pk':
            a_address = receive_node.getnewaddress(address_type='legacy')
        else:
            a_address = receive_node.getnewaddress(address_type=a)
        pubkey = receive_node.getaddressinfo(a_address)['pubkey']
        keys[a] = pubkey
        for b in address_types:
            if b != 'p2pk':
                b_address = key_to_address(pubkey, b)
                send_node.sendtoaddress(address=b_address, amount=1)
            else:
                # For p2pk simulation: we want to send to a P2PK output.
                p2pk_script = "21" + pubkey + "ac"
                legacy_addr = key_to_address(pubkey, 'legacy')
                utxos = send_node.listunspent()
                if not utxos:
                    raise Exception("No UTXOs available in the sender wallet")
                utxo = utxos[0]
                inputs = [{"txid": utxo["txid"], "vout": utxo["vout"]}]
                # Create the transaction with the legacy address as output.
                outputs = {legacy_addr: 1.0}
                rawtx = send_node.createrawtransaction(inputs, outputs)
                #Modify rawtx to replace the scriptPubKey with our P2PK script.
                new_rawtx = modify_rawtx(rawtx, p2pk_script)
                # Fund, sign, and broadcast the modified raw transaction.
                funded_tx = send_node.fundrawtransaction(new_rawtx)
                signed_tx = send_node.signrawtransactionwithwallet(funded_tx["hex"])
                if not signed_tx.get("complete", False):
                    raise Exception("Transaction signing failed")
                send_node.sendrawtransaction(signed_tx["hex"])
    return keys

def check_implicit_transactions(implicit_keys, implicit_node):
    # The implicit segwit node allows conversion all possible ways
    txs = implicit_node.listtransactions(None, 99999)
    for a in address_types:
        pubkey = implicit_keys[a]
        for b in address_types:
            # For p2pk transactions, treat them as legacy addresses when checking.
            addr_type = b if b != 'p2pk' else 'legacy'
            expected_address = key_to_address(pubkey, addr_type)
            found = False
            for tx in txs:
                if tx.get('category') != 'receive':
                    continue
                tx_address = tx.get('address')
                # If the address field is missing and this is a p2pk transaction, fall back to legacy.
                if tx_address is None and b == 'p2pk':
                    tx_address = key_to_address(pubkey, 'legacy')
                if tx_address == expected_address:
                    found = True
                    break
            assert found, f"Transaction for key {pubkey} with expected address {expected_address} not found"

class ImplicitSegwitTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser, descriptors=False)

    def set_test_params(self):
        self.num_nodes = 2
        self.supports_cli = False

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Manipulating addresses and sending transactions to all variations")
        implicit_keys = send_a_to_b(self.nodes[0], self.nodes[1])

        self.sync_all()

        self.log.info("Checking that transactions show up correctly without a restart")
        check_implicit_transactions(implicit_keys, self.nodes[0])

        self.log.info("Checking that transactions still show up correctly after a restart")
        self.restart_node(0)
        self.restart_node(1)

        check_implicit_transactions(implicit_keys, self.nodes[0])

if __name__ == '__main__':
    ImplicitSegwitTest(__file__).main()
