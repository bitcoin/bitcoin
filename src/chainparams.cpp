// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
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
static void GenerateGenesisBlock(CBlockHeader &genesisBlock, uint256 &phash)
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
}   
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

void CChainParams::TurnOffSegwitForUnitTests ()
{
  consensus.BIP16Height = 1000000;
  consensus.BIP34Height = 1000000;
}
void CChainParams::SetSYSXAssetForUnitTests (uint32_t asset)
{
  consensus.nSYSXAsset = asset;
}
/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 525600; 
        // 35% increase after 1 year, 100% increase after 2.5 years
        consensus.nSeniorityHeight1 = 525600; 
        consensus.nSeniorityLevel1 = 0.35;
        consensus.nSeniorityHeight2 = 525600 * 2.5; 
        consensus.nSeniorityLevel2 = 1.0;        
        consensus.nSuperblockStartBlock = 1;
        consensus.nSuperblockCycle = 43800;
        consensus.nGovernanceMinQuorum = 10; 
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;
        
        consensus.BIP16Height = 1;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1; 
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 6 * 60 * 60; // 6h retarget
        consensus.nPowTargetSpacing = 1 * 60; // Syscoin: 1 minute
        consensus.nAuxpowChainId = 0x1000;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = consensus.nMinerConfirmationWindow;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // CSV (BIP68, BIP112 and BIP113) as well as segwit (BIP141, BIP143 and
        // BIP147) are deployed together with P2SH.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000007184257f8428fd0189c9a8");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0xd467a893337ea1d042b8ec254efde8637b5957abb5bea63676486a7c1ffb56c7");
        consensus.nAuxpowChainId = 0x1000;
        consensus.nAuxpowStartHeight = 1;
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 1;
        consensus.nSYSXAsset = 1045909988;
        consensus.vchSYSXBurnMethodSignature = ParseHex("5f959b69");
        consensus.vchSYSXERC20Manager = ParseHex("30929228931464ee18AF52b9f655F8176555b6e7");
        consensus.vchTokenFreezeMethod = ParseHex("aabab1db49e504b5156edf3f99042aeecb9607a08f392589571cd49743aaba8d");
        consensus.nBridgeStartBlock = 275000;
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
        
        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        strSporkAddress = "SSZvS59ddqG87koeUPu1J8ivg5yJsQiWGN";    
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour
        m_is_test_chain = false;

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
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 0000000000000000000f1c54590ee18d15ec70e68c8cd4cfbadb1b4f11697eee
            /* nTime    */ 0,
            /* nTxCount */ 0,
            /* dTxRate  */ 0
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 525600;
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
        consensus.BIP16Height = 1;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetTimespan = 6 * 60 * 60; // 6h retarget
        consensus.nPowTargetSpacing = 1 * 60; // Syscoin: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = consensus.nMinerConfirmationWindow;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL; // December 31, 2008
        // CSV (BIP68, BIP112 and BIP113) as well as segwit (BIP141, BIP143 and
        // BIP147) are deployed together with P2SH.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00000c04c5926f539074420b40088d4b099d748d07795df891ca391799b6e54c");
        consensus.nAuxpowStartHeight = 1;
        consensus.nAuxpowChainId = 0x1000;
        consensus.fStrictChainId = false;
        consensus.nLegacyBlocksBefore = 1;
        consensus.nSYSXAsset = 1965866356;
        consensus.vchSYSXBurnMethodSignature = ParseHex("5f959b69");
        consensus.vchSYSXERC20Manager = ParseHex("4Cafd654dA402B4f14aCbE7cf2E8F0a43Cd68e19");
        consensus.vchTokenFreezeMethod = ParseHex("aabab1db49e504b5156edf3f99042aeecb9607a08f392589571cd49743aaba8d");
        consensus.nBridgeStartBlock = 1000;
        pchMessageStart[0] = 0xce;
        pchMessageStart[1] = 0xe2;
        pchMessageStart[2] = 0xca;
        pchMessageStart[3] = 0xfe;
        nDefaultPort = 18369;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 30;
        m_assumed_chain_state_size = 2;

        genesis = CreateGenesisBlock(1576000000, 297648, 0x1e0fffff, 1, 50 * COIN);
        
        /*uint256 hash;
        CBlockHeader genesisHeader = genesis.GetBlockHeader();
        GenerateGenesisBlock(genesisHeader, hash);*/
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000066e1a6b9cfeac8295dce0cc8d9170690a74bc4878cf8a0b412554f5c222"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        /*vSeeds.emplace_back("testnet-seed.syscoin.jonasschnelli.ch");
        vSeeds.emplace_back("seed.tsys.petertodd.org");
        vSeeds.emplace_back("seed.testnet.syscoin.sprovoost.nl");
        vSeeds.emplace_back("testnet-seed.bluematt.me"); // Just a static list of stable node(s), only supports x9
*/
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tsys";



        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;

        // privKey: cU52TqHDWJg6HoL3keZHBvrJgsCLsduRvDFkPyZ5EmeMwoEHshiT
        strSporkAddress = "TCGpumHyMXC5BmfkaAQXwB7Bf4kbkhM9BX";
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
        checkpointData = {
            {
                {360, uint256S("0x00000c04c5926f539074420b40088d4b099d748d07795df891ca391799b6e54c")},
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 0000000000000037a8cd3e06cd5edbfe9dd1dbcc5dacab279376ef7cfc2b4c75
            /* nTime    */ 0,
            /* nTxCount */ 0,
            /* dTxRate  */ 0
        };
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        //consensus.BIP16Exception = uint256();
        consensus.BIP16Height = 1;
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432; // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0; // SEGWIT is always activated on regtest unless overridden
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
        consensus.nPowTargetTimespan = 6 * 60 * 60; // Syscoin: 6 hour
        consensus.nPowTargetSpacing = 1 * 60; // Syscoin: 1 minute
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.MinBIP9WarningHeight = consensus.nMinerConfirmationWindow;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        // CSV (BIP68, BIP112 and BIP113) as well as segwit (BIP141, BIP143 and
        // BIP147) are deployed together with P2SH.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");
        consensus.nAuxpowStartHeight = 0;
        consensus.nAuxpowChainId = 0x1000;
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 0;
        consensus.nSYSXAsset = 0;
        consensus.vchSYSXBurnMethodSignature = ParseHex("5f959b69");
        consensus.vchSYSXERC20Manager = ParseHex("38945d8004cf4671c45686853452A6510812117c");
        consensus.vchTokenFreezeMethod = ParseHex("aabab1db49e504b5156edf3f99042aeecb9607a08f392589571cd49743aaba8d");
        consensus.nBridgeStartBlock = 100;
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
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

        m_is_test_chain = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        // privKey: cPPpaK9LCXjGGXVJUqcrtEMVQw5tALMuN3WsVuPCWFf9tswYYDvY
        strSporkAddress = "TCSJVL68KFq9FdbfxB2KhTcWp6rHD7vePs";
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes
       /* 
        checkpointData = {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},
            }
        };*/

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "sysrt";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (gArgs.IsArgSet("-segwitheight")) {
        int64_t height = gArgs.GetArg("-segwitheight", 0);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() != 3) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end");
        }
        int64_t nStartTime, nTimeout;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld\n", vDeploymentParams[0], nStartTime, nTimeout);
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

std::unique_ptr<const CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}
void TurnOffSegwitForUnitTests ()
{
  /* TODO: It is ugly that we need a const-cast here, but this is only for
     unit testing.  Upstream avoids this by turning off segwit through
     forcing command-line args in the tests.  For that to work in our case,
     we would have to have an explicit argument for BIP16.  */
  auto* params = const_cast<CChainParams*> (globalChainParams.get ());
  params->TurnOffSegwitForUnitTests ();
}
void SetSYSXAssetForUnitTests (uint32_t asset)
{
  /* TODO: It is ugly that we need a const-cast here, but this is only for
     unit testing.  Upstream avoids this by turning off segwit through
     forcing command-line args in the tests.  For that to work in our case,
     we would have to have an explicit argument for BIP16.  */
  auto* params = const_cast<CChainParams*> (globalChainParams.get ());
  params->SetSYSXAssetForUnitTests (asset);
}
