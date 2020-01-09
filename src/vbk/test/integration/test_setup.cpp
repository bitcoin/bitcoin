#include <vbk/test/integration/test_setup.hpp>

#include <vbk/init.hpp>

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

namespace VeriBlockTest {

IntegrationTestFixture::IntegrationTestFixture() : TestChain100Setup()
{
    When(Method(util_service_mock, compareForks)).AlwaysDo([&](const CBlockIndex& left, const CBlockIndex& right) -> int {
        return util_service_impl.compareForks(left, right);
    });
    When(Method(util_service_mock, CheckPopInputs)).AlwaysReturn(true);
    When(Method(util_service_mock, EvalScript)).AlwaysDo([&](const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, VeriBlock::Publications* pub, bool with_checks) -> bool {
        return ::TestEvalScript(script, stack, serror, pub, with_checks);
    });
    When(Method(util_service_mock, validatePopTx)).AlwaysReturn(true);
    When(Method(util_service_mock, checkCoinbaseTxWithPopRewards)).AlwaysReturn(true);
    When(Method(util_service_mock, getPopRewards)).AlwaysDo([&](const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) -> VeriBlock::PoPRewards {
        return util_service_impl.getPopRewards(pindexPrev, consensusParams);
    });
    When(Method(util_service_mock, addPopPayoutsIntoCoinbaseTx)).AlwaysDo([&](CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) -> void {
        return util_service_impl.addPopPayoutsIntoCoinbaseTx(coinbaseTx, pindexPrev, consensusParams);
    });

    When(Method(util_service_mock, makeTopLevelRoot)).AlwaysDo([&](int height, const VeriBlock::KeystoneArray& keystones, const uint256& txRoot) -> uint256 {
        return util_service_impl.makeTopLevelRoot(height, keystones, txRoot);
    });

    When(Method(util_service_mock, getKeystoneHashesForTheNextBlock)).AlwaysDo([&](const CBlockIndex* pindexPrev) -> VeriBlock::KeystoneArray {
        return util_service_impl.getKeystoneHashesForTheNextBlock(pindexPrev);
    });

    VeriBlockTest::setServiceMock<VeriBlock::UtilService>(util_service_mock);

    VeriBlock::GrpcIntegrationService* validationService = new VeriBlock::GrpcIntegrationService();
    VeriBlock::setService<VeriBlock::GrpcIntegrationService>(validationService);

    VeriBlock::InitPopService();

    VeriBlockTest::setUpGenesisBlocks();
}
} // namespace VeriBlockTest
