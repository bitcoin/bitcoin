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

template <typename Stream>
Stream& operator<<(Stream&& s, std::string data)
{
    return s << std::vector<uint8_t>{data.begin(), data.end()};
}

inline std::vector<uint8_t> operator""_v(const char* c, size_t s)
{
    return std::vector<uint8_t>{c, c + s};
}

struct EvalScriptFixture {
    static std::string str(const std::vector<uint8_t>& v)
    {
        return std::string{v.begin(), v.end()};
    }

    static bool CastToBool(const std::vector<uint8_t>& vch)
    {
        for (unsigned int i = 0; i < vch.size(); i++) {
            if (vch[i] != 0) {
                // Can be negative zero
                if (i == vch.size() - 1 && vch[i] == 0x80)
                    return false;
                return true;
            }
        }
        return false;
    }


    EvalScriptFixture()
    {
        VeriBlockTest::setServiceMock<VeriBlock::PopService>(pop);

        config.min_atv_size = 1;
        config.max_atv_size = 10;
        config.min_vtb_size = 1;
        config.max_vtb_size = 10;
        config.max_pop_script_size = 1000;

        When(Method(pop, checkATVinternally)).AlwaysReturn(true);
        When(Method(pop, checkVTBinternally)).AlwaysReturn(true);
    }

    Mock<VeriBlock::PopService> pop;
    VeriBlock::UtilService& util{VeriBlock::InitUtilService()};
    VeriBlock::Config& config{VeriBlock::InitConfig()};

    std::vector<std::vector<uint8_t>> stack{};
    VeriBlock::Publications pub{};
    ScriptError serror{};
    CScript script = CScript()
                     << "ATV" << OP_CHECKATV
                     << "VTB1" << OP_CHECKVTB
                     << "VTB2" << OP_CHECKVTB
                     << "VTB3" << OP_CHECKVTB
                     << OP_CHECKPOP;
};

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_valid, EvalScriptFixture)
{
    BOOST_CHECK(util.EvalScript(script, stack, &serror, &pub));
    BOOST_REQUIRE(!stack.empty());
    BOOST_CHECK(CastToBool(stack.back()));
    BOOST_CHECK(pub.atv == "ATV"_v);
    BOOST_CHECK(pub.vtbs.size() == 3);
    BOOST_CHECK(pub.vtbs[0] == "VTB1"_v);
    BOOST_CHECK(pub.vtbs[1] == "VTB2"_v);
    BOOST_CHECK(pub.vtbs[2] == "VTB3"_v);
    Verify_Method(Method(pop, checkATVinternally)).Once();
    Verify_Method(Method(pop, checkVTBinternally)).Exactly(3);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_atv_size_invalid, EvalScriptFixture)
{
    config.min_atv_size = 10; // more than 3
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
    config.min_atv_size = 0;
    config.max_atv_size = 1; // less than 3
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_vtb_size_invalid, EvalScriptFixture)
{
    config.min_vtb_size = 10; // more than 4
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, &pub));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
    config.min_vtb_size = 0;
    config.max_vtb_size = 2; // less than 4
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, &pub));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_only_atv, EvalScriptFixture)
{
    script = CScript() << "ATV" << OP_CHECKATV << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, &pub));
    BOOST_CHECK(serror == SCRIPT_ERR_VBK_VTBFAIL);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_only_vtbs, EvalScriptFixture)
{
    script = CScript() << "VTB1" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr));
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_out_of_order, EvalScriptFixture)
{
    script = CScript() << "VTB1" << OP_CHECKVTB << "ATV" << OP_CHECKATV << "VTB2" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(util.EvalScript(script, stack, &serror, &pub));
    BOOST_REQUIRE(!stack.empty());
    BOOST_CHECK(CastToBool(stack.back()));
    BOOST_CHECK(pub.atv == "ATV"_v);
    BOOST_CHECK(pub.vtbs.size() == 2);
    BOOST_CHECK(pub.vtbs[0] == "VTB1"_v);
    BOOST_CHECK(pub.vtbs[1] == "VTB2"_v);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_too_big, EvalScriptFixture)
{
    config.max_pop_script_size = 10;
    script = CScript() << "this is too long, I can not handle script this long" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, &pub));
    BOOST_CHECK(serror == SCRIPT_ERR_SCRIPT_SIZE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_checkpop_is_not_last, EvalScriptFixture)
{
    script = CScript() << "ATV" << OP_CHECKATV << "VTB" << OP_CHECKVTB << OP_CHECKPOP << "VTB2";
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, &pub));
    BOOST_CHECK(serror == SCRIPT_ERR_VBK_EXTRA_OPCODE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_checkpop_stack_is_not_empty, EvalScriptFixture)
{
    // missing OP_CHECKATV
    script = CScript() << "ATV"
                       << "VTB" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, &pub));
    BOOST_CHECK(serror == SCRIPT_ERR_EVAL_FALSE);
}

BOOST_AUTO_TEST_SUITE_END()
