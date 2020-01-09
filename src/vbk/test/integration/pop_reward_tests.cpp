#include <boost/test/unit_test.hpp>

#include <chainparams.h>
#include <script/interpreter.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vbk/init.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/integration/grpc_integration_service.hpp>
#include <vbk/test/integration/test_setup.hpp>
#include <vbk/test/integration/utils.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>
#include <vbk/util_service.hpp>
#include <vbk/util_service/util_service_impl.hpp>

#include <algorithm>

#include <fakeit.hpp>

using namespace fakeit;

//Before starting this test you should configurate the alt-service properties file with the concrete bootstrap blocks
//The bootstrap blocks are contained in the vbk/test/integration/utils.cpp source file
BOOST_FIXTURE_TEST_SUITE(pop_reward_tests, VeriBlockTest::IntegrationTestFixture)

BOOST_AUTO_TEST_CASE(basic_test)
{
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
