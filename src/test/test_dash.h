// Copyright (c) 2015 The Bitcoin Core developers
// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_TEST_DASH_H
#define BITCOIN_TEST_TEST_DASH_H

#include <chainparamsbase.h>
#include <fs.h>
#include <key.h>
#include <pubkey.h>
#include <random.h>
#include <scheduler.h>
#include <txdb.h>
#include <txmempool.h>

#include <memory>

#include <boost/thread.hpp>

thread_local extern FastRandomContext g_insecure_rand_ctx;

/**
 * Flag to make GetRand in random.h return the same number
 */
extern bool g_mock_deterministic_tests;

static inline void SeedInsecureRand(bool deterministic = false)
{
    g_insecure_rand_ctx = FastRandomContext(deterministic);
}

static inline uint32_t InsecureRand32() { return g_insecure_rand_ctx.rand32(); }
static inline uint256 InsecureRand256() { return g_insecure_rand_ctx.rand256(); }
static inline uint64_t InsecureRandBits(int bits) { return g_insecure_rand_ctx.randbits(bits); }
static inline uint64_t InsecureRandRange(uint64_t range) { return g_insecure_rand_ctx.randrange(range); }
static inline bool InsecureRandBool() { return g_insecure_rand_ctx.randbool(); }

static constexpr CAmount CENT{1000000};

/** Basic testing setup.
 * This just configures logging and chain parameters.
 */
struct BasicTestingSetup {
    ECCVerifyHandle globalVerifyHandle;

    explicit BasicTestingSetup(const std::string& chainName = CBaseChainParams::MAIN);
    ~BasicTestingSetup();

    fs::path SetDataDir(const std::string& name);

private:
    const fs::path m_path_root;
};

/** Testing setup that configures a complete environment.
 * Included are data directory, coins database, script check threads setup.
 */
class CConnman;
class CNode;

class PeerLogicValidation;
struct TestingSetup : public BasicTestingSetup {
    boost::thread_group threadGroup;
    CScheduler scheduler;

    explicit TestingSetup(const std::string& chainName = CBaseChainParams::MAIN);
    ~TestingSetup();
};

class CBlock;
struct CMutableTransaction;
class CScript;

struct TestChainSetup : public TestingSetup
{
    TestChainSetup(int blockCount);
    ~TestChainSetup();

    // Create a new block with just given transactions, coinbase paying to
    // scriptPubKey, and try to add it to the current chain.
    CBlock CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns,
                                 const CScript& scriptPubKey);
    CBlock CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns,
                                 const CKey& scriptKey);
    CBlock CreateBlock(const std::vector<CMutableTransaction>& txns,
                       const CScript& scriptPubKey);
    CBlock CreateBlock(const std::vector<CMutableTransaction>& txns,
                       const CKey& scriptKey);

    std::vector<CTransactionRef> m_coinbase_txns; // For convenience, coinbase transactions
    CKey coinbaseKey; // private/public key needed to spend coinbase transactions
};

//
// Testing fixture that pre-creates a
// 100-block REGTEST-mode block chain
//
struct TestChain100Setup : public TestChainSetup {
    TestChain100Setup() : TestChainSetup(100) {}
};

struct TestChainDIP3Setup : public TestChainSetup
{
    TestChainDIP3Setup() : TestChainSetup(431) {}
};

struct TestChainDIP3BeforeActivationSetup : public TestChainSetup
{
    TestChainDIP3BeforeActivationSetup() : TestChainSetup(430) {}
};

class CTxMemPoolEntry;

struct TestMemPoolEntryHelper
{
    // Default values
    CAmount nFee;
    int64_t nTime;
    unsigned int nHeight;
    bool spendsCoinbase;
    unsigned int sigOpCount;
    LockPoints lp;

    TestMemPoolEntryHelper() :
        nFee(0), nTime(0), nHeight(1),
        spendsCoinbase(false), sigOpCount(4) { }

    CTxMemPoolEntry FromTx(const CMutableTransaction& tx);
    CTxMemPoolEntry FromTx(const CTransactionRef& tx);

    // Change the default value
    TestMemPoolEntryHelper &Fee(CAmount _fee) { nFee = _fee; return *this; }
    TestMemPoolEntryHelper &Time(int64_t _time) { nTime = _time; return *this; }
    TestMemPoolEntryHelper &Height(unsigned int _height) { nHeight = _height; return *this; }
    TestMemPoolEntryHelper &SpendsCoinbase(bool _flag) { spendsCoinbase = _flag; return *this; }
    TestMemPoolEntryHelper &SigOps(unsigned int _sigops) { sigOpCount = _sigops; return *this; }
};

CBlock getBlock13b8a();

// BOOST_CHECK_EXCEPTION predicates to check the specific validation error
class HasReason {
public:
    explicit HasReason(const std::string& reason) : m_reason(reason) {}
    bool operator() (const std::runtime_error& e) const {
        return std::string(e.what()).find(m_reason) != std::string::npos;
    };
private:
    const std::string m_reason;
};

// define an implicit conversion here so that uint256 may be used directly in BOOST_CHECK_*
std::ostream& operator<<(std::ostream& os, const uint256& num);

#endif
