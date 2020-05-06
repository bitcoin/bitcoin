// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vbk/test/integration/test_setup.hpp>

#include <chain.h>
#include <vbk/init.hpp>

namespace VeriBlockTest {

IntegrationTestFixture::IntegrationTestFixture() : TestChain100Setup()
{
//    ON_CALL(util_service_mock, compareForks).WillByDefault(
//      [&](const CBlockIndex& left, const CBlockIndex& right) -> int {
//        return util_service_impl.compareForks(left, right);
//      });
//    ON_CALL(util_service_mock, CheckPopInputs).WillByDefault(Return(true));
//    ON_CALL(util_service_mock, EvalScript).WillByDefault(
//      [&](const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, VeriBlock::Publications* pub, VeriBlock::Context* ctx, VeriBlock::PopTxType* type, bool with_checks) -> bool {
//        return VeriBlock::EvalScriptImpl(script, stack, serror, pub, ctx, type, with_checks);
//      });
//    ON_CALL(util_service_mock, validatePopTx).WillByDefault(Return(true));
//    ON_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).WillByDefault(Return(true));
//    ON_CALL(util_service_mock, getPopRewards).WillByDefault(
//      [&](const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) -> VeriBlock::PoPRewards {
//        return util_service_impl.getPopRewards(pindexPrev, consensusParams);
//      });
//    ON_CALL(util_service_mock, addPopPayoutsIntoCoinbaseTx).WillByDefault(
//      [&](CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) -> void {
//        return util_service_impl.addPopPayoutsIntoCoinbaseTx(coinbaseTx, pindexPrev, consensusParams);
//      });
//    ON_CALL(util_service_mock, makeTopLevelRoot).WillByDefault(
//      [&](int height, const VeriBlock::KeystoneArray& keystones, const uint256& txRoot) -> uint256 {
//        return util_service_impl.makeTopLevelRoot(height, keystones, txRoot);
//      });
//    ON_CALL(util_service_mock, getKeystoneHashesForTheNextBlock).WillByDefault(
//      [&](const CBlockIndex* pindexPrev) -> VeriBlock::KeystoneArray {
//        return util_service_impl.getKeystoneHashesForTheNextBlock(pindexPrev);
//      });
//
//    VeriBlockTest::setServiceMock<VeriBlock::UtilService>(util_service_mock);
//
//    auto* validationService = new VeriBlock::GrpcIntegrationService();
//    VeriBlock::setService<VeriBlock::GrpcIntegrationService>(validationService);
//
//    VeriBlock::Config& config = VeriBlock::getService<VeriBlock::Config>();
//    config.bootstrap_bitcoin_blocks = {"010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010270000FFFF7F2000000000"};
//    config.bitcoin_first_block_height = 1610826;
//    config.bootstrap_veriblock_blocks = {"00000000000200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000027100101000000000000"};
//
//    VeriBlock::InitPopService();
//
//    VeriBlockTest::setUpGenesisBlocks();
}
} // namespace VeriBlockTest
