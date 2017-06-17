// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;

#include "chainparamsseeds.h"

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
        consensus.nMajorityEnforceBlockUpgrade1 = 750;
        consensus.nMajorityRejectBlockOutdated1 = 950;
        consensus.nMajorityWindow1 = 8000;
        consensus.nMajorityEnforceBlockUpgrade2 = 7500;
        consensus.nMajorityRejectBlockOutdated2 = 9500;
        consensus.nMajorityWindow2 = 10000;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 2 * 60; // two minutes
        consensus.nPowTargetSpacing = 30;
        consensus.fPowAllowMinDifficultyBlocks = false;
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xb5;
        pchMessageStart[2] = 0x03;
        pchMessageStart[3] = 0xdf;
        vAlertPubKey = ParseHex("045337216002ca6a71d63edf062895417610a723d453e722bf4728996c58661cdac3d4dec5cecd449b9086e9602b35cc726a9e0163e1a4d40f521fbdaebb674658");
        nDefaultPort = 17333;
        nMinerThreads = 0;
        nMaxTipAge = 24 * 60 * 60;
        nPruneAfterHeight = 100000000;

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
        const char* pszTimestamp = "3 Aug 2013 - M&G - Mugabe wins Zim election with more than 60% of votes";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 1000 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock.SetNull();
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1375548986;
        genesis.nBits    = 0x1e0fffff;
        genesis.nNonce   = 2089928209;

        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000006cab7aa2be2da91015902aa4458dd5fbb8778d175c36d429dc986f2bff4"));
        assert(genesis.hashMerkleRoot == uint256S("0xd0227b8c3e3d07bce9656b3d9e474f050d23458aaead93357dcfdac9ab9b79f9"));

        vSeeds.push_back(CDNSSeedData("seed1.zeta-coin.com", "seed1.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed2.zeta-coin.com", "seed2.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed3.zeta-coin.com", "seed3.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed4.zeta-coin.com", "seed4.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed5.zeta-coin.com", "seed5.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed6.zeta-coin.com", "seed6.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed7.zeta-coin.com", "seed7.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed8.zeta-coin.com", "seed8.zeta-coin.com"));
        vSeeds.push_back(CDNSSeedData("seed1.zetac.org", "seed1.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed2.zetac.org", "seed2.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed3.zetac.org", "seed3.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed4.zetac.org", "seed4.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed5.zetac.org", "seed5.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed6.zetac.org", "seed6.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed7.zetac.org", "seed7.zetac.org"));
        vSeeds.push_back(CDNSSeedData("seed8.zetac.org", "seed8.zetac.org"));
        vSeeds.push_back(CDNSSeedData("zet2.ignorelist.com", "zet2.ignorelist.com"));
        vSeeds.push_back(CDNSSeedData("zet.strangled.net", "zet.strangled.net"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,80);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,9);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,224);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (Checkpoints::CCheckpointData) {
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
            (5086747, uint256S("0x000000000004f4f433813246fd475d1b85357c1e6ab8d9ceecf372f4adae1e00")),
            1466312086, // * UNIX timestamp of last checkpoint block
            6028389,    // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            2880.0      // * estimated number of transactions per day after checkpoint
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nMajorityEnforceBlockUpgrade1 = 51;
        consensus.nMajorityRejectBlockOutdated1 = 75;
        consensus.nMajorityWindow1 = 100;
        consensus.nMajorityEnforceBlockUpgrade2 = 51;
        consensus.nMajorityRejectBlockOutdated2 = 75;
        consensus.nMajorityWindow2 = 100;
        consensus.fPowAllowMinDifficultyBlocks = true;
        pchMessageStart[0] = 0x05;
        pchMessageStart[1] = 0xfe;
        pchMessageStart[2] = 0xa9;
        pchMessageStart[3] = 0x01;
        vAlertPubKey = ParseHex("04deffaef5b9552d1635013708eff25f2fac734cd6720d86fe83f9618572eb095b738efd752128b885c40ca0a37535df5a4b2b2cae5c80cea9bf315fb67ce9fcb2");
        nDefaultPort = 27333;
        nMinerThreads = 0;
        nMaxTipAge = 0x7fffffff;
        nPruneAfterHeight = 1000;

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1374901773;
        genesis.nNonce = 414708675;
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000007717e2e2df52a9ff29b0771901c9c12f5cbb4914cdf0c8047b459bb21d8"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("test1.zetatestnet.pw", "test1.zetatestnet.pw"));
        vSeeds.push_back(CDNSSeedData("test2.zetatestnet.pw", "test2.zetatestnet.pw"));
        vSeeds.push_back(CDNSSeedData("test3.zetatestnet.pw", "test3.zetatestnet.pw"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,88);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,188);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (Checkpoints::CCheckpointData) {
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
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMajorityEnforceBlockUpgrade1 = 750;
        consensus.nMajorityRejectBlockOutdated1 = 950;
        consensus.nMajorityWindow1 = 1000;
        consensus.nMajorityEnforceBlockUpgrade2 = 750;
        consensus.nMajorityRejectBlockOutdated2 = 950;
        consensus.nMajorityWindow2 = 1000;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0x0f;
        pchMessageStart[2] = 0xa5;
        pchMessageStart[3] = 0x5a;
        nMinerThreads = 1;
        nMaxTipAge = 24 * 60 * 60;
        genesis.nTime = 1296688602;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 2;
        consensus.hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        //assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        nPruneAfterHeight = 1000;

        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (Checkpoints::CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206")),
            0,
            0,
            0
        };
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams &Params(CBaseChainParams::Network network) {
    switch (network) {
        case CBaseChainParams::MAIN:
            return mainParams;
        case CBaseChainParams::TESTNET:
            return testNetParams;
        case CBaseChainParams::REGTEST:
            return regTestParams;
        default:
            assert(false && "Unimplemented network");
            return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network) {
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
