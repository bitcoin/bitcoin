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

#include "bootstraps.h"
#include <veriblock/blockchain/alt_chain_params.hpp>

#define VBK_GAMMA  0xb1
#define VBK_1  0xc0
#define VBK_VERSION (VBK_GAMMA + 0x1)

/**
 * Main network
 */
class CMainParams : public CChainParams
{
public:
    CMainParams()
    {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22");
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = 1;  // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 1;  // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.CSVHeight = 1;    // 000000000000000004a1b34462cb8aeebd5799177f7a29cf28f2d1961716b5b5
        consensus.SegwitHeight = 1; // 0000000000000000001c8018d9cb3b742ef25114f27563e3fc4a1902167f9893
        consensus.MinBIP9WarningHeight = consensus.SegwitHeight + consensus.nMinerConfirmationWindow;
        consensus.powLimit = uint256S("0000007fffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016;       // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 2574873576;   // some distant future

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000008ea3cf107ae0dec57f03fe8");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00000000000000000005f8920febd3925f8272a6a71237563d78c2edfdd09ddf"); // 597379

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 1;
        pchMessageStart[1] = 1;
        pchMessageStart[2] = 1;
        pchMessageStart[3] = 1 + VBK_VERSION;
        nDefaultPort = 8333;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 280;
        m_assumed_chain_state_size = 4;

        // same as test
        // CBlock(hash=00000006cf4c4e6177695242a1347023b7b1bdef8119237a11511b9e490bf8d9, ver=0x00000001, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=06c6c7131bc50a1fcab02107d0f24bcfe22c1808080074dcedf5fea412a0fe1c, nTime=1340, nBits=1d0fffff, nNonce=3768745, vtx=1)
        //   CTransaction(hash=e494ea0589, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //     CTxIn(COutPoint(0000000000, 4294967295), coinbase 04ffff0f1d01040956657269426c6f636b)
        //     CScriptWitness()
        //     CTxOut(nValue=50.00000000, scriptPubKey=41047c62bbf7f5aa4dd5c16bad99ac)
        genesis = VeriBlock::CreateGenesisBlock(
            1340, 3768745, 0x1d0fffff, 1, 50 * COIN,
            "047c62bbf7f5aa4dd5c16bad99ac621b857fac4e93de86e45f5ada73404eeb44dedcf377b03c14a24e9d51605d9dd2d8ddaef58760d9c4bb82d9c8f06d96e79488",
            "VeriBlock");
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("00000006cf4c4e6177695242a1347023b7b1bdef8119237a11511b9e490bf8d9"));
        assert(genesis.hashMerkleRoot == uint256S("06c6c7131bc50a1fcab02107d0f24bcfe22c1808080074dcedf5fea412a0fe1c"));

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
            // Data from RPC: getchaintxstats 4096 00000000000000000005f8920febd3925f8272a6a71237563d78c2edfdd09ddf
            /* nTime    */ 1569926786,
            /* nTxCount */ 460596047,
            /* dTxRate  */ 3.77848885073875,
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams
{
public:
    CTestNetParams()
    {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Exception = uint256S("0x00000000dd30457c001f4095d208cc1296b0eed002427aa599874af7a432b105");
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.MinBIP9WarningHeight = consensus.SegwitHeight + consensus.nMinerConfirmationWindow;
        consensus.powLimit = uint256S("0000007fffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016;       // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;                                   // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0000000000000000000000000000000000000000000000000000000010000100");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00000000000000b7ab6ce61eb6d571003fbe5fe892da4c9b740c49a07542462d"); // 1580000

        pchMessageStart[0] = 2;
        pchMessageStart[1] = 2;
        pchMessageStart[2] = 2;
        pchMessageStart[3] = 2 + VBK_VERSION;
        nDefaultPort = 18333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 30;
        m_assumed_chain_state_size = 2;

        // CBlock(hash=00000006cf4c4e6177695242a1347023b7b1bdef8119237a11511b9e490bf8d9, ver=0x00000001, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=06c6c7131bc50a1fcab02107d0f24bcfe22c1808080074dcedf5fea412a0fe1c, nTime=1340, nBits=1d0fffff, nNonce=3768745, vtx=1)
        //   CTransaction(hash=e494ea0589, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //     CTxIn(COutPoint(0000000000, 4294967295), coinbase 04ffff0f1d01040956657269426c6f636b)
        //     CScriptWitness()
        //     CTxOut(nValue=50.00000000, scriptPubKey=41047c62bbf7f5aa4dd5c16bad99ac)
        genesis = VeriBlock::CreateGenesisBlock(
            1340, 3768745, 0x1d0fffff, 1, 50 * COIN,
            "047c62bbf7f5aa4dd5c16bad99ac621b857fac4e93de86e45f5ada73404eeb44dedcf377b03c14a24e9d51605d9dd2d8ddaef58760d9c4bb82d9c8f06d96e79488",
            "VeriBlock");
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("00000006cf4c4e6177695242a1347023b7b1bdef8119237a11511b9e490bf8d9"));
        assert(genesis.hashMerkleRoot == uint256S("06c6c7131bc50a1fcab02107d0f24bcfe22c1808080074dcedf5fea412a0fe1c"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 196);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));
        vSeeds.clear(); // Clear DNS seeds

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;


        checkpointData = {
            {}};

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 00000000000000b7ab6ce61eb6d571003fbe5fe892da4c9b740c49a07542462d
            /* nTime    */ 0,
            /* nTxCount */ 0,
            /* dTxRate  */ 0,
        };
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams
{
public:
    explicit CRegTestParams(const ArgsManager& args)
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

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 3;
        pchMessageStart[1] = 3;
        pchMessageStart[2] = 3;
        pchMessageStart[3] = 3 + VBK_VERSION;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = VeriBlock::CreateGenesisBlock(
            1337, 0, 0x207fffff, 1, 50 * COIN,
            "047c62bbf7f5aa4dd5c16bad99ac621b857fac4e93de86e45f5ada73404eeb44dedcf377b03c14a24e9d51605d9dd2d8ddaef58760d9c4bb82d9c8f06d96e79488",
            "VeriBlock");
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("55c49883fa107754db67455f30291935e4a4f6d8960b897098f027d5e62ce95c"));
        assert(genesis.hashMerkleRoot == uint256S("41159e19a678894968919c2c4250302d277074de5fda813002e43fe502bf6bed"));

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
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
    assert(globalChainParams != nullptr);
}
