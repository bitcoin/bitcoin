// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

//#include "chainparamsseeds.h"

unsigned int pnSeed[] =
{
    0x047ffe33, 0xa646c405, 0x0e550905,
};


int64_t CChainParams::GetProofOfWorkReward(int nHeight, int64_t nFees) const
{
    // miner's coin base reward
    int64_t nSubsidy = 0 * COIN;
 
    if(pindexBest->nHeight+1 == 1)
    {
        nSubsidy = 1000000 * COIN; // GameUnits Headquarter Funds 
		                           // Is not about greed. It's about innovation and fairness.
								   // Any successful entrepreneur is greedy. They have an insatiable desire to see their product come to market. They want to see their invention in the hands of as many people as possible. They want their Games to hit the New York Times bestseller list. They will do whatever it takes to achieve their goal. They will stay up later and get up earlier.
    }
	    else if(pindexBest->nHeight+1 >= 2 && pindexBest->nHeight+1 <= 30)
    {
        nSubsidy = 0 * COIN;
		LogPrintf("We talk a lot about hope, helping, and teamwork. Our whole message is that we are more powerful together.");
    }
        else if(pindexBest->nHeight+1 >= 30 && pindexBest->nHeight+1 <= 144)
    {
        nSubsidy = 50 * COIN;
    }
        else if(pindexBest->nHeight+1 >= 144 && pindexBest->nHeight+1 <= 288)
    {
        nSubsidy = 25 * COIN;
    }
        else if(pindexBest->nHeight+1 >= 288 && pindexBest->nHeight+1 <= nLastPOWBlock)
    {
        nSubsidy = 1 * COIN;
    }
    
    if (fDebug && GetBoolArg("-printcreation"))
        LogPrintf("GetProofOfWorkReward() : create=%s nSubsidy=%d\n", FormatMoney(nSubsidy).c_str(), nSubsidy);
    
    return nSubsidy + nFees;
};


int64_t CChainParams::GetProofOfStakeReward(int64_t nCoinAge, int64_t nFees) const
{
   // miner's coin stake reward based on coin age spent (coin-days)
   // proof of stake rewards. POS begins at block 2500

    int64_t nSubsidy = nCoinAge * COIN_YEAR_REWARD * 33 / (365 * 33 + 8); //default 10% year
	
        if(pindexBest->nHeight+1 >= 25 && pindexBest->nHeight+1 <= 35)
    {
        nSubsidy = 3 * COIN;
    }
        else if(pindexBest->nHeight+1 >= 35 && pindexBest->nHeight+1 <= 45)
    {
        nSubsidy = 5 * COIN;
    }
        else if(pindexBest->nHeight+1 >= 45 && pindexBest->nHeight+1 <= 55)
    {
        nSubsidy = 7 * COIN;
    }
        else if(pindexBest->nHeight+1 >= 55 && pindexBest->nHeight+1 <= 75)
    {
        nSubsidy = 10 * COIN;
    }
        else if(pindexBest->nHeight+1 >= 75 && pindexBest->nHeight+1 <= 85)
    {
        nSubsidy = 50 * COIN;
    }
        else if(pindexBest->nHeight+1 >= 85 && pindexBest->nHeight+1 <= 90)
    {
        nSubsidy = 10 * COIN;
    }
		else if(pindexBest->nHeight+1 > 90)
    {
        nSubsidy = nCoinAge * COIN_YEAR_REWARD * 33 / (365 * 33 + 8);  //default 10% year
    }    
    


    
    if (fDebug && GetBoolArg("-printcreation"))
        LogPrintf("GetProofOfStakeReward(): create=%s nCoinAge=%d\n", FormatMoney(nSubsidy).c_str(), nCoinAge);
    
    return nSubsidy + nFees;
}



//
// Main network
//

// Convert the pnSeeds6 array into usable address objects.
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

// Convert the pnSeeds6 array into usable address objects.
static void convertSeeds(std::vector<CAddress> &vSeedsOut, unsigned int *data, unsigned int count, int port)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in_addr ip;
        memcpy(&ip, &pnSeed[i], sizeof(ip));
        CAddress addr(CService(ip, Params().GetDefaultPort()));
        addr.nTime = GetTime()-GetRand(nOneWeek)-nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x13;
        pchMessageStart[1] = 0xfc;
        pchMessageStart[2] = 0x37;
        pchMessageStart[3] = 0x9c;
        
        vAlertPubKey = ParseHex("04aae3014be6b24dac41112e10c26d2af8c4ece223650d72491c8a273f80281e5b49809f6edae129b4042778f14d9dc5672b470ad60fba040b74981ba084df3683");
        
        nDefaultPort = 1338;
        nRPCPort = 1337;
        
        nFirstPosBlock = 111; 
        nFirstPosv2Block = 300;
        
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20); // "standard" scrypt target limit for proof of work, results with 0,000244140625 proof-of-work difficulty
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 20);
        bnProofOfStakeLimitV2 = CBigNum(~uint256(0) >> 48);
        
        const char* pszTimestamp = "-Price is what you pay.Value is what you get.- Lady Gaga nails national anthem at Super Bowl 50";
        CTransaction txNew;
        txNew.nTime = GENESIS_BLOCK_TIME;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 0 << CBigNum(42) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].SetEmpty();
        
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = GENESIS_BLOCK_TIME;
        genesis.nBits    = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 113492;
		
        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0000002bc49bd2069830809f05078b1dc660db172d2ce12c2fdeb752898dcc7d"));
        assert(genesis.hashMerkleRoot == uint256("0x5fe523908bf29ec313671bbd4d0ebeb6ab481e24c7cd3a2731de5aae2e1346cc"));
        
        vSeeds.push_back(CDNSSeedData("gameunits.net", "gameunits.net"));
        vSeeds.push_back(CDNSSeedData("5.9.85.14", "5.9.85.14"));
		vSeeds.push_back(CDNSSeedData("51.254.127.4", "51.254.127.4"));
		vSeeds.push_back(CDNSSeedData("5.196.70.166", "5.196.70.166"));

        
        base58Prefixes[PUBKEY_ADDRESS] = list_of(68)                    .convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SCRIPT_ADDRESS] = list_of(115)                   .convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SECRET_KEY]     = list_of(197)                   .convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0xEA)(0x91)(0x10)(0x48).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0xEA)(0x91)(0x52)(0xB7).convert_to_container<std::vector<unsigned char> >();
        
        //convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));
        convertSeeds(vFixedSeeds, pnSeed, ARRAYLEN(pnSeed), nDefaultPort);

        nFirstYearBlock = 529900;
        nSecondYearBlock = 1055500; 
        nThirdYearBlock = 1581100;  
        nFourthYearBlock = 2106700; 
        
        nLastPOWBlock = 10000000;
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const std::vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    std::vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;

//
// Testnet
//

class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0x13;
        pchMessageStart[2] = 0x73;
        pchMessageStart[3] = 0x2f;
        
        nFirstPosBlock = 111; 
        nFirstPosv2Block = 301;
        
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 16);
        bnProofOfStakeLimit = CBigNum(~uint256(0) >> 20);
        bnProofOfStakeLimitV2 = CBigNum(~uint256(0) >> 16);
        
        vAlertPubKey = ParseHex("04cb8e1c5a0d005e95f90c3d7d8dc10fc0b6695696e0d360fc43973ea2dff96a9e59bdd5b14723e9fa250ae1fb41481253bca58400a45c44dde20e0797425fb9b8");
        nDefaultPort = 17997;
        nRPCPort = 17996;
        strDataDir = "testnet";

        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 31276;		

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0000fa6e0c0439c972dc814e38be9387b98ede13aabf091cce3a2a5958d6909e"));
        assert(genesis.hashMerkleRoot == uint256("0x5fe523908bf29ec313671bbd4d0ebeb6ab481e24c7cd3a2731de5aae2e1346cc"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = list_of(112)                   .convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SCRIPT_ADDRESS] = list_of(177)                   .convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[SECRET_KEY]     = list_of(250)                   .convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x02)(0x6F)(0xB3)(0xA3).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x02)(0x6F)(0x02)(0x0D).convert_to_container<std::vector<unsigned char> >();
        
        //convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));
        convertSeeds(vFixedSeeds, pnSeed, ARRAYLEN(pnSeed), nDefaultPort);

        //nLastPOWBlock = 0x7fffffff;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;


//
// Regression test
//
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        pchMessageStart[0] = 0x57;
        pchMessageStart[1] = 0xad;
        pchMessageStart[2] = 0x2f;
        pchMessageStart[3] = 0xda;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
    //    genesis.nTime = 1411111111;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 0;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        strDataDir = "regtest";
        
     //   assert(hashGenesisBlock == uint256("0x"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

const CChainParams &TestNetParams() {
    return testNetParams;
}

const CChainParams &MainNetParams() {
    return mainParams;
}

void SelectParams(CChainParams::Network network)
{
    switch (network)
    {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    };
};

bool SelectParamsFromCommandLine()
{
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest)
    {
        return false;
    };

    if (fRegTest)
    {
        SelectParams(CChainParams::REGTEST);
    } else
    if (fTestNet)
    {
        SelectParams(CChainParams::TESTNET);
    } else
    {
        SelectParams(CChainParams::MAIN);
    };
    
    return true;
}
