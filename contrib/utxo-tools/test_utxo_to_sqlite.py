#!/usr/bin/env python3
# Copyright (c) 2026 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Run with python3 contrib/utxo-tools/test_utxo_to_sqlite.py."""

import io
from pathlib import Path
import sqlite3
import subprocess
import sys
import tempfile
import unittest

from utxo_to_sqlite import decompress_script, read_compactsize, read_varint


class UtxoToSqliteTest(unittest.TestCase):
    def test_compactsize_truncation(self):
        for encoded, expected in [
            (b'\xfd\xfd\x00', 253),
            (b'\xfe\x00\x00\x01\x00', 65536),
            (b'\xff\x00\x00\x00\x00\x01\x00\x00\x00', 2**32),
        ]:
            self.assertEqual(read_compactsize(io.BytesIO(encoded)), expected)
            for size in range(len(encoded)):
                with self.subTest(encoded=encoded, size=size), self.assertRaises(ValueError):
                    read_compactsize(io.BytesIO(encoded[:size]))

    def test_varint_truncation(self):
        self.assertEqual(read_varint(io.BytesIO(b'\x80\x00')), 128)
        for encoded in [b'', b'\x80']:
            with self.subTest(encoded=encoded), self.assertRaises(ValueError):
                read_varint(io.BytesIO(encoded))

    def test_script_truncation(self):
        generator_x = bytes.fromhex('79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798')
        for encoded, script_size in [
            (b'\x00' + bytes(20), 25),
            (b'\x01' + bytes(20), 23),
            (b'\x02' + generator_x, 35),
            (b'\x03' + generator_x, 35),
            (b'\x04' + generator_x, 67),
            (b'\x05' + generator_x, 67),
            (b'\x0a' + b'\x51' * 4, 4),
        ]:
            self.assertEqual(len(decompress_script(io.BytesIO(encoded))), script_size)
            for size in range(len(encoded)):
                with self.subTest(tag=encoded[0], size=size), self.assertRaises(ValueError):
                    decompress_script(io.BytesIO(encoded[:size]))
        self.assertEqual(decompress_script(io.BytesIO(b'\x06')), b'')

    def test_snapshot_conversion(self):
        # Version 2, regtest, one coin, one txid group, vout 0, height 1,
        # non-coinbase, amount code 1, and a four-byte uncompressed script.
        header = b'utxo\xff' + (2).to_bytes(2, 'little') + bytes.fromhex('fabfb5da')
        header += bytes(32) + (1).to_bytes(8, 'little')
        coin = bytes(32) + bytes([1, 0, 2, 1, 10]) + b'\x51' * 4
        snapshot = header + coin
        for data in [snapshot, snapshot[:-3], header[:-1]]:
            with self.subTest(length=len(data)), tempfile.TemporaryDirectory() as directory:
                source = Path(directory) / 'snapshot.dat'
                output = Path(directory) / 'snapshot.sqlite'
                source.write_bytes(data)
                result = subprocess.run(
                    [sys.executable, str(Path(__file__).with_name('utxo_to_sqlite.py')), str(source), str(output)],
                    capture_output=True, text=True, check=False,
                )
                if data == snapshot:
                    self.assertEqual(result.returncode, 0, result.stderr)
                    with sqlite3.connect(output) as connection:
                        self.assertEqual(connection.execute('SELECT scriptpubkey FROM utxos').fetchall(), [('51515151',)])
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn('Truncated UTXO snapshot', result.stderr)
                    self.assertNotIn('TOTAL:', result.stdout)


if __name__ == '__main__':
    unittest.main()
