// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// The bitcoin-chainstate executable serves to surface the dependencies required
// by a program wishing to use Bitcoin Core's consensus engine as it is right
// now.
//
// DEVELOPER NOTE: Since this is a "demo-only", experimental, etc. executable,
//                 it may diverge from Bitcoin Core's coding style.
//
// It is part of the libbitcoinkernel project.

#include <kernel/chainparams.h>
#include <kernel/chainstatemanager_opts.h>
#include <kernel/checks.h>
#include <kernel/context.h>
#include <kernel/validation_cache_sizes.h>
#include <kernel/warning.h>

#include <consensus/validation.h>
#include <core_io.h>
#include <node/blockstorage.h>
#include <node/caches.h>
#include <node/chainstate.h>
#include <random.h>
#include <script/sigcache.h>
#include <util/chaintype.h>
#include <util/fs.h>
#include <util/signalinterrupt.h>
#include <util/task_runner.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>

int main(int argc, char* argv[])
{
    // SETUP: Argument parsing and handling
    if (argc != 2) {
        std::cerr
            << "Usage: " << argv[0] << " DATADIR" << std::endl
            << "Display DATADIR information, and process hex-encoded blocks on standard input." << std::endl
            << std::endl
            << "IMPORTANT: THIS EXECUTABLE IS EXPERIMENTAL, FOR TESTING ONLY, AND EXPECTED TO" << std::endl
            << "           BREAK IN FUTURE VERSIONS. DO NOT USE ON YOUR ACTUAL DATADIR." << std::endl;
        return 1;
    }
    fs::path abs_datadir{fs::absolute(argv[1])};
    fs::create_directories(abs_datadir);


    // SETUP: Context
    kernel::Context kernel_context{};
    // We can't use a goto here, but we can use an assert since none of the
    // things instantiated so far requires running the epilogue to be torn down
    // properly
    assert(kernel::SanityChecks(kernel_context));

    // Necessary for CheckInputScripts (eventually called by ProcessNewBlock),
    // which will try the script cache first and fall back to actually
    // performing the check with the signature cache.
    kernel::ValidationCacheSizes validation_cache_sizes{};
    Assert(InitSignatureCache(validation_cache_sizes.signature_cache_bytes));
    Assert(InitScriptExecutionCache(validation_cache_sizes.script_execution_cache_bytes));

    ValidationSignals validation_signals{std::make_unique<util::ImmediateTaskRunner>()};

    class KernelNotifications : public kernel::Notifications
    {
    public:
        kernel::InterruptResult blockTip(SynchronizationState, CBlockIndex&) override
        {
            std::cout << "Block tip changed" << std::endl;
            return {};
        }
        void headerTip(SynchronizationState, int64_t height, int64_t timestamp, bool presync) override
        {
            std::cout << "Header tip changed: " << height << ", " << timestamp << ", " << presync << std::endl;
        }
        void progress(const bilingual_str& title, int progress_percent, bool resume_possible) override
        {
            std::cout << "Progress: " << title.original << ", " << progress_percent << ", " << resume_possible << std::endl;
        }
        void warningSet(kernel::Warning id, const bilingual_str& message) override
        {
            std::cout << "Warning " << static_cast<int>(id) << " set: " << message.original << std::endl;
        }
        void warningUnset(kernel::Warning id) override
        {
            std::cout << "Warning " << static_cast<int>(id) << " unset" << std::endl;
        }
        void flushError(const bilingual_str& message) override
        {
            std::cerr << "Error flushing block data to disk: " << message.original << std::endl;
        }
        void fatalError(const bilingual_str& message) override
        {
            std::cerr << "Error: " << message.original << std::endl;
        }
    };
    auto notifications = std::make_unique<KernelNotifications>();


    // SETUP: Chainstate
    auto chainparams = CChainParams::Main();
    const ChainstateManager::Options chainman_opts{
        .chainparams = *chainparams,
        .datadir = abs_datadir,
        .notifications = *notifications,
        .signals = &validation_signals,
    };
    const node::BlockManager::Options blockman_opts{
        .chainparams = chainman_opts.chainparams,
        .blocks_dir = abs_datadir / "blocks",
        .notifications = chainman_opts.notifications,
    };
    util::SignalInterrupt interrupt;
    ChainstateManager chainman{interrupt, chainman_opts, blockman_opts};

    node::CacheSizes cache_sizes;
    cache_sizes.block_tree_db = 2 << 20;
    cache_sizes.coins_db = 2 << 22;
    cache_sizes.coins = (450 << 20) - (2 << 20) - (2 << 22);
    node::ChainstateLoadOptions options;
    auto [status, error] = node::LoadChainstate(chainman, cache_sizes, options);
    if (status != node::ChainstateLoadStatus::SUCCESS) {
        std::cerr << "Failed to load Chain state from your datadir." << std::endl;
        goto epilogue;
    } else {
        std::tie(status, error) = node::VerifyLoadedChainstate(chainman, options);
        if (status != node::ChainstateLoadStatus::SUCCESS) {
            std::cerr << "Failed to verify loaded Chain state from your datadir." << std::endl;
            goto epilogue;
        }
    }

    for (Chainstate* chainstate : WITH_LOCK(::cs_main, return chainman.GetAll())) {
        BlockValidationState state;
        if (!chainstate->ActivateBestChain(state, nullptr)) {
            std::cerr << "Failed to connect best block (" << state.ToString() << ")" << std::endl;
            goto epilogue;
        }
    }

    // Main program logic starts here
    std::cout
        << "Hello! I'm going to print out some information about your datadir." << std::endl
        << "\t"
        << "Path: " << abs_datadir << std::endl;
    {
        LOCK(chainman.GetMutex());
        std::cout
        << "\t" << "Blockfiles Indexed: " << std::boolalpha << chainman.m_blockman.m_blockfiles_indexed.load() << std::noboolalpha << std::endl
        << "\t" << "Snapshot Active: " << std::boolalpha << chainman.IsSnapshotActive() << std::noboolalpha << std::endl
        << "\t" << "Active Height: " << chainman.ActiveHeight() << std::endl
        << "\t" << "Active IBD: " << std::boolalpha << chainman.IsInitialBlockDownload() << std::noboolalpha << std::endl;
        CBlockIndex* tip = chainman.ActiveTip();
        if (tip) {
            std::cout << "\t" << tip->ToString() << std::endl;
        }
    }

    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty()) {
            std::cerr << "Empty line found" << std::endl;
            break;
        }

        std::shared_ptr<CBlock> blockptr = std::make_shared<CBlock>();
        CBlock& block = *blockptr;

        if (!DecodeHexBlk(block, line)) {
            std::cerr << "Block decode failed" << std::endl;
            break;
        }

        if (block.vtx.empty() || !block.vtx[0]->IsCoinBase()) {
            std::cerr << "Block does not start with a coinbase" << std::endl;
            break;
        }

        uint256 hash = block.GetHash();
        {
            LOCK(cs_main);
            const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(hash);
            if (pindex) {
                if (pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
                    std::cerr << "duplicate" << std::endl;
                    break;
                }
                if (pindex->nStatus & BLOCK_FAILED_MASK) {
                    std::cerr << "duplicate-invalid" << std::endl;
                    break;
                }
            }
        }

        {
            LOCK(cs_main);
            const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(block.hashPrevBlock);
            if (pindex) {
                chainman.UpdateUncommittedBlockStructures(block, pindex);
            }
        }

        // Adapted from rpc/mining.cpp
        class submitblock_StateCatcher final : public CValidationInterface
        {
        public:
            uint256 hash;
            bool found;
            BlockValidationState state;

            explicit submitblock_StateCatcher(const uint256& hashIn) : hash(hashIn), found(false), state() {}

        protected:
            void BlockChecked(const CBlock& block, const BlockValidationState& stateIn) override
            {
                if (block.GetHash() != hash)
                    return;
                found = true;
                state = stateIn;
            }
        };

        bool new_block;
        auto sc = std::make_shared<submitblock_StateCatcher>(block.GetHash());
        validation_signals.RegisterSharedValidationInterface(sc);
        bool accepted = chainman.ProcessNewBlock(blockptr, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&new_block);
        validation_signals.UnregisterSharedValidationInterface(sc);
        if (!new_block && accepted) {
            std::cerr << "duplicate" << std::endl;
            break;
        }
        if (!sc->found) {
            std::cerr << "inconclusive" << std::endl;
            break;
        }
        std::cout << sc->state.ToString() << std::endl;
        switch (sc->state.GetResult()) {
        case BlockValidationResult::BLOCK_RESULT_UNSET:
            std::cerr << "initial value. Block has not yet been rejected" << std::endl;
            break;
        case BlockValidationResult::BLOCK_HEADER_LOW_WORK:
            std::cerr << "the block header may be on a too-little-work chain" << std::endl;
            break;
        case BlockValidationResult::BLOCK_CONSENSUS:
            std::cerr << "invalid by consensus rules (excluding any below reasons)" << std::endl;
            break;
        case BlockValidationResult::BLOCK_RECENT_CONSENSUS_CHANGE:
            std::cerr << "Invalid by a change to consensus rules more recent than SegWit." << std::endl;
            break;
        case BlockValidationResult::BLOCK_CACHED_INVALID:
            std::cerr << "this block was cached as being invalid and we didn't store the reason why" << std::endl;
            break;
        case BlockValidationResult::BLOCK_INVALID_HEADER:
            std::cerr << "invalid proof of work or time too old" << std::endl;
            break;
        case BlockValidationResult::BLOCK_MUTATED:
            std::cerr << "the block's data didn't match the data committed to by the PoW" << std::endl;
            break;
        case BlockValidationResult::BLOCK_MISSING_PREV:
            std::cerr << "We don't have the previous block the checked one is built on" << std::endl;
            break;
        case BlockValidationResult::BLOCK_INVALID_PREV:
            std::cerr << "A block this one builds on is invalid" << std::endl;
            break;
        case BlockValidationResult::BLOCK_TIME_FUTURE:
            std::cerr << "block timestamp was > 2 hours in the future (or our clock is bad)" << std::endl;
            break;
        case BlockValidationResult::BLOCK_CHECKPOINT:
            std::cerr << "the block failed to meet one of our checkpoints" << std::endl;
            break;
        }
    }

epilogue:
    // Without this precise shutdown sequence, there will be a lot of nullptr
    // dereferencing and UB.
    if (chainman.m_thread_load.joinable()) chainman.m_thread_load.join();

    validation_signals.FlushBackgroundCallbacks();
    {
        LOCK(cs_main);
        for (Chainstate* chainstate : chainman.GetAll()) {
            if (chainstate->CanFlushToDisk()) {
                chainstate->ForceFlushStateToDisk();
                chainstate->ResetCoinsViews();
            }
        }
    }
}
