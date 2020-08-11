#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

#include <omnicore/createpayload.h>
#include <omnicore/script.h>
#include <omnicore/wallettxbuilder.h>

#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <script/standard.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>

#include <vector>

static void AddKey(CWallet& wallet, const CKey& key)
{
    LOCK(wallet.cs_wallet);
    wallet.AddKeyPubKey(key, key.GetPubKey());
}

class FundedSendTestingSetup : public TestChain100Setup
{
public:
    FundedSendTestingSetup()
    {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        wallet = std::make_shared<CWallet>(*m_chain, WalletLocation(), WalletDatabase::CreateMock());
        bool firstRun;
        wallet->LoadWallet(firstRun);
        AddKey(*wallet, coinbaseKey);
        WalletRescanReserver reserver(wallet.get());
        reserver.reserve();
        CWallet::ScanResult result = wallet->ScanForWalletTransactions(chainActive.Genesis()->GetBlockHash(), {}, reserver, false);
        interface_wallet = interfaces::MakeWallet(wallet);
    }

    ~FundedSendTestingSetup()
    {
        wallet.reset();
    }

    CWalletTx& AddTx(std::vector<CRecipient>& recipients)
    {
        CTransactionRef tx;
        CReserveKey reservekey(wallet.get());
        CAmount fee;
        int changePos = -1;
        std::string error;
        CCoinControl dummy;
        wallet->CreateTransaction(*m_locked_chain, recipients, tx, reservekey, fee, changePos, error, dummy);
        CValidationState state;
        wallet->CommitTransaction(tx, {}, {}, reservekey, nullptr, state);
        CMutableTransaction blocktx;
        {
            LOCK(wallet->cs_wallet);
            blocktx = CMutableTransaction(*wallet->mapWallet.at(tx->GetHash()).tx);
        }
        CreateAndProcessBlock({CMutableTransaction(blocktx)}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        LOCK(wallet->cs_wallet);
        auto it = wallet->mapWallet.find(tx->GetHash());
        it->second.SetMerkleBranch(chainActive.Tip()->GetBlockHash(), 1);
        return it->second;
    }

    // For dust set entry in amounts to -1
    std::vector<CTxDestination> CreateDestinations(std::vector<CAmount> amounts) {
        std::vector<CRecipient> recipients;
        std::vector<CTxDestination> destinations;
        for (auto amount : amounts) {
            CPubKey pubkey;
            wallet->GetKeyFromPool(pubkey);
            CTxDestination dest = GetDestinationForKey(pubkey, OutputType::LEGACY);
            destinations.push_back(dest);
            if (amount > 0) {
                recipients.push_back({GetScriptForDestination(dest), amount, false});
            } else if (amount == -1) {
                recipients.push_back({GetScriptForDestination(dest), OmniGetDustThreshold(GetScriptForDestination(dest)), false});
            }
        }
        AddTx(recipients);

        BOOST_CHECK_EQUAL(destinations.size(), amounts.size());

        return destinations;
    }

    std::unique_ptr<interfaces::Chain> m_chain = interfaces::MakeChain();
    std::unique_ptr<interfaces::Chain::Lock> m_locked_chain = m_chain->assumeLocked();
    std::shared_ptr<CWallet> wallet;
    std::unique_ptr<interfaces::Wallet> interface_wallet;
};

static std::vector<unsigned char> dummy_payload() {
    return CreatePayload_SimpleSend(1, 1);
}

static void check_outputs(uint256& hash, int expected_number) {
    CTransactionRef tx;
    uint256 hash_block;
    BOOST_CHECK(GetTransaction(hash, tx, Params().GetConsensus(), hash_block, nullptr));
    BOOST_CHECK_EQUAL(tx->vout.size(), expected_number);
}

BOOST_FIXTURE_TEST_SUITE(omnicore_funded_send_tests, FundedSendTestingSetup)

BOOST_AUTO_TEST_CASE(create_token_funded_by_source)
{
    std::vector<CTxDestination> destinations = CreateDestinations({1 * COIN, 0});

    uint256 hash;
    BOOST_CHECK_EQUAL(CreateFundedTransaction(EncodeDestination(destinations[0] /* source */), EncodeDestination(destinations[1] /* receiver */), EncodeDestination(destinations[1] /* receiver */), dummy_payload(), hash, interface_wallet.get()), 0);

    // Expect two outputs
    check_outputs(hash, 2);
}

BOOST_AUTO_TEST_CASE(create_token_funded_by_receiver_address)
{
    std::vector<CTxDestination> destinations = CreateDestinations({-1 /* Dust */, 1 * COIN});

    uint256 hash;
    BOOST_CHECK_EQUAL(CreateFundedTransaction(EncodeDestination(destinations[0] /* source */), EncodeDestination(destinations[1] /* receiver */), EncodeDestination(destinations[1] /* receiver */), dummy_payload(), hash, interface_wallet.get()), 0);

    // Expect two outputs
    check_outputs(hash, 2);
}

BOOST_AUTO_TEST_CASE(create_token_funded_by_fee_address)
{
    std::vector<CTxDestination> destinations = CreateDestinations({-1 /* Dust */, 0, 1 * COIN});

    uint256 hash;
    BOOST_CHECK_EQUAL(CreateFundedTransaction(EncodeDestination(destinations[0] /* source */), EncodeDestination(destinations[1] /* receiver */), EncodeDestination(destinations[2] /* fee */), dummy_payload(), hash, interface_wallet.get()), 0);

    // Expect three outputs
    check_outputs(hash, 3);
}

BOOST_AUTO_TEST_SUITE_END()
