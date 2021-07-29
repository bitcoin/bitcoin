// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <hash.h> // for signet block challenge hash
#include <util/system.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
// SYSCOIN includes for gen block
#include <uint256.h>
#include <arith_uint256.h>
#include <hash.h>
#include <streams.h>
#include <time.h>
static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}
// This will figure out a valid hash and Nonce if you're
// creating a different genesis block:
/*static void GenerateGenesisBlock(CBlockHeader &genesisBlock, uint256 &phash)
{
    arith_uint256 bnTarget;
    bnTarget.SetCompact(genesisBlock.nBits);
    uint32_t nOnce = 0;
    while (true) {
        genesisBlock.nNonce = nOnce;
        uint256 hash = genesisBlock.GetHash();
        if (UintToArith256(hash) <= bnTarget) {
            phash = hash;
            break;
        }
        nOnce++;
    }
    tfm::format(std::cout,"genesis.nTime = %u \n", genesisBlock.nTime);
    tfm::format(std::cout,"genesis.nNonce = %u \n", genesisBlock.nNonce);
    tfm::format(std::cout,"Generate hash = %s\n", phash.ToString().c_str());
    tfm::format(std::cout,"genesis.hashMerkleRoot = %s\n", genesisBlock.hashMerkleRoot.ToString().c_str());
}  */ 
/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateDIP3Parameters(int nActivationHeight, int nEnforcementHeight)
{
    consensus.DIP0003Height = nActivationHeight;
    consensus.DIP0003EnforcementHeight = nEnforcementHeight;
}

void CChainParams::UpdateLLMQTestParams(int size, int threshold) {
    auto& params = consensus.llmqs.at(Consensus::LLMQ_TEST);
    params.size = size;
    params.minSize = threshold;
    params.threshold = threshold;
    params.dkgBadVotesThreshold = threshold;
}
// this one is for testing only
static Consensus::LLMQParams llmq_test = {
        .type = Consensus::LLMQ_TEST,
        .name = "llmq_test",
        .size = 3,
        .minSize = 2,
        .threshold = 2,

        .dkgInterval = 24, // one DKG per hour
        .dkgPhaseBlocks = 2,
        .dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 18,
        .dkgBadVotesThreshold = 2,

        .signingActiveQuorumCount = 4, // just a few ones to allow easier testing

        .keepOldConnections = 5,
        .recoveryMembers = 3,
};

static Consensus::LLMQParams llmq50_60 = {
        .type = Consensus::LLMQ_50_60,
        .name = "llmq_50_60",
        .size = 50,
        .minSize = 40,
        .threshold = 30,

        .dkgInterval = 60, // one DKG per hour
        .dkgPhaseBlocks = 5,
        .dkgMiningWindowStart = 25, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 45,
        .dkgBadVotesThreshold = 40,

        .signingActiveQuorumCount = 24, // a full day worth of LLMQs

        .keepOldConnections = 25,
        .recoveryMembers = 25,
};

static Consensus::LLMQParams llmq400_60 = {
        .type = Consensus::LLMQ_400_60,
        .name = "llmq_400_60",
        .size = 400,
        .minSize = 300,
        .threshold = 240,

        .dkgInterval = 24 * 12, // one DKG every 12 hours
        .dkgPhaseBlocks = 4,
        .dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
        .dkgMiningWindowEnd = 28,
        .dkgBadVotesThreshold = 300,

        .signingActiveQuorumCount = 4, // two days worth of LLMQs

        .keepOldConnections = 5,
        .recoveryMembers = 100,
};

/**
 * Main network on which people trade goods and services.
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210240; 
        // 35% increase after 525600 blocks, 100% increase after sr level 2
        consensus.nSeniorityHeight1 = 525600; 
        consensus.nSeniorityLevel1 = 0.35;
        consensus.nSeniorityHeight2 = 525600 * 2.5; 
        consensus.nSeniorityLevel2 = 1.0;        
        consensus.nSuperblockStartBlock = 1;
        consensus.nSuperblockCycle = 17520;
        consensus.nGovernanceMinQuorum = 10; 
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;
        
        consensus.BIP16Height = 1;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1; 
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 6 * 60 * 60;
        consensus.nPowTargetSpacing = 2.5 * 60; // Syscoin: 2.5 minute
        consensus.nAuxpowChainId = 16;
        consensus.nAuxpowOldChainId = 4096;
        consensus.nAuxpowStartHeight = 1;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = consensus.nMinerConfirmationWindow;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000035e351a082cc5d82961caae");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0xe9599cf8d6462f63f17a8ec790803cf77028a380a1de84a976039914a45f5abb"); // 998000
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 1;
        consensus.nSYSXAsset = 1045909988;
        consensus.vchSYSXBurnMethodSignature = ParseHex("54c988ff");
        consensus.vchSYSXERC20Manager = ParseHex("FF957eA28b537b34E0c6E6B50c6c938668DD28a0");
        consensus.vchTokenFreezeMethod = ParseHex("7ca654cf9212e4c3cf0164a529dd6159fc71113f867d0b09fdeb10aa65780732");
        consensus.nBridgeStartBlock = 348000;
        consensus.nNEVMStartBlock = 1348000;
        consensus.nUTXOAssetsBlock = 1004200;
        consensus.nUTXOAssetsBlockProvisioning = consensus.nUTXOAssetsBlock + 10000;
        consensus.DIP0003Height = 1004200;
        consensus.DIP0003EnforcementHeight = 1004200;
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xce;
        pchMessageStart[1] = 0xe2;
        pchMessageStart[2] = 0xca;
        pchMessageStart[3] = 0xff;
        nDefaultPort = 8369;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 30;
        m_assumed_chain_state_size = 2;

        genesis = CreateGenesisBlock(1559520000, 1372898, 0x1e0fffff, 1, 50 * COIN);
        
        /*uint256 hash;
        CBlockHeader genesisHeader = genesis.GetBlockHeader();
        GenerateGenesisBlock(genesisHeader, hash);*/
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000022642db0346b6e01c2a397471f4f12e65d4f4251ec96c1f85367a61a7ab"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vSeeds.emplace_back("seed1.syscoin.org");
        vSeeds.emplace_back("seed2.syscoin.org");
        vSeeds.emplace_back("seed3.syscoin.org");
        vSeeds.emplace_back("seed4.syscoin.org");
        
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,63);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "sys";

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_main), std::end(chainparams_seed_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fRequireRoutableExternalIP = true;
        vSporkAddresses = {"sys1qx0zzzjag402apkw4kn8unr0qa0k3pv3258v4sr", "sys1qk2kq7hhp58ycaevzzu5hugh7flxs7qcg8rjjlh", "sys1qm4ka204x3mn46sk6ussrex8um87qkj0r5xakyg"};
        nMinSporkKeys = 2;   
        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqTypeChainLocks = Consensus::LLMQ_400_60;
        nLLMQConnectionRetryTimeout = 60;
        fAllowMultiplePorts = false;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 250, uint256S("0x00000c9ec0f9d60ce297bf9f9cbe1f2eb39165a0d3f69c1c55fc3f6680fe45c8")},
                { 5000, uint256S("0xeef3554a3f467bcdc7570f799cecdb262058cecf34d555827c99b5719b1df4f6")},
                { 10000, uint256S("0xe44257e8e027e8a67fd647c54e1bd6976988d75b416affabe3f82fd87a67f5ff")},
                { 40000, uint256S("0x4ad1ec207d62fa91485335feaf890150a0f4cf48c39b11e3dbfc22bdecc29dbc")},
                { 100000, uint256S("0xa54904302fd6fd0ee561cb894f15ad8c21c2601b305ffa9e15ef00df1c50db16")},
                { 150000, uint256S("0x73850eb99a6c32b4bfd67a26a7466ce3d0b4412d4174590c501e567c99f038fd")},
                { 200000, uint256S("0xa28fe36c63acb38065dadf09d74de5fdc1dac6433c204b215b37bab312dfab0d")},
                { 240000, uint256S("0x906918ba0cbfbd6e4e4e00d7d47d08bef3e409f47b59cb5bd3303f5276b88f0f")},
                { 280000, uint256S("0x651375427865345d37a090ca561c1ed135c6b8dafa591a59f2abf1eb26dfd538")},
                { 292956, uint256S("0xae6dca1b9dd7adcb8a11c8ea7f9fe72bb47ff6e4156e1d172e2a8612b18a319d")},
                { 350000, uint256S("0x02501c7feba858c83e005acbf0505a892081288dcf7a8a37bd4fc47d7c24c799")},
                { 390000, uint256S("0x8654451a7ed5286ba5c830cdf6e65cbbd7a77f650216541bfbe50af04933741b")},
                { 391285, uint256S("76d13e8f08c2b7027251484078f734f91c485727031be6b4c21c42d5e103d0ad")},
                { 419800, uint256S("4c332acd53ca99ab78fb80a3dacffe234674674e0b682350c492d7fe839d128e")},
                { 600000, uint256S("de2321b2a3b927450835590111bbbc9220d49df865117a33fb3c4687aedbbe9c")},
                { 700000, uint256S("f5d72e57625c2af8cd5147e1e029e8353fea22fde1fcea06f67149d0af1fbf09")},
                { 800000, uint256S("54bf4bd4b5c7d36323fed4b649e75e0ce4902261533d13a15c861fa2ab3c7362")},
                { 998000, uint256S("e9599cf8d6462f63f17a8ec790803cf77028a380a1de84a976039914a45f5abb")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
         // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats at block 998364
            /* nTime    */ 1619451547,
            /* nTxCount */ 1876346,
            /* dTxRate  */ 0.02035896492467413
        };
    }
};

/**
 * Testnet (v3): public test network which is reset from time to time.
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210240;
        // 35% increase after 60 blocks, 100% increase after 150 blocks
        consensus.nSeniorityHeight1 = 60;
        consensus.nSeniorityLevel1 = 0.35;
        consensus.nSeniorityHeight2 = 60*2.5;
        consensus.nSeniorityLevel2 = 1.0;         
        consensus.nSuperblockStartBlock = 1;
        consensus.nSuperblockCycle = 60;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP16Height = 1;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.SegwitHeight = 0;
        consensus.CSVHeight = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 6 * 60 * 60;
        consensus.nPowTargetSpacing = 2.5 * 60; // Syscoin: 2.5 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = consensus.nMinerConfirmationWindow;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000004b8e46524f7"); // 550k

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000013d53482bd69c5403f344643668619f77302910e57ffe7b1d375e73cc91"); // 550k
        consensus.nAuxpowStartHeight = 1;
        consensus.nAuxpowChainId = 8;
        consensus.nAuxpowOldChainId = 4096;
        consensus.fStrictChainId = false;
        consensus.nLegacyBlocksBefore = 1;
        consensus.nSYSXAsset = 1965866356;
        consensus.vchSYSXBurnMethodSignature = ParseHex("54c988ff");
        consensus.vchSYSXERC20Manager = ParseHex("443d9a14fb6ba2A45465bEC3767186f404Ccea25");
        consensus.vchTokenFreezeMethod = ParseHex("7ca654cf9212e4c3cf0164a529dd6159fc71113f867d0b09fdeb10aa65780732");
        consensus.nBridgeStartBlock = 0;
        consensus.nNEVMStartBlock = 205;
        consensus.nUTXOAssetsBlock = 0;
        consensus.nUTXOAssetsBlockProvisioning = consensus.nUTXOAssetsBlock + 10000;
        consensus.DIP0003Height = 545000;
        consensus.DIP0003EnforcementHeight = 545000;
        pchMessageStart[0] = 0xce;
        pchMessageStart[1] = 0xe2;
        pchMessageStart[2] = 0xca;
        pchMessageStart[3] = 0xfe;
        nDefaultPort = 18369;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 30;
        m_assumed_chain_state_size = 2;

        genesis = CreateGenesisBlock(1576000001, 500647, 0x1e0fffff, 1, 50 * COIN);
        /*uint256 hash;
        CBlockHeader genesisHeader = genesis.GetBlockHeader();
        GenerateGenesisBlock(genesisHeader, hash);*/
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000009e79cfb8c84bb786380480ecfd98d04a51bfbfd39e1ac496fb621fabb4b"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testseed1.syscoin.org");
        vSeeds.emplace_back("testseed2.syscoin.org");
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tsys";

        // vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_test), std::end(chainparams_seed_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fRequireRoutableExternalIP = true;
        m_is_test_chain = true;

        // privKey: cU52TqHDWJg6HoL3keZHBvrJgsCLsduRvDFkPyZ5EmeMwoEHshiT
        vSporkAddresses = {"TCGpumHyMXC5BmfkaAQXwB7Bf4kbkhM9BX", "tsys1qgmafz3mqa7glqy92r549w8qmq5535uc2e8ahjm", "tsys1q68gu0fhcchr27w08sjdxwt3rtgwef0nyh9zwk0"};
        nMinSporkKeys = 2;   
        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
        consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
        consensus.llmqTypeChainLocks = Consensus::LLMQ_400_60;
        nLLMQConnectionRetryTimeout = 60;
        fAllowMultiplePorts = false;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {360, uint256S("0x00000c04c5926f539074420b40088d4b099d748d07795df891ca391799b6e54c")},
                {250000, uint256S("0x00000131e97a4cb713338f33b8fa6573c85f1772e4dd7d510ca2281cc0be86e2")},
                {534114, uint256S("0x0000013d53482bd69c5403f344643668619f77302910e57ffe7b1d375e73cc91")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 0000000000000037a8cd3e06cd5edbfe9dd1dbcc5dacab279376ef7cfc2b4c75
            /* nTime    */ 1610136593,
            /* nTxCount */ 538904,
            /* dTxRate  */ 0.01666588574531774
        };
    }
};

/**
 * Signet: test network with an additional consensus parameter (see BIP325).
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            vSeeds.emplace_back("178.128.221.177");
            vSeeds.emplace_back("2a01:7c8:d005:390::5");
            vSeeds.emplace_back("v7ajjeirttkbnt32wpy3c6w3emwnfr3fkla7hpxcfokr3ysd3kqtzmqd.onion:38333");

            consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000d145533ce");
            consensus.defaultAssumeValid = uint256S("0x00000128807d9175c494e24d805fc7854f7d79aa965cbb128342ad8b70cecfa5"); // 5348
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 00000128807d9175c494e24d805fc7854f7d79aa965cbb128342ad8b70cecfa5
                /* nTime    */ 1601382000,
                /* nTxCount */ 5435,
                /* dTxRate  */ 0.001898346323372538,
            };
        } else {
            const auto signet_challenge = args.GetArgs("-signetchallenge");
            if (signet_challenge.size() != 1) {
                throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
            }
            bin = ParseHex(signet_challenge[0]);

            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", signet_challenge[0]);
        }

        if (args.IsArgSet("-signetseednode")) {
            vSeeds = args.GetArgs("-signetseednode");
        }

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.signet_blocks = true;
        // 35% increase after 1 hr, 100% increase after 2.5 hr
        consensus.nSeniorityHeight1 = 60;
        consensus.nSeniorityLevel1 = 0.35;
        consensus.nSeniorityHeight2 = 60*2.5;
        consensus.nSeniorityLevel2 = 1.0;         
        consensus.nSuperblockStartBlock = 1;
        consensus.nSuperblockCycle = 60;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nSYSXAsset = 1965866356;
        consensus.vchSYSXBurnMethodSignature = ParseHex("54c988ff");
        consensus.vchSYSXERC20Manager = ParseHex("443d9a14fb6ba2A45465bEC3767186f404Ccea25");
        consensus.vchTokenFreezeMethod = ParseHex("7ca654cf9212e4c3cf0164a529dd6159fc71113f867d0b09fdeb10aa65780732");
        consensus.nBridgeStartBlock = 1000;
        consensus.nNEVMStartBlock = 1348000;
        consensus.nUTXOAssetsBlock = 0;
        consensus.nUTXOAssetsBlockProvisioning = consensus.nUTXOAssetsBlock + 10000;
        consensus.DIP0003Height = 200;
        consensus.DIP0003EnforcementHeight = 400;
        consensus.nAuxpowStartHeight = 1;
        consensus.nAuxpowChainId = 16;
        consensus.nAuxpowOldChainId = 4096;
        consensus.fStrictChainId = false;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 500;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1351;
        consensus.BIP66Height = 1251;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 2.5 * 60; // Syscoin: 2.5 minute
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000377ae000000000000000000000000000000000000000000000000000000");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // message start is defined as the first 4 bytes of the sha256d of the block script
        CHashWriter h(SER_DISK, 0);
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38369;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1598918400, 52613770, 0x1e0377ae, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00000008819873e925422c1ff0f99f7cc9bbb232af63a077a480a3633bee1ef6"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tsys";
        // privey cTQmhexENtxiSVzaR1Rhx5uwX4V9H9Ajkc2CLa64LUX8BqcHohqr
        vSporkAddresses = {"tsys1qc28aan6qkuhuj30cpx3jet3l0mrr9sfp8a9typ"};
        nMinSporkKeys = 1;
        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_TEST] = llmq_test;
        consensus.llmqTypeChainLocks = Consensus::LLMQ_TEST;
        nLLMQConnectionRetryTimeout = 1; // must be lower then the LLMQ signing session timeout so that tests have control over failing behavior
        fAllowMultiplePorts = true;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test: intended for private networks only. Has minimal difficulty to ensure that
 * blocks can be found instantly.
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.BIP16Exception = uint256();
        consensus.BIP16Height = 1;
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432; // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.nSubsidyHalvingInterval = 150;
        // 35% increase after 1 hr, 100% increase after 2.5 hr
        consensus.nSeniorityHeight1 = 60;
        consensus.nSeniorityLevel1 = 0.35;
        consensus.nSeniorityHeight2 = 60*2.5;
        consensus.nSeniorityLevel2 = 1.0;      
        consensus.nSuperblockStartBlock = 1;
        consensus.nSuperblockCycle = 10;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 6 * 60 * 60;
        consensus.nPowTargetSpacing = 2.5 * 60; // Syscoin: 2.5 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256{};

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256{};
        consensus.nAuxpowStartHeight = 0;
        consensus.nAuxpowChainId = 16;
        consensus.nAuxpowOldChainId = 4096;
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 0;
        consensus.nSYSXAsset = 123456;
        consensus.vchSYSXBurnMethodSignature = ParseHex("54c988ff");
        consensus.vchSYSXERC20Manager = ParseHex("A738a563F9ecb55e0b2245D1e9E380f0fE455ea1");
        consensus.vchTokenFreezeMethod = ParseHex("7ca654cf9212e4c3cf0164a529dd6159fc71113f867d0b09fdeb10aa65780732");
        consensus.nBridgeStartBlock = 0;
        consensus.nNEVMStartBlock = 205;
        consensus.nUTXOAssetsBlock = 0;
        consensus.nUTXOAssetsBlockProvisioning = 1000;
        consensus.DIP0003Height = 432;
        consensus.DIP0003EnforcementHeight = 432;
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = args.GetBoolArg("-fastprune", false) ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(1553040331, 3, 0x207fffff, 1, 50 * COIN);
        
        /*uint256 hash;
        CBlockHeader genesisHeader = genesis.GetBlockHeader();
        GenerateGenesisBlock(genesisHeader, hash);*/
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x28a2c2d251f46fac05ade79085cbcb2ae4ec67ea24f1f1c7b40a348c00521194"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        fRequireRoutableExternalIP = false;
        m_is_test_chain = true;
        m_is_mockable_chain = true;
        // privKey: cVpF924EspNh8KjYsfhgY96mmxvT6DgdWiTYMtMjuM74hJaU5psW
        vSporkAddresses = {"mjTkW3DjgyZck4KbiRusZsqTgaYTxdSz6z"};
        nMinSporkKeys = 1; 
        // long living quorum params
        consensus.llmqs[Consensus::LLMQ_TEST] = llmq_test;
        consensus.llmqTypeChainLocks = Consensus::LLMQ_TEST;
        nLLMQConnectionRetryTimeout = 1; // must be lower then the LLMQ signing session timeout so that tests have control over failing behavior
        fAllowMultiplePorts = true;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
       /* 
        

        checkpointData = {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},
            }
        };*/
        // SYSCOIN
        m_assumeutxo_data = MapAssumeutxo{
            {
                110,
                {AssumeutxoHash{uint256S("0x70b3b480f5476ad86dfd98db1f12bda867a90b6cf1b37644332eb52c2ba74ac9")}, 110},
            },
            {
                200,
                {AssumeutxoHash{uint256S("0x51c8d11d8b5c1de51543c579736e786aa2736206d1e11e627568029ce092cf62")}, 200},
            },
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int min_activation_height)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        consensus.vDeployments[d].min_activation_height = min_activation_height;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-mncollateral")) {
        uint32_t collateral = args.GetArg("-mncollateral", DEFAULT_MN_COLLATERAL_REQUIRED);
        nMNCollateralRequired = collateral*COIN;
    }
    if (args.IsArgSet("-dip3params")) {
        std::string strDIP3Params = args.GetArg("-dip3params", "");
        std::vector<std::string> vDIP3Params;
        boost::split(vDIP3Params, strDIP3Params, boost::is_any_of(":"));
        if (vDIP3Params.size() != 2) {
            throw std::runtime_error("DIP3 parameters malformed, expecting DIP3ActivationHeight:DIP3EnforcementHeight");
        }
        int nDIP3ActivationHeight, nDIP3EnforcementHeight;
        if (!ParseInt32(vDIP3Params[0], &nDIP3ActivationHeight)) {
            throw std::runtime_error(strprintf("Invalid nDIP3ActivationHeight (%s)", vDIP3Params[0]));
        }
        if (!ParseInt32(vDIP3Params[1], &nDIP3EnforcementHeight)) {
            throw std::runtime_error(strprintf("Invalid nDIP3EnforcementHeight (%s)", vDIP3Params[1]));
        }
        UpdateDIP3Parameters(nDIP3ActivationHeight, nDIP3EnforcementHeight);
    }
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        int64_t nStartTime, nTimeout;
        int min_activation_height = 0;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4 && !ParseInt32(vDeploymentParams[3], &min_activation_height)) {
            throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, min_activation_height);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d\n", vDeploymentParams[0], nStartTime, nTimeout, min_activation_height);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new SigNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}
void UpdateLLMQTestParams(int size, int threshold)
{
    auto* params = const_cast<CChainParams*> (globalChainParams.get ());
    params->UpdateLLMQTestParams(size, threshold);
}