#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test utxo_convert conversion tool"""
import csv
from itertools import product
import os
import platform
try:
    import sqlite3
except ImportError:
    pass
import random
import subprocess
import sys

from test_framework.address import (
    key_to_p2pkh,
    key_to_p2wpkh,
    output_key_to_p2tr,
    script_to_p2sh,
    script_to_p2wsh,
)
from test_framework.key import ECKey
from test_framework.messages import (
    COutPoint,
    CTxOut,
    uint256_from_str,
)
from test_framework.crypto.muhash import MuHash3072
from test_framework.script import (
    CScript,
    CScriptOp,
)
from test_framework.script_util import (
    PAY_TO_ANCHOR,
    key_to_p2pk_script,
    key_to_p2pkh_script,
    key_to_p2wpkh_script,
    keys_to_multisig_script,
    output_key_to_p2tr_script,
    script_to_p2sh_script,
    script_to_p2wsh_script,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
)
from test_framework.wallet import MiniWallet


def calculate_muhash_from_sqlite_utxos(filename, txid_format, spk_format):
    muhash = MuHash3072()
    con = sqlite3.connect(filename)
    cur = con.cursor()
    for (txid, vout, value, coinbase, height, spk) in cur.execute("SELECT * FROM utxos"):
        match txid_format:
            case "hex":
                assert type(txid) is str
                txid_bytes = bytes.fromhex(txid)[::-1]
            case "raw":
                assert type(txid) is bytes
                txid_bytes = txid
            case "rawle":
                assert type(txid) is bytes
                txid_bytes = txid[::-1]
        match spk_format:
            case "hex":
                assert type(spk) is str
                spk_bytes = bytes.fromhex(spk)
            case "raw":
                assert type(spk) is bytes
                spk_bytes = spk

        # serialize UTXO for MuHash (see function `TxOutSer` in the  coinstats module)
        utxo_ser = COutPoint(uint256_from_str(txid_bytes), vout).serialize()
        utxo_ser += (height * 2 + coinbase).to_bytes(4, 'little')
        utxo_ser += CTxOut(value, spk_bytes).serialize()
        muhash.insert(utxo_ser)
    con.close()
    return muhash.digest()[::-1].hex()


class UtxoToSqliteTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        # we want to create some UTXOs with non-standard output scripts
        self.extra_args = [['-acceptnonstdtxn=1']]

    def skip_test_if_missing_module(self):
        self.skip_if_no_py_sqlite3()

    def run_test(self):
        node = self.nodes[0]
        wallet = MiniWallet(node)
        key = ECKey()

        self.log.info('Create UTXOs with various output script types')
        expected_csv_records = []
        for i in range(1, 20+1):
            key.generate(compressed=False)
            uncompressed_pubkey = key.get_pubkey().get_bytes()
            key.generate(compressed=True)
            pubkey = key.get_pubkey().get_bytes()
            multisig_k = random.randrange(1, i + 1)

            # add output scripts for compressed script type 0 (P2PKH), type 1 (P2SH),
            # types 2-3 (P2PK compressed), types 4-5 (P2PK uncompressed) and
            # for uncompressed scripts (bare multisig, segwit, etc.)
            p2pkh_script = key_to_p2pkh_script(pubkey)
            large_script = CScript([CScriptOp.encode_op_n(i % 17)]*(10001-i))  # large script (up to 10000 bytes)
            output_scripts = {
                key_to_p2pkh_script(pubkey): f"addr({key_to_p2pkh(pubkey)})",
                script_to_p2sh_script(p2pkh_script): f"addr({script_to_p2sh(p2pkh_script)})",
                key_to_p2pk_script(pubkey): f"pk({pubkey.hex()})",
                key_to_p2pk_script(uncompressed_pubkey): f"pk({uncompressed_pubkey.hex()})",

                keys_to_multisig_script([pubkey]*i, k=multisig_k): f"multi({multisig_k},{','.join([pubkey.hex()] * i)})",
                keys_to_multisig_script([uncompressed_pubkey]*i, k=multisig_k): f"multi({multisig_k},{','.join([uncompressed_pubkey.hex()] * i)})",
                key_to_p2wpkh_script(pubkey): f"addr({key_to_p2wpkh(pubkey)})",
                script_to_p2wsh_script(p2pkh_script): f"addr({script_to_p2wsh(p2pkh_script)})",
                output_key_to_p2tr_script(pubkey[1:]): f"addr({output_key_to_p2tr(pubkey[1:])})",
                PAY_TO_ANCHOR: "addr(bcrt1pfeesnyr2tx)",
                large_script: f"raw({large_script.hex()})",
            }

            # create outputs and mine them in a block
            output_script_keys = list(output_scripts.keys())
            random.shuffle(output_script_keys)
            height = node.getblockcount()
            for k, output_script in enumerate(output_script_keys):
                tx = wallet.send_to(from_node=node, scriptPubKey=output_script, amount=i + k * 100, fee=20000 - k)
                for idx, txout in enumerate(tx['tx'].vout):
                    if txout.nValue == i + k * 100:
                        vout = idx
                        break
                expected_csv_records.append({
                    "txid": tx['txid'],
                    "vout": str(vout),
                    "value": str(i + k * 100),
                    "coinbase": "0",
                    "height": str(height + 1),
                    "descriptor": output_scripts[output_script]
                })
            self.generate(wallet, 1)

        self.log.info('Dump UTXO set via `dumptxoutset` RPC')
        input_filename = os.path.join(self.options.tmpdir, "utxos.dat")
        node.dumptxoutset(input_filename, "latest")

        for i, (txid_format, spk_format) in enumerate(product(["hex", "raw", "rawle"], ["hex", "raw"])):
            self.log.info(f'Test utxo-to-sqlite script using txid format "{txid_format}" and spk format "{spk_format}" ({i+1})')
            self.log.info('-> Convert UTXO set from compact-serialized format to sqlite format')
            output_filename = os.path.join(self.options.tmpdir, f"utxos_{i+1}.sqlite")
            base_dir = self.config["environment"]["SRCDIR"]
            utxo_convert_path = os.path.join(base_dir, "contrib", "utxo-tools", "utxo_convert.py")
            arguments = [input_filename, output_filename, f'--txid={txid_format}', f'--spk={spk_format}']
            subprocess.run([sys.executable, utxo_convert_path] + arguments, check=True, stderr=subprocess.STDOUT)

            self.log.info('-> Verify that both UTXO sets match by comparing their MuHash')
            muhash_sqlite = calculate_muhash_from_sqlite_utxos(output_filename, txid_format, spk_format)
            muhash_compact_serialized = node.gettxoutsetinfo('muhash')['muhash']
            assert_equal(muhash_sqlite, muhash_compact_serialized)
            self.log.info('')

        if platform.system() != "Windows":  # FIFOs are not available on Windows
            self.log.info('Convert UTXO set directly (without intermediate dump) via named pipe')
            fifo_filename = os.path.join(self.options.tmpdir, "utxos.fifo")
            os.mkfifo(fifo_filename)
            output_direct_filename = os.path.join(self.options.tmpdir, "utxos_direct.sqlite")
            p = subprocess.Popen([sys.executable, utxo_to_sqlite_path, fifo_filename, output_direct_filename],
                                 stderr=subprocess.STDOUT)
            node.dumptxoutset(fifo_filename, "latest")
            p.wait(timeout=10)
            muhash_direct_sqlite = calculate_muhash_from_sqlite_utxos(output_direct_filename, "hex", "hex")
            assert_equal(muhash_sqlite, muhash_direct_sqlite)
            os.remove(fifo_filename)

        self.log.info('Test utxo-to-csv script')
        self.log.info('-> Convert UTXO set from compact-serialized format to CSV format')
        output_filename = os.path.join(self.options.tmpdir, "utxos.csv")
        base_dir = self.config["environment"]["SRCDIR"]
        utxo_convert_path = os.path.join(base_dir, "contrib", "utxo-tools", "utxo_convert.py")
        arguments = [input_filename, output_filename]
        subprocess.run([sys.executable, utxo_convert_path] + arguments, check=True, stderr=subprocess.STDOUT)

        self.log.info('-> Verify that the CSV file contains the expected records')
        with open(output_filename, 'r', encoding='utf-8') as csvfile:
            reader = csv.DictReader(csvfile)
            records = []
            # Filter out large UTXOs (coinbase outputs and change).
            for record in reader:
                if int(record['value']) < 10000:
                    records.append(record)
            # Check whether the result matches the expected set of UTXO records.
            records.sort(key=lambda x: (x['txid'], x['vout']))
            expected_csv_records.sort(key=lambda x: (x['txid'], x['vout']))
            assert_equal(records, expected_csv_records)
        self.log.info('')

if __name__ == "__main__":
    UtxoToSqliteTest(__file__).main()
