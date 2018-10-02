// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_TEST_GEN_CRYPTO_GEN_H
#define BITCOIN_TEST_GEN_CRYPTO_GEN_H

#include <key.h>
#include <random.h>
#include <uint256.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Create.h>
#include <rapidcheck/gen/Numeric.h>

/** Generates 1 to 15 keys for OP_CHECKMULTISIG */
rc::Gen<std::vector<CKey>> MultisigKeys();

namespace rc
{
/** Generator for a new CKey */
template <>
struct Arbitrary<CKey> {
    static Gen<CKey> arbitrary()
    {
        return rc::gen::map<int>([](int x) {
            CKey key;
            key.MakeNewKey(true);
            return key;
        });
    };
};

/** Generator for a CPrivKey */
template <>
struct Arbitrary<CPrivKey> {
    static Gen<CPrivKey> arbitrary()
    {
        return gen::map(gen::arbitrary<CKey>(), [](const CKey& key) {
            return key.GetPrivKey();
        });
    };
};

/** Generator for a new CPubKey */
template <>
struct Arbitrary<CPubKey> {
    static Gen<CPubKey> arbitrary()
    {
        return gen::map(gen::arbitrary<CKey>(), [](const CKey& key) {
            return key.GetPubKey();
        });
    };
};
/** Generates a arbitrary uint256 */
template <>
struct Arbitrary<uint256> {
    static Gen<uint256> arbitrary()
    {
        return rc::gen::just(GetRandHash());
    };
};
} //namespace rc
#endif
