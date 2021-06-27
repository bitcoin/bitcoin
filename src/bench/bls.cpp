// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <random.h>
#include <bls/bls_worker.h>
#include <util/time.h>

#include <iostream>

CBLSWorker blsWorker;

void InitBLSTests()
{
    blsWorker.Start();
}

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
    Shuffle(invalid.begin(), invalid.end(), FastRandomContext());

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

static void BLS_PubKeyAggregate_Normal(benchmark::State& state)
{
    CBLSSecretKey secKey1, secKey2;
    secKey1.MakeNewKey();
    secKey2.MakeNewKey();
    CBLSPublicKey pubKey1 = secKey1.GetPublicKey();
    CBLSPublicKey pubKey2 = secKey2.GetPublicKey();

    // Benchmark.
    while (state.KeepRunning()) {
        pubKey1.AggregateInsecure(pubKey2);
    }
}

static void BLS_SecKeyAggregate_Normal(benchmark::State& state)
{
    CBLSSecretKey secKey1, secKey2;
    secKey1.MakeNewKey();
    secKey2.MakeNewKey();

    // Benchmark.
    while (state.KeepRunning()) {
        secKey1.AggregateInsecure(secKey2);
    }
}

static void BLS_SignatureAggregate_Normal(benchmark::State& state)
{
    uint256 hash = GetRandHash();
    CBLSSecretKey secKey1, secKey2;
    secKey1.MakeNewKey();
    secKey2.MakeNewKey();
    CBLSSignature sig1 = secKey1.Sign(hash);
    CBLSSignature sig2 = secKey2.Sign(hash);

    // Benchmark.
    while (state.KeepRunning()) {
        sig1.AggregateInsecure(sig2);
    }
}

static void BLS_Sign_Normal(benchmark::State& state)
{
    CBLSSecretKey secKey;
    secKey.MakeNewKey();

    // Benchmark.
    while (state.KeepRunning()) {
        uint256 hash = GetRandHash();
        secKey.Sign(hash);
    }
}

static void BLS_Verify_Normal(benchmark::State& state)
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


static void BLS_Verify_LargeBlock(size_t txCount, benchmark::State& state)
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

static void BLS_Verify_LargeBlock100(benchmark::State& state)
{
    BLS_Verify_LargeBlock(100, state);
}

static void BLS_Verify_LargeBlock1000(benchmark::State& state)
{
    BLS_Verify_LargeBlock(1000, state);
}

static void BLS_Verify_LargeBlockSelfAggregated(size_t txCount, benchmark::State& state)
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

static void BLS_Verify_LargeBlockSelfAggregated100(benchmark::State& state)
{
    BLS_Verify_LargeBlockSelfAggregated(100, state);
}

static void BLS_Verify_LargeBlockSelfAggregated1000(benchmark::State& state)
{
    BLS_Verify_LargeBlockSelfAggregated(1000, state);
}

static void BLS_Verify_LargeAggregatedBlock(size_t txCount, benchmark::State& state)
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

static void BLS_Verify_LargeAggregatedBlock100(benchmark::State& state)
{
    BLS_Verify_LargeAggregatedBlock(100, state);
}

static void BLS_Verify_LargeAggregatedBlock1000(benchmark::State& state)
{
    BLS_Verify_LargeAggregatedBlock(1000, state);
}

static void BLS_Verify_LargeAggregatedBlock1000PreVerified(benchmark::State& state)
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

static void BLS_Verify_Batched(benchmark::State& state)
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

static void BLS_Verify_BatchedParallel(benchmark::State& state)
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

BENCHMARK(BLS_PubKeyAggregate_Normal, 700 * 1000)
BENCHMARK(BLS_SecKeyAggregate_Normal, 1300 * 1000)
BENCHMARK(BLS_SignatureAggregate_Normal, 300 * 1000)
BENCHMARK(BLS_Sign_Normal, 600)
BENCHMARK(BLS_Verify_Normal, 350)
BENCHMARK(BLS_Verify_LargeBlock100, 3)
BENCHMARK(BLS_Verify_LargeBlock1000, 1)
BENCHMARK(BLS_Verify_LargeBlockSelfAggregated100, 7)
BENCHMARK(BLS_Verify_LargeBlockSelfAggregated1000, 1)
BENCHMARK(BLS_Verify_LargeAggregatedBlock100, 7)
BENCHMARK(BLS_Verify_LargeAggregatedBlock1000, 1)
BENCHMARK(BLS_Verify_LargeAggregatedBlock1000PreVerified, 7)
BENCHMARK(BLS_Verify_Batched, 500)
BENCHMARK(BLS_Verify_BatchedParallel, 1000)
