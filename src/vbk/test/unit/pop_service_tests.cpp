#include <boost/test/unit_test.hpp>
#include <consensus/validation.h>
#include <shutdown.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <vbk/config.hpp>
#include <vbk/init.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/pop_service/pop_service_impl.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>

using ::testing::Return;

static CBlock createBlockWithPopTx(TestChain100Setup& test)
{
    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
    CScript scriptPubKey = CScript() << ToByteVector(test.coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    return test.CreateAndProcessBlock({popTx}, scriptPubKey);
}

inline void setPublicationData(VeriBlock::PublicationData& pub, const CDataStream& stream, const int64_t& index)
{
    pub.set_identifier(index);
    pub.set_header((void*)stream.data(), stream.size());
}

struct PopServiceFixture : public TestChain100Setup {
    testing::NiceMock<VeriBlockTest::PopServiceImplMock> pop_service_impl_mock;

    PopServiceFixture()
    {
        AbortShutdown();
        VeriBlock::InitUtilService();
        VeriBlock::InitConfig();
        VeriBlockTest::setUpPopServiceMock(pop_service_mock);

        ON_CALL(pop_service_impl_mock, parsePopTx)
            .WillByDefault(
                [](const CTransactionRef&, ScriptError* serror, VeriBlock::Publications*, VeriBlock::Context*, VeriBlock::PopTxType* type) -> bool {
                    if (type != nullptr) {
                        *type = VeriBlock::PopTxType::PUBLICATIONS;
                    }
                    if (serror != nullptr) {
                        *serror = ScriptError::SCRIPT_ERR_OK;
                    }
                    return true;
                });
        ON_CALL(pop_service_impl_mock, determineATVPlausibilityWithBTCRules)
            .WillByDefault(Return(true));
        ON_CALL(pop_service_impl_mock, addTemporaryPayloads)
            .WillByDefault(
              [&](const CTransactionRef& tx, const CBlockIndex& pindexPrev, const Consensus::Params& params, TxValidationState& state) {
                return VeriBlock::addTemporaryPayloadsImpl(pop_service_impl_mock, tx, pindexPrev, params, state);
            });
        ON_CALL(pop_service_impl_mock, clearTemporaryPayloads)
            .WillByDefault(
                [&]() {
                    VeriBlock::clearTemporaryPayloadsImpl(pop_service_impl_mock);
                });

        VeriBlock::initTemporaryPayloadsMock(pop_service_impl_mock);
    }

    void setNoAddRemovePayloadsExpectations()
    {
        EXPECT_CALL(pop_service_impl_mock, addPayloads).Times(0);
        EXPECT_CALL(pop_service_impl_mock, removePayloads).Times(0);
    }
};

BOOST_AUTO_TEST_SUITE(pop_service_tests)

BOOST_FIXTURE_TEST_CASE(blockPopValidation_test, PopServiceFixture)
{
    CBlock block = createBlockWithPopTx(*this);

    CBlockIndex* endorsedBlockIndex = ChainActive().Tip()->pprev;
    CBlock endorsedBlock;
    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << endorsedBlock.GetBlockHeader();
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    ON_CALL(pop_service_impl_mock, getPublicationsData)
        .WillByDefault(
            [stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
                setPublicationData(publicationData, stream, config.index.unwrap());
            });

    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(VeriBlock::blockPopValidationImpl(pop_service_impl_mock, block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
    }
}

BOOST_FIXTURE_TEST_CASE(blockPopValidation_test_wrong_index, PopServiceFixture)
{
    CBlock block = createBlockWithPopTx(*this);

    CBlockIndex* endorsedBlockIndex = ChainActive().Tip()->pprev->pprev->pprev;
    CBlock endorsedBlock;
    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << endorsedBlock.GetBlockHeader();

    // make another index
    ON_CALL(pop_service_impl_mock, getPublicationsData)
        .WillByDefault(
            [stream](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
                setPublicationData(publicationData, stream, -1);
            });
    ON_CALL(pop_service_impl_mock, determineATVPlausibilityWithBTCRules)
        .WillByDefault(
            [](VeriBlock::AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader,
              const Consensus::Params& params, TxValidationState& state) -> bool {
                VeriBlock::PopServiceImpl pop_service_impl(false, false);
                return pop_service_impl.determineATVPlausibilityWithBTCRules(altChainIdentifier, popEndorsementHeader, params, state);
            });
    setNoAddRemovePayloadsExpectations();

    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock, block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-altchain-id");
    }

    testing::Mock::VerifyAndClearExpectations(&pop_service_impl_mock);
}

BOOST_FIXTURE_TEST_CASE(blockPopValidation_test_endorsed_block_not_known_orphan_block, PopServiceFixture)
{
    CBlockIndex* endorsedBlockIndex = ChainActive().Tip();
    CBlock endorsedBlock;
    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));
    endorsedBlock.hashPrevBlock.SetHex("ff");

    CBlock block = createBlockWithPopTx(*this);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << endorsedBlock.GetBlockHeader();
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    ON_CALL(pop_service_impl_mock, getPublicationsData)
        .WillByDefault(
            [stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
                setPublicationData(publicationData, stream, config.index.unwrap());
            });
    setNoAddRemovePayloadsExpectations();

    {
        BlockValidationState state;
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock, block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-endorsed-block-not-known-orphan-block");
    }

    testing::Mock::VerifyAndClearExpectations(&pop_service_impl_mock);
}

BOOST_FIXTURE_TEST_CASE(blockPopValidation_test_endorsed_block_not_from_chain, PopServiceFixture)
{
    CBlockIndex* endorsedBlockIndex = ChainActive().Tip()->pprev->pprev;
    CBlock endorsedBlock;
    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));
    int prevHeight = endorsedBlockIndex->nHeight;

    BlockValidationState state;
    BOOST_CHECK(InvalidateBlock(state, Params(), endorsedBlockIndex->pprev));
    BOOST_CHECK(ActivateBestChain(state, Params()));
    BOOST_CHECK(ChainActive().Height() < prevHeight);

    CScript scriptPubKey = CScript() << OP_CHECKSIG;
    CreateAndProcessBlock({}, scriptPubKey);
    CreateAndProcessBlock({}, scriptPubKey);
    CreateAndProcessBlock({}, scriptPubKey);

    CBlock block = createBlockWithPopTx(*this);

    BOOST_CHECK(ChainActive().Height() > prevHeight);
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << endorsedBlock.GetBlockHeader();
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    ON_CALL(pop_service_impl_mock, getPublicationsData)
        .WillByDefault(
            [stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
                setPublicationData(publicationData, stream, config.index.unwrap());
            });
    setNoAddRemovePayloadsExpectations();

    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock, block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-endorsed-block-not-from-this-chain");
    }

    testing::Mock::VerifyAndClearExpectations(&pop_service_impl_mock);
}

BOOST_FIXTURE_TEST_CASE(blockPopValidation_test_wrong_settlement_interval, PopServiceFixture)
{
    CBlockIndex* endorsedBlockIndex = ChainActive().Tip()->pprev->pprev->pprev;
    CBlock endorsedBlock;
    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));

    CBlock block = createBlockWithPopTx(*this);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << endorsedBlock.GetBlockHeader();
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    ON_CALL(pop_service_impl_mock, getPublicationsData)
        .WillByDefault(
            [stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
                setPublicationData(publicationData, stream, config.index.unwrap());
            });
    setNoAddRemovePayloadsExpectations();

    config.POP_REWARD_SETTLEMENT_INTERVAL = 0;
    VeriBlock::setService<VeriBlock::Config>(new VeriBlock::Config(config));

    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock, block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-endorsed-block-too-old");
    }

    testing::Mock::VerifyAndClearExpectations(&pop_service_impl_mock);
}

BOOST_FIXTURE_TEST_CASE(blockPopValidation_test_wrong_addPayloads, PopServiceFixture)
{
    CBlockIndex* endorsedBlockIndex = ChainActive().Tip()->pprev->pprev->pprev;
    CBlock endorsedBlock;
    BOOST_CHECK(ReadBlockFromDisk(endorsedBlock, endorsedBlockIndex, Params().GetConsensus()));

    CBlock block = createBlockWithPopTx(*this);

    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << endorsedBlock.GetBlockHeader();
    auto& config = VeriBlock::getService<VeriBlock::Config>();

    ON_CALL(pop_service_impl_mock, getPublicationsData)
        .WillByDefault(
            [stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
                setPublicationData(publicationData, stream, config.index.unwrap());
            });
    ON_CALL(pop_service_impl_mock, addPayloads)
        .WillByDefault(
            [](std::string hash, const int& nHeight, const VeriBlock::Publications& publications) -> void {
                throw VeriBlock::PopServiceException("fail");
            });
    EXPECT_CALL(pop_service_impl_mock, addPayloads).Times(1);
    EXPECT_CALL(pop_service_impl_mock, removePayloads).Times(0);

    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock, block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-add-payloads-failed");
    }

    testing::Mock::VerifyAndClearExpectations(&pop_service_impl_mock);
}
BOOST_AUTO_TEST_SUITE_END()
