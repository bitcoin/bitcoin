#ifndef BITCOIN_SRC_VBK_TEST_UTIL_MOCK_HPP
#define BITCOIN_SRC_VBK_TEST_UTIL_MOCK_HPP

#include <fakeit.hpp>

#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/util_service.hpp>
#include <vbk/util_service/util_service_impl.hpp>

namespace VeriBlockTest {

template <typename T>
void setServiceMock(fakeit::Mock<T>& mock)
{
    fakeit::Fake(Dtor(mock));
    VeriBlock::setService<T>(std::shared_ptr<T>(&mock.get(), [](T*) { /* empty destructor*/ }));
}

inline void setUpPopServiceMock(fakeit::Mock<VeriBlock::PopService>& mock)
{
    fakeit::When(Method(mock, checkVTBinternally)).AlwaysReturn(true);
    fakeit::When(Method(mock, checkATVinternally)).AlwaysReturn(true);
    fakeit::When(Method(mock, compareTwoBranches)).AlwaysReturn(0);
    fakeit::Fake(Method(mock, rewardsCalculateOutputs));
    fakeit::Fake(Method(mock, savePopTxToDatabase));
    fakeit::When(Method(mock, getLastKnownVBKBlocks)).AlwaysReturn({});
    fakeit::When(Method(mock, getLastKnownBTCBlocks)).AlwaysReturn({});
    fakeit::When(Method(mock, blockPopValidation)).AlwaysReturn(true);
    fakeit::Fake(Method(mock, updateContext));

    fakeit::Fake(OverloadedMethod(mock, addPayloads, void(std::string, const int&, const VeriBlock::Publications&)));
    fakeit::Fake(OverloadedMethod(mock, addPayloads, void(const CBlockIndex &, const CBlock &)));
    fakeit::Fake(OverloadedMethod(mock, removePayloads, void(std::string, const int&)));
    fakeit::Fake(OverloadedMethod(mock, removePayloads, void(const CBlockIndex &)));

    setServiceMock(mock);
}

struct ServicesFixture {
    fakeit::Mock<VeriBlock::UtilService> util_service_mock;
    VeriBlock::UtilServiceImpl util_service_impl;

    ServicesFixture()
    {
        fakeit::When(Method(util_service_mock, getPopRewards)).AlwaysReturn({});
        fakeit::When(Method(util_service_mock, isKeystone)).AlwaysReturn(false);
        fakeit::When(Method(util_service_mock, getPreviousKeystone)).AlwaysReturn(nullptr);
        fakeit::When(Method(util_service_mock, compareForks)).AlwaysReturn(0);
        fakeit::When(Method(util_service_mock, CheckPopInputs)).AlwaysReturn(true);
        fakeit::When(Method(util_service_mock, EvalScript)).AlwaysReturn(true);
        fakeit::When(Method(util_service_mock, validatePopTx)).AlwaysReturn(true);
        fakeit::When(Method(util_service_mock, checkCoinbaseTxWithPopRewards)).AlwaysReturn(true);
        fakeit::Fake(Method(util_service_mock, addPopPayoutsIntoCoinbaseTx));

        fakeit::When(Method(util_service_mock, makeTopLevelRoot)).AlwaysDo([&](int height, const VeriBlock::KeystoneArray& keystones, const uint256& txRoot) -> uint256 {
            return util_service_impl.makeTopLevelRoot(height, keystones, txRoot);
        });

        fakeit::When(Method(util_service_mock, getKeystoneHashesForTheNextBlock)).AlwaysDo([&](const CBlockIndex* pindexPrev) -> VeriBlock::KeystoneArray {
            return util_service_impl.getKeystoneHashesForTheNextBlock(pindexPrev);
        });

        setServiceMock<VeriBlock::UtilService>(util_service_mock);
    }
};

} // namespace VeriBlockTest

#endif //BITCOIN_SRC_VBK_TEST_UTIL_MOCK_HPP
