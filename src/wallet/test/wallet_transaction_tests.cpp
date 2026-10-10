// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/transaction.h>

#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <test/util/common.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(wallet_transaction_tests, WalletTestingSetup)

BOOST_AUTO_TEST_CASE(roundtrip)
{
    for (uint8_t hash = 0; hash < 5; ++hash) {
        for (int index = -2; index < 3; ++index) {
            TxState state = TxStateInterpretSerialized(TxStateUnrecognized{uint256{hash}, index});
            BOOST_CHECK_EQUAL(TxStateSerializedBlockHash(state), uint256{hash});
            BOOST_CHECK_EQUAL(TxStateSerializedIndex(state), index);
        }
    }
}

BOOST_AUTO_TEST_CASE(deserialize_rejects_mismatched_variant_txid)
{
    // Build tx_a and serialise it as a CWalletTx.
    // Needs at least one input: a zero-input tx serialises vin_count as 0x00,
    // which the witness-aware deserialiser misreads as the segwit marker byte.
    CMutableTransaction mtx_a;
    mtx_a.vin.emplace_back(COutPoint{Txid::FromUint256(uint256::ONE), 0});
    mtx_a.vout.emplace_back(COIN, CScript() << OP_TRUE);
    CTransactionRef tx_a = MakeTransactionRef(std::move(mtx_a));
    CWalletTx wtx_a{tx_a, TxStateInactive{}};
    DataStream ss;
    ss << wtx_a;

    // Build tx_b with a different txid to use as a bogus variant.
    CMutableTransaction mtx_b;
    mtx_b.vout.emplace_back(2 * COIN, CScript() << OP_TRUE);
    CTransactionRef tx_b = MakeTransactionRef(std::move(mtx_b));
    BOOST_REQUIRE(tx_b->GetHash() != tx_a->GetHash());

    // A variant whose txid doesn't match the canonical txid must be rejected.
    std::map<Wtxid, CTransactionRef> bad_variants{{tx_b->GetWitnessHash(), tx_b}};
    BOOST_CHECK_EXCEPTION(CWalletTx(deserialize, ss, bad_variants), std::runtime_error, HasReason{"variant txid does not match wallet txid"});
}

BOOST_AUTO_TEST_CASE(silent_payments_flag_roundtrip)
{
    CMutableTransaction mtx;
    mtx.vin.emplace_back(COutPoint{Txid::FromUint256(uint256::ONE), 0});
    mtx.vout.emplace_back(COIN, CScript() << OP_TRUE);
    const CTransactionRef tx{MakeTransactionRef(std::move(mtx))};
    const Txid replacement{Txid::FromUint256(uint256{2})};
    const std::map<Wtxid, CTransactionRef> no_variants;

    // Unserialize throws on unknown string values, so a successful roundtrip also checks that the
    // flag is not written as a new value, which releases that throw on them could not load
    const auto roundtrip{[&](const CWalletTx& wtx) {
        DataStream ss;
        ss << wtx;
        return CWalletTx(deserialize, ss, no_variants);
    }};

    // A silent payments tx is written as replaced by itself, which is read back as the flag
    CWalletTx sp_wtx{tx, TxStateInactive{}};
    sp_wtx.m_is_sp_tx = true;
    const CWalletTx sp_loaded{roundtrip(sp_wtx)};
    BOOST_CHECK(sp_loaded.IsSilentPaymentsTx());
    BOOST_CHECK(!sp_loaded.m_replaced_by_txid);

    // Once replaced, the replacement is kept
    sp_wtx.m_replaced_by_txid = replacement;
    const CWalletTx replaced_loaded{roundtrip(sp_wtx)};
    BOOST_CHECK(replaced_loaded.m_replaced_by_txid == replacement);

    // Other txs are not flagged
    CWalletTx wtx{tx, TxStateInactive{}};
    const CWalletTx loaded{roundtrip(wtx)};
    BOOST_CHECK(!loaded.IsSilentPaymentsTx());
    BOOST_CHECK(!loaded.m_replaced_by_txid);
    wtx.m_replaced_by_txid = replacement;
    BOOST_CHECK(roundtrip(wtx).m_replaced_by_txid == replacement);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
