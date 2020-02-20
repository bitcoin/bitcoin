#include <boost/test/unit_test.hpp>

#include <chainparams.h>
#include <consensus/validation.h>
#include <miner.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <test/util/setup_common.h>
#include <txmempool.h>
#include <validation.h>
#include <wallet/wallet.h>

#include <fakeit.hpp>
#include <vbk/init.hpp>
#include <vbk/pop_service/pop_service_impl.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>

#include <algorithm>
#include <vector>

using namespace fakeit;

BOOST_AUTO_TEST_SUITE(updated_mempool_tests)

struct BlockAssemblerTest : public BlockAssembler {
    BlockAssemblerTest(const CChainParams& params) : BlockAssembler(params) {}

    template <typename MempoolComparatorTagName>
    void addPackageTxs(int& nPackagesSelected, int& nDescendantsUpdated)
    {
        LOCK2(cs_main, mempool.cs);
        BlockAssembler::addPackageTxs<MempoolComparatorTagName>(nPackagesSelected, nDescendantsUpdated);
    }

    void prepareBlockTest()
    {
        BlockAssembler::resetBlock();
        pblocktemplate.reset(new CBlockTemplate());
        pblock = &pblocktemplate->block;
    }

    CBlock getBlock()
    {
        return *this->pblock;
    }
};

template <typename name>
static void CheckSort(CTxMemPool& pool, std::vector<std::string>& sortedOrder) EXCLUSIVE_LOCKS_REQUIRED(pool.cs)
{
    BOOST_CHECK_EQUAL(pool.size(), sortedOrder.size());
    typename CTxMemPool::indexed_transaction_set::index<name>::type::iterator it = pool.mapTx.get<name>().begin();
    int count = 0;
    for (; it != pool.mapTx.get<name>().end(); ++it, ++count) {
        BOOST_CHECK_EQUAL(it->GetTx().GetHash().ToString(), sortedOrder[count]);
    }
}

BOOST_AUTO_TEST_CASE(MempoolPoPPriorityIndexing)
{
    CTxMemPool pool;
    TestMemPoolEntryHelper entry;

    /* 3rd highest fee */
    CMutableTransaction tx1 = CMutableTransaction();
    tx1.vout.resize(1);
    tx1.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx1.vout[0].nValue = 10 * COIN;
    {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(entry.Fee(10000LL).FromTx(tx1));
    }

    /* highest fee */
    CMutableTransaction tx2 = CMutableTransaction();
    tx2.vout.resize(1);
    tx2.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx2.vout[0].nValue = 2 * COIN;
    {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(entry.Fee(20000LL).FromTx(tx2));
    }

    /* lowest fee */
    CMutableTransaction tx3 = CMutableTransaction();
    tx3.vout.resize(1);
    tx3.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx3.vout[0].nValue = 5 * COIN;
    {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(entry.Fee(0LL).FromTx(tx3));
    }

    /* 2nd highest fee */
    CMutableTransaction tx4 = CMutableTransaction();
    tx4.vout.resize(1);
    tx4.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx4.vout[0].nValue = 6 * COIN;
    {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(entry.Fee(15000LL).FromTx(tx4));
    }

    /* equal fee rate to tx1, but newer */
    CMutableTransaction tx5 = CMutableTransaction();
    tx5.vout.resize(1);
    tx5.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
    tx5.vout[0].nValue = 11 * COIN;
    {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(entry.Fee(10000LL).FromTx(tx5));
        BOOST_CHECK_EQUAL(pool.size(), 5U);
    }

    std::vector<std::string> sortedOrder;
    sortedOrder.resize(5);
    sortedOrder[0] = tx2.GetHash().ToString(); // 20000
    sortedOrder[1] = tx4.GetHash().ToString(); // 15000
    // tx1 and tx5 are both 10000
    // Ties are broken by hash, not timestamp, so determine which
    // hash comes first.
    if (tx1.GetHash() < tx5.GetHash()) {
        sortedOrder[2] = tx1.GetHash().ToString();
        sortedOrder[3] = tx5.GetHash().ToString();
    } else {
        sortedOrder[2] = tx5.GetHash().ToString();
        sortedOrder[3] = tx1.GetHash().ToString();
    }
    sortedOrder[4] = tx3.GetHash().ToString(); // 0

    {
        LOCK(pool.cs);
        CheckSort<ancestor_score>(pool, sortedOrder);
    }

    CMutableTransaction popTx1 = VeriBlockTest::makePopTx({1}, {{1, 2, 3}});
    {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(entry.Fee(0LL).FromTx(popTx1));
    }
    sortedOrder.insert(sortedOrder.begin(), popTx1.GetHash().ToString());


    CMutableTransaction popTx2 = VeriBlockTest::makePopTx({2}, {{1, 2, 3}});

    {
        LOCK2(cs_main, pool.cs);
        pool.addUnchecked(entry.Fee(0LL).FromTx(popTx2));
    }
    sortedOrder.insert(sortedOrder.begin(), popTx2.GetHash().ToString());

    {
        LOCK(pool.cs);
        CheckSort<VeriBlock::poptx_priority<ancestor_score>>(pool, sortedOrder);
    }
}

BOOST_FIXTURE_TEST_CASE(AcceptToMemoryPool_pop_tx_test, TestingSetup)
{
    VeriBlockTest::ServicesFixture service_fixture;

    CMutableTransaction popTx1 = VeriBlockTest::makePopTx({1}, {std::vector<uint8_t>(100, 2)});
    CMutableTransaction popTx2 = VeriBlockTest::makePopTx({2}, {std::vector<uint8_t>(100, 3)});

    {
        LOCK(cs_main);

        TxValidationState state;
        BOOST_CHECK(mempool.size() == 0);
        BOOST_CHECK(AcceptToMemoryPool(mempool, state, MakeTransactionRef(popTx1), nullptr, false, DEFAULT_TRANSACTION_MAXFEE));

        BOOST_CHECK(AcceptToMemoryPool(mempool, state, MakeTransactionRef(popTx2), nullptr, false, DEFAULT_TRANSACTION_MAXFEE));
        BOOST_CHECK(mempool.size() == 2);
    }
}

BOOST_FIXTURE_TEST_CASE(get_pop_tx_from_mempool_1, TestingSetup)
{
    VeriBlockTest::ServicesFixture service_fixture;

    BlockAssemblerTest blockAssembler(Params());

    CMutableTransaction popTx1 = VeriBlockTest::makePopTx({1}, {std::vector<uint8_t>(100, 2)});
    CMutableTransaction popTx2 = VeriBlockTest::makePopTx({2}, {std::vector<uint8_t>(100, 3)});

    TestMemPoolEntryHelper entry;
    {
        LOCK2(cs_main, mempool.cs);
        mempool.addUnchecked(entry.Fee(0LL).FromTx(popTx1));
        mempool.addUnchecked(entry.Fee(0LL).FromTx(popTx2));
    }

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;

    blockAssembler.prepareBlockTest();
    blockAssembler.addPackageTxs<VeriBlock::poptx_priority<ancestor_score>>(nPackagesSelected, nDescendantsUpdated);


    // in this test case we dont have coinbase transaction in the block
    BOOST_CHECK(blockAssembler.getBlock().vtx.size() == 2);
}

BOOST_FIXTURE_TEST_CASE(get_pop_tx_from_mempool_2, TestingSetup)
{
    VeriBlockTest::ServicesFixture service_fixture;

    BlockAssemblerTest blockAssembler(Params());

    TestMemPoolEntryHelper entry;

    size_t commonTxAmount = 20000;
    for (size_t i = 0; i < commonTxAmount; ++i) {
        LOCK2(cs_main, mempool.cs);
        CMutableTransaction tx = CMutableTransaction();
        tx.vout.resize(1);
        tx.vout[0].scriptPubKey = CScript() << OP_11 << OP_EQUAL;
        tx.vout[0].nValue = i;
        mempool.addUnchecked(entry.Fee(0LL).FromTx(tx));
    }

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;

    blockAssembler.prepareBlockTest();

    blockAssembler.addPackageTxs<ancestor_score>(nPackagesSelected, nDescendantsUpdated);


    // check that we have the max amount of txs in block
    BOOST_CHECK(blockAssembler.getBlock().vtx.size() < commonTxAmount);
    BOOST_CHECK(mempool.size() == commonTxAmount);

    // add one popTx into mempool
    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {std::vector<uint8_t>(100, 1)});
    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(popTx)));
    {
        LOCK2(cs_main, mempool.cs);
        mempool.addUnchecked(entry.Fee(0LL).FromTx(popTx));
    }


    nPackagesSelected = 0;
    nDescendantsUpdated = 0;

    blockAssembler.prepareBlockTest();

    // get txs with poptx comparator
    blockAssembler.addPackageTxs<VeriBlock::poptx_priority<ancestor_score>>(nPackagesSelected, nDescendantsUpdated);


    // check that we have one pop tx in block
    std::vector<CTransactionRef> vtx = blockAssembler.getBlock().vtx;
    BOOST_CHECK(std::find_if(vtx.begin(), vtx.end(), [](const CTransactionRef& tx) { return VeriBlock::isPopTx(*tx); }) != vtx.end());
}

BOOST_FIXTURE_TEST_CASE(check_the_pop_tx_limits_in_block, TestingSetup)
{
    auto& config = VeriBlock::getService<VeriBlock::Config>();
    VeriBlockTest::ServicesFixture service_fixture;

    BlockAssemblerTest blockAssembler(Params());

    TestMemPoolEntryHelper entry;

    for (size_t i = 0; i < config.max_pop_tx_amount + 10; ++i) {
        LOCK2(cs_main, mempool.cs);
        CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {std::vector<uint8_t>(100, i)});
        mempool.addUnchecked(entry.Fee(0LL).FromTx(popTx));
    }

    int nPackagesSelected = 0;
    int nDescendantsUpdated = 0;

    blockAssembler.prepareBlockTest();

    blockAssembler.addPackageTxs<VeriBlock::poptx_priority<ancestor_score>>(nPackagesSelected, nDescendantsUpdated);


    BOOST_CHECK(blockAssembler.getBlock().vtx.size() == config.max_pop_tx_amount);
}

BOOST_FIXTURE_TEST_CASE(check_CreateNewBlock_with_blockPopValidation_fail, TestingSetup)
{
   Fake(OverloadedMethod(pop_service_mock, addPayloads, void(std::string, const int&, const VeriBlock::Publications&)));
   Fake(OverloadedMethod(pop_service_mock, removePayloads, void(std::string, const int&)));
   Fake(Method(pop_service_mock, clearTemporaryPayloads));
   When(Method(pop_service_mock, determineATVPlausibilityWithBTCRules)).AlwaysReturn(true);

   When(Method(pop_service_mock, blockPopValidation)).AlwaysDo([](const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state) -> bool { return VeriBlock::blockPopValidationImpl((VeriBlock::PopServiceImpl&)VeriBlock::getService<VeriBlock::PopService>(), block, pindexPrev, params, state); });
   When(Method(pop_service_mock, parsePopTx)).AlwaysDo([](const CTransactionRef&, ScriptError* serror, VeriBlock::Publications*, VeriBlock::Context*, VeriBlock::PopTxType* type) -> bool {
       if (type != nullptr) {
           *type = VeriBlock::PopTxType::CONTEXT;
       }
       if (serror != nullptr) {
           *serror = ScriptError::SCRIPT_ERR_OK;
       }
       return true; });

   // Simulate that we have 8 invalid popTxs
   When(Method(pop_service_mock, updateContext)).Throw(8_Times(VeriBlock::PopServiceException("fail"))).AlwaysDo([](const std::vector<std::vector<uint8_t>>& veriBlockBlocks, const std::vector<std::vector<uint8_t>>& bitcoinBlocks) {});

   const size_t popTxCount = 10;

   TestMemPoolEntryHelper entry;
   for (size_t i = 0; i < popTxCount; ++i) {
       LOCK2(cs_main, mempool.cs);
       CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {std::vector<uint8_t>(100, i)});
       mempool.addUnchecked(entry.Fee(0LL).FromTx(popTx));
   }

   BlockAssembler blkAssembler(Params());
   CScript scriptPubKey = CScript() << OP_CHECKSIG;

   // repeatedly attempt to create a new block until either it
   // succeeds or we make a suspiciously large number of attempts
   // intentionally generate the correct block mutiple times to make sure it's repeatable
   bool success = false;
   for(size_t i = 0; i < popTxCount*2; i++) {
       try {
           std::unique_ptr<CBlockTemplate> pblockTemplate = blkAssembler.CreateNewBlock(scriptPubKey);

           BOOST_TEST(pblockTemplate->block.vtx.size() == 3);

           success = true;
       }
       catch (const std::runtime_error&) {}
   }
   BOOST_TEST(success);
}

BOOST_AUTO_TEST_SUITE_END()
