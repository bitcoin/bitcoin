// Copyright (c) 2011-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <pubkey.h>
#include <script/sign.h>
#include <script/signingprovider.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <txorphanage.h>

#include <array>
#include <cstdint>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(orphanage_tests, TestingSetup)

class TxOrphanageTest : public TxOrphanage
{
public:
    inline size_t CountOrphans() const EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans)
    {
        return m_orphans.size();
    }

    CTransactionRef RandomOrphan() EXCLUSIVE_LOCKS_REQUIRED(g_cs_orphans)
    {
        std::map<uint256, OrphanTx>::iterator it;
        it = m_orphans.lower_bound(InsecureRand256());
        if (it == m_orphans.end())
            it = m_orphans.begin();
        return it->second.tx;
    }
};

static void MakeNewKeyWithFastRandomContext(CKey& key)
{
    std::vector<unsigned char> keydata;
    keydata = g_insecure_rand_ctx.randbytes(32);
    key.Set(keydata.data(), keydata.data() + keydata.size(), /*fCompressedIn=*/true);
    assert(key.IsValid());
}

BOOST_AUTO_TEST_CASE(DoS_mapOrphans)
{
    // This test had non-deterministic coverage due to
    // randomly selected seeds.
    // This seed is chosen so that all branches of the function
    // ecdsa_signature_parse_der_lax are executed during this test.
    // Specifically branches that run only when an ECDSA
    // signature's R and S values have leading zeros.
    g_insecure_rand_ctx = FastRandomContext{uint256{33}};

    TxOrphanageTest orphanage;
    CKey key;
    MakeNewKeyWithFastRandomContext(key);
    FillableSigningProvider keystore;
    BOOST_CHECK(keystore.AddKey(key));

    LOCK(g_cs_orphans);

    // 50 orphan transactions:
    for (int i = 0; i < 50; i++)
    {
        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = InsecureRand256();
        tx.vin[0].scriptSig << OP_1;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));

        orphanage.AddTx(MakeTransactionRef(tx), i);
    }

    // ... and 50 that depend on other orphans:
    for (int i = 0; i < 50; i++)
    {
        CTransactionRef txPrev = orphanage.RandomOrphan();

        CMutableTransaction tx;
        tx.vin.resize(1);
        tx.vin[0].prevout.n = 0;
        tx.vin[0].prevout.hash = txPrev->GetHash();
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        BOOST_CHECK(SignSignature(keystore, *txPrev, tx, 0, SIGHASH_ALL));

        orphanage.AddTx(MakeTransactionRef(tx), i);
    }

    // This really-big orphan should be ignored:
    for (int i = 0; i < 10; i++)
    {
        CTransactionRef txPrev = orphanage.RandomOrphan();

        CMutableTransaction tx;
        tx.vout.resize(1);
        tx.vout[0].nValue = 1*CENT;
        tx.vout[0].scriptPubKey = GetScriptForDestination(PKHash(key.GetPubKey()));
        tx.vin.resize(2777);
        for (unsigned int j = 0; j < tx.vin.size(); j++)
        {
            tx.vin[j].prevout.n = j;
            tx.vin[j].prevout.hash = txPrev->GetHash();
        }
        BOOST_CHECK(SignSignature(keystore, *txPrev, tx, 0, SIGHASH_ALL));
        // Re-use same signature for other inputs
        // (they don't have to be valid for this test)
        for (unsigned int j = 1; j < tx.vin.size(); j++)
            tx.vin[j].scriptSig = tx.vin[0].scriptSig;

        BOOST_CHECK(!orphanage.AddTx(MakeTransactionRef(tx), i));
    }

    // Test EraseOrphansFor:
    for (NodeId i = 0; i < 3; i++)
    {
        size_t sizeBefore = orphanage.CountOrphans();
        orphanage.EraseForPeer(i);
        BOOST_CHECK(orphanage.CountOrphans() < sizeBefore);
    }

    // Test LimitOrphanTxSize() function:
    orphanage.LimitOrphans(40);
    BOOST_CHECK(orphanage.CountOrphans() <= 40);
    orphanage.LimitOrphans(10);
    BOOST_CHECK(orphanage.CountOrphans() <= 10);
    orphanage.LimitOrphans(0);
    BOOST_CHECK(orphanage.CountOrphans() == 0);
}

BOOST_AUTO_TEST_CASE(GetCandidatesForBlock)
{
    constexpr CAmount tx_fee{1 * CENT};

    TxOrphanageTest orphanage;
    CKey key;
    MakeNewKeyWithFastRandomContext(key);
    CScript scriptPubKey{GetScriptForDestination(PKHash(key.GetPubKey()))};

    // Construct two transactions, the first transaction splits one input into multiple outputs
    // and the second transaction spends the last output of the first transaction (i.e. our orphan).
    // We want our orphan to try and spend from a transaction with more outputs than inputs.
    CMutableTransaction tx_input;
    tx_input.vin.resize(1);
    tx_input.vin[0].prevout.n = 0;
    tx_input.vin[0].prevout.hash = InsecureRand256();
    tx_input.vin[0].scriptSig << OP_1;
    tx_input.vout.resize(3);
    for (size_t idx{0}; idx < tx_input.vout.size(); idx++) {
        tx_input.vout[idx].nValue = 10 * CENT;
        tx_input.vout[idx].scriptPubKey = scriptPubKey;
    }
    tx_input.vout.back().nValue -= tx_fee;

    CMutableTransaction orphan;
    orphan.vin.resize(1);
    orphan.vin[0].prevout = COutPoint(tx_input.GetHash(), tx_input.vout.size() - 1);
    orphan.vin[0].scriptSig << OP_1;
    orphan.vout.resize(1);
    orphan.vout[0].nValue = tx_input.vout.back().nValue - tx_fee;
    orphan.vout[0].scriptPubKey = scriptPubKey;

    CBlock block;
    block.vtx.push_back(MakeTransactionRef(tx_input));

    // Reprocess orphans based on inclusion of input transaction in block
    std::set<uint256> orphan_work_set{};
    {
        LOCK(g_cs_orphans);
        BOOST_CHECK(orphanage.AddTx(MakeTransactionRef(orphan), /*peer=*/128));
        orphan_work_set = orphanage.GetCandidatesForBlock(block);
    }

    // Old GetCandidatesForBlock() behavior cycled through vin instead of vout and would therefore miss the
    // orphan because there are more vouts than vins in the transaction the orphan is attempting to spend.
    // Let's check to make sure this isn't happening again.
    BOOST_CHECK_EQUAL(orphan_work_set.size(), 1);
    BOOST_CHECK(orphan_work_set.count(orphan.GetHash()) > 0);
}

BOOST_AUTO_TEST_SUITE_END()
