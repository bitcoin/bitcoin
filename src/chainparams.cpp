// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

static Checkpoints::MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (  1500, uint256("0x000000aaf0300f59f49bc3e970bad15c11f961fe2347accffff19d96ec9778e3"))
        (  4991, uint256("0x000000003b01809551952460744d5dbb8fcbd6cbae3c220267bf7fa43f837367"))
        (  9918, uint256("0x00000000213e229f332c0ffbe34defdaa9e74de87f2d8d1f01af8d121c3c170b"))
        ( 16912, uint256("0x00000000075c0d10371d55a60634da70f197548dbbfa4123e12abfcbc5738af9"))
        ( 23912, uint256("0x0000000000335eac6703f3b1732ec8b2f89c3ba3a7889e5767b090556bb9a276"))
        ( 35457, uint256("0x0000000000b0ae211be59b048df14820475ad0dd53b9ff83b010f71a77342d9f"))
        ( 45479, uint256("0x000000000063d411655d590590e16960f15ceea4257122ac430c6fbe39fbf02d"))
        ( 55895, uint256("0x0000000000ae4c53a43639a4ca027282f69da9c67ba951768a20415b6439a2d7"))
        ( 68899, uint256("0x0000000000194ab4d3d9eeb1f2f792f21bb39ff767cb547fe977640f969d77b7"))
        ( 74619, uint256("0x000000000011d28f38f05d01650a502cc3f4d0e793fbc26e2a2ca71f07dc3842"))
        ( 75095, uint256("0x0000000000193d12f6ad352a9996ee58ef8bdc4946818a5fec5ce99c11b87f0d"))
        ( 88805, uint256("0x00000000001392f1652e9bf45cd8bc79dc60fe935277cd11538565b4a94fa85f"))
        ( 107996, uint256("0x00000000000a23840ac16115407488267aa3da2b9bc843e301185b7d17e4dc40"))
        ( 137993, uint256("0x00000000000cf69ce152b1bffdeddc59188d7a80879210d6e5c9503011929c3c"))
        ( 167996, uint256("0x000000000009486020a80f7f2cc065342b0c2fb59af5e090cd813dba68ab0fed"))
        ( 207992, uint256("0x00000000000d85c22be098f74576ef00b7aa00c05777e966aff68a270f1e01a5"))
        ( 312645, uint256("0x0000000000059dcb71ad35a9e40526c44e7aae6c99169a9e7017b7d84b1c2daf"))
        ( 407452, uint256("0x000000000003c6a87e73623b9d70af7cd908ae22fee466063e4ffc20be1d2dbc"))
        ( 477481, uint256("0x0000000000083dbb96b0eb7d3ec9f2c43e015817332c757f8b5a794abf2fdffb"))
        ( 523412, uint256("0x000000000000e54f036576a10597e0e42cc22a5159ce572f999c33975e121d4d"))
        ( 523930, uint256("0x0000000000000bccdb11c2b1cfb0ecab452abf267d89b7f46eaf2d54ce6e652c"))
        ;
static const Checkpoints::CCheckpointData data = {
        &mapCheckpoints,
        1471809614, // * UNIX timestamp of last checkpoint block
        1998064,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        2800        // * estimated number of transactions per day after checkpoint
    };

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 261, uint256("00000c26026d0815a7e2ce4fa270775f61403c040647ff2c3091f99e894a4618"))
        ( 77900, uint256("00000007e5ec67e2a626c07b7d66673c3dd8df0aed5018ca984b99fba2b71024"))
        ( 82313, uint256("000000156a313af1d69fe855609175b276996235e8f4f7da41b10e4a7a750a19"))
        ;
static const Checkpoints::CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1405699509,
        201,
        500
    };

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
        ;
static const Checkpoints::CCheckpointData dataRegtest = {
        &mapCheckpointsRegtest,
        0,
        0,
        0
    };

class CMainParams : public CChainParams {
public:
    CMainParams() {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /** 
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0xbf;
        pchMessageStart[1] = 0x0c;
        pchMessageStart[2] = 0x6b;
        pchMessageStart[3] = 0xbd;
        vAlertPubKey = ParseHex("048240a8748a80a286b270ba126705ced4f2ce5a7847b3610ea3c06513150dade2a8512ed5ea86320824683fc0818f0ac019214973e677acd1244f6d0571fc5103");
        nDefaultPort = 9999;
        bnProofOfWorkLimit = ~uint256(0) >> 20;  // Dash starting difficulty is 1 / 2^12
        nSubsidyHalvingInterval = 210000;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 0;
        nTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        nTargetSpacing = 2.5 * 60; // Dash: 2.5 minutes

        /**
         * Build the genesis block. Note that the output of the genesis coinbase cannot
         * be spent as it did not originally exist in the database.
         * 
         * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
         *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
         *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
         *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
         *   vMerkleTree: e0028e
         */
        const char* pszTimestamp = "Wired 09/Jan/2014 The Grand Experiment Goes Live: Overstock.com Is Now Accepting Bitcoins";
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1390095618;
        genesis.nBits    = 0x1e0ffff0;
        genesis.nNonce   = 28917698;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x00000ffd590b1485b3caadc19b22e6379c733355108f107a430458cdf3407ab6"));
        assert(genesis.hashMerkleRoot == uint256("0xe0028eb9648db56b1ac77cf090b99048a8007e2bb64b68f092c03c7f56a662c7"));

        vSeeds.push_back(CDNSSeedData("darkcoin.io", "dnsseed.darkcoin.io"));
        vSeeds.push_back(CDNSSeedData("masternode.io", "dnsseed.masternode.io"));
        vSeeds.push_back(CDNSSeedData("dashpay.io", "dnsseed.dashpay.io"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of( 76);                    // Dash addresses start with 'X'
        base58Prefixes[SCRIPT_ADDRESS] = list_of( 16);                    // Dash script addresses start with '7'
        base58Prefixes[SECRET_KEY] =     list_of(204);                    // Dash private keys start with '7' or 'X'
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x02)(0xFE)(0x52)(0xF8); // Dash BIP32 pubkeys start with 'drkv'
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x02)(0xFE)(0x52)(0xCC); // Dash BIP32 prvkeys start with 'drkp'
        base58Prefixes[EXT_COIN_TYPE]  = list_of(0x80000005);             // Dash BIP44 coin type is '5'

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        nPoolMaxTransactions = 3;
        strSporkKey = "04549ac134f694c0243f503e8c8a9a986f5de6610049c40b07816809b0d1d06a21b07be27b9bb555931773f62ba6cf35a25fd52f694d4e1106ccd237a7bb899fdd";
        strMasternodePaymentsPubKey = "04549ac134f694c0243f503e8c8a9a986f5de6610049c40b07816809b0d1d06a21b07be27b9bb555931773f62ba6cf35a25fd52f694d4e1106ccd237a7bb899fdd";
        strDarksendPoolDummyAddress = "Xq19GqFvajRrEdDHYRKGYjTsQfpV5jyipF";
        nStartMasternodePayments = 1403728576; //Wed, 25 Jun 2014 20:36:16 GMT
    }

    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0xce;
        pchMessageStart[1] = 0xe2;
        pchMessageStart[2] = 0xca;
        pchMessageStart[3] = 0xff;
        vAlertPubKey = ParseHex("04517d8a699cb43d3938d7b24faaff7cda448ca4ea267723ba614784de661949bf632d6304316b244646dea079735b9a6fc4af804efb4752075b9fe2245e14e412");
        nDefaultPort = 19999;
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        nTargetSpacing = 2.5 * 60; // Dash: 2.5 minutes

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1390666206;
        genesis.nNonce = 3861367235;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x00000bafbc94add76cb75e2ec92894837288a481e5c005f6563d91623bf8bc2c"));

        vFixedSeeds.clear();
        vSeeds.clear();
        /*vSeeds.push_back(CDNSSeedData("dashpay.io", "testnet-seed.dashpay.io"));
        vSeeds.push_back(CDNSSeedData("dash.qa", "testnet-seed.dash.qa"));
        *///legacy seeders
        vSeeds.push_back(CDNSSeedData("darkcoin.io",  "testnet-seed.darkcoin.io"));
        vSeeds.push_back(CDNSSeedData("darkcoin.qa", "testnet-seed.darkcoin.qa"));
        vSeeds.push_back(CDNSSeedData("masternode.io", "test.dnsseed.masternode.io"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of(139);                    // Testnet dash addresses start with 'x' or 'y'
        base58Prefixes[SCRIPT_ADDRESS] = list_of( 19);                    // Testnet dash script addresses start with '8' or '9'
        base58Prefixes[SECRET_KEY]     = list_of(239);                    // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x3a)(0x80)(0x61)(0xa0); // Testnet dash BIP32 pubkeys start with 'DRKV'
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x3a)(0x80)(0x58)(0x37); // Testnet dash BIP32 prvkeys start with 'DRKP'
        base58Prefixes[EXT_COIN_TYPE]  = list_of(0x80000001);             // Testnet dash BIP44 coin type is '5' (All coin's testnet default)

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        nPoolMaxTransactions = 2;
        strSporkKey = "046f78dcf911fbd61910136f7f0f8d90578f68d0b3ac973b5040fb7afb501b5939f39b108b0569dca71488f5bbf498d92e4d1194f6f941307ffd95f75e76869f0e";
        strMasternodePaymentsPubKey = "046f78dcf911fbd61910136f7f0f8d90578f68d0b3ac973b5040fb7afb501b5939f39b108b0569dca71488f5bbf498d92e4d1194f6f941307ffd95f75e76869f0e";
        strDarksendPoolDummyAddress = "y1EZuxhhNMAUofTBEeLqGE1bJrpC2TWRNp";
        nStartMasternodePayments = 1420837558; //Fri, 09 Jan 2015 21:05:58 GMT
    }
    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xc1;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0xdc;
        nSubsidyHalvingInterval = 150;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan = 24 * 60 * 60; // Dash: 1 day
        nTargetSpacing = 2.5 * 60; // Dash: 2.5 minutes
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1417713337;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 1096447;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 19994;
        assert(hashGenesisBlock == uint256("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"));

        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams {
public:
    CUnitTestParams() {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 18445;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Unit test mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const 
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval)  { nSubsidyHalvingInterval=anSubsidyHalvingInterval; }
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority)  { nEnforceBlockUpgradeMajority=anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority)  { nRejectBlockOutdatedMajority=anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority)  { nToCheckBlockUpgradeMajority=anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks)  { fDefaultConsistencyChecks=afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) {  fAllowMinDifficultyBlocks=afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams *pCurrentParams = 0;

CModifiableParams *ModifiableParams()
{
   assert(pCurrentParams);
   assert(pCurrentParams==&unitTestParams);
   return (CModifiableParams*)&unitTestParams;
}

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
        case CBaseChainParams::UNITTEST:
            return unitTestParams;
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
