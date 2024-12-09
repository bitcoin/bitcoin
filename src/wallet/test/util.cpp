// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/test/util.h>

#include <chain.h>
#include <key.h>
#include <key_io.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <validationinterface.h>
#include <wallet/context.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

#include <memory>

namespace wallet {
std::unique_ptr<CWallet> CreateSyncedWallet(interfaces::Chain& chain, CChain& cchain, const CKey& key)
{
    auto wallet = std::make_unique<CWallet>(&chain, "", CreateMockableWalletDatabase());
    {
        LOCK2(wallet->cs_wallet, ::cs_main);
        wallet->SetLastBlockProcessed(cchain.Height(), cchain.Tip()->GetBlockHash());
    }
    {
        LOCK(wallet->cs_wallet);
        wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
        wallet->SetupDescriptorScriptPubKeyMans();

        FlatSigningProvider provider;
        std::string error;
        auto descs = Parse("combo(" + EncodeSecret(key) + ")", provider, error, /* require_checksum=*/ false);
        assert(descs.size() == 1);
        auto& desc = descs.at(0);
        WalletDescriptor w_desc(std::move(desc), 0, 0, 1, 1);
        if (!wallet->AddWalletDescriptor(w_desc, provider, "", false)) assert(false);
    }
    WalletRescanReserver reserver(*wallet);
    reserver.reserve();
    CWallet::ScanResult result = wallet->ScanForWalletTransactions(cchain.Genesis()->GetBlockHash(), /*start_height=*/0, /*max_height=*/{}, reserver, /*fUpdate=*/false, /*save_progress=*/false);
    assert(result.status == CWallet::ScanResult::SUCCESS);
    assert(result.last_scanned_block == cchain.Tip()->GetBlockHash());
    assert(*result.last_scanned_height == cchain.Height());
    assert(result.last_failed_block.IsNull());
    return wallet;
}

std::shared_ptr<CWallet> TestLoadWallet(std::unique_ptr<WalletDatabase> database, WalletContext& context, uint64_t create_flags)
{
    bilingual_str error;
    std::vector<bilingual_str> warnings;
    auto wallet = CWallet::Create(context, "", std::move(database), create_flags, error, warnings);
    NotifyWalletLoaded(context, wallet);
    if (context.chain) {
        wallet->postInitProcess();
    }
    return wallet;
}

std::shared_ptr<CWallet> TestLoadWallet(WalletContext& context)
{
    DatabaseOptions options;
    options.create_flags = WALLET_FLAG_DESCRIPTORS;
    DatabaseStatus status;
    bilingual_str error;
    std::vector<bilingual_str> warnings;
    auto database = MakeWalletDatabase("", options, status, error);
    return TestLoadWallet(std::move(database), context, options.create_flags);
}

void TestUnloadWallet(std::shared_ptr<CWallet>&& wallet)
{
    // Calls SyncWithValidationInterfaceQueue
    wallet->chain().waitForNotificationsIfTipChanged({});
    wallet->m_chain_notifications_handler.reset();
    WaitForDeleteWallet(std::move(wallet));
}

std::unique_ptr<WalletDatabase> DuplicateMockDatabase(WalletDatabase& database)
{
    return std::make_unique<MockableDatabase>(dynamic_cast<MockableDatabase&>(database).m_records);
}

std::string getnewaddress(CWallet& w)
{
    constexpr auto output_type = OutputType::BECH32;
    return EncodeDestination(getNewDestination(w, output_type));
}

CTxDestination getNewDestination(CWallet& w, OutputType output_type)
{
    return *Assert(w.GetNewDestination(output_type, ""));
}

MockableDatabase& GetMockableDatabase(CWallet& wallet)
{
    return dynamic_cast<MockableDatabase&>(wallet.GetDatabase());
}
} // namespace wallet
