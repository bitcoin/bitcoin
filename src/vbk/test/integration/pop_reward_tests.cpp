#include <boost/test/unit_test.hpp>

#include <chainparams.h>
#include <script/interpreter.h>
#include <test/setup_common.h>
#include <validation.h>

#include <vbk/init.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/integration/grpc_integration_service.hpp>
#include <vbk/test/integration/utils.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>
#include <vbk/util_service.hpp>
#include <vbk/util_service/util_service_impl.hpp>

#include <algorithm>

#include <fakeit.hpp>

using namespace fakeit;

namespace {
#define stacktop(i) (stack.at(stack.size() + (i)))

inline bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}

inline void popstack(std::vector<std::vector<uint8_t>>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}

static bool TestEvalScript(const CScript& script, std::vector<std::vector<uint8_t>>& stack, ScriptError* serror, VeriBlock::Publications* ret, bool with_checks)
{
    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    static const std::vector<uint8_t> vchFalse(0);
    static const std::vector<uint8_t> vchTrue(1, 1);
    opcodetype opcode;
    std::vector<unsigned char> vchPushValue;
    auto& config = VeriBlock::getService<VeriBlock::Config>();
    auto& pop = VeriBlock::getService<VeriBlock::PopService>();
    VeriBlock::Publications pub;

    if (script.size() > config.max_pop_script_size) {
        return set_error(serror, SCRIPT_ERR_SCRIPT_SIZE);
    }

    size_t maxPushSize = std::max(config.max_atv_size, config.max_vtb_size);
    size_t minPushSize = std::min(config.min_atv_size, config.min_vtb_size);
    try {
        while (pc < pend) {
            if (!script.GetOp(pc, opcode, vchPushValue)) {
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }

            if (!vchPushValue.empty()) {
                // we don't know if it is ATV or VTB yet, so do sanity check here
                if (vchPushValue.size() > maxPushSize || vchPushValue.size() < minPushSize) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }
                stack.push_back(vchPushValue);
                continue;
            }

            // at this point, opcode is always one of these 3
            switch (opcode) {
            case OP_CHECKATV: {
                // tx has one ATV
                if (!pub.atv.empty()) {
                    return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                }

                // validate ATV size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_atv_size || el.size() < config.min_atv_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate ATV content
                if (with_checks && !pop.checkATVinternally(el)) {
                    return set_error(serror, SCRIPT_ERR_VBK_ATVFAIL);
                }

                pub.atv = el;
                popstack(stack);
                break;
            }
            case OP_CHECKVTB: {
                // validate VTB size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_vtb_size || el.size() < config.min_vtb_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate VTB content
                if (with_checks && !pop.checkVTBinternally(el)) {
                    return set_error(serror, SCRIPT_ERR_VBK_VTBFAIL);
                }

                // tx has many VTBs
                pub.vtbs.push_back(el);
                popstack(stack);
                break;
            }
            case OP_CHECKPOP: {
                // OP_CHECKPOP should be last opcode
                if (script.GetOp(pc, opcode, vchPushValue)) {
                    // we could read next opcode. extra opcodes is an error
                    return set_error(serror, SCRIPT_ERR_VBK_EXTRA_OPCODE);
                }

                // stack should be empty at this point
                if (!stack.empty()) {
                    return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
                }

                // we did all checks, put true on stack to signal successful execution
                stack.push_back(vchTrue);
                break;
            }
            default:
                // forbid any other opcodes in pop transactions
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }
        }
    } catch (...) {
        return set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    }

    // set return value
    if (ret != nullptr) {
        *ret = pub;
    }

    return true;
}
} // namespace

//Before starting this test you should configurate the alt-service properties file with the concrete bootstrap blocks
//The bootstrap blocks are contained in the vbk/test/integration/utils.cpp source file

struct PopRewardIntegrationTextFixture : public TestChain100Setup {
    VeriBlock::UtilServiceImpl util_service_impl;
    VeriBlock::PopServiceImpl pop_service_impl;
    fakeit::Mock<VeriBlock::UtilService> util_service_mock;
    fakeit::Mock<VeriBlock::PopService> pop_service_mock;

    PopRewardIntegrationTextFixture() : TestChain100Setup()
    {
        When(Method(util_service_mock, compareForks)).AlwaysReturn(0);
        When(Method(util_service_mock, CheckPopInputs)).AlwaysReturn(true);
        When(Method(util_service_mock, EvalScript)).AlwaysDo([&](const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, VeriBlock::Publications* pub, bool with_checks) -> bool {
            return ::TestEvalScript(script, stack, serror, pub, with_checks);
        });
        When(Method(util_service_mock, validatePopTx)).AlwaysReturn(true);
        When(Method(util_service_mock, checkCoinbaseTxWithPopRewards)).AlwaysReturn(true);
        When(Method(util_service_mock, getPopRewards)).AlwaysDo([&](const CBlockIndex& pindexPrev) -> VeriBlock::PoPRewards {
            return util_service_impl.getPopRewards(pindexPrev);
        });
        When(Method(util_service_mock, addPopPayoutsIntoCoinbaseTx)).AlwaysDo([&](CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev) -> void {
            return util_service_impl.addPopPayoutsIntoCoinbaseTx(coinbaseTx, pindexPrev);
        });

        When(Method(util_service_mock, makeTopLevelRoot)).AlwaysDo([&](int height, const VeriBlock::KeystoneArray& keystones, const uint256& txRoot) -> uint256 {
            return util_service_impl.makeTopLevelRoot(height, keystones, txRoot);
        });

        When(Method(util_service_mock, getKeystoneHashesForTheNextBlock)).AlwaysDo([&](const CBlockIndex* pindexPrev) -> VeriBlock::KeystoneArray {
            return util_service_impl.getKeystoneHashesForTheNextBlock(pindexPrev);
        });

        When(Method(pop_service_mock, checkVTBinternally)).AlwaysReturn(true);
        When(Method(pop_service_mock, checkATVinternally)).AlwaysReturn(true);
        When(Method(pop_service_mock, compareTwoBranches)).AlwaysReturn(0);
        When(Method(pop_service_mock, rewardsCalculateOutputs)).AlwaysDo([&](const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex& difficulty_start_interval, const CBlockIndex& difficulty_end_interval, std::map<CScript, int64_t>& outputs) -> void {
            return pop_service_impl.rewardsCalculateOutputs(blockHeight, endorsedBlock, contaningBlocksTip, difficulty_start_interval, difficulty_end_interval, outputs);
        });
        When(Method(pop_service_mock, savePopTxToDatabase)).AlwaysDo([&](const CBlock& block, const int& nHeight) -> void {
            return pop_service_impl.savePopTxToDatabase(block, nHeight);
        });
        ;
        When(Method(pop_service_mock, getLastKnownVBKBlocks)).AlwaysReturn({});
        When(Method(pop_service_mock, getLastKnownBTCBlocks)).AlwaysReturn({});
        When(Method(pop_service_mock, blockPopValidation)).AlwaysDo([&](const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, CValidationState& state) -> bool {
            return pop_service_impl.blockPopValidation(block, pindexPrev, params, state);
        });

        VeriBlockTest::setServiceMock<VeriBlock::UtilService>(util_service_mock);
        VeriBlockTest::setServiceMock<VeriBlock::PopService>(pop_service_mock);

        VeriBlock::GrpcIntegrationService* validationService = new VeriBlock::GrpcIntegrationService();
        VeriBlock::setService<VeriBlock::GrpcIntegrationService>(validationService);
    }
};

BOOST_FIXTURE_TEST_SUITE(pop_reward_tests, PopRewardIntegrationTextFixture)

BOOST_AUTO_TEST_CASE(basic_test)
{
    VeriBlockTest::setUpGenesisBlocks();

    auto& grpc_integration_service = VeriBlock::getService<VeriBlock::GrpcIntegrationService>();
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    CBlockIndex* endorsedBlockIndex = ChainActive().Tip();
    CBlock endorsedBlock;

    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));

    CScript payout1;
    payout1 << std::vector<uint8_t>(5, 1);
    CScript payout2;
    payout2 << std::vector<uint8_t>(5, 2);

    VeriBlock::AltPublication altPublication1 = VeriBlockTest::generateSignedAltPublication(endorsedBlock, VeriBlock::getService<VeriBlock::Config>().index.unwrap(), payout1);
    VeriBlock::AltPublication altPublication2 = VeriBlockTest::generateSignedAltPublication(endorsedBlock, VeriBlock::getService<VeriBlock::Config>().index.unwrap(), payout2);

    BOOST_CHECK(grpc_integration_service.verifyAltPublication(altPublication1));
    BOOST_CHECK(grpc_integration_service.checkATVAgainstView(altPublication1));
    BOOST_CHECK(grpc_integration_service.verifyAltPublication(altPublication2));
    BOOST_CHECK(grpc_integration_service.checkATVAgainstView(altPublication2));

    std::vector<uint8_t> atvBytes1 = grpc_integration_service.serializeAltPublication(altPublication1);
    std::vector<uint8_t> atvBytes2 = grpc_integration_service.serializeAltPublication(altPublication2);

    CMutableTransaction popTx1 = VeriBlockTest::makePopTx(atvBytes1, {});
    CMutableTransaction popTx2 = VeriBlockTest::makePopTx(atvBytes2, {});

    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    CBlock blockWithPopTx1 = CreateAndProcessBlock({popTx1}, scriptPubKey);
    CBlock blockWithPopTx2 = CreateAndProcessBlock({popTx2}, scriptPubKey);

    int height = ChainActive().Height() - endorsedBlockIndex->nHeight;

    for (int i = 0; i < config.POP_REWARD_PAYMENT_DELAY - height - 1; ++i) {
        CreateAndProcessBlock({}, scriptPubKey);
    }

    CBlock blockWithRewards = CreateAndProcessBlock({}, scriptPubKey);

    BOOST_CHECK(std::find_if(blockWithRewards.vtx[0]->vout.begin(), blockWithRewards.vtx[0]->vout.end(), [payout1](const CTxOut& out) -> bool {
        return out.scriptPubKey == payout1;
    }) != blockWithRewards.vtx[0]->vout.end());

    BOOST_CHECK(std::find_if(blockWithRewards.vtx[0]->vout.begin(), blockWithRewards.vtx[0]->vout.end(), [payout2](const CTxOut& out) -> bool {
        return out.scriptPubKey == payout2;
    }) != blockWithRewards.vtx[0]->vout.end());
}
BOOST_AUTO_TEST_SUITE_END()
