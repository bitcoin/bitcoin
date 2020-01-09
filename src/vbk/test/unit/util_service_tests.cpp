#include <boost/test/unit_test.hpp>
#include <consensus/validation.h>
#include <script/interpreter.h>
#include <string>
#include <test/util/setup_common.h>
#include <validation.h>
#include <vbk/config.hpp>
#include <vbk/init.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>
#include <vbk/util.hpp>
#include <vbk/util_service/util_service_impl.hpp>

#include <fakeit.hpp>

using namespace fakeit;

BOOST_AUTO_TEST_SUITE(util_service_tests)

BOOST_AUTO_TEST_CASE(poptx_validate)
{
    VeriBlock::InitConfig();
    auto& util = VeriBlock::InitUtilService();
    auto tx = VeriBlockTest::makePopTx({0}, {{1}});
    TxValidationState state;
    BOOST_CHECK_MESSAGE(util.validatePopTx(CTransaction(tx), state), "valid tx");
    tx.vin[0].prevout = COutPoint(uint256S("0x123"), 5);
    BOOST_CHECK_MESSAGE(!util.validatePopTx(CTransaction(tx), state), "invalid tx");
}

BOOST_AUTO_TEST_CASE(poptx_valid)
{
    auto tx = VeriBlockTest::makePopTx({1}, {{2}, {3}});
    BOOST_CHECK(VeriBlock::isVBKNoInput(tx.vin[0].prevout));
    BOOST_CHECK(VeriBlock::isPopTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(poptx_multiple_inputs)
{
    auto tx = VeriBlockTest::makePopTx({1}, {{2}, {3}});
    tx.vin.resize(2);
    BOOST_CHECK(!VeriBlock::isPopTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(poptx_input_wrong_scriptPubKey)
{
    auto tx = VeriBlockTest::makePopTx({1}, {{2}, {3}});
    tx.vout[0].scriptPubKey << OP_RETURN;
    BOOST_CHECK(!VeriBlock::isPopTx(CTransaction(tx)));
}

BOOST_AUTO_TEST_CASE(is_keystone)
{
    auto& config = VeriBlock::InitConfig();
    auto& util = VeriBlock::InitUtilService();
    CBlockIndex index;
    config.keystone_interval = 5;
    index.nHeight = 100; // multiple of 5
    BOOST_CHECK(util.isKeystone(index));
    index.nHeight = 99; // not multiple of 5
    BOOST_CHECK(!util.isKeystone(index));
}

BOOST_AUTO_TEST_CASE(get_previous_keystone)
{
    auto& config = VeriBlock::InitConfig();
    auto& util = VeriBlock::InitUtilService();
    config.keystone_interval = 3;

    std::vector<CBlockIndex> blocks;
    blocks.resize(6);
    blocks[0].pprev = nullptr;
    blocks[0].nHeight = 0;
    blocks[1].pprev = &blocks[0];
    blocks[1].nHeight = 1;
    blocks[2].pprev = &blocks[1];
    blocks[2].nHeight = 2;
    blocks[3].pprev = &blocks[2];
    blocks[3].nHeight = 3;
    blocks[4].pprev = &blocks[3];
    blocks[4].nHeight = 4;
    blocks[5].pprev = &blocks[4];
    blocks[5].nHeight = 5;

    BOOST_CHECK(util.getPreviousKeystone(blocks[5]) == &blocks[3]);
    BOOST_CHECK(util.getPreviousKeystone(blocks[4]) == &blocks[3]);
    BOOST_CHECK(util.getPreviousKeystone(blocks[3]) == &blocks[0]);
    BOOST_CHECK(util.getPreviousKeystone(blocks[2]) == &blocks[0]);
    BOOST_CHECK(util.getPreviousKeystone(blocks[1]) == &blocks[0]);
    BOOST_CHECK(util.getPreviousKeystone(blocks[0]) == nullptr);
}

BOOST_AUTO_TEST_CASE(make_context_info)
{
    TestChain100Setup blockchain;

    auto& util = VeriBlock::InitUtilService();
    auto& config = VeriBlock::InitConfig();
    config.keystone_interval = 3;

    CScript scriptPubKey = CScript() << ToByteVector(blockchain.coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock block = blockchain.CreateAndProcessBlock({}, scriptPubKey);

    LOCK(cs_main);

    CBlockIndex* index = LookupBlockIndex(block.GetHash());
    BOOST_REQUIRE(index != nullptr);

    uint256 txRoot{};
    auto keystones = util.getKeystoneHashesForTheNextBlock(index->pprev);
    auto container = VeriBlock::ContextInfoContainer(index->nHeight, keystones, txRoot);

    // TestChain100Setup has blockchain with 100 blocks, new block is 101
    BOOST_CHECK(container.height == 101);
    BOOST_CHECK(container.keystones == keystones);
    BOOST_CHECK(container.getAuthenticated().size() == container.getUnauthenticated().size() + 32);
    BOOST_CHECK(container.getUnauthenticated().size() == 4 + VBK_NUM_KEYSTONES * 32);
}

BOOST_AUTO_TEST_CASE(check_pop_inputs)
{
    VeriBlock::InitConfig();
    auto& util = VeriBlock::InitUtilService();
    Mock<VeriBlock::PopService> pop_service_mock;
    VeriBlockTest::setServiceMock<VeriBlock::PopService>(pop_service_mock);

    CTransaction tx = VeriBlockTest::makePopTx({1, 2, 3, 4, 5}, {{2, 3, 4, 5, 6, 7}});
    TxValidationState state;

    PrecomputedTransactionData data(tx);

    When(Method(pop_service_mock, checkATVinternally)).AlwaysReturn(true);
    When(Method(pop_service_mock, checkVTBinternally)).AlwaysReturn(true);
    BOOST_CHECK(util.CheckPopInputs(tx, state, 0, false, data));

    When(Method(pop_service_mock, checkATVinternally)).AlwaysReturn(false);
    BOOST_CHECK(!util.CheckPopInputs(tx, state, 0, false, data));
}

BOOST_AUTO_TEST_CASE(RegularTxesTotalSize_test)
{
    CTransaction popTx1 = VeriBlockTest::makePopTx({1}, {{2}});
    CTransaction popTx2 = VeriBlockTest::makePopTx({1}, {{3}});
    CTransaction popTx3 = VeriBlockTest::makePopTx({1}, {{3}});
    CTransaction popTx4 = VeriBlockTest::makePopTx({1}, {{3}});

    CTransaction commonTx;

    std::vector<CTransactionRef> vtx = {MakeTransactionRef(popTx1), MakeTransactionRef(popTx2), MakeTransactionRef(popTx3), MakeTransactionRef(popTx4),
        MakeTransactionRef(commonTx), MakeTransactionRef(commonTx)};

    BOOST_CHECK(VeriBlock::RegularTxesTotalSize(vtx) == 2);
}

BOOST_AUTO_TEST_SUITE_END()
