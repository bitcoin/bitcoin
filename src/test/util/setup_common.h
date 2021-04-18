// Copyright (c) 2015-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_TEST_UTIL_SETUP_COMMON_H
#define WIDECOIN_TEST_UTIL_SETUP_COMMON_H

#include <chainparamsbase.h>
#include <fs.h>
#include <key.h>
#include <node/context.h>
#include <pubkey.h>
#include <random.h>
#include <stdexcept>
#include <txmempool.h>
#include <util/check.h>
#include <util/string.h>

#include <type_traits>

#include <boost/thread/thread.hpp>

/** This is connected to the logger. Can be used to redirect logs to any other log */
extern const std::function<void(const std::string&)> G_TEST_LOG_FUN;

// Enable BOOST_CHECK_EQUAL for enum class types
template <typename T>
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

/**
 * This global and the helpers that use it are not thread-safe.
 *
 * If thread-safety is needed, the global could be made thread_local (given
 * that thread_local is supported on all architectures we support) or a
 * per-thread instance could be used in the multi-threaded test.
 */
extern FastRandomContext g_insecure_rand_ctx;

/**
 * Flag to make GetRand in random.h return the same number
 */
extern bool g_mock_deterministic_tests;

enum class SeedRand {
    ZEROS, //!< Seed with a compile time constant of zeros
    SEED,  //!< Call the Seed() helper
};

/** Seed the given random ctx or use the seed passed in via an environment var */
void Seed(FastRandomContext& ctx);

static inline void SeedInsecureRand(SeedRand seed = SeedRand::SEED)
{
    if (seed == SeedRand::ZEROS) {
        g_insecure_rand_ctx = FastRandomContext(/* deterministic */ true);
    } else {
        Seed(g_insecure_rand_ctx);
    }
}

static inline uint32_t InsecureRand32() { return g_insecure_rand_ctx.rand32(); }
static inline uint256 InsecureRand256() { return g_insecure_rand_ctx.rand256(); }
static inline uint64_t InsecureRandBits(int bits) { return g_insecure_rand_ctx.randbits(bits); }
static inline uint64_t InsecureRandRange(uint64_t range) { return g_insecure_rand_ctx.randrange(range); }
static inline bool InsecureRandBool() { return g_insecure_rand_ctx.randbool(); }

static constexpr CAmount CENT{1000000};

/** Basic testing setup.
 * This just configures logging, data dir and chain parameters.
 */
struct BasicTestingSetup {
    ECCVerifyHandle globalVerifyHandle;
    NodeContext m_node;

    explicit BasicTestingSetup(const std::string& chainName = CBaseChainParams::MAIN, const std::vector<const char*>& extra_args = {});
    ~BasicTestingSetup();

private:
    const fs::path m_path_root;
};

/** Testing setup that configures a complete environment.
 * Included are coins database, script check threads setup.
 */
struct TestingSetup : public BasicTestingSetup {
    boost::thread_group threadGroup;

    explicit TestingSetup(const std::string& chainName = CBaseChainParams::MAIN, const std::vector<const char*>& extra_args = {});
    ~TestingSetup();
};

/** Identical to TestingSetup, but chain set to regtest */
struct RegTestingSetup : public TestingSetup {
    RegTestingSetup()
        : TestingSetup{CBaseChainParams::REGTEST} {}
};

class CBlock;
struct CMutableTransaction;
class CScript;

/**
 * Testing fixture that pre-creates a 100-block REGTEST-mode block chain
 */
struct TestChain100Setup : public RegTestingSetup {
    TestChain100Setup();

    /**
     * Create a new block with just given transactions, coinbase paying to
     * scriptPubKey, and try to add it to the current chain.
     */
    CBlock CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns,
                                 const CScript& scriptPubKey);

    ~TestChain100Setup();

    std::vector<CTransactionRef> m_coinbase_txns; // For convenience, coinbase transactions
    CKey coinbaseKey; // private/public key needed to spend coinbase transactions
};

class CTxMemPoolEntry;

struct TestMemPoolEntryHelper
{
    // Default values
    CAmount nFee;
    int64_t nTime;
    unsigned int nHeight;
    bool spendsCoinbase;
    unsigned int sigOpCost;
    LockPoints lp;

    TestMemPoolEntryHelper() :
        nFee(0), nTime(0), nHeight(1),
        spendsCoinbase(false), sigOpCost(4) { }

    CTxMemPoolEntry FromTx(const CMutableTransaction& tx);
    CTxMemPoolEntry FromTx(const CTransactionRef& tx);

    // Change the default value
    TestMemPoolEntryHelper &Fee(CAmount _fee) { nFee = _fee; return *this; }
    TestMemPoolEntryHelper &Time(int64_t _time) { nTime = _time; return *this; }
    TestMemPoolEntryHelper &Height(unsigned int _height) { nHeight = _height; return *this; }
    TestMemPoolEntryHelper &SpendsCoinbase(bool _flag) { spendsCoinbase = _flag; return *this; }
    TestMemPoolEntryHelper &SigOpsCost(unsigned int _sigopsCost) { sigOpCost = _sigopsCost; return *this; }
};

CBlock getBlock13b8a();

// define an implicit conversion here so that uint256 may be used directly in BOOST_CHECK_*
std::ostream& operator<<(std::ostream& os, const uint256& num);

/**
 * BOOST_CHECK_EXCEPTION predicates to check the specific validation error.
 * Use as
 * BOOST_CHECK_EXCEPTION(code that throws, exception type, HasReason("foo"));
 */
class HasReason
{
public:
    explicit HasReason(const std::string& reason) : m_reason(reason) {}
    bool operator()(const std::exception& e) const
    {
        return std::string(e.what()).find(m_reason) != std::string::npos;
    };

private:
    const std::string m_reason;
};

#endif
