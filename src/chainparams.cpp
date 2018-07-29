// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

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
    genesis.vtx.push_back(txNew);
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
    const char* pszTimestamp = "3 Aug 2013 - M&G - Mugabe wins Zim election with more than 60% of votes";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
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
        consensus.nSubsidyHalvingInterval = 80640;
        //consensus.BIP34Height = 227931;
        //consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 2 * 60; // two minutes
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 19160; // 95% of 20160
        consensus.nMinerConfirmationWindow = 20160; // nPowTargetTimespan / nPowTargetSpacing
        
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1523059200; // April 7, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1570406400; // Oct 7, 2019

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1523059200; // April 7, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1570406400; // Oct 7, 2019

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1535760000; // Sep 1, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1598918400; // Sep 1, 2020

        // The best chain should have at least this much work.
        //consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000003418b3ccbe5e93bcb39b43");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xb5;
        pchMessageStart[2] = 0x03;
        pchMessageStart[3] = 0xdf;
        nDefaultPort = 17333;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1375548986, 2089928209, 0x1e0fffff, 1, 1000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000006cab7aa2be2da91015902aa4458dd5fbb8778d175c36d429dc986f2bff4"));
        assert(genesis.hashMerkleRoot == uint256S("0xd0227b8c3e3d07bce9656b3d9e474f050d23458aaead93357dcfdac9ab9b79f9"));

        // Note that of those with the service bits flag, most only support a subset of possible options
        vSeeds.push_back(CDNSSeedData("seed1.zetac.org", "seed1.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed2.zetac.org", "seed2.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed3.zetac.org", "seed3.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed4.zetac.org", "seed4.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed5.zetac.org", "seed5.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed6.zetac.org", "seed6.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed7.zetac.org", "seed7.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed8.zetac.org", "seed8.zetac.org"));
        vSeeds.push_back(CDNSSeedData("zeta1.twilightparadox.com", "zeta1.twilightparadox.com"));
        vSeeds.push_back(CDNSSeedData("zeta2.twilightparadox.com", "zeta2.twilightparadox.com"));
        vSeeds.push_back(CDNSSeedData("zeta3.twilightparadox.com", "zeta3.twilightparadox.com"));
        vSeeds.push_back(CDNSSeedData("zeta4.twilightparadox.com", "zeta4.twilightparadox.com"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,80);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,9);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,224);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (      0, uint256S("0x000006cab7aa2be2da91015902aa4458dd5fbb8778d175c36d429dc986f2bff4"))
            (  30350, uint256S("0x000000000032d087f157871fbc41541a43ac30291f99ce5225d69fd132f8ecdf"))
            (  66438, uint256S("0x00000000000970ae1d1fddcdf363dfc49505caa2884367ad460839d0621d1f56"))
            ( 103010, uint256S("0x000000000007204260b891b9aa8eb476132e74eb7539dc3e9ac2fb7bc7104ab8"))
            ( 252509, uint256S("0x0000000000090c2b77a3247303784289fb6a18752d54e38e96d2b48eac245016"))
            ( 470201, uint256S("0x00000000000102bdfdfc228ee34304f64650825fd1639a1f57a397af854b9df1"))
            ( 523001, uint256S("0x00000000000076412e07ded5bcdf11c0ea6bfcada9e339cb31d312d8e60c3ef8"))
            ( 569410, uint256S("0x000000000000085bbed51c9196314ee52281428ff5b1d8cade9140efe4b33381"))
            ( 587317, uint256S("0x000000000000b81d6626e9fd0c869764dd992d5429442876a75894e24c0c15e2"))
            ( 636469, uint256S("0x0000000000038394fe569fbd5a42484c69f15dae9f10982a7a7ed96bff4a359e"))
            ( 720261, uint256S("0x000000000005658b461195d927cf3347ebf8a36e987a2d2be26ed4fce0f75b13"))
            ( 815426, uint256S("0x000000000004958ee412205bc78e41061e3cb66b55cdd5230efceaaa07990f55"))
            ( 870101, uint256S("0x000000000006fcd0f5cff20c46d9da02f7835137bce0629431f1968c6d1dcab5"))
            ( 978901, uint256S("0x00000000000456f795ce33e9ad1757150c1b5155230e4438b3690004e00f7ede"))
            (1272500, uint256S("0x000000000001567d68a0197b43ec9c764d49a78cee9c318d58c5ae8d3a6a4a88"))
            (1410098, uint256S("0x000000000000ad1fe5f741c497aab1f4c9f2799ed2cce1c6715601e84c543368"))
            (1538097, uint256S("0x00000000000002f54303f5b45c1ec74c75f085034fe0438834bb6ed2cb2f78f1"))
            (1967101, uint256S("0x0000000000006f92c571a6b1a6923efd03320b6bb6bc0656c4f23d01e8664a85"))
            (2062289, uint256S("0x000000000001b7e95495d1f418f69498804397745f29e024d40dbe1ef4725af8"))
            (2229225, uint256S("0x000000000003c9990b62822e5be8a49bebae5e270c39db223d3504d2ecd38604"))
            (2647621, uint256S("0x0000000000019c07fe91065d5dee6b42af812830e04ce59efa7fad10cb020396"))
            (4086591, uint256S("0x00000000000284dcc409a09957de00a54bc63bcc3348305375f5df8b150fc4c4"))
            (4725861, uint256S("0x00000000000018b6b3d846b28153c03f009515216fb9ccc9b2b3e441d343af56"))
            (4849869, uint256S("0x000000000000a7d04b122d2e798cc6c8f8418aca94254e49dee420e69765bc11"))
            (4874572, uint256S("0x000000000000ac41b139efe9257159e8f4c42d0425d5344dab303ac0150c2384"))
            (5086747, uint256S("0x000000000004f4f433813246fd475d1b85357c1e6ab8d9ceecf372f4adae1e00"))
            (6950776, uint256S("0x000000000000291d02c1ec999510e59be6972f2988ca32f2d3e4077a315ab856"))
            (7953504, uint256S("0x0000000000005d5b27438369531e68854113bede432fab20943fd9f7b9c19a8f")),
            1513318583, // * UNIX timestamp of last checkpoint block
            9106966,    // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            2880.0      // * estimated number of transactions per day after checkpoint
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        //consensus.BIP34Height = 21111;
        //consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1520380800; // March 7, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1570406400; // Oct 7, 2019

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1520380800; // March 7, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1570406400; // Oct 7, 2019

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1535760000; // Sep 1, 2018
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1598918400; // Sep 1, 2020

        // The best chain should have at least this much work.
        //consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000001b3fcc3e766e365e4b");

        pchMessageStart[0] = 0x05;
        pchMessageStart[1] = 0xfe;
        pchMessageStart[2] = 0xa9;
        pchMessageStart[3] = 0x01;
        nDefaultPort = 27333;
        nPruneAfterHeight = 1000;
       
        genesis = CreateGenesisBlock(1374901773, 414708675, 0x1e0fffff, 1, 1000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000007717e2e2df52a9ff29b0771901c9c12f5cbb4914cdf0c8047b459bb21d8"));
        assert(genesis.hashMerkleRoot == uint256S("0xd0227b8c3e3d07bce9656b3d9e474f050d23458aaead93357dcfdac9ab9b79f9"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.push_back(CDNSSeedData("test1.zetatestnet.pw", "test1.zetatestnet.pw"));
        vSeeds.push_back(CDNSSeedData("test2.zetatestnet.pw", "test2.zetatestnet.pw"));
        vSeeds.push_back(CDNSSeedData("test3.zetatestnet.pw", "test3.zetatestnet.pw"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,88);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,188);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;


        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x000007717e2e2df52a9ff29b0771901c9c12f5cbb4914cdf0c8047b459bb21d8")),
            1374901773,
            0,
            2880
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        //consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        //consensus.BIP34Hash = uint256();
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
        //consensus.nMinimumChainWork = uint256S("0x00");

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0x0f;
        pchMessageStart[2] = 0xa5;
        pchMessageStart[3] = 0x5a;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 1000 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        // assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        assert(genesis.hashMerkleRoot == uint256S("0xd0227b8c3e3d07bce9656b3d9e474f050d23458aaead93357dcfdac9ab9b79f9"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")),
            0,
            0,
            0
        };
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
 
