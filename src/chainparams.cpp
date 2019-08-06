// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcointalkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <consensus/consensus.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>
#include <arith_uint256.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/thread.hpp>

#ifdef ENABLE_MOMENTUM_HASH_ALGO
	static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBirthdayA, uint32_t nBirthdayB, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
	{
		CMutableTransaction txNew;
		txNew.nVersion = 1;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = CScript() << 0 << OP_0;
		txNew.vout[0].nValue = genesisReward;
		txNew.vout[0].scriptPubKey = genesisOutputScript;

		CBlock genesis;
		genesis.nTime    = nTime;
		genesis.nBits    = nBits;
		genesis.nNonce   = nNonce;
		genesis.nBirthdayA   = nBirthdayA;
		genesis.nBirthdayB   = nBirthdayB;
		genesis.nVersion = nVersion;
		genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
		genesis.hashPrevBlock.SetNull();
		genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
		return genesis;
	}

#else
	static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
	{
		CMutableTransaction txNew;
		txNew.nVersion = 1;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = CScript() << 0 << OP_0;
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
#endif



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

#ifdef ENABLE_MOMENTUM_HASH_ALGO
	static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBirthdayA, uint32_t nBirthdayB, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
	{
		const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
		const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
		return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBirthdayA, nBirthdayB, nBits, nVersion, genesisReward);
	}
#else
	static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
	{
		const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
		const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
		return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce,nBits, nVersion, genesisReward);
	}
#endif

void static MineNewGenesisBlock(const Consensus::Params& consensus,CBlock &genesis)
{
    //fPrintToConsole = true;
    LogPrintf("Searching for genesis block...\n");
    std::cout << "Searching for genesis block...\n";
    arith_uint256 hashTarget = UintToArith256(consensus.powLimit);

    while(true) {
#ifdef ENABLE_MOMENTUM_HASH_ALGO
        arith_uint256 thash = UintToArith256(genesis.CalculateBestBirthdayHash());
#else
        arith_uint256 thash = UintToArith256(genesis.GetHash());
#endif

		LogPrintf("test Hash %s\n", thash.ToString().c_str());
        std::cout << "testHash = " << thash.ToString().c_str() << "\n";
		LogPrintf("Hash Target %s\n", hashTarget.ToString().c_str());
        std::cout << "Hash Target = " << hashTarget.ToString().c_str() << "\n";

      if (thash <= hashTarget)
            break;
        if ((genesis.nNonce & 0xFFF) == 0)
            LogPrintf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), hashTarget.ToString().c_str());

        ++genesis.nNonce;
        if (genesis.nNonce == 0) {
            LogPrintf("NONCE WRAPPED, incrementing time\n");
            ++genesis.nTime;
        }
    }
    std::cout << "genesis.nTime = " << genesis.nTime << "\n";
    std::cout << "genesis.nNonce = " << genesis.nNonce<< "\n";
#ifdef ENABLE_MOMENTUM_HASH_ALGO
    std::cout << "genesis.nBirthdayA = " << genesis.nBirthdayA<< "\n";
    std::cout << "genesis.nBirthdayB = " << genesis.nBirthdayB<< "\n";
#endif
    std::cout << "genesis.nBits = " << genesis.nBits<< "\n";
    std::cout << "genesis.GetHash = " << genesis.GetHash().ToString().c_str()<< "\n";
    std::cout << "genesis.hashMerkleRoot = " << genesis.hashMerkleRoot.ToString().c_str()<< "\n";

    LogPrintf("genesis.nTime = %u \n",  genesis.nTime);
    LogPrintf("genesis.nNonce = %u \n",  genesis.nNonce);
#ifdef ENABLE_MOMENTUM_HASH_ALGO
	LogPrintf("genesis.nBirthdayA: %d\n", genesis.nBirthdayA);
	LogPrintf("genesis.nBirthdayB: %d\n", genesis.nBirthdayB);
#endif
    LogPrintf("genesis.nBits = %u \n",  genesis.nBits);
    LogPrintf("genesis.GetHash = %s\n",  genesis.GetHash().ToString().c_str());
    LogPrintf("genesis.hashMerkleRoot = %s\n",  genesis.hashMerkleRoot.ToString().c_str());

    exit(1);
}
/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x00");
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x00");
        consensus.BIP65Height = 0; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 0; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
#ifdef ENABLE_MOMENTUM_HASH_ALGO
        consensus.powLimit = uint256S("1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
#else
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
#endif
#ifdef ENABLE_PROOF_OF_STAKE
        consensus.posLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.fPoSNoRetargeting = false;
        consensus.nStakeTimestampMask = 0xf;
        consensus.nLastPOWBlock = 10000;
        consensus.nStakeMinAge = 4 * 60 * 60;
#endif
        consensus.nPowTargetTimespan = 60; // two weeks
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 100; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000000000201a8ab833d");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x8620e23b97f0cda9301336ead918317dc73ee5378ac724effa15101da0264416"); //563378

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf7;
        pchMessageStart[1] = 0xba;
        pchMessageStart[2] = 0xd4;
        pchMessageStart[3] = 0xa8;
        nDefaultPort = 8372;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;
#ifdef ENABLE_MOMENTUM_HASH_ALGO
        genesis = CreateGenesisBlock(1564536165, 1, 7752872, 23894037, 0x201fffff, 1, 5 * COIN);
#else
        genesis = CreateGenesisBlock(1562734642, 221227,0x1e0fffff, 1, 5 * COIN);
#endif
        consensus.hashGenesisBlock = genesis.GetHash();
        
	    //static boost::thread_group* minerThreads = NULL; 
        //minerThreads = new boost::thread_group();
        //for (int i = 0; i < 4; i++)    
        //    minerThreads->create_thread(boost::bind(&MineNewGenesisBlock,consensus,genesis));
        //MilliSleep(20000000);
        //MineNewGenesisBlock(consensus,genesis);
        assert(consensus.hashGenesisBlock == uint256S("0x042715fc38fd772456cb34ecf601ef3f0bdc9a8656050be9a0082b7c97997170"));
        assert(genesis.hashMerkleRoot == uint256S("0x202b7ffe141c1d8b99e1b76845a24d1933be562c32e56663768cf4c161f772e4"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("192.3.83.68"); 
        vSeeds.emplace_back("193.29.56.90"); 
        vSeeds.emplace_back("192.3.83.8"); 
        vSeeds.emplace_back("192.3.83.87"); 
        vSeeds.emplace_back("193.29.56.179");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,65);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "tc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = {
            {
                { 0, uint256S("0x042715fc38fd772456cb34ecf601ef3f0bdc9a8656050be9a0082b7c97997170")},
                { 4, uint256S("0x0116724b234fa66129dd910ca70eda475141b8a889dd22c408ce681bf260e581")},
                { 600, uint256S("0x0000180a86e5f4e47a488496de31c0f070a1106f3df5fe44f62a89b89304e8eb")},
                { 3440, uint256S("0x182c22ed21cea5161aa091003c5616dd1e4d995281eeeb961412230c986c6f70")},
                { 14358, uint256S("0x23599282d5f953a73fac7d1710b055078f8397f2da3dbf767195a0db74160728")},
                { 17266, uint256S("0xbe705a2a1da8e801acde811050e6fc2500c496374e960a375fb17b7af030205c")},
                { 19184, uint256S("0x8620e23b97f0cda9301336ead918317dc73ee5378ac724effa15101da0264416")},
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 23599282d5f953a73fac7d1710b055078f8397f2da3dbf767195a0db74160728
            /* nTime    */ 1565128848,
            /* nTxCount */ 38154,
            /* dTxRate  */ 0.07267276855399461
        };

        /* disable fallback fee on mainnet */
        m_fallback_fee_enabled = false;
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x00000000dd30457c001f4095d208cc1296b0eed002427aa599874af7a432b105");
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.BIP65Height = 581885; // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
        consensus.BIP66Height = 330776; // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
#ifdef ENABLE_MOMENTUM_HASH_ALGO
        consensus.powLimit = uint256S("1fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
#else
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
#endif
        consensus.nPowTargetTimespan = 60; // two weeks
        consensus.nPowTargetSpacing = 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1456790400; // March 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1462060800; // May 1st 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1493596800; // May 1st 2017

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000007dbe94253893cbd463");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000000000000037a8cd3e06cd5edbfe9dd1dbcc5dacab279376ef7cfc2b4c75"); //1354312

        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        nDefaultPort = 18333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

#ifdef ENABLE_MOMENTUM_HASH_ALGO
        genesis = CreateGenesisBlock(1562994039, 6, 9657011, 12474766, 0x201fffff, 1, 5 * COIN);
#else
        genesis = CreateGenesisBlock(1562734642, 221227,0x1e0fffff, 1, 5 * COIN);
#endif
        consensus.hashGenesisBlock = genesis.GetHash();
        //MineNewGenesisBlock(consensus,genesis);
        assert(consensus.hashGenesisBlock == uint256S("0x13bc3f42175c6b3b744c28ec7e37ac59d822483b5eb21a5c5259154746378266"));
        assert(genesis.hashMerkleRoot == uint256S("0x202b7ffe141c1d8b99e1b76845a24d1933be562c32e56663768cf4c161f772e4"));
        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.bitcointalkcoin.jonasschnelli.ch");
        vSeeds.emplace_back("seed.tbtc.petertodd.org");
        vSeeds.emplace_back("seed.testnet.bitcointalkcoin.sprovoost.nl");
        vSeeds.emplace_back("testnet-seed.bluematt.me"); // Just a static list of stable node(s), only supports x9

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = {
            {
                {0, uint256S("0x13bc3f42175c6b3b744c28ec7e37ac59d822483b5eb21a5c5259154746378266")},
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 0000000000000037a8cd3e06cd5edbfe9dd1dbcc5dacab279376ef7cfc2b4c75
            /* nTime    */ 1531929919,
            /* nTxCount */ 19438708,
            /* dTxRate  */ 0.626
        };

        /* enable fallback fee on testnet */
        m_fallback_fee_enabled = true;
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Exception = uint256();
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateVersionBitsParametersFromArgs(args);

#ifdef ENABLE_MOMENTUM_HASH_ALGO
        genesis = CreateGenesisBlock(1561468723, 260848, 0, 0 ,0x201fffff, 1, 5 * COIN);
#else
        genesis = CreateGenesisBlock(1561928393, 221227,0x1e0fffff, 1, 5 * COIN);
#endif
        consensus.hashGenesisBlock = genesis.GetHash();
       // assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
      //  assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = {
            {
                {0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")},
            }
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

        /* enable fallback fee on regtest */
        m_fallback_fee_enabled = true;
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateVersionBitsParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateVersionBitsParametersFromArgs(const ArgsManager& args)
{
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
