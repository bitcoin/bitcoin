// Copyright (c) 2016-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <coins.h>
#include <policy/policy.h>
#include <wallet/crypter.h>
#include <validation.h>
#include <util.h>
#include <txdb.h>
#include <chainparams.h>
#include <validationinterface.h>

#include <random.h>


#include <vector>

const uint64_t N_CACHE_SCALE = 1; // 40 MB (bench needs ~3x dbcache size RAM)
                                  // FIXME: make this a parameter
const uint64_t N_DEFAULT_CACHE_ENTRIES = 200 * 1000;

// FIXME: dedup from src/test/test_bitcoin.h
FastRandomContext insecure_rand_ctx;
uint64_t InsecureRandRange(uint64_t range) { return insecure_rand_ctx.randrange(range); }

static std::vector<CKey> SetupDummyKeys(CBasicKeyStore& keystoreRet, int n_keys)
{
    std::vector<CKey> keys;
    keys.resize(n_keys);

    // Add some keys to the keystore:
    for (int i = 0; i < n_keys; i++) {
        keys[i].MakeNewKey(true);
        keystoreRet.AddKey(keys[i]);
    }
    
    return keys;
}

static std::vector<CTransaction> SetupDummyTransactions(std::vector<CKey> keys, int n_transactions)
{    
    std::vector<CTransaction> transactions;
    
    FastRandomContext rng(true);
    
    // Create dummy transactions
    for (int i = 0; i < n_transactions; i++) {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vout.resize(1);

        // Random input to prevent duplicate CCoinsView(Cache) entries:        
        tx.vin[0].scriptSig << std::vector<unsigned char>(65, 0) << std::vector<unsigned char>(33, 4); 
        tx.vin[0].prevout.hash = rng.rand256();
        tx.vin[0].prevout.n = 0;
        
        tx.vout[0].nValue = 10 * CENT;
        tx.vout[0].scriptPubKey << ToByteVector(keys[(i + 0) % keys.size()].GetPubKey()) << OP_CHECKSIG;     

        transactions.push_back(tx);
    }
    
    return transactions;
}

static std::vector<COutPoint>
SetupDummyCoins(CBasicKeyStore& keystoreRet, CCoinsViewCache& coinsViewCache, int n_coins, int n_keys)
{
    std::vector<COutPoint> outpoints;
    outpoints.resize(n_coins);
    
    const std::vector<CKey> keys = SetupDummyKeys(keystoreRet, n_keys);
    
    const std::vector<CTransaction> transactions = SetupDummyTransactions(keys, n_coins);
        
    for (const CTransaction tx : transactions) {
        AddCoins(coinsViewCache, tx, 0);
        outpoints.push_back(COutPoint(tx.GetHash(), 0));
    }
    
    return outpoints;
}

// Add coins to cache that doesn't exist on disk.
static void CCoinsViewCacheAddCoinFresh(benchmark::State& state)
{
    int n_keys = 1000;
    int n_txs = N_DEFAULT_CACHE_ENTRIES * N_CACHE_SCALE;
    
    // Ignore scaling parameter.
    state.m_scaling = 1;
    state.m_num_iters = n_txs;
    state.m_num_iters_left = n_txs;

    CBasicKeyStore keystore;
    
    const std::vector<CKey> keys = SetupDummyKeys(keystore, n_keys);
    
    const std::vector<CTransaction> transactions = SetupDummyTransactions(keys, n_txs);

    while (state.IsNewEval()) {
        CCoinsView coinsView;
        CCoinsViewCache coinsViewCache(&coinsView);
        
        int i = 0;
        
        state.ResetTimer();
                
        // Benchmark:
        while (state.KeepRunning()) {
            AddCoins(coinsViewCache, transactions[i], 0);
            
            if (state.IsLastIteration()) {
                // fprintf(stderr, "Cached coins: %u\n", coinsViewCache.GetCacheSize());
                // fprintf(stderr, "Cache size: %zu MiB\n", coinsViewCache.DynamicMemoryUsage() / 1024 / 1024);
                break;
            }
            
            i++;
        }
    }
}

// Flush cache
static void CCoinsViewCacheFlush(benchmark::State& state)
{
    int n_keys = 1000;
    int n_txs = N_DEFAULT_CACHE_ENTRIES * N_CACHE_SCALE;
    
    // Ignore scaling parameter.
    state.m_scaling = 1;
    state.m_num_iters = n_txs;
    state.m_num_iters_left = n_txs;

    CBasicKeyStore keystore;
    
    const std::vector<CKey> keys = SetupDummyKeys(keystore, n_keys);
    
    const std::vector<CTransaction> transactions = SetupDummyTransactions(keys, n_txs);
    

    while (state.IsNewEval()) {
        // TODO: dedup from test suite
        SelectParams(CBaseChainParams::REGTEST);
        // const CChainParams& chainparams = Params();
        ClearDatadirCache();
        fs::path pathTemp = fs::temp_directory_path() / "bench_bitcoin" / strprintf("%lu_%i", (unsigned long)GetTime(), (int)(InsecureRandRange(1 << 30)));
        fs::create_directories(pathTemp);
        gArgs.ForceSetArg("-datadir", pathTemp.string());
        // fprintf(stderr, "Path: %s\n", pathTemp.string().c_str());

        int64_t nCoinDBCache = 50 * N_CACHE_SCALE;
        pcoinsdbview.reset(new CCoinsViewDB(nCoinDBCache, false, true));
        pcoinsTip.reset(new CCoinsViewCache(pcoinsdbview.get()));

        CCoinsViewCache coinsViewCache(pcoinsdbview.get());
        FastRandomContext rng(true);
        coinsViewCache.SetBestBlock(rng.rand256());
                
        // Add coins to cache:
        for (const CTransaction tx : transactions) {
            AddCoins(coinsViewCache, tx, 0);
        }
                
        state.ResetTimer();
                
        // Benchmark:
        while (state.KeepRunning()) {
            // Flush only in the last iteration so we get the average time per transaction
            // for the flush.
            
            if (state.m_num_iters_left == 0) {
                // fprintf(stderr, "Cached coins: %u\n", coinsViewCache.GetCacheSize());
                // fprintf(stderr, "Cache size: %zu MiB\n", coinsViewCache.DynamicMemoryUsage() / 1024 / 1024);
                coinsViewCache.Flush();
                // fprintf(stderr, "Cache size: %zu MiB\n", coinsViewCache.DynamicMemoryUsage() / 1024 / 1024);
            }
            
            if (state.IsLastIteration()) break;         
        }
        
        // Clean up:
        pcoinsTip.reset();
        pcoinsdbview.reset();
        fs::remove_all(pathTemp);
    }
}


// Read coin from cache
static void CCoinsViewCacheAccess(benchmark::State& state)
{
    int n_keys = 1000;
    uint64_t n_txs = N_DEFAULT_CACHE_ENTRIES * N_CACHE_SCALE;
    CBasicKeyStore keystore;
    CCoinsView coinsDummy;
    CCoinsViewCache coins(&coinsDummy);
    std::vector<COutPoint> outpoints = SetupDummyCoins(keystore, coins, n_txs, n_keys);

    FastRandomContext rng(true);
    std::vector <uint64_t> sequence;
    sequence.resize(state.m_num_iters * state.m_num_evals);
    for (uint64_t i = 0; i < state.m_num_iters * state.m_num_evals; i++) {
        sequence[i] = rng.rand64() % outpoints.size();
    }

    // Benchmark:
    uint64_t i = 0;
    while (state.KeepRunning()) {
        coins.AccessCoin(outpoints[sequence[i]]);
        i++;
    }
}

BENCHMARK(CCoinsViewCacheAccess, 3500 * 1000);

// These ignore -scaling param:
BENCHMARK(CCoinsViewCacheAddCoinFresh, N_DEFAULT_CACHE_ENTRIES * N_CACHE_SCALE);
BENCHMARK(CCoinsViewCacheFlush, N_DEFAULT_CACHE_ENTRIES * N_CACHE_SCALE);
