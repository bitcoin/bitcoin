// Copyright (c) 2021-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <consensus/validation.h>
#include <evo/mnhftx.h>
#include <evo/specialtx.h>
#include <llmq/context.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <util/string.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>
#include <test/util/setup_common.h>

#include <cstdint>
#include <vector>


static CMutableTransaction CreateMNHFTx(const uint256& quorumHash, const CBLSSignature& cblSig, const uint16_t& versionBit)
{
    MNHFTxPayload extraPayload;
    extraPayload.nVersion = 1;
    extraPayload.signal.versionBit = versionBit;
    extraPayload.signal.quorumHash = quorumHash;
    extraPayload.signal.sig = cblSig;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_MNHF_SIGNAL;
    SetTxPayload(tx, extraPayload);

    return tx;
}

BOOST_FIXTURE_TEST_SUITE(evo_mnhf_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(verify_mnhf_specialtx_tests)
{
    int count = 10;
    uint16_t bit = 1;

    std::vector<CBLSSignature> vec_sigs;
    std::vector<CBLSPublicKey> vec_pks;
    std::vector<CBLSSecretKey> vec_sks;

    CBLSSecretKey sk;
    for (int i = 0; i < count; i++) {
        sk.MakeNewKey();
        vec_pks.push_back(sk.GetPublicKey());
        vec_sks.push_back(sk);
    }

    CBLSSecretKey ag_sk = CBLSSecretKey::AggregateInsecure(vec_sks);
    CBLSPublicKey ag_pk = CBLSPublicKey::AggregateInsecure(vec_pks);

    BOOST_CHECK(ag_sk.IsValid());
    BOOST_CHECK(ag_pk.IsValid());

    const bool use_legacy{false};
    uint256 verHash = uint256S(ToString(bit));
    auto sig = ag_sk.Sign(verHash, use_legacy);
    BOOST_CHECK(sig.VerifyInsecure(ag_pk, verHash, use_legacy));

    auto& chainman = Assert(m_node.chainman);
    auto& qman = *Assert(m_node.llmq_ctx)->qman;
    const CBlockIndex* pindex = chainman->ActiveChain().Tip();
    uint256 hash = GetRandHash();
    TxValidationState state;

    { // wrong quorum (we don't have any indeed)
        const CTransaction tx{CTransaction(CreateMNHFTx(hash, sig, bit))};
        CheckMNHFTx(*chainman, qman, CTransaction(tx), pindex, state);
        BOOST_CHECK_EQUAL(state.ToString(), "bad-mnhf-quorum-hash");
    }

    { // non EHF fork
        const CTransaction tx{CTransaction(CreateMNHFTx(hash, sig, 28))};
        CheckMNHFTx(*chainman, qman, CTransaction(tx), pindex, state);
        BOOST_CHECK_EQUAL(state.ToString(), "bad-mnhf-non-ehf");
    }
}

BOOST_AUTO_TEST_SUITE_END()
