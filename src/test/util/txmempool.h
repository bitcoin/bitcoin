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

/** Check that we never get into a state where an ephemeral dust
 *  transaction would be mined without the spend of the dust
 *  also being mined. This assumes standardness checks are being
 *  enforced.
*/
void CheckMempoolEphemeralInvariants(const CTxMemPool& tx_pool);

/** For every transaction in tx_pool, check TRUC invariants:
 * - a TRUC tx's ancestor count must be within TRUC_ANCESTOR_LIMIT
 * - a TRUC tx's descendant count must be within TRUC_DESCENDANT_LIMIT
 * - if a TRUC tx has ancestors, its sigop-adjusted vsize must be within TRUC_CHILD_MAX_VSIZE
 * - any non-TRUC tx must only have non-TRUC parents
 * - any TRUC tx must only have TRUC parents
 *   */
void CheckMempoolTRUCInvariants(const CTxMemPool& tx_pool);

/** One-line wrapper for creating a mempool changeset with a single transaction
 *  and applying it. */
void AddToMempool(CTxMemPool& tx_pool, const CTxMemPoolEntry& entry);

/** Mock the mempool minimum feerate by adding a transaction and calling TrimToSize(0),
 * simulating the mempool "reaching capacity" and evicting by descendant feerate.  Note that
 * this clears the mempool, and the new minimum feerate will depend on the maximum feerate of
 * transactions removed, so this must be called while the mempool is empty.
 *
 * @param target_feerate    The new mempool minimum feerate after this function returns.
 *                          Must be above max(incremental feerate, min relay feerate),
 *                          or 1sat/vB with default settings.
 * @param mempool           The mempool to mock the minimum feerate for. Must be empty
 *                          when called.
 */
void MockMempoolMinFee(const CFeeRate& target_feerate, CTxMemPool& mempool);

#endif // BITCOIN_TEST_UTIL_TXMEMPOOL_H
