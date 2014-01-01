// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
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
    0xe473042e, 0xb177f2ad, 0xd63f3fb2, 0xf864f736, 0x44a23ac7, 0xcf6d9650, 0xd648042e, 0x0536f447,
    0x3c654ed0, 0x3e16a5bc, 0xa38e09b0, 0xdfae795b, 0xabfeca5b, 0x94ad7840, 0xf3b9f1c7, 0xbe70e0ad,
    0x3bbd09b0, 0x8d0c7dd5, 0x3b2a7332, 0x1a06175e, 0x581f175e, 0xca0d2dcc, 0x0fdbc658, 0xcf591ec7,
    0x295a12b2, 0xb4707bce, 0x68bb09b0, 0x4e735747, 0x89709553, 0x05a7814e, 0x5b8ec658, 0x402c5512,
    0xe80d0905, 0x17681a5e, 0xc02aa748, 0x9f811741, 0x5f321cb0, 0x23e1ee47, 0xaf7f170c, 0xaa240ab0,
    0xedea6257, 0x76106bc1, 0x2cf310cc, 0x08612acb, 0x9c682e4e, 0x8e963c6c, 0x443c795b, 0x22e246b8,
    0xfa1f2dcc, 0x90118140, 0x3821042e, 0x33c3fd2e, 0x10046d5b, 0x40d14b3e, 0x7fb8f8ce, 0x67696550,
    0xeeecbe58, 0x4f341745, 0x46b8fbd5, 0xc8463932, 0x6b73e862, 0x4c715932, 0x4a6785d5, 0xce3a64c2,
    0xde9604c7, 0x9b06884f, 0x18002a45, 0xea9bc345, 0xc4f1c658, 0xe475c1c7, 0xdd3e795b, 0x9722175e,
    0x34562f4e, 0x66c46e4e, 0x40bb1243, 0x7d9171d0, 0x17b8dbd5, 0x63cbfd2e, 0x1a08b8d8, 0x6175a73b,
    0x228d2660, 0x8627c658, 0x9c566644, 0x38cca5bc, 0x3089de5b, 0x92e25f5d, 0xa393f73f, 0xcc92dc3e,
    0x27487446, 0x62cbfd2e, 0x9d983b45, 0xf72a09b0, 0xf75f042e, 0x6434bb6a, 0xb29e77d8, 0x19be4fd9,
    0x76443243, 0x9dd72645, 0x694cef43, 0x89c2efd5, 0x5f1c5058, 0x46c6e45b, 0xe1391b40, 0x77ccefd5,
    0x472e5a6d, 0x85709553, 0xdd4f5d4c, 0x64ef5a46, 0x7f0ae502, 0xcf08d850, 0x3460042e, 0xeafa2d42,
    0x793c9044, 0x9d094746, 0x1ab9b153, 0xbfe9a5bc, 0x34771fb0, 0xb7722e32, 0x1168964b, 0x19b06ab8,
    0x19243b25, 0x13188045, 0xb4070905, 0x728ebb5d, 0x44f24ac8, 0xa317fead, 0x642f6a57, 0x3d951f32,
    0x3d312e4e, 0xfac4d048, 0xefc4dd50, 0x52b9f1c7, 0xc14d3cc3, 0x0219ea44, 0x3b79d058, 0xfa217242,
    0x39c80647, 0xfb697252, 0x1d495a42, 0x0aa81f4e, 0x58249ab8, 0xe6a8e6c3, 0x2bc4dad8, 0x85963c6c,
    0xa4ce09b0, 0x2005f536, 0x5cc2703e, 0x1992de43, 0x74e86b4c, 0xe7085653, 0xf5e15a51, 0xb4872b60,
    0x29e2b162, 0xa07ea053, 0x8229fd18, 0x4562ec4d, 0x8dec814e, 0x36cfa4cf, 0x96461032, 0x3c8770de,
    0xd10a1f5f, 0x95934641, 0x97cd65d0, 0x2e35324a, 0x2566ba1f, 0x1ca1a9d1, 0xb808b8d5, 0xf9a24a5d,
    0xafc8d431, 0xe4b8d9b2, 0x0f5321b2, 0x330bc658, 0x74b347ce, 0x972babd5, 0x044f7d4f, 0x06562f4e,
    0x8b8d3c6c, 0x3507c658, 0xe4174e4d, 0xf1c009b0, 0x52249ab8, 0x27211772, 0xf6a9ba59, 0x7a391b40,
    0x855dc6c0, 0x291f20b2, 0xe29bc345, 0x90963c6c, 0x0af70732, 0x4242a91f, 0x4c531d48, 0xa32df948,
    0x627e3044, 0x65be1f54, 0x1a0cbf83, 0x6a443532, 0x8d5f1955, 0xbafa8132, 0x3534bdd5, 0xca019dd9,
    0x8a0d9332, 0x5584e7d8, 0x7cd1f25e, 0xeabe3fb2, 0x2945d0d1, 0x46415718, 0x70d6042e, 0x99eb76d0,
    0x9ece09b0, 0xb3777418, 0x5e5e91d9, 0x237a3ab0, 0xf512b62e, 0x45dec347, 0x59b7f862, 0x4c443b25,
    0x3cc6484b, 0x9a8ec6d1, 0x021eea44, 0xc9483944, 0xfd567e32, 0xfd204bb2, 0xc5330bcc, 0x5202894e,
    0xf9e309b0, 0x4cc17557, 0xdb9064ae, 0xe19e77d8, 0x25857f60, 0xeb4a15ad, 0x1f47f554, 0xea4472d9,
    0xd20de593, 0xf5733b25, 0x11892b54, 0x5729d35f, 0xe6188cd1, 0x488b132e, 0x541c534a, 0xa8e854ae,
    0xa255a66c, 0x33688763, 0xc6629ac6, 0xc20a6265, 0xcd92a059, 0x72029d3b, 0x4c298f5e, 0x51452e4e,
    0xbb065058, 0x15fd2dcc, 0xf40c135e, 0x615a0bad, 0x0c6a6805, 0x4971a7ad, 0x17f2a5d5, 0xf8babf47,
    0xb61f50ad, 0x4e1451b1, 0xf72d9252, 0x5c2abe58, 0xbd987c61, 0x084ae5cf, 0x20781fb0, 0x38b0f160,
    0x18aac705, 0x14f86dc1, 0x5556f481, 0x0a36c144, 0xeb446e4c, 0x2c1c0d6c, 0xbd0ff860, 0x869f92db,
    0x36c94f4c, 0x05502444, 0x148fe55b, 0xd5301e59, 0xd57a8f45, 0x110dc04a, 0x8670fc36, 0xee733b25,
    0xca56f481, 0x2a5c3bae, 0x844b0905, 0x1e51fe53, 0x0241c244, 0x59c0614e, 0x94e70a55, 0x7312fead,
    0xb735be44, 0xa55d0905, 0x2f63962e, 0x14a4e15b, 0x63f8f05c, 0x62d0d262, 0x3cab41ad, 0x87f1b1cb,
    0x018da6b8, 0xb3967dd5, 0xcb56f481, 0x685ad718, 0x3b4aeeca, 0x8d106bc1, 0x51180905, 0x72660f48,
    0x1521a243, 0x5b56f481, 0x6390e560, 0xdd61464e, 0x58353b25, 0x553fc062, 0x27c45d59, 0xacc62e4e,
    0x0d5a1cd9, 0x7f65f442, 0xbdeef660, 0xf1bd1855, 0xf8473cae, 0x13b120b2, 0x442440d0, 0x53fd4352,
    0xa305fc57, 0x458be84d, 0x639ce1c3, 0xebaaee47, 0x95e2c247, 0xf056f481, 0x6256f481, 0x1d87c65e,
    0x0a453418, 0x5beb175e, 0xd64f1618, 0xc360795b, 0x2fbf5753, 0xa8c58e53, 0x651cec52, 0x9d37b043,
    0x124a9758, 0x5242e4a9, 0x89913c6c, 0x880efe2e, 0x2f2f2f0c, 0x72b26751, 0x2896e46d, 0x80f4166c,
    0x320d59ad, 0xc50151d0, 0x11a8aa43, 0xccf56057, 0x5fbad118, 0x4719b151, 0x2b5f4bc0, 0x4d7a4a50,
    0xad06e047, 0x62ef5a46, 0x5aebde58, 0xdf7aa66c, 0x851acb50, 0x66b9a559, 0x3e9bb153, 0xcc512f2e,
    0xc073b08e, 0xd519be58, 0xe981ea4d, 0x12fd50cb, 0x378739ad, 0x06683cae, 0xa22310b2, 0xc185c705,
    0x8741b545, 0xa26c8318, 0x22d5bc43, 0x39201ec0, 0x68581e3e, 0xdc9bcf62, 0xd508cc82, 0xb149675b,
    0x4c9609b0, 0x84feb84c, 0x08291e2e, 0xfd2253b2, 0x1fd269c1, 0xc9483932, 0x4d641fb0, 0x7d37c918,
    0xa9de20ad, 0x77e2d655, 0x6d421b59, 0xd7668f80, 0xced09b62, 0xa9e5a5bc, 0xa4074e18, 0x60fc5ecc,
    0x01300148, 0x68062444, 0xb4224847, 0xed3aa443, 0xb772fb43, 0x9f56f481, 0x220dfd18, 0x8e1c3d6c,
    0xc44f09b0, 0x7df2bb73, 0xe22fb844, 0xea534242, 0xb6a755d4, 0xa036654b, 0x138ece5b, 0xda65d3c3,
    0x955871bc, 0x792124b0, 0xfc82594c, 0x851d494b, 0x2c7aee47, 0x26af46b8, 0x1416252e, 0xa8abb944,
    0x36c49d25, 0x674f645d, 0x363646b8, 0x9e1a2942, 0x66d0c154, 0xc6c2a545, 0x3570f2ad, 0xe7d547c7,
    0x7d104932, 0x18cb9c18, 0x1dcfa4cf, 0xd156f481, 0x2a02b91f, 0x3eeb3fa8, 0xcac4175e, 0x34146d42,
    0x994c4d46, 0x5666f440, 0x85d6713e, 0x5ecb296c, 0x0ea0ae46, 0x87e69f42, 0xc58409b0, 0x1f3436ae,
    0x21dc6a57, 0x4ad1cd42, 0xfb8c1a4c, 0x52d3dab2, 0x3769894b, 0xb52f1c62, 0x3677916d, 0x82b3fe57,
    0x493d4ac6, 0x9f963c6c, 0x5d91ff60, 0x458e0dad, 0xa49d0947, 0x491a3e18, 0x4aadcd5b, 0x0e46494b,
    0x1d1610ad, 0x1a10af5d, 0x4956f481, 0x207a3eae, 0x77e73244, 0xfa3b8742, 0x3261fc36, 0xfcebf536,
    0x1662e836, 0xf655f636, 0xa2dbd0ad, 0x23036693, 0x30448432, 0xa2b03463, 0x30730344, 0x8e4a6882,
    0x0c50a1cb, 0xc8d8c06b, 0xc9cd6191, 0xf443db50, 0xa9553c50, 0x23145847, 0xc35da66c, 0x29c12a60,
    0x55c2b447, 0x7434f75c, 0x61660640, 0xde2a7018, 0xc639494c, 0x1c306fce, 0x19b89244, 0xd29a6462,
    0x462cd1b2, 0x29902f44, 0x2817fa53, 0x21a30905, 0x7777ae46, 0x288443a1, 0x7bee5148, 0xc2a8b043,
    0xf5c3d35f, 0x2311ef84, 0x57de08a4, 0x6b221bb2, 0xf2625846, 0x4b9e09b0, 0xa24f880e, 0x22b11447,
    0xb3a0c744, 0x919e77d8, 0xec8b64ae, 0xff5c8d45, 0x7b15b484, 0x32679a5f, 0xba80b62e, 0x05c25c61,
    0x60014746, 0x5e8fb04c, 0xe67c0905, 0x4329c658, 0xac8fe555, 0xf875e647, 0x67406386, 0x35ceea18,
    0xbb79484b, 0xd7b9fa62, 0x238209b0, 0x208a1d32, 0x9630995e, 0x039c1318, 0x6e48006c, 0x60582344,
    0xadbb0150, 0x853fd462, 0x03772e4e, 0x652ce960, 0x49b630ad, 0x9993af43, 0x3735b34b, 0x548a07d9,
    0x55a44aad, 0xa23d1bcc, 0xfdbb2f4e, 0x530b24a0, 0x0a44b451, 0x6827c657, 0x1f66494b, 0x4e680a47,
    0x77e7b747, 0xa5eb3fa8, 0x6649764a, 0xd4e76c4b, 0x2c691fb0, 0xf1292e44, 0xc6d6c774, 0x85d23775,
    0x28275f4d, 0x259ae46d, 0x02424e81, 0x5f16be58, 0xe707c658, 0x49eae5c7, 0xd5d147ad, 0x9a7abdc3,
    0xe8ac7fc7, 0x84ec3aae, 0xc24942d0, 0x294aa318, 0x08ac3d18, 0x8894042e, 0xb24609b0, 0x9bcaab58,
    0xc400f712, 0xd5c512b8, 0x2c02cc62, 0x25080fd8, 0xed74a847, 0x18a5ec5e, 0x9850ec6d, 0xf8909758,
    0x7f56f481, 0x4496f23c, 0xae27784f, 0xcb7cd93e, 0x06e32860, 0x50b9a84f, 0x3660434a, 0x09161f5f,
    0x900486bc, 0x08055459, 0xe7ec1017, 0x7e39494c, 0x4f443b25, 0x14751a8a, 0x717d03d4, 0xbd0e24d8,
    0x054b6f56, 0x854c496c, 0xd92a454a, 0xc39bd054, 0x6093614b, 0x9dbad754, 0x5bf0604a, 0x99f22305
};

class CMainParams : public CChainParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;
        vAlertPubKey = ParseHex("04fc9702847840aaf195de8442ebecedf5b095cdbb9bc716bda9110971b28a49e0ead8564ff0db22209e0374782c093bb899692d524e9d6a6956e7c5ecbcd68284");
        nDefaultPort = 8333;
        nRPCPort = 8332;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 32);
        nSubsidyHalvingInterval = 210000;

        // Build the genesis block. Note that the output of the genesis coinbase cannot
        // be spent as it did not originally exist in the database.
        //
        // CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
        //   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
        //     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
        //   vMerkleTree: 4a5e1e
        const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 1;
        genesis.nTime    = 1231006505;
        genesis.nBits    = 0x1d00ffff;
        genesis.nNonce   = 2083236893;

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"));
        assert(genesis.hashMerkleRoot == uint256("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vSeeds.push_back(CDNSSeedData("bitcoin.sipa.be", "seed.bitcoin.sipa.be"));
        vSeeds.push_back(CDNSSeedData("bluematt.me", "dnsseed.bluematt.me"));
        vSeeds.push_back(CDNSSeedData("dashjr.org", "dnsseed.bitcoin.dashjr.org"));
        vSeeds.push_back(CDNSSeedData("bitcoinstats.com", "seed.bitcoinstats.com"));
        vSeeds.push_back(CDNSSeedData("xf2.org", "bitseed.xf2.org"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of(0);
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
        pchMessageStart[0] = 0x0b;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x09;
        pchMessageStart[3] = 0x07;
        vAlertPubKey = ParseHex("04302390343f91cc401d56d68b123028bf52e5fca1939df127f63c6467cdf9c8e2c14b61104cf817d0b780da337893ecc4aaff1309e536162dabbdb45200ca2b0a");
        nDefaultPort = 18333;
        nRPCPort = 18332;
        strDataDir = "testnet3";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1296688602;
        genesis.nNonce = 414098458;
        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943"));

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
        assert(hashGenesisBlock == uint256("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));

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
