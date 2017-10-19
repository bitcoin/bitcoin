// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017 IoP Ventures LLC
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include "chainparamsseeds.h"

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
    const char* pszTimestamp = "La Nacion May 16th 2016 - Sarmiento cerca del descenso";
    const CScript genesisOutputScript = CScript() << ParseHex("04ce49f9cdc8d23176c818fd7e27e7b614d128a47acfdad0e4542300e7efbd8879f1337af3188c0dcb0747fdf26d0cb3b0fca0f4e5d7aec53c43f4a933f570ae86") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        /* **** IOP CHANGE //
        IoP Chain uses the coinbase content to store the miner signature, so we can not enforce BIP34 in its current form.
        So deactivate the check for BIP34 completely
        // **** IOP CHANGE */
        // consensus.BIP34Height = -1; // never enforce BIP34
        // consensus.BIP34Hash = uint256();
        // consensus.BIP65Height = 0; // always enforce BIP 65 and BIP 66
        // consensus.BIP66Height = 0; 
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0; // undefined
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 0; // undefined

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000e20799b006d2c");
        
        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000000002733bb68bb8e61e375f18cb071500d9be729051b0203a4371b3962a"); //50400


        // Miner White list params
        consensus.minerWhiteListActivationHeight = 110; //block height that activates the white list.
        consensus.minerWhiteListAdminPubKey.insert("03902b311c298f7d32eb2ccb71abde7afd39745f505e6e677cabc3964eea7960dc"); //pub key required to sign add / remove transactions
        consensus.minerWhiteListAdminPubKey.insert("02fbb99b84746c13489fa81ff8540fa76d496e26d34bebd1b0e81a0997e97ea952"); //pub key required to sign add / remove transactions
        consensus.minerWhiteListAdminAddress.insert("pUSydiLr9kFjtL7VbtfMYXMz7GLV413coQ"); //default miner address
        consensus.minerWhiteListAdminAddress.insert("pN4SiSeN1btEAoVCnVToKf9RSb5gKZgpU4"); //default miner address
        consensus.minerCapSystemChangeHeight = 40320;
        // Voting System Params
        // consensus.ccBlockStartAdditionalHeight = 1000;
        consensus.ccLastCCBlockHeight = 47520;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xfd;
        pchMessageStart[1] = 0xb0;
        pchMessageStart[2] = 0xbb;
        pchMessageStart[3] = 0xd3;
        nDefaultPort = 4877;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1463452181, 1875087468, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00000000bf5f2ee556cb9be8be64e0776af14933438dbb1af72c41bfb6c82db3"));
        assert(genesis.hashMerkleRoot == uint256S("0x951bc46d2da95047fb4a2c0f9b7d6e45591c3ffb49ab2fdffb8e96ef2b8f2be1"));

        vSeeds.push_back(CDNSSeedData("Mainnet Explorer", "mainnet.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 1", "main1.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 2", "main2.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 3", "main3.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 4", "main4.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 5", "main5.iop.cash"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,117);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,174);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,49);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x27, 0x80, 0x91, 0x5F};
        base58Prefixes[EXT_SECRET_KEY] = {0xAE, 0x34, 0x16, 0xF6};

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;

        checkpointData = (CCheckpointData) {
            {
                {     0, uint256S("0x00000000bf5f2ee556cb9be8be64e0776af14933438dbb1af72c41bfb6c82db3")},
                { 20000, uint256S("0x000000000205ce279aed9220fbac67f6f7a863f898f98ef0cdeae863e2d19bc1")},
                { 47654, uint256S("0x00000000118494c6822e81f33ef08f074991a76fbae32425482a6bfecc26ec0a")},
            }
        };

        chainTxData = ChainTxData{
            // Data as of block 000000000000000000d97e53664d17967bd4ee50b23abb92e54a34eb222d15ae (height 478913).
            1502262660, // * UNIX timestamp of last known number of transactions
            51903,      // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.01        // * estimated number of transactions per second after that timestamp
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
        consensus.nSubsidyHalvingInterval = 210000;
        /* **** IOP CHANGE //
        IoP Chain uses the coinbase content to store the miner signature, so we can not enforce BIP34 in its current form.
        So deactivate the check for BIP34 completely
        // **** IOP CHANGE */
        // consensus.BIP34Height = -1; // never enforce BIP34
        // consensus.BIP34Hash = uint256();
        // consensus.BIP65Height = 0; // always enforce BIP 65 and BIP 66
        // consensus.BIP66Height = 0; 
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
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
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0; // undefined
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 0; // undefined

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00"); 


        // Miner White list params
        consensus.minerWhiteListActivationHeight = 110; //block height that activates the white list.
        consensus.minerWhiteListAdminPubKey.insert("03f331bdfe024cf106fa1dcedb8b78e084480fa665d91c50b61822d7830c9ea840"); //pub key required to sign add / remove transactions
        consensus.minerWhiteListAdminAddress.insert("uh2SKjE6R1uw3b5smZ8i1G8rDoQv458Lsj"); //default miner address
        consensus.minerCapSystemChangeHeight=7800;
        // Voting System Params
        // consensus.ccBlockStartAdditionalHeight = 10;
        consensus.ccLastCCBlockHeight = 9000;

        pchMessageStart[0] = 0xb1;
        pchMessageStart[1] = 0xfc;
        pchMessageStart[2] = 0x50;
        pchMessageStart[3] = 0xb3;
        nDefaultPort = 7475;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1463452342, 3335213172, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000006f2bb863230cda4f4fbee520314077e599a90b9c6072ea2018d7f3a3"));
        assert(genesis.hashMerkleRoot == uint256S("0x951bc46d2da95047fb4a2c0f9b7d6e45591c3ffb49ab2fdffb8e96ef2b8f2be1"));


        vFixedSeeds.clear(); 
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("Testnet Explorer", "testnet.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 6", "test1.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 7", "test2.iop.cash"));
        vSeeds.push_back(CDNSSeedData("Seed server 8", "test3.iop.cash"));
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,130);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,49);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,76);
        base58Prefixes[EXT_PUBLIC_KEY] = {0xBB, 0x8F, 0x48, 0x52};
        base58Prefixes[EXT_SECRET_KEY] = {0x2B, 0x7F, 0xA4, 0x2A};

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        checkpointData = (CCheckpointData) {
            {
                {7800, uint256S("0x000000008c8074841b9ed5c62d61a84f659f19f716b56f774a3d9a28fec693a4")},
            }
        };

        chainTxData = ChainTxData{
            // Data as of block 00000000000001c200b9790dc637d3bb141fe77d155b966ed775b17e109f7c6c (height 1156179)
            1496247512,
            8820,
            0.001
        };

    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        /* **** IOP CHANGE //
        IoP Chain uses the coinbase content to store the miner signature, so we can not enforce BIP34 in its current form.
        So deactivate the check for BIP34 completely
        // **** IOP CHANGE */
        // consensus.BIP34Height = -1; // never enforce BIP34
        // consensus.BIP34Hash = uint256();
        // consensus.BIP65Height = 0; // always enforce BIP 65 and BIP 66
        // consensus.BIP66Height = 0; 
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");
        
        // Miner White list params
        consensus.minerWhiteListActivationHeight = 5000; //block height that activates the white list.
        consensus.minerWhiteListAdminPubKey.insert("03760087582c5e225aea2a6781f4df8b12d7124e4f039fbd3e6d053fdcaacc60eb"); //pub key required to sign add / remove transactions
        consensus.minerWhiteListAdminAddress.insert("ucNbB1K3BaHWY5tXrWiyWn11QB51vPDuVE"); //default miner address
        consensus.minerCapSystemChangeHeight = 5200;
        // Voting System Params
        // consensus.ccBlockStartAdditionalHeight = 10;
        consensus.ccLastCCBlockHeight = 7000;

        pchMessageStart[0] = 0x35;
        pchMessageStart[1] = 0xb2;
        pchMessageStart[2] = 0xcc;
        pchMessageStart[3] = 0x9e;
        nDefaultPort = 14877;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1463452384, 2528424328, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x13ac5baa4b3656eec3ae4ab24b44ae602b9d1e549d9f1f238c1bfce54571b8b5"));
        assert(genesis.hashMerkleRoot == uint256S("0x951bc46d2da95047fb4a2c0f9b7d6e45591c3ffb49ab2fdffb8e96ef2b8f2be1"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData) {
            {
                {0, uint256S("0x13ac5baa4b3656eec3ae4ab24b44ae602b9d1e549d9f1f238c1bfce54571b8b5")},
            }
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,130);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,49);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,76);
        base58Prefixes[EXT_PUBLIC_KEY] = {0xBB, 0x8F, 0x48, 0x52};
        base58Prefixes[EXT_SECRET_KEY] = {0x2B, 0x7F, 0xA4, 0x2A};
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
