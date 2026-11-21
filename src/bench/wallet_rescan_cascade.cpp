// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include <addresstype.h>
#include <bench/bench.h>
#include <chain.h>
#include <consensus/amount.h>
#include <key.h>
#include <key_io.h>
#include <policy/feerate.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <sync.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/check.h>
#include <util/hasher.h>
#include <validation.h>
#include <wallet/db.h>
#include <wallet/scan.h>
#include <wallet/test/util.h>
#include <wallet/transaction.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace wallet {
namespace {

// Matches the node's own default (-keypool=1000), so the benchmark reflects
// a realistic look-ahead pool size rather than a test-only small one.
constexpr int KEYPOOL_SIZE = 1000;

// Models a block where the wallet's own transactions are a small minority
// among many unrelated ones, interleaved around a pool-expanding cascade.
constexpr size_t NUM_FOREIGN_TX = 2000;

constexpr CAmount FUNDING_OUTPUT_VALUE = 100000;
constexpr CAmount SPEND_OUTPUT_VALUE = 90000;

// Derive the scriptPubKey at a given index directly from the descriptor
// string, independent of any wallet cache or range_end bookkeeping -- the
// same technique the deriveaddresses RPC uses -- so building the test block
// cannot itself influence the very look-ahead state the benchmark measures.
CScript ScriptAtIndex(const std::string& desc_str, int index)
{
    FlatSigningProvider provider;
    std::string error;
    auto descs{Parse(desc_str, provider, error, /*require_checksum=*/false)};
    Assert(descs.size() == 1);
    std::vector<CScript> scripts;
    FlatSigningProvider out;
    Assert(descs.at(0)->Expand(index, provider, scripts, out));
    Assert(scripts.size() == 1);
    return scripts.at(0);
}

//! Fixture built once: a real, consensus-valid chain containing one block
//! with NUM_FOREIGN_TX foreign transactions and a three-level look-ahead
//! pool cascade (E expands the pool, which makes A visible; A's own
//! recognition expands the pool again, which makes B visible) interleaved
//! among them, exercising ChainScanner::ScanBlock's retry-until-stable loop
//! the same way a large block with embedded wallet activity would.
struct CascadeChain {
    std::unique_ptr<TestChain100Setup> node{MakeNoLogFileContext<TestChain100Setup>()};
    std::string desc_str;
    uint256 cascade_block_hash;
    int cascade_block_height;

    CascadeChain()
    {
        CExtKey ext_key;
        constexpr std::array<std::byte, 32> seed{std::byte{1}};
        ext_key.SetSeed(seed);
        desc_str = "wpkh(" + EncodeExtKey(ext_key) + "/0/*)";

        const int idx_e = KEYPOOL_SIZE - 1;
        const int idx_a = KEYPOOL_SIZE;
        const int idx_b = 2 * KEYPOOL_SIZE;
        const CScript script_e{ScriptAtIndex(desc_str, idx_e)};
        const CScript script_a{ScriptAtIndex(desc_str, idx_a)};
        const CScript script_b{ScriptAtIndex(desc_str, idx_b)};

        // Fund a disposable key with enough small UTXOs, in their own block,
        // to build every foreign and cascade transaction below from. Doing
        // this ahead of time keeps the funding tx's own validation out of
        // the chain the benchmark later scans.
        const CKey funding_key{GenerateRandomKey()};
        const CScript funding_spk{GetScriptForDestination(WitnessV0KeyHash(funding_key.GetPubKey()))};
        const size_t num_outputs = NUM_FOREIGN_TX + 3;
        std::vector<CTxOut> funding_outputs(num_outputs, CTxOut(FUNDING_OUTPUT_VALUE, funding_spk));
        auto [funding_tx, _]{node->CreateValidTransaction(
            {node->m_coinbase_txns[0]}, {COutPoint(node->m_coinbase_txns[0]->GetHash(), 0)},
            /*input_height=*/1, {node->coinbaseKey}, funding_outputs, std::nullopt, std::nullopt)};
        node->CreateAndProcessBlock({funding_tx}, funding_spk);
        const CTransactionRef funding_tx_ref{MakeTransactionRef(funding_tx)};
        const int funding_height = WITH_LOCK(Assert(node->m_node.chainman)->GetMutex(), return node->m_node.chainman->ActiveChain().Height());

        // One small 1-in-1-out spend per funding output: foreign ones churn
        // back to the funding key, cascade ones pay the wallet's own
        // look-ahead addresses at the positions computed above.
        auto make_spend = [&](size_t vout_index, const CScript& dest) {
            return node->CreateValidTransaction(
                {funding_tx_ref}, {COutPoint(funding_tx_ref->GetHash(), vout_index)},
                funding_height, {funding_key}, {CTxOut(SPEND_OUTPUT_VALUE, dest)},
                std::nullopt, std::nullopt).first;
        };

        std::vector<CMutableTransaction> block_txs;
        block_txs.reserve(num_outputs);
        size_t next_output = 0;
        // Reverse dependency order: E (resolvable immediately, within the
        // initial pool) sits LAST in vtx order; B (resolvable only after
        // both E's and A's expansions) sits FIRST. A single forward pass
        // can then resolve at most one new level per pass -- whichever
        // one's dependency was satisfied by the previous pass -- forcing
        // genuine retries instead of resolving the whole chain in one
        // sweep.
        const size_t foreign_per_segment = NUM_FOREIGN_TX / 4;
        for (const CScript* cascade_script : {&script_b, &script_a, &script_e}) {
            for (size_t i = 0; i < foreign_per_segment; ++i) {
                block_txs.push_back(make_spend(next_output++, funding_spk));
            }
            block_txs.push_back(make_spend(next_output++, *cascade_script));
        }
        while (next_output < num_outputs) {
            block_txs.push_back(make_spend(next_output++, funding_spk));
        }

        const CBlock cascade_block{node->CreateAndProcessBlock(block_txs, funding_spk)};
        cascade_block_hash = cascade_block.GetHash();
        cascade_block_height = WITH_LOCK(Assert(node->m_node.chainman)->GetMutex(), return node->m_node.chainman->ActiveChain().Height());
    }
};

void RescanCascadeInLargeBlock(benchmark::Bench& bench)
{
    CascadeChain chain;

    std::unique_ptr<CWallet> wallet;
    std::unique_ptr<WalletRescanReserver> reserver;
    bench.setup([&] {
        // Release the previous iteration's reservation before tearing down
        // its wallet: WalletRescanReserver holds a CWallet&, so destroying
        // the wallet first would leave it dangling.
        reserver.reset();
        wallet = std::make_unique<CWallet>(chain.node->m_node.chain.get(), "", CreateMockableWalletDatabase());
        {
            LOCK(wallet->cs_wallet);
            LOCK(Assert(chain.node->m_node.chainman)->GetMutex());
            wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            wallet->m_keypool_size = KEYPOOL_SIZE;
            auto* tip{chain.node->m_node.chainman->ActiveChain().Tip()};
            wallet->SetLastBlockProcessed(tip->nHeight, tip->GetBlockHash());

            FlatSigningProvider provider;
            std::string error;
            auto descs{Parse(chain.desc_str, provider, error, /*require_checksum=*/false)};
            WalletDescriptor w_desc(std::move(descs.at(0)), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/0, /*next_index=*/0);
            Assert(wallet->AddWalletDescriptor(w_desc, provider, "", /*internal=*/false));
        }
        reserver = std::make_unique<WalletRescanReserver>(*wallet);
        Assert(reserver->reserve());
    }).run([&] {
        ScanResult result{wallet->Scanner().Scan(
            chain.cascade_block_hash, chain.cascade_block_height, /*max_height=*/chain.cascade_block_height,
            *reserver, /*save_progress=*/false)};
        assert(result.status == ScanResult::SUCCESS);
    });

    // Guard against the benchmark silently measuring a cascade that never
    // fired: all three of E, A and B must have been recognized.
    assert(WITH_LOCK(wallet->cs_wallet, return wallet->mapWallet.size()) == 3);
}

} // namespace

BENCHMARK(RescanCascadeInLargeBlock);

} // namespace wallet
