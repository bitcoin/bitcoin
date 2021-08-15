// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <poc/poc.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint64_t nNonce, uint64_t nBaseTarget, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(2);
    txNew.vin[0].scriptSig = CScript() << static_cast<unsigned int>(0)
        << CScriptNum(static_cast<int64_t>(nNonce)) << CScriptNum(static_cast<int64_t>(0))
        << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;
    txNew.vout[1].nValue = 0;
    txNew.vout[1].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime       = nTime;
    genesis.nBaseTarget = nBaseTarget;
    genesis.nNonce      = nNonce;
    genesis.nVersion    = nVersion;
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
 * CBlock(hash=8cec494f7f02ad, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=6b80acabaf0fef, nTime=1531292789, nBaseTarget=24433591728, nNonce=0, vtx=1)
 *   CTransaction(hash=6b80acabaf0fef, ver=1, vin.size=1, vout.size=2, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=150.00000000, scriptPubKey=0x2102CD2103A86877937A05)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint64_t nNonce, uint64_t nBaseTarget, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("02cd2103a86877937a05eff85cf487424b52796542149f2888f9a17fbe6d66ce9d") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBaseTarget, nVersion, genesisReward);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.BIP34Height = 0;
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 0;
        consensus.SegwitHeight = 0;

        // Wed, 16 May 2021 16:00:00 GMT
        consensus.nBeginMiningTime = 1621180800;

        consensus.vFundAddressPool = {
            "3LX1uGfaDm6LGj6gy7aFJc7azpyzKhUaRs", // 10%, 10500000 QTC
            "3JSgHDJjzDSHr1o5Lx2b1Fe6AwfFn8LNSX", // 10%, 10500000 QTC
        };
        consensus.FundAddress = "3LX1uGfaDm6LGj6gy7aFJc7azpyzKhUaRs";
        consensus.nPowTargetSpacing = 180; // 3 minutes
        consensus.fPowNoRetargeting = false;
        consensus.nCapacityEvalWindow = 3360; // About 1 week
        consensus.nSubsidyHalvingInterval = 700000; // About 4 years. 700000*180/(365*24*3600) = 3.99543379
        consensus.fAllowMinDifficultyBlocks = false; // For test
        consensus.nRuleChangeActivationThreshold = 3192; // 95% of 3360
        consensus.nMinerConfirmationWindow = 3360; // About 7 days
        consensus.nBindPlotterCheckHeight = consensus.nCapacityEvalWindow / consensus.nCapacityEvalWindow * consensus.nCapacityEvalWindow; // 3360
        consensus.nBindPlotterCheckHeightV2 = 33600; // About active on Thu, 05 Aug 2021 18:00:00 GMT
        consensus.nPledgeRatio = 5 * COIN;
        consensus.nPledgeFullRewardRatio = 800; // 80%
        consensus.nPledgeLowRewardRatio = 50; // 5%

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000000049083a814648b529");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00000000000000000000000000000000000000000000000049083a814648b529");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf6;
        pchMessageStart[1] = 0xba;
        pchMessageStart[2] = 0xb0;
        pchMessageStart[3] = 0xd5;
        nDefaultPort = 3333;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 3;
        m_assumed_chain_state_size = 1;

        // Wed, 15 May 2021 16:00:00 GMT
        genesis = CreateGenesisBlock(consensus.nBeginMiningTime - 86400, 0, poc::INITIAL_BASE_TARGET, 2, 75 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        printf("Genesis block(%s): BlockHash=0x%s MerkleRoot=0x%s\n", strNetworkID.c_str(), consensus.hashGenesisBlock.ToString().c_str(), genesis.hashMerkleRoot.ToString().c_str()),
        assert(consensus.hashGenesisBlock == uint256S("0x90001790809e09c1b2af490ed1bdcd687fab32f28da8012ef8b5fa08fbb4a9e4"));
        assert(genesis.hashMerkleRoot == uint256S("0x96fb099b59ba1f90c6ae8e16dbefa16941a07a0785743db700d682a7d6461084"));
        assert(genesis.nTime <= consensus.nBeginMiningTime || consensus.nBeginMiningTime == 0);

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.push_back("seed1.qitchain.org");
        vSeeds.push_back("seed2.qitchain.org");
        vSeeds.push_back("seed3.qitchain.org");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "qc";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;

        checkpointData = {
            {
                {      0, consensus.hashGenesisBlock },
                // Offset +10000. Sync batch by 10000, accelerate block verify
                {   10000, uint256S("0x619b4e3860b55cbd9dcd380bb2cddc6c3405829e97e945b123db7a41dffee8b5") },
                {   20000, uint256S("0x47057bdeb24f2a814377e9c2bcf54b6fa7b372d39702e8336a1322901fd45b41") },
                {   30000, uint256S("0x107b00510f1303c8504d00fe5a324f3f1c3091a111dd791674955ce29eec6917") },
                {   33600, uint256S("0xbcf4db39c06d50f86d22d12619cd7840f6cf7bd953652045b28283fdaed3402f") }, // fork
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 915e3ef622459f8b1b04dc274e1097b31111b0c6e0a9e9cd2da60c9d692f2c93
            /* nTime    */ 1620835200,
            /* nTxCount */ 1,
            /* dTxRate  */ 1,
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.BIP34Height = 0;
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 0;
        consensus.SegwitHeight = 0;

        // Sun, 21 Mar 2021 00:00:00 GMT
        consensus.nBeginMiningTime = 1616284800;

        consensus.vFundAddressPool = {
            "2NBauTsEVHcvZerMjDfydgFkFAmwZK9QfFT", // cV7bkg1MP5iDXZ2fRQk4sE3V3pmmXd35ygm6HSZVTekQjePNfrYA
            "2N6AHuBdqpuergjXGLPVLFrdU6ybDZEBbgE", // cRni7MdQ4Rm3hHCfVaPnnZPjyyVTwxJsoDQBqx9L5QAjtAjdW5UJ
        };
        consensus.FundAddress = "2NBauTsEVHcvZerMjDfydgFkFAmwZK9QfFT";
        consensus.nPowTargetSpacing = 180;
        consensus.fPowNoRetargeting = false;
        consensus.nCapacityEvalWindow = 3360;
        consensus.nSubsidyHalvingInterval = 700000;
        consensus.fAllowMinDifficultyBlocks = false;
        consensus.nRuleChangeActivationThreshold = 3192; // 95% of 3360
        consensus.nMinerConfirmationWindow = 3360; // About 7 days
        consensus.nBindPlotterCheckHeight = consensus.nCapacityEvalWindow / consensus.nCapacityEvalWindow * consensus.nCapacityEvalWindow; // 3360
        consensus.nBindPlotterCheckHeightV2 = consensus.nBindPlotterCheckHeight;
        consensus.nPledgeRatio = 5 * COIN;
        consensus.nPledgeFullRewardRatio = 800; // 80%
        consensus.nPledgeLowRewardRatio = 50; // 5%

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0x2f;
        pchMessageStart[1] = 0x12;
        pchMessageStart[2] = 0xa0;
        pchMessageStart[3] = 0x08;
        nDefaultPort = 13333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 3;
        m_assumed_chain_state_size = 1;

        // Sun, 21 Mar 2021 00:00:00 GMT
        genesis = CreateGenesisBlock(consensus.nBeginMiningTime - 86400, 1, poc::INITIAL_BASE_TARGET, 2, 75 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        printf("Genesis block(%s): BlockHash=0x%s MerkleRoot=0x%s\n", strNetworkID.c_str(), consensus.hashGenesisBlock.ToString().c_str(), genesis.hashMerkleRoot.ToString().c_str()),
        assert(consensus.hashGenesisBlock == uint256S("0x5907f477f61c92161f6754986f1288989830524facc4e6c74ef16877f4eedd3c"));
        assert(genesis.hashMerkleRoot == uint256S("0x3bd777834b355b8e64c78b405fcd352345a7b35c0e957d5e906c1687ec309870"));
        assert(genesis.nTime <= consensus.nBeginMiningTime || consensus.nBeginMiningTime == 0);

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.push_back("testnet-seed1.qitchain.org");
        vSeeds.push_back("testnet-seed2.qitchain.org");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tq";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;


        checkpointData = {
            {
               {      0, consensus.hashGenesisBlock },
                // // Offset +2000. Sync batch by 2000
                // {   8600, uint256S("0x85328fd04bf8ece91dbb0e5d494059517a579c09e1c00cb1699aa832de42f825") },
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 87ea715185228eaaefada076b0550893e36c3a35e716e33949566ae00d703a3b
            /* nTime    */ 1587626066,
            /* nTxCount */ 229918,
            /* dTxRate  */ 0.005529,
        };
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID = "regtest";
        consensus.BIP34Height = 0;
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 0;
        consensus.SegwitHeight = 0;

        consensus.nBeginMiningTime = 1616198400;

        consensus.vFundAddressPool = {
            "2MsRETb2aCZDJR2QB8FBjs948YnS1XX4pq4", // cNL8aYs9KJoHKZyYh5UoZZYN8nZbgJtT5D3e67gw3zHEeiRmbJqb
            "2MsRETb2aCZDJR2QB8FBjs948YnS1XX4pq4", // cNL8aYs9KJoHKZyYh5UoZZYN8nZbgJtT5D3e67gw3zHEeiRmbJqb
        };
        consensus.FundAddress = "2MsRETb2aCZDJR2QB8FBjs948YnS1XX4pq4";
        consensus.nPowTargetSpacing = 180;
        consensus.fPowNoRetargeting = true;
        consensus.nCapacityEvalWindow = 1680;
        consensus.nSubsidyHalvingInterval = 350000;
        consensus.fAllowMinDifficultyBlocks = true;
        consensus.nRuleChangeActivationThreshold = 1596; // 95% for testchains
        consensus.nMinerConfirmationWindow = 1680;
        consensus.nBindPlotterCheckHeight = consensus.nCapacityEvalWindow / consensus.nCapacityEvalWindow * consensus.nCapacityEvalWindow;
        consensus.nBindPlotterCheckHeightV2 = consensus.nBindPlotterCheckHeight;
        consensus.nPledgeRatio = 5 * COIN;
        consensus.nPledgeFullRewardRatio = 800; // 80%
        consensus.nPledgeLowRewardRatio = 50; // 5%

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xf7;
        pchMessageStart[1] = 0xbb;
        pchMessageStart[2] = 0xb1;
        pchMessageStart[3] = 0xd6;
        nDefaultPort = 13344;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateGenesisBlock(consensus.nBeginMiningTime - 86400, 2, poc::INITIAL_BASE_TARGET, 2, 75 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        printf("Genesis block(%s): BlockHash=0x%s MerkleRoot=0x%s\n", strNetworkID.c_str(), consensus.hashGenesisBlock.ToString().c_str(), genesis.hashMerkleRoot.ToString().c_str()),
        assert(consensus.hashGenesisBlock == uint256S("0x6afb318a0bd35f53fea1704762bc4d9cbe15da1ec80667ae5bec4033b6ebd21e"));
        assert(genesis.hashMerkleRoot == uint256S("0xc3f93c9ecea48b58b08211398ba8a15d8147fbbefb1a5fb3be4607d6a0572dfb"));
        assert(genesis.nTime <= consensus.nBeginMiningTime || consensus.nBeginMiningTime == 0);

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;

        checkpointData = {
            {
                {      0, consensus.hashGenesisBlock },
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

        bech32_hrp = "qcrt";
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
