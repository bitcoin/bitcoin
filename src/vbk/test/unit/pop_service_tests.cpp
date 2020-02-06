#include <boost/test/unit_test.hpp>
#include <consensus/validation.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <fakeit.hpp>
#include <vbk/config.hpp>
#include <vbk/init.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/pop_service/pop_service_impl.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>

using namespace fakeit;

static CBlock createBlockWithPopTx(TestChain100Setup& test)
{
    CMutableTransaction popTx = VeriBlockTest::makePopTx({1}, {{2}});
    CScript scriptPubKey = CScript() << ToByteVector(test.coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    return test.CreateAndProcessBlock({popTx}, scriptPubKey);
}

inline void setPublicationData(VeriBlock::PublicationData& pub, const CDataStream& stream, const int64_t& index)
{
    pub.set_identifier(index);
    pub.set_header(stream.data(), stream.size());
}

struct PopServiceFixture : public TestChain100Setup {
    Mock<VeriBlock::PopServiceImpl> pop_service_impl_mock{};

    PopServiceFixture()
    {
        VeriBlock::InitUtilService();
        VeriBlock::InitConfig();
        Fake(Method(pop_service_impl_mock, addPayloads));
        Fake(Method(pop_service_impl_mock, removePayloads));
        Fake(Method(pop_service_impl_mock, clearTemporaryPayloads));
        When(OverloadedMethod(pop_service_impl_mock, parsePopTx, bool(const CTransactionRef&, ScriptError*, VeriBlock::Publications*, VeriBlock::Context*, VeriBlock::PopTxType*)))
            .Do([](const CTransactionRef&, ScriptError* serror, VeriBlock::Publications*, VeriBlock::Context*, VeriBlock::PopTxType* type) -> bool {
                if (type != nullptr) {
                    *type = VeriBlock::PopTxType::PUBLICATIONS;
                }
                if (serror != nullptr) {
                    *serror = ScriptError::SCRIPT_ERR_OK;
                }
                return true;
            });
        When(Method(pop_service_impl_mock, determineATVPlausibilityWithBTCRules)).AlwaysReturn(true);
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

    When(OverloadedMethod(pop_service_impl_mock, getPublicationsData, void(const VeriBlock::Publications&, VeriBlock::PublicationData&)))
        .Do([stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
            setPublicationData(publicationData, stream, config.index.unwrap());
        });


    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(VeriBlock::blockPopValidationImpl(pop_service_impl_mock.get(), block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
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
    When(OverloadedMethod(pop_service_impl_mock, getPublicationsData, void(const VeriBlock::Publications&, VeriBlock::PublicationData&)))
        .Do([stream](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
            setPublicationData(publicationData, stream, -1);
        });

    When(Method(pop_service_impl_mock, determineATVPlausibilityWithBTCRules)).AlwaysDo([](VeriBlock::AltchainId altChainIdentifier, const CBlockHeader& popEndorsementHeader, const Consensus::Params& params, TxValidationState& state) -> bool {
        return VeriBlock::PopServiceImpl().determineATVPlausibilityWithBTCRules(altChainIdentifier, popEndorsementHeader, params, state);
    });

    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock.get(), block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-altchain-id");
    }
    Verify_Method(Method(pop_service_impl_mock, removePayloads)).Once();
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

    When(OverloadedMethod(pop_service_impl_mock, getPublicationsData, void(const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData)))
        .Do([stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
            setPublicationData(publicationData, stream, config.index.unwrap());
        });

    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock.get(), block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-endorsed-block-not-from-this-chain");
    }
    Verify_Method(Method(pop_service_impl_mock, removePayloads)).Once();
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

    When(OverloadedMethod(pop_service_impl_mock, getPublicationsData, void(const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData)))
        .Do([stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
            setPublicationData(publicationData, stream, config.index.unwrap());
        });

    {
        BlockValidationState state;
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock.get(), block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-endorsed-block-not-known-orphan-block");
    }
    Verify_Method(Method(pop_service_impl_mock, removePayloads)).Once();
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

    When(OverloadedMethod(pop_service_impl_mock, getPublicationsData, void(const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData)))
        .Do([stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
            setPublicationData(publicationData, stream, config.index.unwrap());
        });

    config.POP_REWARD_SETTLEMENT_INTERVAL = 0;
    VeriBlock::setService<VeriBlock::Config>(new VeriBlock::Config(config));

    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock.get(), block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-endorsed-block-too-old");
    }
    Verify_Method(Method(pop_service_impl_mock, removePayloads)).Once();
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

    When(OverloadedMethod(pop_service_impl_mock, getPublicationsData, void(const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData)))
        .Do([stream, config](const VeriBlock::Publications& pub, VeriBlock::PublicationData& publicationData) {
            setPublicationData(publicationData, stream, config.index.unwrap());
        });

    When(Method(pop_service_impl_mock, addPayloads)).AlwaysDo([](std::string hash, const int& nHeight, const VeriBlock::Publications& publications) -> void {
        throw VeriBlock::PopServiceException("fail");
    });

    BlockValidationState state;
    {
        LOCK(cs_main);
        BOOST_CHECK(!blockPopValidationImpl(pop_service_impl_mock.get(), block, *ChainActive().Tip()->pprev, Params().GetConsensus(), state));
        BOOST_CHECK_EQUAL(state.GetRejectReason(), "pop-tx-add-payloads-failed");
    }
    Verify_Method(Method(pop_service_impl_mock, removePayloads)).Once();
}
BOOST_AUTO_TEST_SUITE_END()
