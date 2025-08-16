#!/usr/bin/env python3
# Copyright (c) 2017-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the RPC call related to the uptime command.

Test corresponds to code in rpc/server.cpp.
"""

import time
import json

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_raises_rpc_error


class TestP2Qrh(BitcoinTestFramework):
    WALLET_NAME = "regtest"
    WALLET_PASSPHRASE = "regtest"
    TOTAL_LEAF_COUNT = 5
    LEAF_OF_INTEREST = 4
    BITCOIN_ADDRESS_INFO = '{ "taptree_return": { "leaf_script_priv_key_hex": "cd99b36815af6c7c70fe3e029800714599f0be1725b7d6a88c8b653cac694007", "leaf_script_hex": "202104496c8c514c38a2e31e36d2acb2d43d9a3e7cf5da5fe1d2b0a770f6c3c10bac", "tree_root_hex": "1e5445529fda20fd2ecea2322fdf9ae07adaaad859b9f9856b631bccc46cf0be", "control_block_hex": "c1fa37ef0058db467387b796e16e2c6e082068a70370ba027e15419a310d5ae3678d24f91c62d8cdfe0c5193f0c33d2e5e71d9cec577b0b48f7fb622af8b7dbc8d761051217d788a00b625036265ba3ef60d0644df00fb37da4516c4ac7c4c5abb" }, "utxo_return": { "script_pubkey_hex": "53201e5445529fda20fd2ecea2322fdf9ae07adaaad859b9f9856b631bccc46cf0be", "bech32m_address": "bcrt1rre2y255lmgs06tkw5gezlhu6upad42kctxulnpttvvdue3rv7zlqqukutx", "bitcoin_network": "regtest" } }'
    FUNDING_UTXO_INDEX = 0
    BLOCK_COUNT_AFTER_COINBASE_FUNDING = 110
    SPEND_DETAILS = '{ "tx_hex": "02000000000101e8076c387194ec46bbd234c812b12fa3313e645ed2f143a7db6a899ecb246cbc0000000000ffffffff0178de052a010000001600140de745dc58d8e62e6f47bde30cd5804a82016f9e03412e2ebc83bf39ff9b31c94ff165b1f731950d25d221f5571102b95c01f4b8e2670894e6b6f1a2a2a285f690ce496d70fd7b818a32576a7c51ae0c9e2edad2b7a00122202104496c8c514c38a2e31e36d2acb2d43d9a3e7cf5da5fe1d2b0a770f6c3c10bac61c1fa37ef0058db467387b796e16e2c6e082068a70370ba027e15419a310d5ae3678d24f91c62d8cdfe0c5193f0c33d2e5e71d9cec577b0b48f7fb622af8b7dbc8d761051217d788a00b625036265ba3ef60d0644df00fb37da4516c4ac7c4c5abb00000000", "sighash": "14ddf2df5f790fc2fbb8c3db54d903730e191518b7b3bb279f6d0ce1df64c7e8", "sig_bytes": "2e2ebc83bf39ff9b31c94ff165b1f731950d25d221f5571102b95c01f4b8e2670894e6b6f1a2a2a285f690ce496d70fd7b818a32576a7c51ae0c9e2edad2b7a001", "derived_witness_vec": "2e2ebc83bf39ff9b31c94ff165b1f731950d25d221f5571102b95c01f4b8e2670894e6b6f1a2a2a285f690ce496d70fd7b818a32576a7c51ae0c9e2edad2b7a001202104496c8c514c38a2e31e36d2acb2d43d9a3e7cf5da5fe1d2b0a770f6c3c10bacc1fa37ef0058db467387b796e16e2c6e082068a70370ba027e15419a310d5ae3678d24f91c62d8cdfe0c5193f0c33d2e5e71d9cec577b0b48f7fb622af8b7dbc8d761051217d788a00b625036265ba3ef60d0644df00fb37da4516c4ac7c4c5abb" }'

    def set_test_params(self):
        self.num_nodes = 1
        self.setup_clean_chain = True
        self.uses_wallet = True  # Enable wallet functionality
        # Enable txindex so getrawtransaction can find transactions in the blockchain
        self.extra_args = [["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        # self._list_available_methods()
        self._create_wallet()
        self._parse_bitcoin_address_info()
        self._fund_p2qrh_address()
        self._first_p2qrh_tx()
        self._get_funding_utxo_details()
        self._generate_post_coinbase_blocks()
        self._set_spend_detail_variables()
        self._test_mempool_accept()
        self._submit_tx_and_mine()
        self._view_blockhash_info()
    
    def _list_available_methods(self):
        """List available RPC methods to debug the issue"""
        node = self.nodes[0]
        
        # Get all available commands
        all_help = node.help()
        self.log.info("All available RPC commands:")
        self.log.info(all_help)
        
        # Check if wallet-related commands are available
        wallet_commands = [cmd for cmd in all_help.split('\n') if 'wallet' in cmd.lower()]
        self.log.info(f"Wallet-related commands: {wallet_commands}")
    
    def _create_wallet(self):
        try:
            # Capture the response from createwallet
            response = self.nodes[0].createwallet(wallet_name=self.WALLET_NAME, descriptors=True, passphrase=self.WALLET_PASSPHRASE, load_on_startup=True)
            
            # Log the response
            self.log.info(f"createwallet response: {response}")
        except Exception as e:
            self.log.error(f"Error creating wallet: {e}")
            # Try alternative wallet creation method
            try:
                response = self.nodes[0].createwallet(self.WALLET_NAME)
                self.log.info(f"Alternative createwallet response: {response}")
            except Exception as e2:
                self.log.error(f"Alternative method also failed: {e2}")

    def _parse_bitcoin_address_info(self):
        try:
            # Parse the JSON response
            data = json.loads(self.BITCOIN_ADDRESS_INFO)
            
            # Extract the relevant information
            taptree_return = data.get('taptree_return', {})
            utxo_return = data.get('utxo_return', {})
            
            # Extract individual fields (equivalent to your bash exports)
            self.quantum_root = taptree_return.get('tree_root_hex')
            self.leaf_script_priv_key_hex = taptree_return.get('leaf_script_priv_key_hex')
            self.leaf_script_hex = taptree_return.get('leaf_script_hex')
            self.control_block_hex = taptree_return.get('control_block_hex')
            self.funding_script_pubkey = utxo_return.get('script_pubkey_hex')
            self.p2qrh_addr = utxo_return.get('bech32m_address')
            
            # Log the extracted values for debugging
            self.log.info(f"quantum_root: {self.quantum_root}")
            self.log.info(f"leaf_script_priv_key_hex: {self.leaf_script_priv_key_hex}")
            self.log.info(f"leaf_script_hex: {self.leaf_script_hex}")
            self.log.info(f"control_block_hex: {self.control_block_hex}")
            self.log.info(f"funding_script_pubkey: {self.funding_script_pubkey}")
            self.log.info(f"p2qrh_addr: {self.p2qrh_addr}")
            
            # Verify all required fields were extracted
            required_fields = [
                self.quantum_root,
                self.leaf_script_priv_key_hex,
                self.leaf_script_hex,
                self.control_block_hex,
                self.funding_script_pubkey,
                self.p2qrh_addr
            ]
            
            if all(required_fields):
                self.log.info("All required fields successfully extracted")
            else:
                missing_fields = []
                if not self.quantum_root: missing_fields.append("tree_root_hex")
                if not self.leaf_script_priv_key_hex: missing_fields.append("leaf_script_priv_key_hex")
                if not self.leaf_script_hex: missing_fields.append("leaf_script_hex")
                if not self.control_block_hex: missing_fields.append("control_block_hex")
                if not self.funding_script_pubkey: missing_fields.append("script_pubkey_hex")
                if not self.p2qrh_addr: missing_fields.append("bech32m_address")
                
                self.log.error(f"Missing required fields: {missing_fields}")
                
        except json.JSONDecodeError as e:
            self.log.error(f"Failed to parse JSON: {e}")
        except Exception as e:
            self.log.error(f"Error parsing bitcoin address info: {e}")

    def _fund_p2qrh_address(self):
        try:
            # Fund the p2qrh address
            response = self.generatetoaddress(self.nodes[0], nblocks=1, address=self.p2qrh_addr, maxtries=10)
            self.log.info(f"Funding p2qrh address: {response}")
        except Exception as e:
            self.log.error(f"Error funding p2qrh address: {e}")

    def _first_p2qrh_tx(self):
        try:
            self.p2qrh_descriptor_info = self.nodes[0].getdescriptorinfo(descriptor=f"addr({self.p2qrh_addr})")
            self.log.info(f"P2QRH descriptor info: {self.p2qrh_descriptor_info}")
            desc = self.p2qrh_descriptor_info['descriptor']
            scan_objects = [{"desc": desc}]
            response = self.nodes[0].scantxoutset(action="start", scanobjects=scan_objects)
            self.log.info(f"scantxoutset response: {response}")
            
            # Extract the funding transaction ID from the first unspent
            if response.get('unspents') and len(response['unspents']) > 0:
                self.funding_tx_id = response['unspents'][0]['txid']
                self.log.info(f"Funding transaction ID: {self.funding_tx_id}")
            else:
                self.log.error("No unspent outputs found in scantxoutset response")
        except Exception as e:
            self.log.error(f"Error getting txs for p2qrh address: {e}")

    def _get_funding_utxo_details(self):
        try:
            response = self.nodes[0].getrawtransaction(txid=self.funding_tx_id, verbosity=1)
            
            self.log.info(f"Funding utxo details: {response}")
            
            # Extract the value from the specific UTXO index and convert to satoshis
            if 'vout' in response and len(response['vout']) > self.FUNDING_UTXO_INDEX:
                utxo = response['vout'][self.FUNDING_UTXO_INDEX]
                if 'value' in utxo:
                    # Convert BTC value to satoshis (multiply by 100,000,000)
                    btc_value = float(utxo['value'])
                    self.funding_utxo_amount_sats = int(btc_value * 100000000)
                    self.log.info(f"Funding UTXO amount in satoshis: {self.funding_utxo_amount_sats}")
                else:
                    self.log.error("No 'value' field found in UTXO")
            else:
                self.log.error(f"UTXO index {self.FUNDING_UTXO_INDEX} not found in transaction outputs")
                
        except Exception as e:
            self.log.error(f"Error getting funding utxo details: {e}")

    def _generate_post_coinbase_blocks(self):
        try:
            response = self.generate(self.nodes[0], nblocks=self.BLOCK_COUNT_AFTER_COINBASE_FUNDING)
            self.log.info(f"Generating post-coinbase blocks: {response}")
            
            # Count the block IDs in the response
            block_count = len(response)
            self.log.info(f"Generated {block_count} blocks")
            
        except Exception as e:
            self.log.error(f"Error generating post-coinbase blocks: {e}")

    def _set_spend_detail_variables(self):
        try:
            data = json.loads(self.SPEND_DETAILS)
            self.tx_hex = data.get('tx_hex')
            self.sighash = data.get('sighash')
            self.sig_bytes = data.get('sig_bytes')
            self.derived_witness_vec = data.get('derived_witness_vec')
        except Exception as e:
            self.log.error(f"Error setting spend detail variables: {e}")

    def _test_mempool_accept(self):
        try:
            response = self.nodes[0].testmempoolaccept(rawtxs=[self.tx_hex])
            self.log.info(f"testmempoolaccept response: {response}")
        except Exception as e:
            self.log.error(f"Error testing testmempoolaccept: {e}")
    
    def _submit_tx_and_mine(self):
        try:
            self.p2qrh_spending_tx_id = self.nodes[0].sendrawtransaction(self.tx_hex)
            self.log.info(f"sendrawtransaction response: {self.p2qrh_spending_tx_id}")
            self.generate(self.nodes[0], nblocks=1)
        except Exception as e:
            self.log.error(f"Error submitting tx: {e}")

    def _view_blockhash_info(self):
        try:
            response = self.nodes[0].getrawtransaction(txid=self.p2qrh_spending_tx_id, verbosity=1)
            self.block_hash = response['blockhash']
            self.log.info(f"blockhash containing p2qrh spend tx: {self.block_hash}")
        except Exception as e:
            self.log.error(f"Error getting blockhash: {e}")

if __name__ == '__main__':
    TestP2Qrh(__file__).main()
