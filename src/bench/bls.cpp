// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"
#include "random.h"
#include "bls/bls_worker.h"
#include "utiltime.h"

#include <iostream>

CBLSWorker blsWorker;

void CleanupBLSTests()
{
    blsWorker.Stop();
}

static void BuildTestVectors(size_t count, size_t invalidCount,
                             BLSPublicKeyVector& pubKeys, BLSSecretKeyVector& secKeys, BLSSignatureVector& sigs,
                             std::vector<uint256>& msgHashes,
                             std::vector<bool>& invalid)
{
    secKeys.resize(count);
    pubKeys.resize(count);
    sigs.resize(count);
    msgHashes.resize(count);

    invalid.resize(count);
    for (size_t i = 0; i < invalidCount; i++) {
        invalid[i] = true;
    }
    std::random_shuffle(invalid.begin(), invalid.end());

    for (size_t i = 0; i < count; i++) {
        secKeys[i].MakeNewKey();
        pubKeys[i] = secKeys[i].GetPublicKey();
        msgHashes[i] = GetRandHash();
        sigs[i] = secKeys[i].Sign(msgHashes[i]);

        if (invalid[i]) {
            CBLSSecretKey s;
            s.MakeNewKey();
            sigs[i] = s.Sign(msgHashes[i]);
        }
    }
}

static void BLSPubKeyAggregate_Normal(benchmark::State& state)
{
    CBLSSecretKey secKey1, secKey2;
    secKey1.MakeNewKey();
    secKey2.MakeNewKey();
    CBLSPublicKey pubKey1 = secKey1.GetPublicKey();
    CBLSPublicKey pubKey2 = secKey2.GetPublicKey();

    // Benchmark.
    while (state.KeepRunning()) {
        CBLSPublicKey k(pubKey1);
        k.AggregateInsecure(pubKey2);
    }
}

static void BLSSecKeyAggregate_Normal(benchmark::State& state)
{
    CBLSSecretKey secKey1, secKey2;
    secKey1.MakeNewKey();
    secKey2.MakeNewKey();
    CBLSPublicKey pubKey1 = secKey1.GetPublicKey();
    CBLSPublicKey pubKey2 = secKey2.GetPublicKey();

    // Benchmark.
    while (state.KeepRunning()) {
        CBLSSecretKey k(secKey1);
        k.AggregateInsecure(secKey2);
    }
}

static void BLSSign_Normal(benchmark::State& state)
{
    CBLSSecretKey secKey;
    secKey.MakeNewKey();
    CBLSPublicKey pubKey = secKey.GetPublicKey();

    // Benchmark.
    while (state.KeepRunning()) {
        uint256 hash = GetRandHash();
        secKey.Sign(hash);
    }
}

static void BLSVerify_Normal(benchmark::State& state)
{
    BLSPublicKeyVector pubKeys;
    BLSSecretKeyVector secKeys;
    BLSSignatureVector sigs;
    std::vector<uint256> msgHashes;
    std::vector<bool> invalid;
    BuildTestVectors(1000, 10, pubKeys, secKeys, sigs, msgHashes, invalid);

    // Benchmark.
    size_t i = 0;
    while (state.KeepRunning()) {
        bool valid = sigs[i].VerifyInsecure(pubKeys[i], msgHashes[i]);
        if (valid && invalid[i]) {
            std::cout << "expected invalid but it is valid" << std::endl;
            assert(false);
        } else if (!valid && !invalid[i]) {
            std::cout << "expected valid but it is invalid" << std::endl;
            assert(false);
        }
        i = (i + 1) % pubKeys.size();
    }
}


static void BLSVerify_LargeBlock(size_t txCount, benchmark::State& state)
{
    BLSPublicKeyVector pubKeys;
    BLSSecretKeyVector secKeys;
    BLSSignatureVector sigs;
    std::vector<uint256> msgHashes;
    std::vector<bool> invalid;
    BuildTestVectors(txCount, 0, pubKeys, secKeys, sigs, msgHashes, invalid);

    // Benchmark.
    while (state.KeepRunning()) {
        for (size_t i = 0; i < pubKeys.size(); i++) {
            sigs[i].VerifyInsecure(pubKeys[i], msgHashes[i]);
        }
    }
}

static void BLSVerify_LargeBlock1000(benchmark::State& state)
{
    BLSVerify_LargeBlock(1000, state);
}

static void BLSVerify_LargeBlock10000(benchmark::State& state)
{
    BLSVerify_LargeBlock(10000, state);
}

static void BLSVerify_LargeBlockSelfAggregated(size_t txCount, benchmark::State& state)
{
    BLSPublicKeyVector pubKeys;
    BLSSecretKeyVector secKeys;
    BLSSignatureVector sigs;
    std::vector<uint256> msgHashes;
    std::vector<bool> invalid;
    BuildTestVectors(txCount, 0, pubKeys, secKeys, sigs, msgHashes, invalid);

    // Benchmark.
    while (state.KeepRunning()) {
        CBLSSignature aggSig = CBLSSignature::AggregateInsecure(sigs);
        aggSig.VerifyInsecureAggregated(pubKeys, msgHashes);
    }
}

static void BLSVerify_LargeBlockSelfAggregated1000(benchmark::State& state)
{
    BLSVerify_LargeBlockSelfAggregated(1000, state);
}

static void BLSVerify_LargeBlockSelfAggregated10000(benchmark::State& state)
{
    BLSVerify_LargeBlockSelfAggregated(10000, state);
}

static void BLSVerify_LargeAggregatedBlock(size_t txCount, benchmark::State& state)
{
    BLSPublicKeyVector pubKeys;
    BLSSecretKeyVector secKeys;
    BLSSignatureVector sigs;
    std::vector<uint256> msgHashes;
    std::vector<bool> invalid;
    BuildTestVectors(txCount, 0, pubKeys, secKeys, sigs, msgHashes, invalid);

    CBLSSignature aggSig = CBLSSignature::AggregateInsecure(sigs);

    // Benchmark.
    while (state.KeepRunning()) {
        aggSig.VerifyInsecureAggregated(pubKeys, msgHashes);
    }
}

static void BLSVerify_LargeAggregatedBlock1000(benchmark::State& state)
{
    BLSVerify_LargeAggregatedBlock(1000, state);
}

static void BLSVerify_LargeAggregatedBlock10000(benchmark::State& state)
{
    BLSVerify_LargeAggregatedBlock(10000, state);
}

static void BLSVerify_LargeAggregatedBlock1000PreVerified(benchmark::State& state)
{
    BLSPublicKeyVector pubKeys;
    BLSSecretKeyVector secKeys;
    BLSSignatureVector sigs;
    std::vector<uint256> msgHashes;
    std::vector<bool> invalid;
    BuildTestVectors(1000, 0, pubKeys, secKeys, sigs, msgHashes, invalid);

    CBLSSignature aggSig = CBLSSignature::AggregateInsecure(sigs);

    std::set<size_t> prevalidated;

    while (prevalidated.size() < 900) {
        int idx = GetRandInt((int)pubKeys.size());
        if (prevalidated.count((size_t)idx)) {
            continue;
        }
        prevalidated.emplace((size_t)idx);
    }

    // Benchmark.
    while (state.KeepRunning()) {
        BLSPublicKeyVector nonvalidatedPubKeys;
        std::vector<uint256> nonvalidatedHashes;
        nonvalidatedPubKeys.reserve(pubKeys.size());
        nonvalidatedHashes.reserve(msgHashes.size());

        for (size_t i = 0; i < sigs.size(); i++) {
            if (prevalidated.count(i)) {
                continue;
            }
            nonvalidatedPubKeys.emplace_back(pubKeys[i]);
            nonvalidatedHashes.emplace_back(msgHashes[i]);
        }

        CBLSSignature aggSigCopy = aggSig;
        for (auto idx : prevalidated) {
            aggSigCopy.SubInsecure(sigs[idx]);
        }

        bool valid = aggSigCopy.VerifyInsecureAggregated(nonvalidatedPubKeys, nonvalidatedHashes);
        assert(valid);
    }
}

static void BLSVerify_Batched(benchmark::State& state)
{
    BLSPublicKeyVector pubKeys;
    BLSSecretKeyVector secKeys;
    BLSSignatureVector sigs;
    std::vector<uint256> msgHashes;
    std::vector<bool> invalid;
    BuildTestVectors(1000, 10, pubKeys, secKeys, sigs, msgHashes, invalid);

    // Benchmark.
    size_t i = 0;
    size_t j = 0;
    size_t batchSize = 16;
    while (state.KeepRunning()) {
        j++;
        if ((j % batchSize) != 0) {
            continue;
        }

        BLSPublicKeyVector testPubKeys;
        BLSSignatureVector testSigs;
        std::vector<uint256> testMsgHashes;
        testPubKeys.reserve(batchSize);
        testSigs.reserve(batchSize);
        testMsgHashes.reserve(batchSize);
        size_t startI = i;
        for (size_t k = 0; k < batchSize; k++) {
            testPubKeys.emplace_back(pubKeys[i]);
            testSigs.emplace_back(sigs[i]);
            testMsgHashes.emplace_back(msgHashes[i]);
            i = (i + 1) % pubKeys.size();
        }

        CBLSSignature batchSig = CBLSSignature::AggregateInsecure(testSigs);
        bool batchValid = batchSig.VerifyInsecureAggregated(testPubKeys, testMsgHashes);
        std::vector<bool> valid;
        if (batchValid) {
            valid.assign(batchSize, true);
        } else {
            for (size_t k = 0; k < batchSize; k++) {
                bool valid1 = testSigs[k].VerifyInsecure(testPubKeys[k], testMsgHashes[k]);
                valid.emplace_back(valid1);
            }
        }
        for (size_t k = 0; k < batchSize; k++) {
            if (valid[k] && invalid[(startI + k) % pubKeys.size()]) {
                std::cout << "expected invalid but it is valid" << std::endl;
                assert(false);
            } else if (!valid[k] && !invalid[(startI + k) % pubKeys.size()]) {
                std::cout << "expected valid but it is invalid" << std::endl;
                assert(false);
            }
        }
    }
}

static void BLSVerify_BatchedParallel(benchmark::State& state)
{
    BLSPublicKeyVector pubKeys;
    BLSSecretKeyVector secKeys;
    BLSSignatureVector sigs;
    std::vector<uint256> msgHashes;
    std::vector<bool> invalid;
    BuildTestVectors(1000, 10, pubKeys, secKeys, sigs, msgHashes, invalid);

    std::list<std::pair<size_t, std::future<bool>>> futures;

    volatile bool cancel = false;
    auto cancelCond = [&]() {
        return cancel;
    };

    // Benchmark.
    size_t i = 0;
    while (state.KeepRunning()) {
        if (futures.size() < 100) {
            while (futures.size() < 10000) {
                auto f = blsWorker.AsyncVerifySig(sigs[i], pubKeys[i], msgHashes[i], cancelCond);
                futures.emplace_back(std::make_pair(i, std::move(f)));
                i = (i + 1) % pubKeys.size();
            }
        }

        auto fp = std::move(futures.front());
        futures.pop_front();

        size_t j = fp.first;
        bool valid = fp.second.get();

        if (valid && invalid[j]) {
            std::cout << "expected invalid but it is valid" << std::endl;
            assert(false);
        } else if (!valid && !invalid[j]) {
            std::cout << "expected valid but it is invalid" << std::endl;
            assert(false);
        }
    }
    cancel = true;
    while (blsWorker.IsAsyncVerifyInProgress()) {
        MilliSleep(100);
    }
}

BENCHMARK(BLSPubKeyAggregate_Normal)
BENCHMARK(BLSSecKeyAggregate_Normal)
BENCHMARK(BLSSign_Normal)
BENCHMARK(BLSVerify_Normal)
BENCHMARK(BLSVerify_LargeBlock1000)
BENCHMARK(BLSVerify_LargeBlockSelfAggregated1000)
BENCHMARK(BLSVerify_LargeBlockSelfAggregated10000)
BENCHMARK(BLSVerify_LargeAggregatedBlock1000)
BENCHMARK(BLSVerify_LargeAggregatedBlock10000)
BENCHMARK(BLSVerify_LargeAggregatedBlock1000PreVerified)
BENCHMARK(BLSVerify_Batched)
BENCHMARK(BLSVerify_BatchedParallel)
