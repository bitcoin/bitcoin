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

BOOST_AUTO_TEST_SUITE(pop_interpreter_tests)

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
        config.btc_header_size = 3;
        config.vbk_header_size = 4;

        When(Method(pop, checkATVinternally)).AlwaysReturn(true);
        When(Method(pop, checkVTBinternally)).AlwaysReturn(true);
    }

    Mock<VeriBlock::PopService> pop;
    VeriBlock::UtilService& util{VeriBlock::InitUtilService()};
    VeriBlock::Config& config{VeriBlock::InitConfig()};

    std::vector<std::vector<uint8_t>> stack{};
    VeriBlock::Publications pub{};
    VeriBlock::Context ctx{};
    VeriBlock::PopTxType type{};
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
    BOOST_CHECK(util.EvalScript(script, stack, &serror, &pub, &ctx, &type, true));
    BOOST_REQUIRE(!stack.empty());
    BOOST_CHECK(type == VeriBlock::PopTxType::PUBLICATIONS);
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
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
    config.min_atv_size = 0;
    config.max_atv_size = 1; // less than 3
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_vtb_size_invalid, EvalScriptFixture)
{
    config.min_vtb_size = 10; // more than 4
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
    config.min_vtb_size = 0;
    config.max_vtb_size = 2; // less than 4
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_only_atv, EvalScriptFixture)
{
    script = CScript() << "ATV" << OP_CHECKATV << OP_CHECKPOP;
    BOOST_CHECK(util.EvalScript(script, stack, &serror, &pub, &ctx, &type, true));
    BOOST_REQUIRE(!stack.empty());
    BOOST_CHECK(type == VeriBlock::PopTxType::PUBLICATIONS);
    BOOST_CHECK(CastToBool(stack.back()));
    BOOST_CHECK(pub.vtbs.empty());
    BOOST_CHECK(pub.atv == "ATV"_v);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_only_vtbs, EvalScriptFixture)
{
    script = CScript() << "VTB1" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_VBK_ATVFAIL);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_out_of_order, EvalScriptFixture)
{
    script = CScript() << "VTB1" << OP_CHECKVTB << "ATV" << OP_CHECKATV << "VTB2" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(util.EvalScript(script, stack, &serror, &pub, &ctx, &type, true));
    BOOST_REQUIRE(!stack.empty());
    BOOST_CHECK(CastToBool(stack.back()));
    BOOST_CHECK(pub.atv == "ATV"_v);
    BOOST_CHECK(pub.vtbs.size() == 2);
    BOOST_CHECK(pub.vtbs[0] == "VTB1"_v);
    BOOST_CHECK(pub.vtbs[1] == "VTB2"_v);
    BOOST_CHECK(type == VeriBlock::PopTxType::PUBLICATIONS);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_too_big, EvalScriptFixture)
{
    config.max_pop_script_size = 10;
    script = CScript() << "this is too long, I can not handle script this long" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_SCRIPT_SIZE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_checkpop_is_not_last, EvalScriptFixture)
{
    script = CScript() << "ATV" << OP_CHECKATV << "VTB" << OP_CHECKVTB << OP_CHECKPOP << "VTB2";
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_VBK_EXTRA_OPCODE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_checkpop_stack_is_not_empty, EvalScriptFixture)
{
    // missing OP_CHECKATV
    script = CScript() << "ATV"
                       << "VTB" << OP_CHECKVTB << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, true));
    BOOST_CHECK(serror == SCRIPT_ERR_EVAL_FALSE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_only_context_valid, EvalScriptFixture)
{
    script = CScript() << "btc" << "vbk1" << OP_POPVBKHEADER << "vbk2" << OP_POPVBKHEADER << OP_POPBTCHEADER << OP_CHECKPOP;
    BOOST_CHECK(util.EvalScript(script, stack, &serror, &pub, &ctx, &type, false));
    BOOST_CHECK(type == VeriBlock::PopTxType::CONTEXT);
    BOOST_CHECK(ctx.btc.size() == 1);
    BOOST_CHECK(ctx.btc[0] == "btc"_v);
    BOOST_CHECK(ctx.vbk.size() == 2);
    BOOST_CHECK(ctx.vbk[0] == "vbk1"_v);
    BOOST_CHECK(ctx.vbk[1] == "vbk2"_v);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_mixed_pub_and_context, EvalScriptFixture)
{
    script = CScript() << "ATV" << OP_CHECKATV << "VTB" << OP_CHECKVTB << "btc" << "vbk1" << OP_POPVBKHEADER << "vbk2" << OP_POPVBKHEADER << OP_POPBTCHEADER << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, false));
    BOOST_CHECK(serror == SCRIPT_ERR_BAD_OPCODE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_ctx_invalid_size_vbk, EvalScriptFixture)
{
    script = CScript() << "btc" << "veriblock header that is not equalto expected size" << OP_POPVBKHEADER << "vbk2" << OP_POPVBKHEADER << OP_POPBTCHEADER << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, false));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
}

BOOST_FIXTURE_TEST_CASE(eval_pop_tx_scriptSig_ctx_invalid_size_btc, EvalScriptFixture)
{
    script = CScript() << "bitcoin header that is not equal to expected size" << OP_POPBTCHEADER << "vbk2" << OP_POPVBKHEADER << OP_CHECKPOP;
    BOOST_CHECK(!util.EvalScript(script, stack, &serror, nullptr, nullptr, nullptr, false));
    BOOST_CHECK(serror == SCRIPT_ERR_PUSH_SIZE);
}


BOOST_AUTO_TEST_SUITE_END()
