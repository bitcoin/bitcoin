// Copyright (c) 2015-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_SETUP_COMMON_H
#define BITCOIN_TEST_UTIL_SETUP_COMMON_H

#include <common/args.h> // IWYU pragma: export
#include <kernel/context.h>
#include <key.h>
#include <node/caches.h>
#include <node/context.h> // IWYU pragma: export
#include <optional>
#include <ostream>
#include <primitives/transaction.h>
#include <pubkey.h>
#include <stdexcept>
#include <test/util/random.h>
#include <util/chaintype.h> // IWYU pragma: export
#include <util/check.h>
#include <util/fs.h>
#include <util/signalinterrupt.h>
#include <util/string.h>
#include <util/vector.h>

#include <functional>
#include <type_traits>
#include <vector>

class arith_uint256;
class CFeeRate;
class Chainstate;
class FastRandomContext;
class uint160;
class uint256;

/** This is connected to the logger. Can be used to redirect logs to any other log */
extern const std::function<void(const std::string&)> G_TEST_LOG_FUN;

/** Retrieve the command line arguments. */
extern const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS;

/** Retrieve the unit test name. */
extern const std::function<std::string()> G_TEST_GET_FULL_NAME;

static constexpr CAmount CENT{1000000};

struct TestOpts {
    std::vector<const char*> extra_args{};
    bool coins_db_in_memory{true};
    bool block_tree_db_in_memory{true};
    bool setup_net{true};
    bool setup_validation_interface{true};
    bool min_validation_cache{false}; // Equivalent of -maxsigcachebytes=0
};

/** Basic testing setup.
 * This just configures logging, data dir and chain parameters.
 */
struct BasicTestingSetup {
    util::SignalInterrupt m_interrupt;
    node::NodeContext m_node; // keep as first member to be destructed last

    FastRandomContext m_rng;
    /** Seed the global RNG state and m_rng for testing and log the seed value. This affects all randomness, except GetStrongRandBytes(). */
    void SeedRandomForTest(SeedRand seed)
    {
        SeedRandomStateForTest(seed);
        m_rng.Reseed(GetRandHash());
    }

    explicit BasicTestingSetup(const ChainType chainType = ChainType::MAIN, TestOpts = {});
    ~BasicTestingSetup();

    fs::path m_path_root;
    fs::path m_path_lock;
    bool m_has_custom_datadir{false};
    /** @brief Test-specific arguments and settings.
     *
     * This member is intended to be the primary source of settings for code
     * being tested by unit tests. It exists to make tests more self-contained
     * and reduce reliance on global state.
     *
     * Usage guidelines:
     * 1. Prefer using m_args where possible in test code.
     * 2. If m_args is not accessible, use m_node.args as a fallback.
     * 3. Avoid direct references to gArgs in test code.
     *
     * Note: Currently, m_node.args points to gArgs for backwards
     * compatibility. In the future, it will point to m_args to further isolate
     * test environments.
     *
     * @see https://github.com/bitcoin/bitcoin/issues/25055 for additional context.
     */
    ArgsManager m_args;
};

/** Testing setup that performs all steps up until right before
 * ChainstateManager gets initialized. Meant for testing ChainstateManager
 * initialization behaviour.
 */
struct ChainTestingSetup : public BasicTestingSetup {
    node::CacheSizes m_cache_sizes{};
    bool m_coins_db_in_memory{true};
    bool m_block_tree_db_in_memory{true};
    std::function<void()> m_make_chainman{};

    explicit ChainTestingSetup(const ChainType chainType = ChainType::MAIN, TestOpts = {});
    ~ChainTestingSetup();

    // Supplies a chainstate, if one is needed
    void LoadVerifyActivateChainstate();
};

/** Testing setup that configures a complete environment.
 */
struct TestingSetup : public ChainTestingSetup {
    explicit TestingSetup(
        const ChainType chainType = ChainType::MAIN,
        TestOpts = {});
};

/** Identical to TestingSetup, but chain set to regtest */
struct RegTestingSetup : public TestingSetup {
    RegTestingSetup()
        : TestingSetup{ChainType::REGTEST} {}
};

class CBlock;
struct CMutableTransaction;
class CScript;

/**
 * Testing fixture that pre-creates a 100-block REGTEST-mode block chain
 */
struct TestChain100Setup : public TestingSetup {
    TestChain100Setup(
        const ChainType chain_type = ChainType::REGTEST,
        TestOpts = {});

    /**
     * Create a new block with just given transactions, coinbase paying to
     * scriptPubKey, and try to add it to the current chain.
     * If no chainstate is specified, default to the active.
     */
    CBlock CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns,
                                 const CScript& scriptPubKey,
                                 Chainstate* chainstate = nullptr);

    /**
     * Create a new block with just given transactions, coinbase paying to
     * scriptPubKey.
     */
    CBlock CreateBlock(
        const std::vector<CMutableTransaction>& txns,
        const CScript& scriptPubKey,
        Chainstate& chainstate);

    //! Mine a series of new blocks on the active chain.
    void mineBlocks(int num_blocks);

    /**
    * Create a transaction, optionally setting the fee based on the feerate.
    * Note: The feerate may not be met exactly depending on whether the signatures can have different sizes.
    *
    * @param input_transactions   The transactions to spend
    * @param inputs               Outpoints with which to construct transaction vin.
    * @param input_height         The height of the block that included the input transactions.
    * @param input_signing_keys   The keys to spend the input transactions.
    * @param outputs              Transaction vout.
    * @param feerate              The feerate the transaction should pay.
    * @param fee_output           The index of the output to take the fee from.
    * @return The transaction and the fee it pays
    */
    std::pair<CMutableTransaction, CAmount> CreateValidTransaction(const std::vector<CTransactionRef>& input_transactions,
                                                                   const std::vector<COutPoint>& inputs,
                                                                   int input_height,
                                                                   const std::vector<CKey>& input_signing_keys,
                                                                   const std::vector<CTxOut>& outputs,
                                                                   const std::optional<CFeeRate>& feerate,
                                                                   const std::optional<uint32_t>& fee_output);
    /**
     * Create a transaction and, optionally, submit to the mempool.
     *
     * @param input_transactions   The transactions to spend
     * @param inputs               Outpoints with which to construct transaction vin.
     * @param input_height         The height of the block that included the input transaction(s).
     * @param input_signing_keys   The keys to spend inputs.
     * @param outputs              Transaction vout.
     * @param submit               Whether or not to submit to mempool
     */
    CMutableTransaction CreateValidMempoolTransaction(const std::vector<CTransactionRef>& input_transactions,
                                                      const std::vector<COutPoint>& inputs,
                                                      int input_height,
                                                      const std::vector<CKey>& input_signing_keys,
                                                      const std::vector<CTxOut>& outputs,
                                                      bool submit = true);

    /**
     * Create a 1-in-1-out transaction and, optionally, submit to the mempool.
     *
     * @param input_transaction  The transaction to spend
     * @param input_vout         The vout to spend from the input_transaction
     * @param input_height       The height of the block that included the input_transaction
     * @param input_signing_key  The key to spend the input_transaction
     * @param output_destination Where to send the output
     * @param output_amount      How much to send
     * @param submit             Whether or not to submit to mempool
     */
    CMutableTransaction CreateValidMempoolTransaction(CTransactionRef input_transaction,
                                                      uint32_t input_vout,
                                                      int input_height,
                                                      CKey input_signing_key,
                                                      CScript output_destination,
                                                      CAmount output_amount = CAmount(1 * COIN),
                                                      bool submit = true);

    /** Create transactions spending from m_coinbase_txns. These transactions will only spend coins
     * that exist in the current chain, but may be premature coinbase spends, have missing
     * signatures, or violate some other consensus rules. They should only be used for testing
     * mempool consistency. All transactions will have some random number of inputs and outputs
     * (between 1 and 24). Transactions may or may not be dependent upon each other; if dependencies
     * exit, every parent will always be somewhere in the list before the child so each transaction
     * can be submitted in the same order they appear in the list.
     * @param[in]   submit      When true, submit transactions to the mempool.
     *                          When false, return them but don't submit them.
     * @returns A vector of transactions that can be submitted to the mempool.
     */
    std::vector<CTransactionRef> PopulateMempool(FastRandomContext& det_rand, size_t num_transactions, bool submit);

    /** Mock the mempool minimum feerate by adding a transaction and calling TrimToSize(0),
     * simulating the mempool "reaching capacity" and evicting by descendant feerate.  Note that
     * this clears the mempool, and the new minimum feerate will depend on the maximum feerate of
     * transactions removed, so this must be called while the mempool is empty.
     *
     * @param target_feerate    The new mempool minimum feerate after this function returns.
     *                          Must be above max(incremental feerate, min relay feerate),
     *                          or 1sat/vB with default settings.
     */
    void MockMempoolMinFee(const CFeeRate& target_feerate);

    std::vector<CTransactionRef> m_coinbase_txns; // For convenience, coinbase transactions
    CKey coinbaseKey; // private/public key needed to spend coinbase transactions
};

/**
 * Make a test setup that has disk access to the debug.log file disabled. Can
 * be used in "hot loops", for example fuzzing or benchmarking.
 */
template <class T = const BasicTestingSetup>
std::unique_ptr<T> MakeNoLogFileContext(const ChainType chain_type = ChainType::REGTEST, TestOpts opts = {})
{
    opts.extra_args = Cat(
        {
            "-nodebuglogfile",
            "-nodebug",
        },
        opts.extra_args);

    return std::make_unique<T>(chain_type, opts);
}

CBlock getBlock13b8a();

// Make types usable in BOOST_CHECK_* @{
namespace std {
template <typename T> requires std::is_enum_v<T>
inline std::ostream& operator<<(std::ostream& os, const T& e)
{
    return os << static_cast<std::underlying_type_t<T>>(e);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    return v ? os << *v
             : os << "std::nullopt";
}
} // namespace std

std::ostream& operator<<(std::ostream& os, const arith_uint256& num);
std::ostream& operator<<(std::ostream& os, const uint160& num);
std::ostream& operator<<(std::ostream& os, const uint256& num);
// @}

/**
 * BOOST_CHECK_EXCEPTION predicates to check the specific validation error.
 * Use as
 * BOOST_CHECK_EXCEPTION(code that throws, exception type, HasReason("foo"));
 */
class HasReason
{
public:
    explicit HasReason(std::string_view reason) : m_reason(reason) {}
    bool operator()(std::string_view s) const { return s.find(m_reason) != std::string_view::npos; }
    bool operator()(const std::exception& e) const { return (*this)(e.what()); }

private:
    const std::string m_reason;
};

#endif // BITCOIN_TEST_UTIL_SETUP_COMMON_H
