// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "test_dash.h"

#include "chainparams.h"
#include "consensus/consensus.h"
#include "consensus/validation.h"
#include "crypto/sha256.h"
#include "fs.h"
#include "key.h"
#include "validation.h"
#include "miner.h"
#include "net_processing.h"
#include "pubkey.h"
#include "random.h"
#include "txdb.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "rpc/server.h"
#include "rpc/register.h"
#include "script/sigcache.h"

#include "evo/specialtx.h"
#include "evo/deterministicmns.h"
#include "evo/cbtx.h"
#include "llmq/quorums_init.h"

#include <memory>

void CConnmanTest::AddNode(CNode& node)
{
    LOCK(g_connman->cs_vNodes);
    g_connman->vNodes.push_back(&node);
}

void CConnmanTest::ClearNodes()
{
    LOCK(g_connman->cs_vNodes);
    g_connman->vNodes.clear();
}

uint256 insecure_rand_seed = GetRandHash();
FastRandomContext insecure_rand_ctx(insecure_rand_seed);

extern bool fPrintToConsole;
extern void noui_connect();

BasicTestingSetup::BasicTestingSetup(const std::string& chainName)
{
        SHA256AutoDetect();
        RandomInit();
        ECC_Start();
        BLSInit();
        SetupEnvironment();
        SetupNetworking();
        InitSignatureCache();
        InitScriptExecutionCache();
        fPrintToDebugLog = false; // don't want to write to debug.log file
        fCheckBlockIndex = true;
        SelectParams(chainName);
        evoDb = new CEvoDB(1 << 20, true, true);
        deterministicMNManager = new CDeterministicMNManager(*evoDb);
        noui_connect();
}

BasicTestingSetup::~BasicTestingSetup()
{
        delete deterministicMNManager;
        delete evoDb;

        ECC_Stop();
}

TestingSetup::TestingSetup(const std::string& chainName) : BasicTestingSetup(chainName)
{
    const CChainParams& chainparams = Params();
        // Ideally we'd move all the RPC tests to the functional testing framework
        // instead of unit tests, but for now we need these here.
        RegisterAllCoreRPCCommands(tableRPC);
        ClearDatadirCache();
        pathTemp = fs::temp_directory_path() / strprintf("test_dash_%lu_%i", (unsigned long)GetTime(), (int)(InsecureRandRange(100000)));
        fs::create_directories(pathTemp);
        gArgs.ForceSetArg("-datadir", pathTemp.string());

        // Note that because we don't bother running a scheduler thread here,
        // callbacks via CValidationInterface are unreliable, but that's OK,
        // our unit tests aren't testing multiple parts of the code at once.
        GetMainSignals().RegisterBackgroundSignalScheduler(scheduler);
        mempool.setSanityCheck(1.0);
        g_connman = std::unique_ptr<CConnman>(new CConnman(0x1337, 0x1337)); // Deterministic randomness for tests.
        connman = g_connman.get();
        pblocktree = new CBlockTreeDB(1 << 20, true);
        pcoinsdbview = new CCoinsViewDB(1 << 23, true);
        llmq::InitLLMQSystem(*evoDb, nullptr, true);
        pcoinsTip = new CCoinsViewCache(pcoinsdbview);
        if (!LoadGenesisBlock(chainparams)) {
            throw std::runtime_error("LoadGenesisBlock failed.");
        }
        {
            CValidationState state;
            if (!ActivateBestChain(state, chainparams)) {
                throw std::runtime_error("ActivateBestChain failed.");
            }
        }
        nScriptCheckThreads = 3;
        for (int i=0; i < nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&ThreadScriptCheck);
        peerLogic.reset(new PeerLogicValidation(connman, scheduler));
}

TestingSetup::~TestingSetup()
{
        llmq::InterruptLLMQSystem();
        threadGroup.interrupt_all();
        threadGroup.join_all();
        GetMainSignals().FlushBackgroundCallbacks();
        GetMainSignals().UnregisterBackgroundSignalScheduler();
        g_connman.reset();
        peerLogic.reset();
        UnloadBlockIndex();
        delete pcoinsTip;
        llmq::DestroyLLMQSystem();
        delete pcoinsdbview;
        delete pblocktree;
        fs::remove_all(pathTemp);
}

TestChainSetup::TestChainSetup(int blockCount) : TestingSetup(CBaseChainParams::REGTEST)
{
    // Generate a 100-block chain:
    coinbaseKey.MakeNewKey(true);
    CScript scriptPubKey = CScript() << ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    for (int i = 0; i < blockCount; i++)
    {
        std::vector<CMutableTransaction> noTxns;
        CBlock b = CreateAndProcessBlock(noTxns, scriptPubKey);
        coinbaseTxns.push_back(*b.vtx[0]);
    }
}

//
// Create a new block with just given transactions, coinbase paying to
// scriptPubKey, and try to add it to the current chain.
//
CBlock
TestChainSetup::CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns, const CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();
    auto block = CreateBlock(txns, scriptPubKey);

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
    ProcessNewBlock(chainparams, shared_pblock, true, nullptr);

    CBlock result = block;
    return result;
}

CBlock TestChainSetup::CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns, const CKey& scriptKey)
{
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    return CreateAndProcessBlock(txns, scriptPubKey);
}

CBlock TestChainSetup::CreateBlock(const std::vector<CMutableTransaction>& txns, const CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();
    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    CBlock& block = pblocktemplate->block;

    std::vector<CTransactionRef> llmqCommitments;
    for (const auto& tx : block.vtx) {
        if (tx->nVersion == 3 && tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            llmqCommitments.emplace_back(tx);
        }
    }

    // Replace mempool-selected txns with just coinbase plus passed-in txns:
    block.vtx.resize(1);
    // Re-add quorum commitments
    block.vtx.insert(block.vtx.end(), llmqCommitments.begin(), llmqCommitments.end());
    for (const CMutableTransaction& tx : txns)
        block.vtx.push_back(MakeTransactionRef(tx));

    // Manually update CbTx as we modified the block here
    if (block.vtx[0]->nType == TRANSACTION_COINBASE) {
        LOCK(cs_main);
        CCbTx cbTx;
        if (!GetTxPayload(*block.vtx[0], cbTx)) {
            BOOST_ASSERT(false);
        }
        CValidationState state;
        if (!CalcCbTxMerkleRootMNList(block, chainActive.Tip(), cbTx.merkleRootMNList, state)) {
            BOOST_ASSERT(false);
        }
        if (!CalcCbTxMerkleRootQuorums(block, chainActive.Tip(), cbTx.merkleRootQuorums, state)) {
            BOOST_ASSERT(false);
        }
        CMutableTransaction tmpTx = *block.vtx[0];
        SetTxPayload(tmpTx, cbTx);
        block.vtx[0] = MakeTransactionRef(tmpTx);
    }

    // IncrementExtraNonce creates a valid coinbase and merkleRoot
    unsigned int extraNonce = 0;
    IncrementExtraNonce(&block, chainActive.Tip(), extraNonce);

    while (!CheckProofOfWork(block.GetHash(), block.nBits, chainparams.GetConsensus())) ++block.nNonce;

    CBlock result = block;
    return result;
}

CBlock TestChainSetup::CreateBlock(const std::vector<CMutableTransaction>& txns, const CKey& scriptKey)
{
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    return CreateBlock(txns, scriptPubKey);
}

TestChainSetup::~TestChainSetup()
{
}


CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction &tx) {
    CTransaction txn(tx);
    return FromTx(txn);
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransaction &txn) {
    return CTxMemPoolEntry(MakeTransactionRef(txn), nFee, nTime, nHeight,
                           spendsCoinbase, sigOpCount, lp);
}
