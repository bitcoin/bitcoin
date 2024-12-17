// Copyright 2020 Chia Network Inc

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
#include "bls.hpp"
#include "test-utils.hpp"

extern "C" {
#include "relic.h"
}

using std::string;
using std::vector;
using std::cout;
using std::endl;

using namespace bls;


void benchSigs() {
    string testName = "Signing";
    const int numIters = 5000;
    PrivateKey sk = BasicSchemeMPL().KeyGen(getRandomSeed());
    vector<uint8_t> message1 = sk.GetG1Element().Serialize();

    auto start = startStopwatch();

    for (int i = 0; i < numIters; i++) {
        BasicSchemeMPL().Sign(sk, message1);
    }
    endStopwatch(testName, start, numIters);
}

void benchVerification() {
    string testName = "Verification";
    const int numIters = 1000;
    PrivateKey sk = BasicSchemeMPL().KeyGen(getRandomSeed());
    G1Element pk = sk.GetG1Element();

    std::vector<G2Element> sigs;

    for (int i = 0; i < numIters; i++) {
        uint8_t message[4];
        Util::IntToFourBytes(message, i);
        vector<uint8_t> messageBytes(message, message + 4);
        sigs.push_back(BasicSchemeMPL().Sign(sk, messageBytes));
    }

    auto start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        uint8_t message[4];
        Util::IntToFourBytes(message, i);
        vector<uint8_t> messageBytes(message, message + 4);
        bool ok = BasicSchemeMPL().Verify(pk, messageBytes, sigs[i]);
        ASSERT(ok);
    }
    endStopwatch(testName, start, numIters);
}

void benchBatchVerification() {
    const int numIters = 10000;

    vector<vector<uint8_t>> sig_bytes;
    vector<vector<uint8_t>> pk_bytes;
    vector<vector<uint8_t>> ms;

    auto start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        uint8_t message[4];
        Util::IntToFourBytes(message, i);
        vector<uint8_t> messageBytes(message, message + 4);
        PrivateKey sk = BasicSchemeMPL().KeyGen(getRandomSeed());
        G1Element pk = sk.GetG1Element();
        sig_bytes.push_back(BasicSchemeMPL().Sign(sk, messageBytes).Serialize());
        pk_bytes.push_back(pk.Serialize());
        ms.push_back(messageBytes);
    }
    endStopwatch("Batch verification preparation", start, numIters);

    vector<G1Element> pks;
    pks.reserve(numIters);

    start = startStopwatch();
    for (auto const& pk : pk_bytes) {
        pks.emplace_back(G1Element::FromBytes(Bytes(pk)));
    }
    endStopwatch("Public key validation", start, numIters);

    vector<G2Element> sigs;
    sigs.reserve(numIters);

    start = startStopwatch();
    for (auto const& sig : sig_bytes) {
        sigs.emplace_back(G2Element::FromBytes(Bytes(sig)));
    }
    endStopwatch("Signature validation", start, numIters);

    start = startStopwatch();
    G2Element aggSig = BasicSchemeMPL().Aggregate(sigs);
    endStopwatch("Aggregation", start, numIters);

    start = startStopwatch();
    bool ok = BasicSchemeMPL().AggregateVerify(pks, ms, aggSig);
    ASSERT(ok);
    endStopwatch("Batch verification", start, numIters);
}

void benchSerialize() {
    const int numIters = 5000000;
    PrivateKey sk = BasicSchemeMPL().KeyGen(getRandomSeed());
    G1Element pk = sk.GetG1Element();
    vector<uint8_t> message = sk.GetG1Element().Serialize();
    G2Element sig = BasicSchemeMPL().Sign(sk, message);

    auto start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        sk.Serialize();
    }
    endStopwatch("Serialize PrivateKey", start, numIters);

    start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        pk.Serialize();
    }
    endStopwatch("Serialize G1Element", start, numIters);

    start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        sig.Serialize();
    }
    endStopwatch("Serialize G2Element", start, numIters);
}

void benchSerializeToArray() {
    const int numIters = 5000000;
    PrivateKey sk = BasicSchemeMPL().KeyGen(getRandomSeed());
    G1Element pk = sk.GetG1Element();
    vector<uint8_t> message = sk.GetG1Element().Serialize();
    G2Element sig = BasicSchemeMPL().Sign(sk, message);

    auto start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        sk.SerializeToArray();
    }
    endStopwatch("SerializeToArray PrivateKey", start, numIters);

    start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        pk.SerializeToArray();
    }
    endStopwatch("SerializeToArray G1Element", start, numIters);

    start = startStopwatch();
    for (int i = 0; i < numIters; i++) {
        sig.SerializeToArray();
    }
    endStopwatch("SerializeToArray G2Element", start, numIters);
}

int main(int argc, char* argv[]) {
    benchSigs();
    benchVerification();
    benchBatchVerification();
    benchSerialize();
    benchSerializeToArray();
}
