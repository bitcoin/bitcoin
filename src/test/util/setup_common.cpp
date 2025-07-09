// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <addrman.h>
#include <banman.h>
#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <crypto/sha256.h>
#include <index/txindex.h>
#include <init.h>
#include <interfaces/chain.h>
#include <net.h>
#include <net_processing.h>
#include <noui.h>
#include <node/blockstorage.h>
#include <node/chainstate.h>
#include <node/miner.h>
#include <policy/fees.h>
#include <pow.h>
#include <rpc/blockchain.h>
#include <rpc/register.h>
#include <rpc/server.h>
#include <scheduler.h>
#include <script/sigcache.h>
#include <shutdown.h>
#include <streams.h>
#include <test/util/index.h>
#include <txdb.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/thread.h>
#include <util/threadnames.h>
#include <util/time.h>
#include <util/translation.h>
#include <util/url.h>
#include <util/vector.h>
#include <validation.h>
#include <validationinterface.h>
#include <walletinitinterface.h>

#include <bls/bls.h>
#include <coinjoin/context.h>
#include <evo/cbtx.h>
#include <evo/creditpool.h>
#include <evo/deterministicmns.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <flat-database.h>
#include <governance/governance.h>
#include <llmq/context.h>
#include <masternode/meta.h>
#include <masternode/sync.h>
#include <netfulfilledman.h>
#include <spork.h>
#include <stats/client.h>

#ifdef ENABLE_WALLET
#include <interfaces/coinjoin.h>
#include <interfaces/wallet.h>
#endif // ENABLE_WALLET

#include <algorithm>
#include <memory>
#include <stdexcept>

using node::BlockAssembler;
using node::CalculateCacheSizes;
using node::DashChainstateSetup;
using node::DashChainstateSetupClose;
using node::DEFAULT_ADDRESSINDEX;
using node::DEFAULT_SPENTINDEX;
using node::DEFAULT_TIMESTAMPINDEX;
using node::LoadChainstate;
using node::NodeContext;
using node::VerifyLoadedChainstate;
using node::fPruneMode;
using node::fReindex;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;
UrlDecodeFn* const URL_DECODE = nullptr;

FastRandomContext g_insecure_rand_ctx;
/** Random context to get unique temp data dirs. Separate from g_insecure_rand_ctx, which can be seeded from a const env var */
static FastRandomContext g_insecure_rand_ctx_temp_path;

/** Return the unsigned from the environment var if available, otherwise 0 */
static uint256 GetUintFromEnv(const std::string& env_name)
{
    const char* num = std::getenv(env_name.c_str());
    if (!num) return {};
    return uint256S(num);
}

void Seed(FastRandomContext& ctx)
{
    // Should be enough to get the seed once for the process
    static uint256 seed{};
    static const std::string RANDOM_CTX_SEED{"RANDOM_CTX_SEED"};
    if (seed.IsNull()) seed = GetUintFromEnv(RANDOM_CTX_SEED);
    if (seed.IsNull()) seed = GetRandHash();
    LogPrintf("%s: Setting random seed for current tests to %s=%s\n", __func__, RANDOM_CTX_SEED, seed.GetHex());
    ctx = FastRandomContext(seed);
}

std::ostream& operator<<(std::ostream& os, const uint256& num)
{
    os << num.ToString();
    return os;
}

void DashChainstateSetup(ChainstateManager& chainman,
                         NodeContext& node,
                         bool fReset,
                         bool fReindexChainState,
                         const Consensus::Params& consensus_params)
{
    DashChainstateSetup(chainman, *Assert(node.govman.get()), *Assert(node.mn_metaman.get()), *Assert(node.mn_sync.get()),
                        *Assert(node.sporkman.get()), node.mn_activeman, node.chain_helper, node.cpoolman, node.dmnman,
                        node.evodb, node.mnhf_manager, node.llmq_ctx, Assert(node.mempool.get()), fReset, fReindexChainState,
                        consensus_params);
}

void DashChainstateSetupClose(NodeContext& node)
{
    DashChainstateSetupClose(node.chain_helper, node.cpoolman, node.dmnman, node.mnhf_manager, node.llmq_ctx,
                             Assert(node.mempool.get()));
}

BasicTestingSetup::BasicTestingSetup(const std::string& chainName, const std::vector<const char*>& extra_args)
    : m_path_root{fs::temp_directory_path() / "test_common_" PACKAGE_NAME / g_insecure_rand_ctx_temp_path.rand256().ToString()},
      m_args{}
{
    m_node.args = &gArgs;
    std::vector<const char*> arguments = Cat(
        {
            "dummy",
            "-printtoconsole=0",
            "-logsourcelocations",
            "-logtimemicros",
            "-logthreadnames",
            "-loglevel=trace",
            "-debug",
            "-debugexclude=libevent",
            "-debugexclude=leveldb",
        },
        extra_args);
    if (G_TEST_COMMAND_LINE_ARGUMENTS) {
        arguments = Cat(arguments, G_TEST_COMMAND_LINE_ARGUMENTS());
    }
    util::ThreadRename("test");
    fs::create_directories(m_path_root);
    m_args.ForceSetArg("-datadir", fs::PathToString(m_path_root));
    gArgs.ForceSetArg("-datadir", fs::PathToString(m_path_root));
    gArgs.ClearPathCache();
    {
        SetupServerArgs(*m_node.args);
        std::string error;
        if (!m_node.args->ParseParameters(arguments.size(), arguments.data(), error)) {
            m_node.args->ClearArgs();
            throw std::runtime_error{error};
        }
    }
    SelectParams(chainName);
    SeedInsecureRand();
    if (G_TEST_LOG_FUN) LogInstance().PushBackCallback(G_TEST_LOG_FUN);
    InitLogging(*m_node.args);
    AppInitParameterInteraction(*m_node.args);
    LogInstance().StartLogging();
    SHA256AutoDetect();
    ECC_Start();
    BLSInit();
    SetupEnvironment();
    SetupNetworking();
    InitSignatureCache();
    InitScriptExecutionCache();
    ::g_stats_client = InitStatsClient(*m_node.args);
    m_node.chain = interfaces::MakeChain(m_node);

    m_node.netgroupman = std::make_unique<NetGroupManager>(/*asmap=*/std::vector<bool>());
    m_node.addrman = std::make_unique<AddrMan>(*m_node.netgroupman,
                                               /*deterministic=*/false,
                                               m_node.args->GetIntArg("-checkaddrman", 0));

    std::string sem_str = m_args.GetArg("-socketevents", DEFAULT_SOCKETEVENTS);
    ::g_socket_events_mode = SEMFromString(sem_str);
    if (::g_socket_events_mode == SocketEventsMode::Unknown) {
        throw std::runtime_error(
            strprintf("Invalid -socketevents ('%s') specified. Only these modes are supported: %s",
                      sem_str, GetSupportedSocketEventsStr()));
    }

    m_node.connman = std::make_unique<CConnman>(0x1337, 0x1337, *m_node.addrman, *m_node.netgroupman); // Deterministic randomness for tests.

    fCheckBlockIndex = true;
    m_node.evodb = std::make_unique<CEvoDB>(1 << 20, true, true);
    m_node.mnhf_manager = std::make_unique<CMNHFManager>(*m_node.evodb);
    m_node.cpoolman = std::make_unique<CCreditPoolManager>(*m_node.evodb);
    static bool noui_connected = false;
    if (!noui_connected) {
        noui_connect();
        noui_connected = true;
    }
    bls::bls_legacy_scheme.store(true);
}

BasicTestingSetup::~BasicTestingSetup()
{
    SetMockTime(0s); // Reset mocktime for following tests
    m_node.cpoolman.reset();
    m_node.mnhf_manager.reset();
    m_node.evodb.reset();
    ::g_socket_events_mode = SocketEventsMode::Unknown;
    m_node.connman.reset();
    m_node.addrman.reset();
    m_node.netgroupman.reset();
    ::g_stats_client.reset();

    LogInstance().DisconnectTestLogger();
    fs::remove_all(m_path_root);
    gArgs.ClearArgs();
    ECC_Stop();
}

ChainTestingSetup::ChainTestingSetup(const std::string& chainName, const std::vector<const char*>& extra_args)
    : BasicTestingSetup(chainName, extra_args)
{
    // We have to run a scheduler thread to prevent ActivateBestChain
    // from blocking due to queue overrun.
    m_node.scheduler = std::make_unique<CScheduler>();
    m_node.scheduler->m_service_thread = std::thread(util::TraceThread, "scheduler", [&] { m_node.scheduler->serviceQueue(); });
    GetMainSignals().RegisterBackgroundSignalScheduler(*m_node.scheduler);

    m_node.fee_estimator = std::make_unique<CBlockPolicyEstimator>();
    m_node.mempool = std::make_unique<CTxMemPool>(m_node.fee_estimator.get(), m_node.args->GetIntArg("-checkmempool", 1));

    m_cache_sizes = CalculateCacheSizes(m_args);

    m_node.chainman = std::make_unique<ChainstateManager>();
    m_node.chainman->m_blockman.m_block_tree_db = std::make_unique<CBlockTreeDB>(m_cache_sizes.block_tree_db, true);

    m_node.mn_metaman = std::make_unique<CMasternodeMetaMan>();
    m_node.netfulfilledman = std::make_unique<CNetFulfilledRequestManager>();
    m_node.sporkman = std::make_unique<CSporkManager>();
    m_node.mn_sync = std::make_unique<CMasternodeSync>(*m_node.connman, *m_node.netfulfilledman);
    m_node.govman = std::make_unique<CGovernanceManager>(*m_node.mn_metaman, *m_node.netfulfilledman, *m_node.chainman, m_node.dmnman, *m_node.mn_sync);

    // Start script-checking threads. Set g_parallel_script_checks to true so they are used.
    constexpr int script_check_threads = 2;
    StartScriptCheckWorkerThreads(script_check_threads);
    g_parallel_script_checks = true;
}

ChainTestingSetup::~ChainTestingSetup()
{
    m_node.scheduler->stop();
    StopScriptCheckWorkerThreads();
    GetMainSignals().FlushBackgroundCallbacks();
    GetMainSignals().UnregisterBackgroundSignalScheduler();
    m_node.mn_sync.reset();
    m_node.govman.reset();
    m_node.sporkman.reset();
    m_node.netfulfilledman.reset();
    m_node.mn_metaman.reset();
    m_node.args = nullptr;
    m_node.mempool.reset();
    m_node.scheduler.reset();
    m_node.chainman.reset();
}

TestingSetup::TestingSetup(const std::string& chainName, const std::vector<const char*>& extra_args)
    : ChainTestingSetup(chainName, extra_args)
{
    const CChainParams& chainparams = Params();
    // Ideally we'd move all the RPC tests to the functional testing framework
    // instead of unit tests, but for now we need these here.
    RegisterAllCoreRPCCommands(tableRPC);

    auto maybe_load_error = LoadChainstate(fReindex.load(),
                                           *Assert(m_node.chainman.get()),
                                           *Assert(m_node.govman.get()),
                                           *Assert(m_node.mn_metaman.get()),
                                           *Assert(m_node.mn_sync.get()),
                                           *Assert(m_node.sporkman.get()),
                                           m_node.mn_activeman,
                                           m_node.chain_helper,
                                           m_node.cpoolman,
                                           m_node.dmnman,
                                           m_node.evodb,
                                           m_node.mnhf_manager,
                                           m_node.llmq_ctx,
                                           Assert(m_node.mempool.get()),
                                           fPruneMode,
                                           m_args.GetBoolArg("-addressindex", DEFAULT_ADDRESSINDEX),
                                           !m_args.GetBoolArg("-disablegovernance", !DEFAULT_GOVERNANCE_ENABLE),
                                           m_args.GetBoolArg("-spentindex", DEFAULT_SPENTINDEX),
                                           m_args.GetBoolArg("-timestampindex", DEFAULT_TIMESTAMPINDEX),
                                           m_args.GetBoolArg("-txindex", DEFAULT_TXINDEX),
                                           chainparams.GetConsensus(),
                                           chainparams.NetworkIDString(),
                                           m_args.GetBoolArg("-reindex-chainstate", false),
                                           m_cache_sizes.block_tree_db,
                                           m_cache_sizes.coins_db,
                                           m_cache_sizes.coins,
                                           /*block_tree_db_in_memory=*/true,
                                           /*coins_db_in_memory=*/true);
    assert(!maybe_load_error.has_value());

    auto maybe_verify_error = VerifyLoadedChainstate(
        *Assert(m_node.chainman),
        *Assert(m_node.evodb.get()),
        fReindex.load(),
        m_args.GetBoolArg("-reindex-chainstate", false),
        chainparams.GetConsensus(),
        m_args.GetIntArg("-checkblocks", DEFAULT_CHECKBLOCKS),
        m_args.GetIntArg("-checklevel", DEFAULT_CHECKLEVEL),
        /*get_unix_time_seconds=*/static_cast<int64_t(*)()>(GetTime),
        [](bool bls_state) {
            LogPrintf("%s: bls_legacy_scheme=%d\n", __func__, bls_state);
        });
    assert(!maybe_verify_error.has_value());

    m_node.banman = std::make_unique<BanMan>(m_args.GetDataDirBase() / "banlist", nullptr, DEFAULT_MISBEHAVING_BANTIME);
    m_node.peerman = PeerManager::make(chainparams, *m_node.connman, *m_node.addrman, m_node.banman.get(),
                                       *m_node.chainman, *m_node.mempool, *m_node.mn_metaman, *m_node.mn_sync,
                                       *m_node.govman, *m_node.sporkman, /* mn_activeman = */ nullptr, m_node.dmnman,
                                       m_node.cj_ctx, m_node.llmq_ctx, /* ignore_incoming_txs = */ false);
    {
        CConnman::Options options;
        options.m_msgproc = m_node.peerman.get();
        options.socketEventsMode = ::g_socket_events_mode;
        m_node.connman->Init(options);
    }

    m_node.cj_ctx = std::make_unique<CJContext>(*m_node.chainman, *m_node.connman, *m_node.dmnman, *m_node.mn_metaman, *m_node.mempool,
                                                /*mn_activeman=*/nullptr, *m_node.mn_sync, *m_node.llmq_ctx->isman, m_node.peerman,
                                                /*relay_txes=*/true);

#ifdef ENABLE_WALLET
    // WalletInit::Construct()-like logic needed for wallet tests that run on
    // TestingSetup and its children (e.g. TestChain100Setup) instead of
    // WalletTestingSetup
    m_node.coinjoin_loader = interfaces::MakeCoinJoinLoader(m_node);

    auto wallet_loader = interfaces::MakeWalletLoader(*m_node.chain, *m_node.args, m_node, *m_node.coinjoin_loader);
    m_node.wallet_loader = wallet_loader.get();
    m_node.chain_clients.emplace_back(std::move(wallet_loader));
#endif // ENABLE_WALLET

    BlockValidationState state;
    if (!m_node.chainman->ActiveChainstate().ActivateBestChain(state)) {
        throw std::runtime_error(strprintf("ActivateBestChain failed. (%s)", state.ToString()));
    }
}

TestingSetup::~TestingSetup()
{
#ifdef ENABLE_WALLET
    for (auto& client : m_node.chain_clients) {
        client.reset();
    }
    m_node.wallet_loader = nullptr;

    m_node.coinjoin_loader.reset();
#endif // ENABLE_WALLET
    m_node.cj_ctx.reset();

    // Interrupt() and PrepareShutdown() routines
    if (m_node.llmq_ctx) {
        m_node.llmq_ctx->Interrupt();
        m_node.llmq_ctx->Stop();
    }
    if (m_node.connman) {
        m_node.connman->Stop();
    }

    // DashChainstateSetup() is called by LoadChainstate() internally but
    // winding them down is our responsibility
    DashChainstateSetupClose(m_node);

    m_node.peerman.reset();
    m_node.banman.reset();
}

TestChain100Setup::TestChain100Setup(const std::string& chain_name, const std::vector<const char*>& extra_args)
    : TestChainSetup{100, chain_name, extra_args}
{
}

TestChainSetup::TestChainSetup(int num_blocks, const std::string& chain_name, const std::vector<const char*>& extra_args)
    : TestingSetup{chain_name, extra_args}
{
    SetMockTime(1598887952);
    constexpr std::array<unsigned char, 32> vchKey = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
    coinbaseKey.Set(vchKey.begin(), vchKey.end(), true);

    // Generate a num_blocks length chain:
    this->mineBlocks(num_blocks);

    // Initialize transaction index *after* chain has been constructed
    g_txindex = std::make_unique<TxIndex>(1 << 20, true);
    assert(!g_txindex->BlockUntilSyncedToCurrentChain());
    if (!g_txindex->Start(m_node.chainman->ActiveChainstate())) {
        throw std::runtime_error("TxIndex::Start() failed.");
    }
    IndexWaitSynced(*g_txindex);

    CCheckpointData checkpoints{
        {
            /*TestChainDATSetup=*/
            {   98, uint256S("0x150e127929d578d8129b77a6cb7e2e343a1379aa3feaaa9cce59e0a645756a81") },
            /*TestChain100Setup=*/
            {  100, uint256S("0x6ffb83129c19ebdf1ae3771be6a67fe34b35f4c956326b9ba152fac1649f65ae") },
            /*TestChainDIP3BeforeActivationSetup=*/
            {  430, uint256S("0x0bcefaa33fec56cd84d05d0e76cd6a78badcc20f627d91903646de6a07930a14") },
            /*TestChainBRRBeforeActivationSetup=*/
            {  497, uint256S("0x0857a9b5db51835b1c828f019f4c664b5fe6c28ac44a6d868436930f832d31e5") },
            /*TestChainV19BeforeActivationSetup=*/
            {  494, uint256S("0x44ee5c8a5e5cbd4437d63c54ddc1d40329be811b25c492fa901e11cdf408f905") },
        }
    };

    {
        LOCK(::cs_main);
        auto hash = checkpoints.mapCheckpoints.find(num_blocks);
        assert(
            hash != checkpoints.mapCheckpoints.end() &&
            m_node.chainman->ActiveChain().Tip()->GetBlockHash() == hash->second);
    }
}

void TestChainSetup::mineBlocks(int num_blocks)
{
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    for (int i = 0; i < num_blocks; i++) {
        std::vector<CMutableTransaction> noTxns;
        CBlock b = CreateAndProcessBlock(noTxns, scriptPubKey);
        SetMockTime(GetTime() + 1);
        m_coinbase_txns.push_back(b.vtx[0]);
    }

    // Allow tx index to catch up with the block index.
    if (g_txindex) {
        IndexWaitSynced(*g_txindex);
    }
}

CBlock TestChainSetup::CreateAndProcessBlock(
    const std::vector<CMutableTransaction>& txns,
    const CScript& scriptPubKey,
    CChainState* chainstate)
{
    if (!chainstate) {
        chainstate = &Assert(m_node.chainman)->ActiveChainstate();
    }

    const CChainParams& chainparams = Params();
    auto block = this->CreateBlock(txns, scriptPubKey, *chainstate);

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
    Assert(m_node.chainman)->ProcessNewBlock(chainparams, shared_pblock, true, nullptr);

    return block;
}

CBlock TestChainSetup::CreateAndProcessBlock(
    const std::vector<CMutableTransaction>& txns,
    const CKey& scriptKey,
    CChainState* chainstate)
{
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    return CreateAndProcessBlock(txns, scriptPubKey, chainstate);
}

CBlock TestChainSetup::CreateBlock(
    const std::vector<CMutableTransaction>& txns,
    const CScript& scriptPubKey,
    CChainState& chainstate)
{
    const CChainParams& chainparams = Params();
    CTxMemPool empty_pool;
    CBlock block = BlockAssembler(chainstate, m_node, &empty_pool, chainparams).CreateNewBlock(scriptPubKey)->block;

    std::vector<CTransactionRef> llmqCommitments;
    for (const auto& tx : block.vtx) {
        if (tx->IsSpecialTxVersion() && tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            llmqCommitments.emplace_back(tx);
        }
    }

    // Replace mempool-selected txns with just coinbase plus passed-in txns:
    block.vtx.resize(1);
    Assert(block.vtx.size() == 1);

    // Re-add quorum commitments
    block.vtx.insert(block.vtx.end(), llmqCommitments.begin(), llmqCommitments.end());
    for (const CMutableTransaction& tx : txns) {
        block.vtx.push_back(MakeTransactionRef(tx));
    }

    // Manually update CbTx as we modified the block here
    if (block.vtx[0]->nType == TRANSACTION_COINBASE) {
        LOCK(cs_main);
        auto cbTx = GetTxPayload<CCbTx>(*block.vtx[0]);
        Assert(cbTx.has_value());
        BlockValidationState state;
        CDeterministicMNList mn_list;
        if (!m_node.dmnman->BuildNewListFromBlock(block, chainstate.m_chain.Tip(), state, chainstate.CoinsTip(), mn_list, *m_node.llmq_ctx->qsnapman, true)) {
            Assert(false);
        }
        if (!CalcCbTxMerkleRootMNList(cbTx->merkleRootMNList, CSimplifiedMNList(mn_list), state)) {
            Assert(false);
        }
        if (!CalcCbTxMerkleRootQuorums(block, chainstate.m_chain.Tip(), *m_node.llmq_ctx->quorum_block_processor, cbTx->merkleRootQuorums, state)) {
            Assert(false);
        }
        CMutableTransaction tmpTx{*block.vtx[0]};
        SetTxPayload(tmpTx, *cbTx);
        block.vtx[0] = MakeTransactionRef(tmpTx);
    }

    // Create a valid coinbase and merkleRoot
    {
        LOCK(::cs_main);
        block.hashPrevBlock = chainstate.m_chain.Tip()->GetBlockHash();
        CMutableTransaction tx_coinbase{*block.vtx[0]};
        tx_coinbase.vin[0].scriptSig = CScript{} << (chainstate.m_chain.Height() + 1) << CScriptNum{1};
        block.vtx[0] = MakeTransactionRef(std::move(tx_coinbase));
        block.hashMerkleRoot = BlockMerkleRoot(block);
    }

    while (!CheckProofOfWork(block.GetHash(), block.nBits, chainparams.GetConsensus())) ++block.nNonce;

    CBlock result = block;
    return result;
}

CBlock TestChainSetup::CreateBlock(
    const std::vector<CMutableTransaction>& txns,
    const CKey& scriptKey,
    CChainState& chainstate)
{
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    return CreateBlock(txns, scriptPubKey, chainstate);
}


CMutableTransaction TestChainSetup::CreateValidMempoolTransaction(CTransactionRef input_transaction,
                                                                  int input_vout,
                                                                  int input_height,
                                                                  CKey input_signing_key,
                                                                  CScript output_destination,
                                                                  CAmount output_amount,
                                                                  bool submit)
{
    // Transaction we will submit to the mempool
    CMutableTransaction mempool_txn;

    // Create an input
    COutPoint outpoint_to_spend(input_transaction->GetHash(), input_vout);
    CTxIn input(outpoint_to_spend);
    mempool_txn.vin.push_back(input);

    // Create an output
    CTxOut output(output_amount, output_destination);
    mempool_txn.vout.push_back(output);

    // Sign the transaction
    // - Add the signing key to a keystore
    FillableSigningProvider keystore;
    keystore.AddKey(input_signing_key);
    // - Populate a CoinsViewCache with the unspent output
    CCoinsView coins_view;
    CCoinsViewCache coins_cache(&coins_view);
    AddCoins(coins_cache, *input_transaction.get(), input_height);
    // - Use GetCoin to properly populate utxo_to_spend,
    Coin utxo_to_spend;
    assert(coins_cache.GetCoin(outpoint_to_spend, utxo_to_spend));
    // - Then add it to a map to pass in to SignTransaction
    std::map<COutPoint, Coin> input_coins;
    input_coins.insert({outpoint_to_spend, utxo_to_spend});
    // - Default signature hashing type
    int nHashType = SIGHASH_ALL;
    std::map<int, bilingual_str> input_errors;
    assert(SignTransaction(mempool_txn, &keystore, input_coins, nHashType, input_errors));

    // If submit=true, add transaction to the mempool.
    if (submit) {
        LOCK(cs_main);
        const MempoolAcceptResult result = m_node.chainman->ProcessTransaction(MakeTransactionRef(mempool_txn));
        assert(result.m_result_type == MempoolAcceptResult::ResultType::VALID);
    }

    return mempool_txn;
}

TestChainSetup::~TestChainSetup()
{
    // Allow tx index to catch up with the block index cause otherwise
    // we might be destroying it while scheduler still has some work for it
    // e.g. via BlockConnected signal
    IndexWaitSynced(*g_txindex);
    g_txindex->Interrupt();
    g_txindex->Stop();
    SyncWithValidationInterfaceQueue();
    g_txindex.reset();
}

std::vector<CTransactionRef> TestChainSetup::PopulateMempool(FastRandomContext& det_rand, size_t num_transactions, bool submit)
{
    std::vector<CTransactionRef> mempool_transactions;
    std::deque<std::pair<COutPoint, CAmount>> unspent_prevouts;
    std::transform(m_coinbase_txns.begin(), m_coinbase_txns.end(), std::back_inserter(unspent_prevouts),
        [](const auto& tx){ return std::make_pair(COutPoint(tx->GetHash(), 0), tx->vout[0].nValue); });
    while (num_transactions > 0 && !unspent_prevouts.empty()) {
        // The number of inputs and outputs are random, between 1 and 24.
        CMutableTransaction mtx = CMutableTransaction();
        const size_t num_inputs = det_rand.randrange(24) + 1;
        CAmount total_in{0};
        for (size_t n{0}; n < num_inputs; ++n) {
            if (unspent_prevouts.empty()) break;
            const auto& [prevout, amount] = unspent_prevouts.front();
            mtx.vin.push_back(CTxIn(prevout, CScript()));
            total_in += amount;
            unspent_prevouts.pop_front();
        }
        const size_t num_outputs = det_rand.randrange(24) + 1;
        // Approximately 1000sat "fee," equal output amounts.
        const CAmount amount_per_output = (total_in - 1000) / num_outputs;
        for (size_t n{0}; n < num_outputs; ++n) {
            CScript spk = CScript() << CScriptNum(num_transactions + n);
            mtx.vout.push_back(CTxOut(amount_per_output, spk));
        }
        CTransactionRef ptx = MakeTransactionRef(mtx);
        mempool_transactions.push_back(ptx);
        if (amount_per_output > 2000) {
            // If the value is high enough to fund another transaction + fees, keep track of it so
            // it can be used to build a more complex transaction graph. Insert randomly into
            // unspent_prevouts for extra randomness in the resulting structures.
            for (size_t n{0}; n < num_outputs; ++n) {
                unspent_prevouts.push_back(std::make_pair(COutPoint(ptx->GetHash(), n), amount_per_output));
                std::swap(unspent_prevouts.back(), unspent_prevouts[det_rand.randrange(unspent_prevouts.size())]);
            }
        }
        if (submit) {
            LOCK2(m_node.mempool->cs, cs_main);
            LockPoints lp;
            m_node.mempool->addUnchecked(CTxMemPoolEntry(ptx, 1000, 0, 1, false, 4, lp));
        }
        --num_transactions;
    }
    return mempool_transactions;
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction& tx) const
{
    return FromTx(MakeTransactionRef(tx));
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransactionRef& tx) const
{
    return CTxMemPoolEntry(tx, nFee, nTime, nHeight,
                           spendsCoinbase, sigOpCount, lp);
}

/**
 * @returns a real block (0000000000013b8ab2cd513b0261a14096412195a72a0c4827d229dcc7e0f7af)
 *      with 9 txs.
 */
CBlock getBlock13b8a()
{
    CBlock block;
    CDataStream stream(ParseHex("0100000090f0a9f110702f808219ebea1173056042a714bad51b916cb6800000000000005275289558f51c9966699404ae2294730c3c9f9bda53523ce50e9b95e558da2fdb261b4d4c86041b1ab1bf930901000000010000000000000000000000000000000000000000000000000000000000000000ffffffff07044c86041b0146ffffffff0100f2052a01000000434104e18f7afbe4721580e81e8414fc8c24d7cfacf254bb5c7b949450c3e997c2dc1242487a8169507b631eb3771f2b425483fb13102c4eb5d858eef260fe70fbfae0ac00000000010000000196608ccbafa16abada902780da4dc35dafd7af05fa0da08cf833575f8cf9e836000000004a493046022100dab24889213caf43ae6adc41cf1c9396c08240c199f5225acf45416330fd7dbd022100fe37900e0644bf574493a07fc5edba06dbc07c311b947520c2d514bc5725dcb401ffffffff0100f2052a010000001976a914f15d1921f52e4007b146dfa60f369ed2fc393ce288ac000000000100000001fb766c1288458c2bafcfec81e48b24d98ec706de6b8af7c4e3c29419bfacb56d000000008c493046022100f268ba165ce0ad2e6d93f089cfcd3785de5c963bb5ea6b8c1b23f1ce3e517b9f022100da7c0f21adc6c401887f2bfd1922f11d76159cbc597fbd756a23dcbb00f4d7290141042b4e8625a96127826915a5b109852636ad0da753c9e1d5606a50480cd0c40f1f8b8d898235e571fe9357d9ec842bc4bba1827daaf4de06d71844d0057707966affffffff0280969800000000001976a9146963907531db72d0ed1a0cfb471ccb63923446f388ac80d6e34c000000001976a914f0688ba1c0d1ce182c7af6741e02658c7d4dfcd388ac000000000100000002c40297f730dd7b5a99567eb8d27b78758f607507c52292d02d4031895b52f2ff010000008b483045022100f7edfd4b0aac404e5bab4fd3889e0c6c41aa8d0e6fa122316f68eddd0a65013902205b09cc8b2d56e1cd1f7f2fafd60a129ed94504c4ac7bdc67b56fe67512658b3e014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffffca5065ff9617cbcba45eb23726df6498a9b9cafed4f54cbab9d227b0035ddefb000000008a473044022068010362a13c7f9919fa832b2dee4e788f61f6f5d344a7c2a0da6ae740605658022006d1af525b9a14a35c003b78b72bd59738cd676f845d1ff3fc25049e01003614014104732012cb962afa90d31b25d8fb0e32c94e513ab7a17805c14ca4c3423e18b4fb5d0e676841733cb83abaf975845c9f6f2a8097b7d04f4908b18368d6fc2d68ecffffffff01001ec4110200000043410469ab4181eceb28985b9b4e895c13fa5e68d85761b7eee311db5addef76fa8621865134a221bd01f28ec9999ee3e021e60766e9d1f3458c115fb28650605f11c9ac000000000100000001cdaf2f758e91c514655e2dc50633d1e4c84989f8aa90a0dbc883f0d23ed5c2fa010000008b48304502207ab51be6f12a1962ba0aaaf24a20e0b69b27a94fac5adf45aa7d2d18ffd9236102210086ae728b370e5329eead9accd880d0cb070aea0c96255fae6c4f1ddcce1fd56e014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff02404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac002d3101000000001976a9141befba0cdc1ad56529371864d9f6cb042faa06b588ac000000000100000001b4a47603e71b61bc3326efd90111bf02d2f549b067f4c4a8fa183b57a0f800cb010000008a4730440220177c37f9a505c3f1a1f0ce2da777c339bd8339ffa02c7cb41f0a5804f473c9230220585b25a2ee80eb59292e52b987dad92acb0c64eced92ed9ee105ad153cdb12d001410443bd44f683467e549dae7d20d1d79cbdb6df985c6e9c029c8d0c6cb46cc1a4d3cf7923c5021b27f7a0b562ada113bc85d5fda5a1b41e87fe6e8802817cf69996ffffffff0280651406000000001976a9145505614859643ab7b547cd7f1f5e7e2a12322d3788ac00aa0271000000001976a914ea4720a7a52fc166c55ff2298e07baf70ae67e1b88ac00000000010000000586c62cd602d219bb60edb14a3e204de0705176f9022fe49a538054fb14abb49e010000008c493046022100f2bc2aba2534becbdf062eb993853a42bbbc282083d0daf9b4b585bd401aa8c9022100b1d7fd7ee0b95600db8535bbf331b19eed8d961f7a8e54159c53675d5f69df8c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff03ad0e58ccdac3df9dc28a218bcf6f1997b0a93306faaa4b3a28ae83447b2179010000008b483045022100be12b2937179da88599e27bb31c3525097a07cdb52422d165b3ca2f2020ffcf702200971b51f853a53d644ebae9ec8f3512e442b1bcb6c315a5b491d119d10624c83014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff2acfcab629bbc8685792603762c921580030ba144af553d271716a95089e107b010000008b483045022100fa579a840ac258871365dd48cd7552f96c8eea69bd00d84f05b283a0dab311e102207e3c0ee9234814cfbb1b659b83671618f45abc1326b9edcc77d552a4f2a805c0014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffffdcdc6023bbc9944a658ddc588e61eacb737ddf0a3cd24f113b5a8634c517fcd2000000008b4830450221008d6df731df5d32267954bd7d2dda2302b74c6c2a6aa5c0ca64ecbabc1af03c75022010e55c571d65da7701ae2da1956c442df81bbf076cdbac25133f99d98a9ed34c014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffffe15557cd5ce258f479dfd6dc6514edf6d7ed5b21fcfa4a038fd69f06b83ac76e010000008b483045022023b3e0ab071eb11de2eb1cc3a67261b866f86bf6867d4558165f7c8c8aca2d86022100dc6e1f53a91de3efe8f63512850811f26284b62f850c70ca73ed5de8771fb451014104462e76fd4067b3a0aa42070082dcb0bf2f388b6495cf33d789904f07d0f55c40fbd4b82963c69b3dc31895d0c772c812b1d5fbcade15312ef1c0e8ebbb12dcd4ffffffff01404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000010000000166d7577163c932b4f9690ca6a80b6e4eb001f0a2fa9023df5595602aae96ed8d000000008a4730440220262b42546302dfb654a229cefc86432b89628ff259dc87edd1154535b16a67e102207b4634c020a97c3e7bbd0d4d19da6aa2269ad9dded4026e896b213d73ca4b63f014104979b82d02226b3a4597523845754d44f13639e3bf2df5e82c6aab2bdc79687368b01b1ab8b19875ae3c90d661a3d0a33161dab29934edeb36aa01976be3baf8affffffff02404b4c00000000001976a9144854e695a02af0aeacb823ccbc272134561e0a1688ac40420f00000000001976a914abee93376d6b37b5c2940655a6fcaf1c8e74237988ac0000000001000000014e3f8ef2e91349a9059cb4f01e54ab2597c1387161d3da89919f7ea6acdbb371010000008c49304602210081f3183471a5ca22307c0800226f3ef9c353069e0773ac76bb580654d56aa523022100d4c56465bdc069060846f4fbf2f6b20520b2a80b08b168b31e66ddb9c694e240014104976c79848e18251612f8940875b2b08d06e6dc73b9840e8860c066b7e87432c477e9a59a453e71e6d76d5fe34058b800a098fc1740ce3012e8fc8a00c96af966ffffffff02c0e1e400000000001976a9144134e75a6fcb6042034aab5e18570cf1f844f54788ac404b4c00000000001976a9142b6ba7c9d796b75eef7942fc9288edd37c32f5c388ac00000000"), SER_NETWORK, PROTOCOL_VERSION);
    stream >> block;
    return block;
}

