#include <vbk/test/integration/test_setup.hpp>

#include <chain.h>
#include <vbk/entity/pop.hpp>
#include <vbk/init.hpp>
#include <vbk/interpreter.hpp>

using namespace fakeit;

namespace VeriBlockTest {

IntegrationTestFixture::IntegrationTestFixture() : TestChain100Setup()
{
    When(Method(util_service_mock, compareForks)).AlwaysDo([&](const CBlockIndex& left, const CBlockIndex& right) -> int {
        return util_service_impl.compareForks(left, right);
    });
    When(Method(util_service_mock, CheckPopInputs)).AlwaysReturn(true);
    When(Method(util_service_mock, EvalScript)).AlwaysDo([&](const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, VeriBlock::Publications* pub, VeriBlock::Context* ctx, VeriBlock::PopTxType* type, bool with_checks) -> bool {
        return VeriBlock::EvalScriptImpl(script, stack, serror, pub, ctx, type, with_checks);
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

    auto* validationService = new VeriBlock::GrpcIntegrationService();
    VeriBlock::setService<VeriBlock::GrpcIntegrationService>(validationService);

    VeriBlock::InitPopService();

    VeriBlockTest::setUpGenesisBlocks();
}
} // namespace VeriBlockTest
