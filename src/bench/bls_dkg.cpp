// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <random.h>
#include <bls/bls_worker.h>

extern CBLSWorker blsWorker;

struct Member {
    CBLSId id;

    BLSVerificationVectorPtr vvec;
    BLSSecretKeyVector skShares;
};

struct DKG
{
    std::vector<Member> members;
    std::vector<CBLSId> ids;

    std::vector<BLSVerificationVectorPtr> receivedVvecs;
    BLSSecretKeyVector receivedSkShares;

    BLSVerificationVectorPtr quorumVvec;

    DKG(int quorumSize)
    {
        members.reserve(quorumSize);
        ids.reserve(quorumSize);

        for (int i = 0; i < quorumSize; i++) {
            uint256 id;
            WriteLE64(id.begin(), i + 1);
            members.push_back({CBLSId(id), {}, {}});
            ids.emplace_back(id);
        }

        for (int i = 0; i < quorumSize; i++) {
            blsWorker.GenerateContributions(quorumSize / 2 + 1, ids, members[i].vvec, members[i].skShares);
        }

        //printf("initialized quorum %d\n", quorumSize);
    }

    void ReceiveVvecs()
    {
        receivedVvecs.clear();
        for (size_t i = 0; i < members.size(); i++) {
            receivedVvecs.emplace_back(members[i].vvec);
        }
        quorumVvec = blsWorker.BuildQuorumVerificationVector(receivedVvecs);
    }

    void ReceiveShares(size_t whoAmI)
    {
        receivedSkShares.clear();
        for (size_t i = 0; i < members.size(); i++) {
            receivedSkShares.emplace_back(members[i].skShares[whoAmI]);
        }
    }

    void BuildQuorumVerificationVector(bool parallel)
    {
        quorumVvec = blsWorker.BuildQuorumVerificationVector(receivedVvecs, 0, 0, parallel);
        //assert(worker.VerifyVerificationVector(*members[memberIdx].quorumVvec));
    }

    void Bench_BuildQuorumVerificationVectors(benchmark::Bench& bench, bool parallel)
    {
        ReceiveVvecs();

        bench.run([&] {
            BuildQuorumVerificationVector(parallel);
        });
    }

    void VerifyContributionShares(size_t whoAmI, const std::set<size_t>& invalidIndexes, bool parallel, bool aggregated)
    {
        auto result = blsWorker.VerifyContributionShares(members[whoAmI].id, receivedVvecs, receivedSkShares, parallel, aggregated);
        for (size_t i = 0; i < receivedVvecs.size(); i++) {
            if (invalidIndexes.count(i)) {
                assert(!result[i]);
            } else {
                assert(result[i]);
            }
        }
    }

    void Bench_VerifyContributionShares(benchmark::Bench& bench, int invalidCount, bool parallel, bool aggregated)
    {
        ReceiveVvecs();

        // Benchmark.
        size_t memberIdx = 0;
        bench.run([&] {
            ReceiveShares(memberIdx);

            std::set<size_t> invalidIndexes;
            for (int i = 0; i < invalidCount; i++) {
                int shareIdx = GetRandInt(receivedSkShares.size());
                receivedSkShares[shareIdx].MakeNewKey();
                invalidIndexes.emplace(shareIdx);
            }

            VerifyContributionShares(memberIdx, invalidIndexes, parallel, aggregated);

            memberIdx = (memberIdx + 1) % members.size();
        });
    }
};

std::shared_ptr<DKG> dkg10;
std::shared_ptr<DKG> dkg100;
std::shared_ptr<DKG> dkg400;

void InitIfNeeded()
{
    if (dkg10 == nullptr) {
        dkg10 = std::make_shared<DKG>(10);
    }
    if (dkg100 == nullptr) {
        dkg100 = std::make_shared<DKG>(100);
    }
    if (dkg400 == nullptr) {
        dkg400 = std::make_shared<DKG>(400);
    }
}

void CleanupBLSDkgTests()
{
    dkg10.reset();
    dkg100.reset();
    dkg400.reset();
}



#define BENCH_BuildQuorumVerificationVectors(name, quorumSize, parallel) \
    static void BLSDKG_BuildQuorumVerificationVectors_##name##_##quorumSize(benchmark::Bench& bench) \
    { \
        InitIfNeeded(); \
        dkg##quorumSize->Bench_BuildQuorumVerificationVectors(bench, parallel); \
    } \
    BENCHMARK(BLSDKG_BuildQuorumVerificationVectors_##name##_##quorumSize)

/*BENCH_BuildQuorumVerificationVectors(simple, 10, false)
BENCH_BuildQuorumVerificationVectors(simple, 100, false)
BENCH_BuildQuorumVerificationVectors(simple, 400, false)
BENCH_BuildQuorumVerificationVectors(parallel, 10, true)
BENCH_BuildQuorumVerificationVectors(parallel, 100, true)
BENCH_BuildQuorumVerificationVectors(parallel, 400, true)*/

///////////////////////////////



#define BENCH_VerifyContributionShares(name, quorumSize, invalidCount, parallel, aggregated) \
    static void BLSDKG_VerifyContributionShares_##name##_##quorumSize(benchmark::Bench& bench) \
    { \
        InitIfNeeded(); \
        dkg##quorumSize->Bench_VerifyContributionShares(bench, invalidCount, parallel, aggregated); \
    } \
    BENCHMARK(BLSDKG_VerifyContributionShares_##name##_##quorumSize)

/*BENCH_VerifyContributionShares(simple, 10, 5, false, false)
BENCH_VerifyContributionShares(simple, 100, 5, false, false)
BENCH_VerifyContributionShares(simple, 400, 5, false, false)

BENCH_VerifyContributionShares(aggregated, 10, 5, false, true)
BENCH_VerifyContributionShares(aggregated, 100, 5, false, true)
BENCH_VerifyContributionShares(aggregated, 400, 5, false, true)

BENCH_VerifyContributionShares(parallel, 10, 5, true, false)
BENCH_VerifyContributionShares(parallel, 100, 5, true, false)
BENCH_VerifyContributionShares(parallel, 400, 5, true, false)

BENCH_VerifyContributionShares(parallel_aggregated, 10, 5, true, true)
BENCH_VerifyContributionShares(parallel_aggregated, 100, 5, true, true)
BENCH_VerifyContributionShares(parallel_aggregated, 400, 5, true, true)*/
