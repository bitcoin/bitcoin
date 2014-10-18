// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "assert.h"
#include "core.h"
#include "protocol.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

//
// Main network
//

unsigned int pnSeed[] =
{
};

class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x5a;
        pchMessageStart[1] = 0x45;
        pchMessageStart[2] = 0x4e;
        pchMessageStart[3] = 0x59;
        vAlertPubKey = ParseHex("049a3063c7aeb29a7114400bf0be0ddf4191b70730ecc93d3a99deb549d47f0e8b4a92e74cf92aa9c38d2870581fda3b34f039ddbcbf0db9aab290d2e59550093e");
        nDefaultPort = 9253;
        nRPCPort = 9252;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 14);
        nSubsidyHalvingInterval = 210000;

        // Build the genesis block. Note that the output of the genesis coinbase cannot
        // be spent as it did not originally exist in the database.
        //
        // CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
        //   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
        //     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
        //   vMerkleTree: 4a5e1e
        const char* pszTimestamp = "The Times 13/Oct/2014 BitZeny";
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1231006505;
        genesis.nBits    = 0x1f03ffff;
        genesis.nNonce   = 51481;

        hashGenesisBlock = genesis.GetHash();
#if 0
        {
            printf("calc new genesis block\n");
            printf("hashMerkleRoot %s\n", genesis.hashMerkleRoot.ToString().c_str());
            printf("bnProofOfWorkLimit 0x%x\n", bnProofOfWorkLimit.GetCompact());
            printf("genesis.nBits 0x%x\n", genesis.nBits);

            for (genesis.nNonce = 0; ; genesis.nNonce++) {
                hashGenesisBlock = genesis.GetHash();
                if (hashGenesisBlock <= bnProofOfWorkLimit.getuint256()) break;
            }

            printf("hashGenesisBlock %s\n", hashGenesisBlock.ToString().c_str());
            printf("genesis.nNonce %d\n", genesis.nNonce);
        }
#endif
        assert(hashGenesisBlock == uint256("0x0001ec690b3c6971e51e40ec24d4c62ba4d8383f7f74645e256d57e1d533ccb7"));
        assert(genesis.hashMerkleRoot == uint256("0x1d2f70d467851c55b9f547f813f703daf6d2501e586332d4606fbdc77ce1449f"));

        //vSeeds.push_back(CDNSSeedData("bitcoin.sipa.be", "seed.bitcoin.sipa.be"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of(81);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(5);
        base58Prefixes[SECRET_KEY] =     list_of(128);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4);

        // Convert the pnSeeds array into usable address objects.
        for (unsigned int i = 0; i < ARRAYLEN(pnSeed); i++)
        {
            // It'll only connect to one or two seed nodes because once it connects,
            // it'll get a pile of addresses with newer timestamps.
            // Seed nodes are given a random 'last seen time' of between one and two
            // weeks ago.
            const int64_t nOneWeek = 7*24*60*60;
            struct in_addr ip;
            memcpy(&ip, &pnSeed[i], sizeof(ip));
            CAddress addr(CService(ip, GetDefaultPort()));
            addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
            vFixedSeeds.push_back(addr);
        }
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;


//
// Testnet (v3)
//
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0x59;
        pchMessageStart[1] = 0x45;
        pchMessageStart[2] = 0x4e;
        pchMessageStart[3] = 0x59;
        vAlertPubKey = ParseHex("04cf6a570bbb8cc7f5c01fc0b818d610a0ebf6fc2a77a4a5331a61e44433b2018741bd7492aeda79d6e15a6cf836b150bdd9f5c4c54514485757efd2d86275836c");
        nDefaultPort = 19253;
        nRPCPort = 19252;
        strDataDir = "testnet3";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1296688602;
        genesis.nNonce = 21468;
        hashGenesisBlock = genesis.GetHash();
#if 0
        {
            printf("(test)calc new genesis block\n");
            printf("hashMerkleRoot %s\n", genesis.hashMerkleRoot.ToString().c_str());
            printf("bnProofOfWorkLimit 0x%x\n", bnProofOfWorkLimit.GetCompact());
            printf("genesis.nBits 0x%x\n", genesis.nBits);

            for (genesis.nNonce = 0; ; genesis.nNonce++) {
                hashGenesisBlock = genesis.GetHash();
                if (hashGenesisBlock <= bnProofOfWorkLimit.getuint256()) break;
            }

            printf("hashGenesisBlock %s\n", hashGenesisBlock.ToString().c_str());
            printf("genesis.nNonce %d\n", genesis.nNonce);

        }
#endif
        assert(hashGenesisBlock == uint256("0x0002e0f786a970cd158ed0032832ce8fdc85856ada18b505c383460578a018ac"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("bitcoin.petertodd.org", "testnet-seed.bitcoin.petertodd.org"));
        vSeeds.push_back(CDNSSeedData("bluematt.me", "testnet-seed.bluematt.me"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of(111);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(196);
        base58Prefixes[SECRET_KEY]     = list_of(239);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94);
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
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nSubsidyHalvingInterval = 150;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1296688602;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 2;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        strDataDir = "regtest";
        //printf("hashGenesisBlock %s\n", hashGenesisBlock.ToString().c_str());
        assert(hashGenesisBlock == uint256("0x4fe31d85144eb99b8e09d1018c940d68ca292d2fb6097d7c4109c972f436ae42"));

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

void SelectParams(CChainParams::Network network) {
    switch (network) {
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
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
