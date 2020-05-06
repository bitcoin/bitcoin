// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_TEST_UTIL_MOCK_HPP
#define BITCOIN_SRC_VBK_TEST_UTIL_MOCK_HPP

#include <chain.h>

#include <vbk/config.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/pop_service_impl.hpp>
#include <vbk/service_locator.hpp>

namespace VeriBlockTest {

//class PopServiceMock : public ::testing::NiceMock<VeriBlock::PopService>
//{
//public:
//    MOCK_METHOD(std::vector<VeriBlock::BlockBytes>, getLastKnownVBKBlocks, (size_t blocks), ());
//    MOCK_METHOD(std::vector<VeriBlock::BlockBytes>, getLastKnownBTCBlocks, (size_t blocks), ());
//    MOCK_METHOD(bool, checkVTBinternally, (const std::vector<uint8_t>& bytes), ());
//    MOCK_METHOD(bool, checkATVinternally, (const std::vector<uint8_t>& bytes), ());
//    MOCK_METHOD(int, compareTwoBranches,
//      (const CBlockIndex* commonKeystone, const CBlockIndex* leftForkTip, const CBlockIndex* rightForkTip), ());
//    MOCK_METHOD(void, rewardsCalculateOutputs,
//      (const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip,
//        const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval,
//        (std::map<CScript, int64_t>)&outputs), ());
//    MOCK_METHOD(bool, blockPopValidation,
//      (const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params,
//        BlockValidationState& state), ());
//    MOCK_METHOD(bool, determineATVPlausibilityWithBTCRules,
//      (VeriBlock::AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader,
//        const Consensus::Params& params, TxValidationState& state), ());
//    MOCK_METHOD(bool, commitPayloads, (const CBlockIndex& blockIndex, const CBlock& block, TxValidationState& state), ());
//    MOCK_METHOD(bool, removePayloads, (const CBlockIndex& block), ());
//};
//
//class UtilServiceMock : public ::testing::NiceMock<VeriBlock::UtilService>
//{
//public:
//    MOCK_METHOD(bool, CheckPopInputs, (const CTransaction& tx, TxValidationState& state, unsigned int flags,
//      bool cacheSigStore, PrecomputedTransactionData& txdata), ());
//    MOCK_METHOD(bool, isKeystone, (const CBlockIndex& block), ());
//    MOCK_METHOD(const CBlockIndex*, getPreviousKeystone, (const CBlockIndex& block), ());
//    MOCK_METHOD(VeriBlock::KeystoneArray, getKeystoneHashesForTheNextBlock,
//      (const CBlockIndex* pindexPrev), ());
//    MOCK_METHOD(uint256, makeTopLevelRoot, (int height, const VeriBlock::KeystoneArray& keystones,
//      const uint256& txRoot), ());
//    MOCK_METHOD(int, compareForks, (const CBlockIndex& left, const CBlockIndex& right), ());
//    MOCK_METHOD(VeriBlock::PoPRewards, getPopRewards,
//      (const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams), ());
//    MOCK_METHOD(void, addPopPayoutsIntoCoinbaseTx,
//      (CMutableTransaction & coinbaseTx, const CBlockIndex& pindexPrev,
//      const Consensus::Params& consensusParams), ());
//    MOCK_METHOD(bool, checkCoinbaseTxWithPopRewards, (const CTransaction& tx, const CAmount& PoWBlockReward,
//      const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state), ());
//    MOCK_METHOD(bool, validatePopTx, (const CTransaction& tx, TxValidationState& state), ());
//    MOCK_METHOD(bool, EvalScript, (const CScript& script,
//      std::vector<std::vector<unsigned char>>& stack,
//      ScriptError* serror,
//      VeriBlock::Publications* pub, VeriBlock::Context* ctx, VeriBlock::PopTxType* type,
//      bool with_checks), ());
//};
//
//class PopServiceImplMock : public VeriBlock::PopServiceImpl
//{
//public:
//    PopServiceImplMock() : VeriBlock::PopServiceImpl() {}
//
//    MOCK_METHOD(bool, determineATVPlausibilityWithBTCRules,
//      (VeriBlock::AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader,
//        const Consensus::Params& params, TxValidationState& state), (override));
//    MOCK_METHOD(bool, commitPayloads,
//      (const CBlockIndex& prev, const CBlock& connecting, TxValidationState& state), (override));
//    MOCK_METHOD(bool, removePayloads,
//      (const CBlockIndex& block), (override));
//};
//
//template <typename T>
//void setServiceMock(T& mock)
//{
//    VeriBlock::setService<T>(std::shared_ptr<T>(&mock, [](T*) { /* empty destructor*/ }));
//}
//
//inline void setUpPopServiceMock(PopServiceMock& mock)
//{
//    ON_CALL(mock, checkVTBinternally).WillByDefault(Return(true));
//    ON_CALL(mock, checkATVinternally).WillByDefault(Return(true));
//    ON_CALL(mock, compareTwoBranches).WillByDefault(Return(0));
//    ON_CALL(mock, getLastKnownVBKBlocks).WillByDefault(Return(std::vector<VeriBlock::BlockBytes>()));
//    ON_CALL(mock, getLastKnownBTCBlocks).WillByDefault(Return(std::vector<VeriBlock::BlockBytes>()));
//    ON_CALL(mock, blockPopValidation).WillByDefault(Return(true));
//
//    setServiceMock<VeriBlock::PopService>(mock);
//}
//
//struct ServicesFixture {
//    UtilServiceMock util_service_mock;
//    VeriBlock::UtilServiceImpl util_service_impl;
//
//    ServicesFixture()
//    {
//        ON_CALL(util_service_mock, getPopRewards).WillByDefault(Return(VeriBlock::PoPRewards()));
//        ON_CALL(util_service_mock, isKeystone).WillByDefault(Return(false));
//        ON_CALL(util_service_mock, getPreviousKeystone).WillByDefault(Return(nullptr));
//        ON_CALL(util_service_mock, compareForks).WillByDefault(Return(0));
//        ON_CALL(util_service_mock, CheckPopInputs).WillByDefault(Return(true));
//        ON_CALL(util_service_mock, EvalScript).WillByDefault(Return(true));
//        ON_CALL(util_service_mock, validatePopTx).WillByDefault(Return(true));
//        ON_CALL(util_service_mock, checkCoinbaseTxWithPopRewards).WillByDefault(Return(true));
//
//        ON_CALL(util_service_mock, makeTopLevelRoot)
//            .WillByDefault(
//                [&](int height, const VeriBlock::KeystoneArray& keystones, const uint256& txRoot) -> uint256 {
//                    return util_service_impl.makeTopLevelRoot(height, keystones, txRoot);
//                });
//        ON_CALL(util_service_mock, getKeystoneHashesForTheNextBlock)
//            .WillByDefault(
//                [&](const CBlockIndex* pindexPrev) -> VeriBlock::KeystoneArray {
//                    return util_service_impl.getKeystoneHashesForTheNextBlock(pindexPrev);
//                });
//
//        setServiceMock<VeriBlock::UtilService>(util_service_mock);
//    }
//};

} // namespace VeriBlockTest

#endif //BITCOIN_SRC_VBK_TEST_UTIL_MOCK_HPP
