// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_IPC_FUZZ_H
#define BITCOIN_TEST_FUZZ_IPC_FUZZ_H

#include <primitives/transaction.h>
#include <script/script.h>

#include <algorithm>
#include <vector>

class IpcFuzzImplementation
{
public:
    int add(int a, int b) { return a + b; }
    COutPoint passOutPoint(COutPoint o) { return COutPoint{o.hash, o.n ^ 0xFFFFFFFFu}; }
    std::vector<uint8_t> passVectorUint8(std::vector<uint8_t> v) { std::reverse(v.begin(), v.end()); return v; }
    CScript passScript(CScript s) { s << OP_NOP; return s; }
};

#endif // BITCOIN_TEST_FUZZ_IPC_FUZZ_H
