// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <bls/bls.h>
#include <coinjoin/coinjoin.h>
#include <masternode/node.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coinjoin_queue_tests, BasicTestingSetup)

static CBLSSecretKey MakeSecretKey()
{
    // Generate a dummy operator keypair for signing
    CBLSSecretKey sk;
    sk.MakeNewKey();
    return sk;
}

BOOST_AUTO_TEST_CASE(queue_sign_and_verify)
{
    // Build active MN manager with operator key using node context wiring
    CActiveMasternodeManager mn_activeman(MakeSecretKey(), *Assert(m_node.connman), m_node.dmnman);

    CCoinJoinQueue q;
    q.nDenom = CoinJoin::AmountToDenomination(CoinJoin::GetSmallestDenomination());
    q.masternodeOutpoint = COutPoint(uint256S("aa"), 1);
    q.m_protxHash = uint256::ONE;
    q.nTime = GetAdjustedTime();
    q.fReady = false;

    // Sign and verify with corresponding pubkey
    BOOST_CHECK(q.Sign(mn_activeman));
    const CBLSPublicKey pub = mn_activeman.GetPubKey();
    BOOST_CHECK(q.CheckSignature(pub));
}

BOOST_AUTO_TEST_CASE(queue_hashes_and_equality)
{
    CCoinJoinQueue a, b;
    a.nDenom = b.nDenom = CoinJoin::AmountToDenomination(CoinJoin::GetSmallestDenomination());
    a.masternodeOutpoint = b.masternodeOutpoint = COutPoint(uint256S("bb"), 2);
    a.m_protxHash = b.m_protxHash = uint256::ONE;
    a.nTime = b.nTime = GetAdjustedTime();
    a.fReady = b.fReady = true;

    BOOST_CHECK(a == b);
    BOOST_CHECK(a.GetHash() == b.GetHash());
    BOOST_CHECK(a.GetSignatureHash() == b.GetSignatureHash());
}

BOOST_AUTO_TEST_SUITE_END()
