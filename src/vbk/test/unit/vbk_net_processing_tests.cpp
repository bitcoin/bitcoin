#include <boost/test/unit_test.hpp>

#include <chainparams.h>
#include <net_processing.h>
#include <validation.h>
#include <wallet/wallet.h>
#include <test/setup_common.h>

#include <vbk/test/util/mock.hpp>
#include <vbk/test/util/tx.hpp>

struct NetServiceFixture : public TestingSetup {
    VeriBlockTest::ServicesFixture service_fixture;
};

static CService ip(uint32_t i)
{
    struct in_addr s;
    s.s_addr = i;
    return CService(CNetAddr(s), Params().GetDefaultPort());
}

BOOST_FIXTURE_TEST_SUITE(vbk_net_processing, NetServiceFixture)

BOOST_AUTO_TEST_CASE(SendMessages_with_pop_tx)
{
    CMutableTransaction popTx1 = VeriBlockTest::makePopTx({ 1 }, { std::vector<uint8_t>(100, 1) });
    CMutableTransaction popTx2 = VeriBlockTest::makePopTx({ 2 }, { std::vector<uint8_t>(100, 2) });
    CMutableTransaction popTx3 = VeriBlockTest::makePopTx({ 3 }, { std::vector<uint8_t>(100, 3) });

    auto connman = MakeUnique<CConnman>(0x1337, 0x1337);
    auto peerLogic = MakeUnique<PeerLogicValidation>(connman.get(), nullptr, scheduler);

    // Mock an outbound peer
    CAddress addr1(ip(0xa0b0c001), NODE_NONE);
    CNode dummyNode1(0, ServiceFlags(NODE_NETWORK|NODE_WITNESS), 0, INVALID_SOCKET, addr1, 0, 0, CAddress(), "", /*fInboundIn=*/ false);
    dummyNode1.SetSendVersion(PROTOCOL_VERSION);

    peerLogic->InitializeNode(&dummyNode1);
    {
        LOCK2(dummyNode1.m_tx_relay->cs_tx_inventory, dummyNode1.m_tx_relay->cs_feeFilter);
        dummyNode1.nVersion = 1;
        dummyNode1.fSuccessfullyConnected = true;
        dummyNode1.m_tx_relay->fSendMempool = true;
        dummyNode1.m_tx_relay->minFeeFilter = 1000;
    }

    {
        LOCK2(cs_main, dummyNode1.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode1)); // should result in getheaders
    }
    {
        LOCK2(cs_main, dummyNode1.m_tx_relay->cs_tx_inventory);
        BOOST_CHECK(!dummyNode1.m_tx_relay->filterInventoryKnown.contains(popTx1.GetHash()));
        BOOST_CHECK(!dummyNode1.m_tx_relay->filterInventoryKnown.contains(popTx2.GetHash())); 
        BOOST_CHECK(!dummyNode1.m_tx_relay->filterInventoryKnown.contains(popTx3.GetHash()));
    }

    {
        TestMemPoolEntryHelper entry;
        LOCK2(cs_main, mempool.cs);
        CValidationState state;
        bool fMissingInputs;
        BOOST_CHECK(AcceptToMemoryPool(mempool, state, MakeTransactionRef(popTx1), &fMissingInputs, nullptr, false, DEFAULT_TRANSACTION_MAXFEE));
        BOOST_CHECK(AcceptToMemoryPool(mempool, state, MakeTransactionRef(popTx2), &fMissingInputs, nullptr, false, DEFAULT_TRANSACTION_MAXFEE));
        BOOST_CHECK(AcceptToMemoryPool(mempool, state, MakeTransactionRef(popTx3), &fMissingInputs, nullptr, false, DEFAULT_TRANSACTION_MAXFEE));
    }

    CNode dummyNode2(0, ServiceFlags(NODE_NETWORK|NODE_WITNESS), 0, INVALID_SOCKET, addr1, 0, 0, CAddress(), "", /*fInboundIn=*/ false);
    dummyNode2.SetSendVersion(PROTOCOL_VERSION);

    peerLogic->InitializeNode(&dummyNode2);
    {
        LOCK2(dummyNode2.m_tx_relay->cs_tx_inventory, dummyNode2.m_tx_relay->cs_feeFilter);
        dummyNode2.nVersion = 1;
        dummyNode2.fSuccessfullyConnected = true;
        dummyNode2.m_tx_relay->fSendMempool = true;
        dummyNode2.m_tx_relay->minFeeFilter = 1000;
    }

    {
        LOCK2(cs_main, dummyNode2.cs_sendProcessing);
        BOOST_CHECK(peerLogic->SendMessages(&dummyNode2)); // should result in getheaders
    }
    {
        LOCK2(cs_main, dummyNode2.m_tx_relay->cs_tx_inventory);
        BOOST_CHECK(dummyNode2.m_tx_relay->filterInventoryKnown.contains(popTx1.GetHash()));
        BOOST_CHECK(dummyNode2.m_tx_relay->filterInventoryKnown.contains(popTx2.GetHash()));
        BOOST_CHECK(dummyNode2.m_tx_relay->filterInventoryKnown.contains(popTx3.GetHash()));
    }
}

BOOST_AUTO_TEST_SUITE_END()