#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Tests share/openapi.yml
"""
import subprocess
import filecmp
import os
import sys

OPENAPI_FILE = "bitcoin-src/share/openapi.yml"
GEN_DIR = "build/openapi-gen"
RPC_HEADER_DIR = "bitcoin-src/src/rpc"

# 1. Generate C++ code from OpenAPI
os.makedirs(GEN_DIR, exist_ok=True)
result = subprocess.run([
    "openapi-generator", "generate",
    "-i", OPENAPI_FILE,
    "-g", "cpp-restsdk",
    "-o", GEN_DIR
], capture_output=True, text=True)

if result.returncode != 0:
    print("OpenAPI code generation failed:", result.stderr)
    sys.exit(1)

# 2. List of header files to compare (add more as needed)
headers_to_check = [
    "blockchain.h",
    "client.h",
    "mempool.h",
    "mining.h",
    "protocol.h",
    "rawtransaction_util.h",
    "register.h",
    "request.h",
    "server.h",
    "server_util.h"
]

# 3. Compare generated headers to actual headers
failures = []
for header in headers_to_check:
    gen_header = os.path.join(GEN_DIR, "include", header)
    actual_header = os.path.join(RPC_HEADER_DIR, header)
    if not os.path.exists(gen_header):
        print(f"Generated header missing: {gen_header}")
        failures.append(header)
        continue
    if not filecmp.cmp(gen_header, actual_header, shallow=False):
        print(f"Header mismatch: {header}")
        failures.append(header)

if failures:
    print("Test failed for headers:", failures)
    sys.exit(1)
else:
    print("All headers match!")
    sys.exit(0)
