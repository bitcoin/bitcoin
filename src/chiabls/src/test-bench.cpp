// Copyright 2018 Chia Network Inc

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//    http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <array>

#include "bls.hpp"
#include "test-utils.hpp"

using std::string;
using std::vector;
using std::cout;
using std::endl;

using namespace bls;

void benchSigs() {
    string testName = "Sigining";
    double numIters = 1000;
    uint8_t seed[32];
    getRandomSeed(seed);
    PrivateKey sk = PrivateKey::FromSeed(seed, 32);
    PublicKey pk = sk.GetPublicKey();
    uint8_t message1[48];
    pk.Serialize(message1);

    auto start = startStopwatch();

    for (size_t i = 0; i < numIters; i++) {
        sk.Sign(message1, sizeof(message1));
    }
    endStopwatch(testName, start, numIters);
}

void benchVerification() {
    string testName = "Verification";
    double numIters = 1000;
    uint8_t seed[32];
    getRandomSeed(seed);
    PrivateKey sk = PrivateKey::FromSeed(seed, 32);

    std::vector<Signature> sigs;

    for (size_t i = 0; i < numIters; i++) {
        uint8_t message[4];
        Util::IntToFourBytes(message, i);
        sigs.push_back(sk.Sign(message, 4));
    }

    auto start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        uint8_t message[4];
        Util::IntToFourBytes(message, i);
        bool ok = sigs[i].Verify();
        ASSERT(ok);
    }
    endStopwatch(testName, start, numIters);
}

void benchAggregateSigsSecure() {
    uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
    double numIters = 1000;

    std::vector<PrivateKey> sks;
    std::vector<PublicKey> pks;
    std::vector<Signature> sigs;

    for (int i = 0; i < numIters; i++) {
        uint8_t seed[32];
        getRandomSeed(seed);

        PrivateKey sk = PrivateKey::FromSeed(seed, 32);
        const PublicKey pk = sk.GetPublicKey();
        sks.push_back(sk);
        pks.push_back(pk);
        sigs.push_back(sk.Sign(message1, sizeof(message1)));
    }

    auto start = startStopwatch();
    Signature aggSig = Signature::AggregateSigs(sigs);
    endStopwatch("Generate aggregate signature, same message",
                 start, numIters);

    auto start2 = startStopwatch();
    const PublicKey aggPubKey = PublicKey::Aggregate(pks);
    endStopwatch("Generate aggregate pk, same message", start2, numIters);

    auto start3 = startStopwatch();
    aggSig.SetAggregationInfo(AggregationInfo::FromMsg(
            aggPubKey, message1, sizeof(message1)));
    ASSERT(aggSig.Verify());
    endStopwatch("Verify agg signature, same message", start3, numIters);
}

void benchBatchVerification() {
    string testName = "Batch verification";
    double numIters = 1000;

    std::vector<Signature> sigs;
    std::vector<Signature> cache;
    for (size_t i = 0; i < numIters; i++) {
        uint8_t seed[32];
        getRandomSeed(seed);

        PrivateKey sk = PrivateKey::FromSeed(seed, 32);
        uint8_t *message = new uint8_t[32];
        getRandomSeed(message);
        sigs.push_back(sk.Sign(message, 1 + (i % 5)));
        // Small message, so some messages are the same
        if (message[0] < 225) {  // Simulate having ~90% cached transactions
            sigs.back().Verify();
            cache.push_back(sigs.back());
        }
    }

    Signature aggregate = Signature::AggregateSigs(sigs);

    auto start = startStopwatch();
    ASSERT(aggregate.Verify());
    endStopwatch(testName, start, numIters);


    start = startStopwatch();
    const Signature aggSmall = aggregate.DivideBy(cache);
    ASSERT(aggSmall.Verify());
    endStopwatch(testName + " with cached verifications", start, numIters);
}

void benchAggregateSigsSimple() {
    double numIters = 1000;
    std::vector<PrivateKey> sks;
    std::vector<Signature> sigs;

    for (int i = 0; i < numIters; i++) {
        uint8_t* message = new uint8_t[48];
        uint8_t seed[32];
        getRandomSeed(seed);

        PrivateKey sk = PrivateKey::FromSeed(seed, 32);
        const PublicKey pk = sk.GetPublicKey();
        pk.Serialize(message);
        sks.push_back(sk);
        sigs.push_back(sk.Sign(message, sizeof(message)));
    }

    auto start = startStopwatch();
    Signature aggSig = Signature::AggregateSigs(sigs);
    endStopwatch("Generate aggregate signature, distinct messages",
                 start, numIters);

    auto start2 = startStopwatch();
    ASSERT(aggSig.Verify());
    endStopwatch("Verify aggregate signature, distinct messages",
                 start2, numIters);
}

void benchDegenerateTree() {
    double numIters = 30;
    uint8_t message1[7] = {100, 2, 254, 88, 90, 45, 23};
    uint8_t seed[32];
    getRandomSeed(seed);
    PrivateKey sk1 = PrivateKey::FromSeed(seed, 32);
    Signature aggSig = sk1.Sign(message1, sizeof(message1));

    auto start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        getRandomSeed(seed);
        PrivateKey sk = PrivateKey::FromSeed(seed, 32);
        Signature sig = sk.Sign(message1, sizeof(message1));
        std::vector<Signature> sigs = {aggSig, sig};
        aggSig = Signature::AggregateSigs(sigs);
    }
    endStopwatch("Generate degenerate aggSig tree",
                 start, numIters);

    start = startStopwatch();
    ASSERT(aggSig.Verify());
    endStopwatch("Verify degenerate aggSig tree",
                 start, numIters);
}

void benchShamir() {
    size_t m = 51;
    size_t n = 100;
    double numIters = 100;

    std::vector<PrivateKey> sks;
    std::vector<PublicKey> pks;
    std::vector<InsecureSignature> sigs;
    std::vector<std::array<uint8_t, BLS::ID_SIZE>> ids(m);
    std::vector<PrivateKey> skShares;
    std::vector<PublicKey> pkShares;
    std::vector<InsecureSignature> sigShares;

    uint8_t message[32];
    getRandomSeed(message);

    for (size_t i = 0; i < m; i++) {
        getRandomSeed(ids[i].data());
    }

    for (size_t i = 0; i < n; i++) {
        uint8_t buf[32];
        getRandomSeed(buf);

        PrivateKey sk = PrivateKey::FromSeed(buf, 32);
        sks.push_back(sk);
        pks.push_back(sk.GetPublicKey());
        sigs.push_back(sk.SignInsecurePrehashed(message));
    }

    auto start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        BLS::PrivateKeyShare(sks, ids[i % m].data());
    }
    endStopwatch("Create private key share",
                 start, numIters);

    start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        BLS::PublicKeyShare(pks, ids[i % m].data());
    }
    endStopwatch("Create public key share",
                 start, numIters);

    start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        BLS::SignatureShare(sigs, ids[i % m].data());
    }
    endStopwatch("Create signature share",
                 start, numIters);

    for (size_t i = 0; i < m; i++) {
        PrivateKey skShare = BLS::PrivateKeyShare(sks, ids[i].data());
        PublicKey pkShare = BLS::PublicKeyShare(pks, ids[i].data());
        InsecureSignature sigShare1 = BLS::SignatureShare(sigs, ids[i].data());

        skShares.emplace_back(skShare);
        pkShares.emplace_back(pkShare);
        sigShares.emplace_back(sigShare1);
    }

    std::vector<PrivateKey> rsks;
    std::vector<PublicKey> rpks;
    std::vector<InsecureSignature> rsigs;
    std::vector<const uint8_t*> rids;

    auto prepare = [&](size_t i, int which) {
        rsks.clear(); rpks.clear(); rsigs.clear(); rids.clear();
        for (size_t j = 0; j < m; j++) {
            switch (which) {
                case 0:
                    rsks.emplace_back(skShares[(i + j) % m]);
                    break;
                case 1:
                    rpks.emplace_back(pkShares[(i + j) % m]);
                    break;
                case 2:
                    rsigs.emplace_back(sigShares[(i + j) % m]);
                    break;
                default:
                    ASSERT(false);
            }
            rids.emplace_back(ids[(i + j) % m].data());
        }
    };


    start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        prepare(i, 0);
        BLS::RecoverPrivateKey(rsks, rids);
    }
    endStopwatch("Recover private key",
                 start, numIters);

    start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        prepare(i, 1);
        BLS::RecoverPublicKey(rpks, rids);
    }
    endStopwatch("Recover public key",
                 start, numIters);

    start = startStopwatch();
    for (size_t i = 0; i < numIters; i++) {
        prepare(i, 2);
        BLS::RecoverSig(rsigs, rids);
    }
    endStopwatch("Recover signature",
                 start, numIters);
}

int main(int argc, char* argv[]) {
    benchSigs();
    benchVerification();
    benchBatchVerification();
    benchAggregateSigsSecure();
    benchAggregateSigsSimple();
    benchDegenerateTree();
    benchShamir();
}
