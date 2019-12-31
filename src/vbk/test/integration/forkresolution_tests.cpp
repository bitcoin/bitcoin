#include <boost/test/unit_test.hpp>

#include <validation.h>

#include <vbk/test/integration/test_setup.hpp>

#include <fakeit.hpp>

using namespace fakeit;

static CBlock CreateTestBlock(TestChain100Setup& test, std::vector<CMutableTransaction> trxs = {})
{
    test.coinbaseKey.MakeNewKey(true);
    CScript scriptPubKey = CScript() << ToByteVector(test.coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    return test.CreateAndProcessBlock(trxs, scriptPubKey);
}

static void InvalidateTestBlock(CBlockIndex* pblock)
{
    CValidationState state;

    InvalidateBlock(state, Params(), pblock);
    ActivateBestChain(state, Params());
    mempool.clear();
}

static void ReconsiderTestBlock(CBlockIndex* pblock)
{
    CValidationState state;

    {
        LOCK(cs_main);
        ResetBlockFailureFlags(pblock);
    }
    ActivateBestChain(state, Params(), std::shared_ptr<const CBlock>());
}

//Before starting this test you should configurate the alt-service properties file with the concrete bootstrap blocks
//The bootstrap blocks are contained in the vbk/test/integration/utils.cpp source file
BOOST_FIXTURE_TEST_SUITE(forkresolution_tests, VeriBlockTest::IntegrationTestFixture)

BOOST_AUTO_TEST_CASE(basic_test)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;

    auto& grpc_integration_service = VeriBlock::getService<VeriBlock::GrpcIntegrationService>();
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    InvalidateTestBlock(ChainActive().Tip());
    CreateTestBlock(*this);

    CBlockIndex* pblock = ChainActive().Tip();

    CBlockIndex* endorsedBlockIndex = ChainActive().Tip();
    CBlock endorsedBlock;

    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));

    VeriBlock::AltPublication altPublication = VeriBlockTest::generateSignedAltPublication(endorsedBlock, VeriBlock::getService<VeriBlock::Config>().index.unwrap(), scriptPubKey);

    BOOST_CHECK(grpc_integration_service.verifyAltPublication(altPublication));
    BOOST_CHECK(grpc_integration_service.checkATVAgainstView(altPublication));


    std::vector<uint8_t> atvBytes = grpc_integration_service.serializeAltPublication(altPublication);
    CMutableTransaction popTx = VeriBlockTest::makePopTx(atvBytes, {});

    CreateTestBlock(*this, {popTx});

    CBlockIndex* pblockwins = ChainActive().Tip();
    BOOST_CHECK(pblockwins->nTx == 2);

    InvalidateTestBlock(pblock);

    CreateTestBlock(*this);
    CreateTestBlock(*this);
    CreateTestBlock(*this);
    CreateTestBlock(*this);
    CreateTestBlock(*this);
    CreateTestBlock(*this); // Add a few blocks which it is mean that for the old variant of the fork resolution it should be the main chain

    CBlockIndex* pblockTip = ChainActive().Tip();

    ReconsiderTestBlock(pblock);
    BOOST_CHECK(pblockwins == ChainActive().Tip());
}
BOOST_AUTO_TEST_SUITE_END()
