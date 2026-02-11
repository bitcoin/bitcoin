#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Signet constants and helper functions used in tests."""

import hashlib

from test_framework.messages import ser_compact_size

SIGNET_DEFAULT_CHALLENGE = '512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae'

def message_start(challenge_hex: str) -> str:
    raw = bytes.fromhex(challenge_hex)
    ser = ser_compact_size(len(raw)) + raw
    digest = hashlib.sha256(hashlib.sha256(ser).digest()).digest()
    return digest[:4].hex()
