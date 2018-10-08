// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_TEST_GEN_TRANSACTION_GEN_H
#define BITCOIN_TEST_GEN_TRANSACTION_GEN_H

#include <test/gen/crypto_gen.h>
#include <test/gen/script_gen.h>

#include <amount.h>
#include <primitives/transaction.h>
#include <script/script.h>

#include <rapidcheck/Gen.h>
#include <rapidcheck/gen/Arbitrary.h>
#include <rapidcheck/gen/Predicate.h>

using SpendingInfo = std::tuple<const CTxOut, const CTransaction, const int>;

/** A signed tx that validly spends a P2PKSPK and the input index */
rc::Gen<SpendingInfo> SignedP2PKTx();

/** A signed tx that validly spends a P2PKHSPK and the input index */
rc::Gen<SpendingInfo> SignedP2PKHTx();

/** A signed tx that validly spends a MultisigSPK and the input index */
rc::Gen<SpendingInfo> SignedMultisigTx();

/** A signed tx that validly spends a P2SHSPK and the input index */
rc::Gen<SpendingInfo> SignedP2SHTx();

/** A signed tx that validly spends a P2WPKH and the input index */
rc::Gen<SpendingInfo> SignedP2WPKHTx();

/** A signed tx that validly spends a P2WSH output and the input index */
rc::Gen<SpendingInfo> SignedP2WSHTx();

/** Generates a arbitrary validly signed tx */
rc::Gen<SpendingInfo> SignedTx();

namespace rc {
/** Generator for a COutPoint */
template <>
struct Arbitrary<COutPoint> {
    static Gen<COutPoint> arbitrary()
    {
        return gen::map(gen::tuple(gen::arbitrary<uint256>(), gen::arbitrary<uint32_t>()), [](std::tuple<uint256, uint32_t> outPointPrimitives) {
            uint32_t nIn;
            uint256 nHashIn;
            std::tie(nHashIn, nIn) = outPointPrimitives;
            return COutPoint(nHashIn, nIn);
        });
    };
};

/** Generator for a CTxIn */
template <>
struct Arbitrary<CTxIn> {
    static Gen<CTxIn> arbitrary()
    {
        return gen::map(gen::tuple(gen::arbitrary<COutPoint>(), gen::arbitrary<CScript>(), gen::arbitrary<uint32_t>()), [](const std::tuple<const COutPoint&, const CScript&, uint32_t>& primitives) {
            COutPoint outpoint;
            CScript script;
            uint32_t sequence;
            std::tie(outpoint, script, sequence) = primitives;
            return CTxIn(outpoint, script, sequence);
        });
    };
};

/** Generator for a CAmount */
template <>
struct Arbitrary<CAmount> {
    static Gen<CAmount> arbitrary()
    {
        //why doesn't this generator call work? It seems to cause an infinite loop.
        //return gen::arbitrary<int64_t>();
        return gen::inRange<int64_t>(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
    };
};

/** Generator for CTxOut */
template <>
struct Arbitrary<CTxOut> {
    static Gen<CTxOut> arbitrary()
    {
        return gen::map(gen::pair(gen::arbitrary<CAmount>(), gen::arbitrary<CScript>()), [](const std::pair<CAmount, CScript>& primitives) {
            return CTxOut(primitives.first, primitives.second);
        });
    };
};

/** Generator for a CTransaction */
template <>
struct Arbitrary<CTransaction> {
    static Gen<CTransaction> arbitrary()
    {
        return gen::map(gen::tuple(gen::arbitrary<int32_t>(),
                            gen::nonEmpty<std::vector<CTxIn>>(), gen::nonEmpty<std::vector<CTxOut>>(), gen::arbitrary<uint32_t>()),
            [](const std::tuple<int32_t, const std::vector<CTxIn>&, const std::vector<CTxOut>&, uint32_t>& primitives) {
                CMutableTransaction tx;
                int32_t nVersion;
                std::vector<CTxIn> vin;
                std::vector<CTxOut> vout;
                uint32_t locktime;
                std::tie(nVersion, vin, vout, locktime) = primitives;
                tx.nVersion = nVersion;
                tx.vin = vin;
                tx.vout = vout;
                tx.nLockTime = locktime;
                return CTransaction(tx);
            });
    };
};

/** Generator for a CTransactionRef */
template <>
struct Arbitrary<CTransactionRef> {
    static Gen<CTransactionRef> arbitrary()
    {
        return gen::map(gen::arbitrary<CTransaction>(), [](const CTransaction& tx) {
            return MakeTransactionRef(tx);
        });
    };
};
} // namespace rc
#endif
