#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Verify the shared published BIP322 simple proofs without a wallet or chain data."""

import json
from pathlib import Path

from test_framework.address import program_to_witness
from test_framework.segwit_addr import decode_segwit_address
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)


class BIP322Test(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        node = self.nodes[0]
        fixture = Path(self.config['environment']['SRCDIR']) / 'src/test/data/bip322_vectors.json'
        vectors = json.loads(fixture.read_text(encoding='utf-8'))
        assert_equal(vectors['provenance']['commit'], 'a34f044bb8d5cd684d7a742d2f6483c14b719046')
        assert_equal(vectors['scope']['counts'], {
            'hash_rows': 3, 'simple_rows': 8, 'simple_signatures': 10, 'errors': 16,
        })
        rows = proofs = errors = 0

        self.log.info('Verify every published simple proof, preserving its program and message bytes')
        for source in ['basic', 'generated']:
            assert_equal(vectors['scope'][source]['simple'], list(range(4)))
            assert_equal(len(vectors[source]['simple']), 4)
            for index, row in enumerate(vectors[source]['simple']):
                self.log.info('%s simple row %d: %s', source, index, row['type'])
                version, program = decode_segwit_address('bc', row['address'])
                address = program_to_witness(version, program)
                assert_equal(decode_segwit_address('bcrt', address), (version, program))
                wrong_program = bytearray(program)
                wrong_program[0] ^= 1
                wrong_address = program_to_witness(version, wrong_program)
                for proof in row['bip322_signatures']:
                    assert_raises_rpc_error(-5, 'Invalid address', node.verifymessage, row['address'], proof, row['message'])
                    raw = proof[3:] if proof.startswith('smp') else proof
                    for signature in [raw, 'smp' + raw]:
                        assert node.verifymessage(address, signature, row['message'])
                        assert not node.verifymessage(address, signature, row['message'] + '!')
                        assert not node.verifymessage(wrong_address, signature, row['message'])
                    for prefix in ['ful', 'pof']:
                        assert not node.verifymessage(address, prefix + raw, row['message'])
                    proofs += 1
                rows += 1

            self.log.info('Check every scoped %s error with its original proof and message', source)
            assert_equal(vectors['scope'][source]['error'], list(range(8)))
            assert_equal(len(vectors[source]['error']), 8)
            for index, row in enumerate(vectors[source]['error']):
                self.log.info('%s error row %d: %s', source, index, row['description'])
                version, program = decode_segwit_address('bc', row['address'])
                address = program_to_witness(version, program)
                args = (address, row['signature'], row['message'])
                if source == 'basic' and index in [0, 6]:
                    assert_raises_rpc_error(-3, 'Malformed base64 encoding', node.verifymessage, *args)
                elif source == 'basic' and index == 1:
                    assert_raises_rpc_error(-3, 'Malformed BIP322 witness', node.verifymessage, *args)
                else:
                    assert not node.verifymessage(*args)
                errors += 1

        assert_equal((rows, proofs, errors), (8, 10, 16))
        self.log.info('Executed 8 published rows / 10 proofs in both encodings and all 16 scoped errors')
        self.log.info('Full and proof-of-funds vectors are outside the simple-only scope')
        unsupported = vectors['scope']['unsupported']['generated']
        assert_equal(len(unsupported['full']), 10)
        assert_equal(len(unsupported['proof_of_funds']), 3)
        assert_equal(len(unsupported['error']), 20)
        assert_equal(node.getblockcount(), 0)


if __name__ == '__main__':
    BIP322Test(__file__).main()
