// Copyright (c) 2025-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/args.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/wallet.h>
#include <test/util/mining.h>
#include <test/util/setup_common.h>
#include <node/miner.h>
#include <index/blockfilterindex.h>
#include <validation.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

namespace wallet {
namespace {

TestingSetup* g_setup;

void initialize()
{
    static const auto testing_setup = MakeNoLogFileContext<TestingSetup>();
    g_setup = testing_setup.get();
}

void remove_and_create_dirs()
{
    const auto blocks_dir{g_setup->m_args.GetBlocksDirPath()};
    const auto data_dir{g_setup->m_args.GetDataDirNet() / "blocks" / "index"};
    fs::remove_all(blocks_dir);
    fs::remove_all(data_dir);
    fs::create_directories(blocks_dir);
    fs::create_directories(data_dir);
}

FUZZ_TARGET(wallet_scan, .init = initialize)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    remove_and_create_dirs();
    g_setup->m_node.chainman.reset();
    g_setup->m_make_chainman();
    g_setup->LoadVerifyActivateChainstate();
    g_setup->m_node.validation_signals->SyncWithValidationInterfaceQueue();

    if (fuzzed_data_provider.ConsumeBool()) {
        InitBlockFilterIndex([&]{ return interfaces::MakeChain(g_setup->m_node); }, BlockFilterType::BASIC, 1 << 20, true, false);
        assert(g_setup->m_node.chain->hasBlockFilterIndex(BlockFilterType::BASIC));
    }

    if (fuzzed_data_provider.ConsumeBool()) DestroyAllBlockFilterIndexes();

    FuzzedWallet fuzzed_wallet{
        *g_setup->m_node.chain,
        "fuzzed_wallet_a",
        "tprv8ZgxMBicQKsPd1QwsGgzfu2pcPYbBosZhJknqreRHgsWx32nNEhMjGQX2cgFL8n6wz9xdDYwLcs78N4nsCo32cxEX8RBtwGsEGgybLiQJfk",
    };

    WalletRescanReserver reserver(*fuzzed_wallet.wallet);
    std::chrono::steady_clock::time_point fake_time;
    reserver.setNow([&] { return fake_time + std::chrono::seconds(fuzzed_data_provider.ConsumeIntegralInRange<uint64_t>(0, 1000)); });
    reserver.reserve();

    bool good_data{true};
    LIMITED_WHILE(good_data && fuzzed_data_provider.ConsumeBool(), 20) {
        node::BlockAssembler::Options options;
        options.test_block_validity = false;
        if (fuzzed_data_provider.ConsumeBool()) options.coinbase_output_script = fuzzed_wallet.GetScriptPubKey(fuzzed_data_provider);
        CBlock block = node::BlockAssembler{g_setup->m_node.chainman->ActiveChainstate(), nullptr, options}.CreateNewBlock()->block;

        assert(block.vtx.size() == 1);
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 20) {
            auto mtx = ConsumeDeserializable<CMutableTransaction>(fuzzed_data_provider, TX_WITH_WITNESS);
            if (!mtx) {
                good_data = false;
                return;
            }
            if (!mtx->vout.empty() && fuzzed_data_provider.ConsumeBool()) {
                auto vout_idx{fuzzed_data_provider.ConsumeIntegralInRange<size_t>(0, mtx->vout.size() - 1)};
                mtx->vout[vout_idx].scriptPubKey = fuzzed_wallet.GetScriptPubKey(fuzzed_data_provider);
            }
            block.vtx.push_back(MakeTransactionRef(*mtx));
        }
        node::RegenerateCommitments(block, *Assert(g_setup->m_node.chainman));
        std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
        auto process_block{g_setup->m_node.chainman->ProcessNewBlock(shared_pblock, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/nullptr)};
        if (process_block) {
            auto height{*Assert(g_setup->m_node.chain->getHeight())};
            {
                LOCK(fuzzed_wallet.wallet->cs_wallet);
                fuzzed_wallet.wallet->SetLastBlockProcessed(height, g_setup->m_node.chain->getBlockHash(height));
            }
        }
    }

    uint256 start_block{g_setup->m_node.chain->getBlockHash(0)};
    if (fuzzed_data_provider.ConsumeBool()) start_block = ConsumeUInt256(fuzzed_data_provider);
    auto start_height{fuzzed_data_provider.ConsumeIntegral<int>()};
    auto max_height{fuzzed_data_provider.ConsumeIntegral<int>()};

    if (fuzzed_data_provider.ConsumeBool()) fuzzed_wallet.wallet->AbortRescan();

    (void)fuzzed_wallet.wallet->ScanForWalletTransactions(start_block, start_height, max_height, reserver, /*fUpdate=*/fuzzed_data_provider.ConsumeBool(), /*save_progress=*/false);
}
} // namespace
} // namespace wallet
