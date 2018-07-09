// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/test_chaincoin.h>

#include <chainparams.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <crypto/sha256.h>
#include <validation.h>
#include <miner.h>
#include <net_processing.h>
#include <pow.h>
#include <ui_interface.h>
#include <streams.h>
#include <rpc/server.h>
#include <rpc/register.h>
#include <script/sigcache.h>

void CConnmanTest::AddNode(CNode& node)
{
    LOCK(g_connman->cs_vNodes);
    g_connman->vNodes.push_back(&node);
}

void CConnmanTest::ClearNodes()
{
    LOCK(g_connman->cs_vNodes);
    for (CNode* node : g_connman->vNodes) {
        delete node;
    }
    g_connman->vNodes.clear();
}

uint256 insecure_rand_seed = GetRandHash();
FastRandomContext insecure_rand_ctx(insecure_rand_seed);

extern bool fPrintToConsole;
extern void noui_connect();

std::ostream& operator<<(std::ostream& os, const uint256& num)
{
    os << num.ToString();
    return os;
}

BasicTestingSetup::BasicTestingSetup(const std::string& chainName)
    : m_path_root(fs::temp_directory_path() / "test_bitcoin" / strprintf("%lu_%i", (unsigned long)GetTime(), (int)(InsecureRandRange(1 << 30))))
{
    SHA256AutoDetect();
    RandomInit();
    ECC_Start();
    SetupEnvironment();
    SetupNetworking();
    InitSignatureCache();
    InitScriptExecutionCache();
    fCheckBlockIndex = true;
    SelectParams(chainName);
    noui_connect();
}

BasicTestingSetup::~BasicTestingSetup()
{
    fs::remove_all(m_path_root);
    ECC_Stop();
}

fs::path BasicTestingSetup::SetDataDir(const std::string& name)
{
    fs::path ret = m_path_root / name;
    fs::create_directories(ret);
    gArgs.ForceSetArg("-datadir", ret.string());
    return ret;
}

TestingSetup::TestingSetup(const std::string& chainName) : BasicTestingSetup(chainName)
{
    SetDataDir("tempdir");
    const CChainParams& chainparams = Params();
        // Ideally we'd move all the RPC tests to the functional testing framework
        // instead of unit tests, but for now we need these here.

        RegisterAllCoreRPCCommands(tableRPC);
        ClearDatadirCache();

        // We have to run a scheduler thread to prevent ActivateBestChain
        // from blocking due to queue overrun.
        threadGroup.create_thread(boost::bind(&CScheduler::serviceQueue, &scheduler));
        GetMainSignals().RegisterBackgroundSignalScheduler(scheduler);

        mempool.setSanityCheck(1.0);
        pblocktree.reset(new CBlockTreeDB(1 << 20, true));
        pcoinsdbview.reset(new CCoinsViewDB(1 << 23, true));
        pcoinsTip.reset(new CCoinsViewCache(pcoinsdbview.get()));
        if (!LoadGenesisBlock(chainparams)) {
            throw std::runtime_error("LoadGenesisBlock failed.");
        }
        {
            CValidationState state;
            if (!ActivateBestChain(state, chainparams)) {
                throw std::runtime_error(strprintf("ActivateBestChain failed. (%s)", FormatStateMessage(state)));
            }
        }
        nScriptCheckThreads = 3;
        for (int i=0; i < nScriptCheckThreads-1; i++)
            threadGroup.create_thread(&ThreadScriptCheck);
        g_connman = std::unique_ptr<CConnman>(new CConnman(0x1337, 0x1337)); // Deterministic randomness for tests.
        connman = g_connman.get();
        peerLogic.reset(new PeerLogicValidation(connman, scheduler, /*enable_bip61=*/true));
}

TestingSetup::~TestingSetup()
{
        threadGroup.interrupt_all();
        threadGroup.join_all();
        GetMainSignals().FlushBackgroundCallbacks();
        GetMainSignals().UnregisterBackgroundSignalScheduler();
        g_connman.reset();
        peerLogic.reset();
        UnloadBlockIndex();
        pcoinsTip.reset();
        pcoinsdbview.reset();
        pblocktree.reset();
}

TestChain100Setup::TestChain100Setup() : TestingSetup(CBaseChainParams::REGTEST)
{
    // CreateAndProcessBlock() does not support building SegWit blocks, so don't activate in these tests.
    // TODO: fix the code to support SegWit blocks.
    UpdateVersionBitsParameters(Consensus::DEPLOYMENT_SEGWIT, 0, Consensus::BIP9Deployment::NO_TIMEOUT);
    // Generate a 100-block chain:
    coinbaseKey.MakeNewKey(true);
    CScript scriptPubKey = CScript() <<  ToByteVector(coinbaseKey.GetPubKey()) << OP_CHECKSIG;
    for (int i = 0; i < COINBASE_MATURITY; i++)
    {
        std::vector<CMutableTransaction> noTxns;
        CBlock b = CreateAndProcessBlock(noTxns, scriptPubKey);
        m_coinbase_txns.push_back(b.vtx[0]);
    }
}

//
// Create a new block with just given transactions, coinbase paying to
// scriptPubKey, and try to add it to the current chain.
//
CBlock
TestChain100Setup::CreateAndProcessBlock(const std::vector<CMutableTransaction>& txns, const CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();
    std::unique_ptr<CBlockTemplate> pblocktemplate = BlockAssembler(chainparams).CreateNewBlock(scriptPubKey);
    CBlock& block = pblocktemplate->block;

    // Replace mempool-selected txns with just coinbase plus passed-in txns:
    block.vtx.resize(1);
    for (const CMutableTransaction& tx : txns)
        block.vtx.push_back(MakeTransactionRef(tx));
    // IncrementExtraNonce creates a valid coinbase and merkleRoot
    {
        LOCK(cs_main);
        unsigned int extraNonce = 0;
        IncrementExtraNonce(&block, chainActive.Tip(), extraNonce);
    }

    while (!CheckProofOfWork(block.GetHash(), block.nBits, chainparams.GetConsensus())) ++block.nNonce;

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(block);
    ProcessNewBlock(chainparams, shared_pblock, true, nullptr);

    CBlock result = block;
    return result;
}

TestChain100Setup::~TestChain100Setup()
{
}


CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CMutableTransaction &tx) {
    return FromTx(MakeTransactionRef(tx));
}

CTxMemPoolEntry TestMemPoolEntryHelper::FromTx(const CTransactionRef& tx)
{
    return CTxMemPoolEntry(tx, nFee, nTime, nHeight,
                           spendsCoinbase, sigOpCost, lp);
}

/**
 * @returns a real block (000000002745c36eb5685f45e0a51d9fd859c016ce2b4ab6ead768a136923980)
 *      with 9 txs.
 */
CBlock getBlock9tx()
{
    CBlock block;
    CDataStream stream(ParseHex("02000000c4ed31f4517dd558fe60878c76800358baf371b0d67a3614d81f3d0d0000000044be08544792fe5c8ee6588d6f747ae42d172a81097e409b55b6a0cc02249c48e534dc5266473e1ce01348600901000000010000000000000000000000000000000000000000000000000000000000000000ffffffff26022905062f503253482f04e434dc5208f8009265000000000d2f7374726174756d506f6f6c2f0000000001205e5e5f000000001976a914a1f93feceeb14e511b9317362bab1a8575e0c22288ac0000000001000000019b96806608cdeb853a5294c838787c7ea5bbedc81081374239a55a16ffadd988000000006b48304502200beb4aaf77b9a163f6571e8f72fe9b507c02d033fc9223f0fd7aefae2d1cb5bb022100a6b38bc722e25530a93932ae4195819e6cef3b81b7e747ca677ff7a96acec3c3012102ad21959e00544abcbc70c23cc538f7d933acb7a545ec9c753fb53afe3d3b1a1cffffffff021e6ba556000000001976a914626ce069095f4d9b995b4ef3e495b307863867a888ace2a4b808000000001976a9148c6f0ffc0499909795f2e4ad6a6cd1d8f4903d8c88ac000000000100000001f15ef6ae49148f51ff7dff31c900928bff65d257a7f657382e4c7ea907363572000000006b483045022100b8311ffad29524f857b72eeeb6deb454d00e77b517f1210d7da56f4cb09ffcff02200b49fbe35af167cf9e9c0c3bc35cf08453466807773fdc37ae4039a69db2a803012102b5e6d062ab1b887a054f627babc589d6684e31ee7dfe3c392a70ac79b475bdd3ffffffff023c6af205000000001976a91491e2fbbcf8ddedbf12f4cc443ef2fcd7b842cd7588ac25829653000000001976a9142471cd18c7e133b5d2acf56fcb1e728dc42bb1eb88ac000000000100000001e1ef497af063132a5e8714aa9ab21bf68cda044df0d6f7e04948db0ee43e703b010000006a47304402206ba9d465ed4f215ae7bce84632744112fd345bade63ecbbd8b770cc86b800c4a02207931c36a9c98be885b1420e27f88a497e78b35a0eaa885bfe5656f0a44347173012103c8a3af3b42ed4e8346b36bb223730a9507afd2ea68f79bc3ca9acf621c1072f9ffffffff024be3861d000000001976a914d485b7122a79693f41e21c97d4b693cd4078c92b88acd24f293b000000001976a9146af1ae9841cc451a63b4589317eb0497f04ea39288ac000000000100000001f8628aa1bebe1dd26953901b13661a90685c7c67bca907602aa22d0f4be0ad34000000006c493046022100b03dce922ad5a916f4548702cec22dc61b6d8ddbee0967f6b271178c88bba04302210094ff605b45c9d26867b8a19169ed345d41885115ea17f948bca140eec8b84fef012102ae0e1155e52d91369e1fe7b1effe67a8ed487f518db9a6e7ff636fddb3a5acc2ffffffff02f782e04b000000001976a914ae496323cdbabd5e3810129897d73e53245a057688aca1243f0d000000001976a91490b8c572f077329e11f71f22db701e54251ead7e88ac000000000100000001ef97252cdb2360a56c34f60675f38a2bbd6e59032507c593657d459600cc272c000000006c4930460221008425642ce4e2106ff438541b31d5b19748c6f0eadf94023f82c24e6dee0c934902210081b7c381b70d2c62701c1180bb6cf089247e40c944aba6225087556a1241dfe0012102282be9821ecade1c96e6272ebf4e9c03c4d278e8784020e89e9b2736b2edf9d7ffffffff02b421e418000000001976a914f47a15801d89a1f3556bda25f26f1a46c52c133f88ac3f558f3d000000001976a914da5efd84be9f290160601760e5cb2319831aef5988ac0000000001000000014e0f09ed5c48610e3f17eec57629877bef19f8449e88daf73dc3881d103346cf000000006b48304502200ebc24805e6e560adb570c9cac98ce92486f00dd9d021dc2569d2f4c83d8f51b022100bb7ac141d4dbc4a88246cd346219d88e4f1fc025261dfe77f8292d5bc482aea601210306ee4236f90f0eace65354606c881db93457475fb04cece617288b1bfaab41dcffffffff02a3e5bf07000000001976a9144d2663c916043dab0880fe6628901ffeae62bb9b88ac358ea14c000000001976a914c2dc4489083e2f3aa86dd41d453e64c104afe8a388ac000000000100000001261a470a6199dcd14a33a7e93385598b08c3a677e2db3a5c2b802e8621c38552000000006b48304502202ec355c90e32077c96baeb009f02a23474b82fa9b8f20b2405ca2e84278fedd90221008d609208a1a9c6331ef2f3b6fcd27ef5f851d2d8c6aca4a96cd7d84f8099b44701210212de37b4a8575106c78231fbe297f03f3d11268c7ffa5b974d1f30256366a3afffffffff0241235e0f000000001976a9145584d2ab8e90905cafe4e8988c3281eb17750f2188ac40eb0931000000001976a91491cd27ab0540fd085ba28f41f11056567f2de49588ac000000000100000002440ac5babf866351465a5e3c4f7b5a136a9c7070949043be31701c83a16bd7c6000000006c4930460221008aceb227276945ea1403f883ca8b0c9150da40feca6c797b64a296f400a24957022100f42ff63818b1d35bbc05211e69b7cdcd86ab5d6fdbaed6ed2ec44443c478884c01210388d0a8a1ea23388d71387b19d5897e8761bf520e5b0f44eef03dc6881e97b856ffffffff6f16cea5f71e2a80d1d9a10ec4af9503a759a05176dd26ad6d5c710b80fc982a010000006b483045022100ca2f21ec3a5e38fee31a1d9a92b39d8e6b8683c64204c834fe902bd7adbf6b5d02205b92539089fad6d1de349767d4b4153b1a1a6a5680f4c8211cb71125b70207b30121022ed39e70b32df0b85aa2a4e71f3bd6919ef16b9279e3c0bc730e86714503ef3dffffffff020b96990b000000001976a9143b129ee79d4ce105753c2d575fb11a388120557288ac5359ea01000000001976a91474eae094c7bf1951fc2e3beb4618b1eb02a4769188ac00000000"), SER_NETWORK, PROTOCOL_VERSION);
    stream >> block;
    return block;
}
