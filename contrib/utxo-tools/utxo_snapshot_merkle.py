#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Compute the chunk Merkle root of a dumptxoutset snapshot file.

The file is split into chunks of 3.9 MiB. Each chunk is hashed
with SHA256d to produce a leaf hash. At each level, if the count
is odd, the last element is duplicated before pairing.

Usage:
  python3 utxo_snapshot_merkle.py <snapshot_file>
"""
import argparse
import hashlib
import os
import sys

UTXO_SET_CHUNK_SIZE = 3_900_000


def sha256d(data: bytes) -> bytes:
    """Double SHA-256"""
    return hashlib.sha256(hashlib.sha256(data).digest()).digest()


def compute_merkle_root(leaf_hashes: list[bytes]) -> bytes:
    """Compute the Merkle root"""
    if not leaf_hashes:
        return b'\x00' * 32
    hashes = list(leaf_hashes)
    while len(hashes) > 1:
        if len(hashes) % 2 == 1:
            hashes.append(hashes[-1])
        next_level = []
        for i in range(0, len(hashes), 2):
            next_level.append(sha256d(hashes[i] + hashes[i + 1]))
        hashes = next_level
    return hashes[0]


def main():
    parser = argparse.ArgumentParser(description="Compute the chunk Merkle root of a dumptxoutset snapshot file.")
    parser.add_argument("snapshot", help="Path to the dumptxoutset snapshot file")
    args = parser.parse_args()

    if not os.path.isfile(args.snapshot):
        print(f"Error: file not found: {args.snapshot}", file=sys.stderr)
        sys.exit(1)

    file_size = os.path.getsize(args.snapshot)
    total_chunks = (file_size + UTXO_SET_CHUNK_SIZE - 1) // UTXO_SET_CHUNK_SIZE

    print(f"File: {args.snapshot}")
    print(f"Size: {file_size} bytes")
    print(f"Chunks: {total_chunks} (chunk size: {UTXO_SET_CHUNK_SIZE})")

    leaf_hashes = []
    with open(args.snapshot, "rb") as f:
        remaining = file_size
        while remaining > 0:
            chunk_size = min(remaining, UTXO_SET_CHUNK_SIZE)
            chunk = f.read(chunk_size)
            leaf_hashes.append(sha256d(chunk))
            remaining -= chunk_size

    root = compute_merkle_root(leaf_hashes)
    root_hex = root[::-1].hex()
    print(f"Merkle root: {root_hex}")


if __name__ == "__main__":
    main()
