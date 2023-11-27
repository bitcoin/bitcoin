// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_TXMEMPOOL_H
#define BITCOIN_TEST_UTIL_TXMEMPOOL_H

#include <policy/packages.h>
#include <txmempool.h>
#include <util/time.h>

namespace node {
struct NodeContext;
}
struct PackageMempoolAcceptResult;

CTxMemPool::Options MemPoolOptionsForTest(const node::NodeContext& node);

struct TestMemPoolEntryHelper {
    // Default values
    CAmount nFee{0};
    NodeSeconds time{};
    unsigned int nHeight{1};
    uint64_t m_sequence{0};
    bool spendsCoinbase{false};
    unsigned int sigOpCost{4};
    LockPoints lp;

    CTxMemPoolEntry FromTx(const CMutableTransaction& tx) const;
    CTxMemPoolEntry FromTx(const CTransactionRef& tx) const;

    // Change the default value
    TestMemPoolEntryHelper& Fee(CAmount _fee) { nFee = _fee; return *this; }
    TestMemPoolEntryHelper& Time(NodeSeconds tp) { time = tp; return *this; }
    TestMemPoolEntryHelper& Height(unsigned int _height) { nHeight = _height; return *this; }
    TestMemPoolEntryHelper& Sequence(uint64_t _seq) { m_sequence = _seq; return *this; }
    TestMemPoolEntryHelper& SpendsCoinbase(bool _flag) { spendsCoinbase = _flag; return *this; }
    TestMemPoolEntryHelper& SigOpsCost(unsigned int _sigopsCost) { sigOpCost = _sigopsCost; return *this; }
};

/** Check expected properties for every PackageMempoolAcceptResult, regardless of value. Returns
 * a string if an error occurs with error populated, nullopt otherwise. If mempool is provided,
 * checks that the expected transactions are in mempool (this should be set to nullptr for a test_accept).
*/
std::optional<std::string>  CheckPackageMempoolAcceptResult(const Package& txns,
                                                            const PackageMempoolAcceptResult& result,
                                                            bool expect_valid,
                                                            const CTxMemPool* mempool);

/** For every transaction in tx_pool, check v3 invariants:
 * - a v3 tx's ancestor count must be within V3_ANCESTOR_LIMIT
 * - a v3 tx's descendant count must be within V3_DESCENDANT_LIMIT
 * - if a v3 tx has ancestors, its sigop-adjusted vsize must be within V3_CHILD_MAX_VSIZE
 * - any non-v3 tx must only have non-v3 parents
 * - any v3 tx must only have v3 parents
 *   */
void CheckMempoolV3Invariants(const CTxMemPool& tx_pool);

#endif // BITCOIN_TEST_UTIL_TXMEMPOOL_H
