// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <vbk/genesis_common.hpp>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <veriblock/pop.hpp>

static CBlock CreateGenesisBlockDefault(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
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
static CBlock CreateGenesisBlockDefault(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlockDefault(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
CMainParams::CMainParams()
{
    strNetworkID = CBaseChainParams::MAIN;
    consensus.nSubsidyHalvingInterval = 1050000;
    consensus.BIP16Exception = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
    consensus.BIP34Height = 1;
    consensus.BIP34Hash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
    consensus.BIP65Height = 1;          // 0x0000000000000000000000000000000000000000000000000000000000000000
    consensus.BIP66Height = 1;          // 0x0000000000000000000000000000000000000000000000000000000000000000
    consensus.CSVHeight = 1;            // 0x0000000000000000000000000000000000000000000000000000000000000000
    consensus.SegwitHeight = 1;         // 0x0000000000000000000000000000000000000000000000000000000000000000
    consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
    consensus.nPowTargetSpacing = 2 * 60;
    consensus.fPowAllowMinDifficultyBlocks = false;
    consensus.fPowNoRetargeting = false;
    consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
    consensus.nMinerConfirmationWindow = 2016;       // nPowTargetTimespan / nPowTargetSpacing
    consensus.MinBIP9WarningHeight = consensus.SegwitHeight + consensus.nMinerConfirmationWindow; // segwit activation height + miner confirmation window
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;   // December 31, 2008

    consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000aaaa");
    consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

    /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
    pchMessageStart[0] = 'v';
    pchMessageStart[1] = 'B';
    pchMessageStart[2] = 'T';
    pchMessageStart[3] = 'C';
    nDefaultPort = 8033;
    nPruneAfterHeight = 100000;
    m_assumed_blockchain_size = 1;
    m_assumed_chain_state_size = 1;

    genesis = VeriBlock::CreateGenesisBlock(1631200000, 862267501, 0x1d00ffff, 1, "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4104ffff001d01043956657269426c6f636b20426974636f696e205265666572656e636520496d706c656d656e746174696f6e2c20536570742031332c2032303231ffffffff010065cd1d000000001600143dd5f2f667315cc98e669deb88d3dfe831aa2cea00000000");
    consensus.hashGenesisBlock = genesis.GetHash();
    assert(consensus.hashGenesisBlock == uint256S("0x00000000ce2a3361df78fb9bb59b089ec637c8b29175725f4d9d643cad74f1e5"));
    assert(genesis.hashMerkleRoot == uint256S("0x5c24f8481961119a2d73a84aef8fc32e839f35cb8407455d37a8efc37e9e915a"));

    // VeriBlock
    consensus.VeriBlockPopSecurityHeight = 200;
    consensus.ZawyLWMAHeight = 100;
    consensus.nZawyLwmaAveragingWindow = 45;
    assert(consensus.ZawyLWMAHeight > consensus.nZawyLwmaAveragingWindow);
    consensus.nZawyLwmaAdjustedWeight = 13772;
    consensus.nZawyLwmaMinDenominator = 10;
    consensus.bZawyLwmaSolvetimeLimitation = true;

    base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 0);
    base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 5);
    base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 128);
    base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
    base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

    bech32_hrp = "bc";

    vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));
    vSeeds.clear(); // Clear DNS seeds

    fDefaultConsistencyChecks = false;
    fRequireStandard = true;
    m_is_test_chain = false;

    checkpointData = {
        {}};

    chainTxData = ChainTxData{
        /**
         * vbitcoin-cli getchaintxstats                                                                                                                                                                                                                                                                                                        bogdan@big
         * {
         *    "time": 1630388498,
         *    "txcount": 51731,
         *    "window_final_block_hash": "0000000f6c2dd40ebc89b81848934657e1acb9e74441ae6fc337b2da990573c0",
         *    "window_final_block_height": 51627,
         *    "window_block_count": 4320,
         *    "window_tx_count": 4324,
         *    "window_interval": 964989,
         *    "txrate": 0.004480880092933702
         *  }
         */
        /* nTime    */ 1630388498,
        /* nTxCount */ 1,
        /* dTxRate  */ 0.004480880092933702,
    };
 }

/**
 * Testnet (v3)
 */
 CTestNetParams::CTestNetParams()
 {
     strNetworkID = CBaseChainParams::TESTNET;
     consensus.nSubsidyHalvingInterval = 1050000;
     consensus.BIP16Exception = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
     consensus.BIP34Height = 1;
     consensus.BIP34Hash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
     consensus.BIP65Height = 1;          // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
     consensus.BIP66Height = 1;          // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
     consensus.CSVHeight = 1;            // 00000000025e930139bac5c6c31a403776da130831ab85be56578f3fa75369bb
     consensus.SegwitHeight = 1;         // 00000000002b980fcd729daaa248fd9316a5200e9b367f4ff2c42453e84201ca
     consensus.powLimit = uint256S("000007ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
     consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
     consensus.nPowTargetSpacing = 2 * 60;
     consensus.fPowAllowMinDifficultyBlocks = true;
     consensus.fPowNoRetargeting = false;
     consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
     consensus.nMinerConfirmationWindow = 2016;       // nPowTargetTimespan / nPowTargetSpacing
     consensus.MinBIP9WarningHeight = consensus.SegwitHeight + consensus.nMinerConfirmationWindow;
     consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
     consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
     consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999;   // December 31, 2008

     consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000aaaa");
     consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

     pchMessageStart[0] = 2;
     pchMessageStart[1] = 2;
     pchMessageStart[2] = 2;
     pchMessageStart[3] = 2;
     nDefaultPort = 18333;
     nPruneAfterHeight = 1000;
     m_assumed_blockchain_size = 1;
     m_assumed_chain_state_size = 1;

     genesis = VeriBlock::CreateGenesisBlock(1631200000, 412533384, 0x1d07ffff, 1, "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4104ffff071d01043956657269426c6f636b20426974636f696e205265666572656e636520496d706c656d656e746174696f6e2c20536570742031332c2032303231ffffffff010065cd1d000000001600143dd5f2f667315cc98e669deb88d3dfe831aa2cea00000000");
     consensus.hashGenesisBlock = genesis.GetHash();
     assert(consensus.hashGenesisBlock == uint256S("0x00000000152547e03b86a4d667ded82a91fe4fe02245affae26ad8b20c950a60"));
     assert(genesis.hashMerkleRoot == uint256S("0x172a104def609f48532c3e4afbd9d9f530b7c359d031ac47d63191cb11ae56fe"));

     vFixedSeeds.clear();
     vSeeds.clear();

     // VeriBlock
     consensus.VeriBlockPopSecurityHeight = 1;
     consensus.ZawyLWMAHeight = 100;
     consensus.nZawyLwmaAveragingWindow = 45;
     assert(consensus.ZawyLWMAHeight > consensus.nZawyLwmaAveragingWindow);
     consensus.nZawyLwmaAdjustedWeight = 13772;
     consensus.nZawyLwmaMinDenominator = 10;
     consensus.bZawyLwmaSolvetimeLimitation = false;

     base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
     base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
     base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
     base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
     base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

     bech32_hrp = "tb";

     vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

     fDefaultConsistencyChecks = false;
     fRequireStandard = false;
     m_is_test_chain = true;

     checkpointData = {
         {}};

     chainTxData = ChainTxData{
         // 00000001da906d7e587fe344ca51b097d621f88aaad0f93389d4df558492e86e
         /* nTime    */ 1630388498,
         /* nTxCount */ 1,
         /* dTxRate  */ 0.004480880092933702,
     };
 }

/**
 * Regression test
 */
CRegTestParams::CRegTestParams(const ArgsManager& args)
{
    strNetworkID = CBaseChainParams::REGTEST;
    consensus.nSubsidyHalvingInterval = 150;
    consensus.BIP16Exception = uint256();
    consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
    consensus.BIP34Hash = uint256();
    consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
    consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
    consensus.CSVHeight = 432;    // CSV activated on regtest (Used in rpc activation tests)
    consensus.SegwitHeight = 0;   // SEGWIT is always activated on regtest unless overridden
    consensus.MinBIP9WarningHeight = 0;
    consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
    consensus.nPowTargetSpacing = 10 * 60;
    consensus.fPowAllowMinDifficultyBlocks = true;
    consensus.fPowNoRetargeting = true;
    consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
    consensus.nMinerConfirmationWindow = 144;       // Faster than normal for regtest (144 instead of 2016)
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

    consensus.nMinimumChainWork = uint256{};
    consensus.defaultAssumeValid = uint256{};

    pchMessageStart[0] = 3;
    pchMessageStart[1] = 3;
    pchMessageStart[2] = 3;
    pchMessageStart[3] = 3;
    nDefaultPort = 18444;
    nPruneAfterHeight = 1000;
    m_assumed_blockchain_size = 0;
    m_assumed_chain_state_size = 0;

    UpdateActivationParametersFromArgs(args);

    genesis = CreateGenesisBlockDefault(1296688602, 2, 0x207fffff, 1, 50 * COIN);
    consensus.hashGenesisBlock = genesis.GetHash();
    assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
    assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

    vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
    vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

    fDefaultConsistencyChecks = true;
    fRequireStandard = true;
    m_is_test_chain = true;

    checkpointData = {
        {}};

    chainTxData = ChainTxData{
        0,
        0,
        0};

    // VeriBlock
    consensus.VeriBlockPopSecurityHeight = 200;
    // do not activate LWMA on regtest
    {
        consensus.ZawyLWMAHeight = std::numeric_limits<int>::max();
        consensus.nZawyLwmaAveragingWindow = 45;
        assert(consensus.ZawyLWMAHeight > consensus.nZawyLwmaAveragingWindow);
        consensus.nZawyLwmaAdjustedWeight = 13772;
        consensus.nZawyLwmaMinDenominator = 10;
        consensus.bZawyLwmaSolvetimeLimitation = true;
    }

    base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
    base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
    base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
    base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
    base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

    bech32_hrp = "bcrt";
}

/**
 * Allows modifying the Version Bits regtest parameters.
 */
void CRegTestParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (gArgs.IsArgSet("-segwitheight")) {
        int64_t height = gArgs.GetArg("-segwitheight", consensus.SegwitHeight);
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
        for (int j = 0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
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

/**
 * Deterministic regression test
 */
CDetRegTestParams::CDetRegTestParams(const ArgsManager& args)
{
    strNetworkID = CBaseChainParams::DETREGTEST;
    consensus.nSubsidyHalvingInterval = 150;
    consensus.BIP16Exception = uint256();
    consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
    consensus.BIP34Hash = uint256();
    consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
    consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
    consensus.CSVHeight = 432;    // CSV activated on regtest (Used in rpc activation tests)
    consensus.SegwitHeight = 0;   // SEGWIT is always activated on regtest unless overridden
    consensus.MinBIP9WarningHeight = 0;
    consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
    consensus.nPowTargetSpacing = 10 * 60;
    consensus.fPowAllowMinDifficultyBlocks = true;
    consensus.fPowNoRetargeting = true;
    consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
    consensus.nMinerConfirmationWindow = 144;       // Faster than normal for regtest (144 instead of 2016)
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
    consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

    consensus.nMinimumChainWork = uint256{};
    consensus.defaultAssumeValid = uint256{};

    pchMessageStart[0] = 3;
    pchMessageStart[1] = 3;
    pchMessageStart[2] = 3;
    pchMessageStart[3] = 3;
    nDefaultPort = 18555;
    nPruneAfterHeight = 1000;
    m_assumed_blockchain_size = 0;
    m_assumed_chain_state_size = 0;

    UpdateActivationParametersFromArgs(args);

    genesis = CreateGenesisBlockDefault(1296688602, 2, 0x207fffff, 1, 50 * COIN);
    consensus.hashGenesisBlock = genesis.GetHash();
    assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
    assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

    vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
    vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

    fDefaultConsistencyChecks = true;
    fRequireStandard = true;
    m_is_test_chain = true;

    checkpointData = {
            {}};

    chainTxData = ChainTxData{
            0,
            0,
            0};

    // VeriBlock
    consensus.VeriBlockPopSecurityHeight = 1200;
    // do not activate LWMA on regtest
    {
        consensus.ZawyLWMAHeight = std::numeric_limits<int>::max();
        consensus.nZawyLwmaAveragingWindow = 45;
        assert(consensus.ZawyLWMAHeight > consensus.nZawyLwmaAveragingWindow);
        consensus.nZawyLwmaAdjustedWeight = 13772;
        consensus.nZawyLwmaMinDenominator = 10;
        consensus.bZawyLwmaSolvetimeLimitation = true;
    }

    base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
    base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
    base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
    base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
    base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

    bech32_hrp = "bcrt";
}

void CDetRegTestParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

void CDetRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (gArgs.IsArgSet("-segwitheight")) {
        int64_t height = gArgs.GetArg("-segwitheight", consensus.SegwitHeight);
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
        for (int j = 0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
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

const CChainParams& Params()
{
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
    else if (chain == CBaseChainParams::DETREGTEST)
        return std::unique_ptr<CChainParams>(new CDetRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
    assert(globalChainParams != nullptr);
}
