// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_TEST_FUZZ_IPC_FUZZ_H
#define BITCOIN_IPC_TEST_FUZZ_IPC_FUZZ_H

#include <primitives/transaction.h>
#include <script/script.h>
#include <univalue.h>

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

class IpcFuzzImplementation
{
public:
    int add(int a, int b)
    {
        assert(a == m_expected_a);
        assert(b == m_expected_b);
        return a + b;
    }

    COutPoint passOutPoint(COutPoint o)
    {
        assert(o == m_expected_outpoint);
        return COutPoint{o.hash, o.n ^ 0xFFFFFFFFu};
    }

    std::vector<uint8_t> passVectorUint8(std::vector<uint8_t> v)
    {
        assert(v == m_expected_vector);
        std::reverse(v.begin(), v.end());
        return v;
    }

    CScript passScript(CScript s)
    {
        assert(s == m_expected_script);
        s << OP_NOP;
        return s;
    }

    UniValue passUniValue(UniValue v)
    {
        assert(v.write() == m_expected_univalue);
        return v;
    }

    CTransactionRef passTransaction(CTransactionRef tx)
    {
        assert(tx);
        assert(m_expected_transaction);
        assert(*tx == *m_expected_transaction);
        return tx;
    }

    // Calls are synchronous, so expected values are not modified while the
    // server is checking them.
    int m_expected_a{0};
    int m_expected_b{0};
    COutPoint m_expected_outpoint;
    std::vector<uint8_t> m_expected_vector;
    CScript m_expected_script;
    std::string m_expected_univalue;
    CTransactionRef m_expected_transaction;
};

#endif // BITCOIN_IPC_TEST_FUZZ_IPC_FUZZ_H
