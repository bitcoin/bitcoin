// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "assert.h"
#include "core.h"
#include "protocol.h"
#include "util.h"
#include "subsidylevels.h"
#include "base58.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

//
// Main network
//

unsigned int bitcredit_pnSeed[] =
{
		0xfcdb9a36, 0xaf9f9a36, 0x06329a36, 0x2b78ae36, 0x4080ae36, 0xb689ae36,
};
unsigned int bitcoin_pnSeed[] =
{
    0x7e6a692e, 0x7d04d1a2, 0x6c0c17d9, 0xdb330ab9, 0xc649c7c6, 0x7895484d, 0x047109b0, 0xb90ca5bc,
    0xd130805f, 0xbd074ea6, 0x578ff1c0, 0x286e09b0, 0xd4dcaf42, 0x529b6bb8, 0x635cc6c0, 0xedde892e,
    0xa976d9c7, 0xea91a4b8, 0x03fa4eb2, 0x6ca9008d, 0xaf62c825, 0x93f3ba51, 0xc2c9efd5, 0x0ed5175e,
    0x487028bc, 0x7297c225, 0x8af0c658, 0x2e57ba1f, 0xd0098abc, 0x46a8853e, 0xcc92dc3e, 0xeb6f1955,
    0x8cce175e, 0x237281ae, 0x9d42795b, 0x4f4f0905, 0xc50151d0, 0xb1ba90c6, 0xaed7175e, 0x204de55b,
    0x4bb03245, 0x932b28bc, 0x2dcce65b, 0xe2708abc, 0x1b08b8d5, 0x12a3dc5b, 0x8a884c90, 0xa386a8b8,
    0x18e417c6, 0x2e709ac3, 0xeb62e925, 0x6f6503ae, 0x05d0814e, 0x8a9ac545, 0x946fd65e, 0x3f57495d,
    0x4a29c658, 0xad454c90, 0x15340905, 0x4c3f3b25, 0x01fe19b9, 0x5620595b, 0x443c795b, 0x44f24ac8,
    0x0442464e, 0xc8665882, 0xed3f3ec3, 0xf585bf5d, 0x5dd141da, 0xf93a084e, 0x1264dd52, 0x0711c658,
    0xf12e7bbe, 0x5b02b740, 0x7d526dd5, 0x0cb04c90, 0x2abe1132, 0x61a39f58, 0x044a0618, 0xf3af7dce,
    0xb994c96d, 0x361c5058, 0xca735d53, 0xeca743b0, 0xec790905, 0xc4d37845, 0xa1c4a2b2, 0x726fd453,
    0x625cc6c0, 0x6c20132e, 0xb7aa0c79, 0xc6ed983d, 0x47e4cbc0, 0xa4ac75d4, 0xe2e59345, 0x4d784ad0,
    0x18a5ec5e, 0x481cc85b, 0x7c6c2fd5, 0x5e4d6018, 0x5b4b6c18, 0xd99b4c90, 0xe63987dc, 0xb817bb25,
    0x141cfeb2, 0x5f005058, 0x0d987f47, 0x242a496d, 0x3e519bc0, 0x02b2454b, 0xdfaf3dc6, 0x888128bc,
    0x1165bb25, 0xabfeca5b, 0x2ef63540, 0x5773c7c6, 0x1280dd52, 0x8ebcacd9, 0x81c439c6, 0x39fcfa45,
    0x62177d41, 0xc975ed62, 0x05cff476, 0xdabda743, 0xaa1ac24e, 0xe255a22e, 0x88aac705, 0xe707c658,
    0xa9e94b5e, 0x2893484b, 0x99512705, 0xd63970ca, 0x45994f32, 0xe519a8ad, 0x92e25f5d, 0x8b84a9c1,
    0x5eaa0a05, 0xa74de55b, 0xb090ff62, 0x5eee326c, 0xc331a679, 0xc1d9b72e, 0x0c6ab982, 0x7362bb25,
    0x4cfedd42, 0x1e09a032, 0xa4c34c5e, 0x3777d9c7, 0x5edcf260, 0x3ce2b548, 0xd2ac0360, 0x2f80b992,
    0x3e4cbb25, 0x3995e236, 0xd03977ae, 0x953cf054, 0x3c654ed0, 0x74024c90, 0xa14f1155, 0x14ce0125,
    0xc15ebb6a, 0x2c08c452, 0xc7fd0652, 0x7604f8ce, 0xffb38332, 0xa4c2efd5, 0xe9614018, 0xab49e557,
    0x1648c052, 0x36024047, 0x0e8cffad, 0x21918953, 0xb61f50ad, 0x9b406b59, 0xaf282218, 0x7f1d164e,
    0x1f560da2, 0xe237be58, 0xbdeb1955, 0x6c0717d9, 0xdaf8ce62, 0x0f74246c, 0xdee95243, 0xf23f1a56,
    0x61bdf867, 0xd254c854, 0xc4422e4e, 0xae0563c0, 0xbdb9a95f, 0xa9eb32c6, 0xd9943950, 0x116add52,
    0x73a54c90, 0xb36b525e, 0xd734175e, 0x333d7f76, 0x51431bc6, 0x084ae5cf, 0xa60a236c, 0x5c67692e,
    0x0177cf45, 0xa6683ac6, 0x7ff4ea47, 0x2192fab2, 0xa03a0f46, 0xfe3e39ae, 0x2cce5fc1, 0xc8a6c148,
    0x96fb7e4c, 0x0a66c752, 0x6b4d2705, 0xeba0c118, 0x3ba0795b, 0x1dccd23e, 0x6912f3a2, 0x22f23c41,
    0x65646b4a, 0x8b9f8705, 0xeb9b9a95, 0x79fe6b4e, 0x0536f447, 0x23224d61, 0x5d952ec6, 0x0cb4f736,
    0xdc14be6d, 0xb24609b0, 0xd3f79b62, 0x6518c836, 0x83a3cf42, 0x9b641fb0, 0x17fef1c0, 0xd508cc82,
    0x91a4369b, 0x39cb4a4c, 0xbbc9536c, 0xaf64c44a, 0x605eca50, 0x0c6a6805, 0xd07e9d4e, 0x78e6d3a2,
    0x1b31eb6d, 0xaa01feb2, 0x4603c236, 0x1ecba3b6, 0x0effe336, 0xc3fdcb36, 0xc290036f, 0x4464692e,
    0x1aca7589, 0x59a9e52e, 0x19aa7489, 0x2622c85e, 0xa598d318, 0x438ec345, 0xc79619b9, 0xaf570360,
    0x5098e289, 0x36add862, 0x83c1a2b2, 0x969d0905, 0xcf3d156c, 0x49c1a445, 0xbd0b7562, 0x8fff1955,
    0x1e51fe53, 0x28d6efd5, 0x2837cc62, 0x02f42d42, 0x070e3fb2, 0xbcb18705, 0x14a4e15b, 0x82096844,
    0xcfcb1c2e, 0x37e27fc7, 0x07923748, 0x0c14bc2e, 0x26100905, 0xcb7cd93e, 0x3bc0d2c0, 0x97131b4c,
    0x6f1e5c17, 0xa7939f43, 0xb7a0bf58, 0xafa83a47, 0xcbb83f32, 0x5f321cb0, 0x52d6c3c7, 0xdeac5bc7,
    0x2cf310cc, 0x108a2bc3, 0x726fa14f, 0x85bad2cc, 0x459e4c90, 0x1a08b8d8, 0xcd7048c6, 0x6d5b4c90,
    0xa66cfe7b, 0xad730905, 0xdaac5bc7, 0x8417fd9f, 0x41377432, 0x1f138632, 0x295a12b2, 0x7ac031b2,
    0x3a87d295, 0xe219bc2e, 0xf485d295, 0x137b6405, 0xcfffd9ad, 0xafe20844, 0x32679a5f, 0xa431c644,
    0x0e5fce8c, 0x305ef853, 0xad26ca32, 0xd9d21a54, 0xddd0d736, 0xc24ec0c7, 0x4aadcd5b, 0x49109852,
    0x9d6b3ac6, 0xf0aa1e8b, 0xf1bfa343, 0x8a30c0ad, 0x260f93d4, 0x2339e760, 0x8869959f, 0xc207216c,
    0x29453448, 0xb651ec36, 0x45496259, 0xa23d1bcc, 0xb39bcf43, 0xa1d29432, 0x3507c658, 0x4a88dd62,
    0x27aff363, 0x7498ea6d, 0x4a6785d5, 0x5e6d47c2, 0x3baba542, 0x045a37ae, 0xa24dc0c7, 0xe981ea4d,
    0xed6ce217, 0x857214c6, 0x6b6c0464, 0x5a4945b8, 0x12f24742, 0xf35f42ad, 0xfd0f5a4e, 0xfb081556,
    0xb24b5861, 0x2e114146, 0xb7780905, 0x33bb0e48, 0x39e26556, 0xa794484d, 0x4225424d, 0x3003795b,
    0x31c8cf44, 0xd65bad59, 0x127bc648, 0xf2bc4d4c, 0x0273dc50, 0x4572d736, 0x064bf653, 0xcdcd126c,
    0x608281ae, 0x4d130087, 0x1016f725, 0xba185fc0, 0x16c1a84f, 0xfb697252, 0xa2942360, 0x53083b6c,
    0x0583f1c0, 0x2d5a2441, 0xc172aa43, 0xcd11cf36, 0x7b14ed62, 0x5c94f1c0, 0x7c23132e, 0x39965a6f,
    0x7890e24e, 0xa38ec447, 0xc187f1c0, 0xef80b647, 0xf20a7432, 0x7ad1d8d2, 0x869e2ec6, 0xccdb5c5d,
    0x9d11f636, 0x2161bb25, 0x7599f889, 0x2265ecad, 0x0f4f0e55, 0x7d25854a, 0xf857e360, 0xf83f3d6c,
    0x9cc93bb8, 0x02716857, 0x5dd8a177, 0x8adc6cd4, 0xe5613d46, 0x6a734f50, 0x2a5c3bae, 0x4a04c3d1,
    0xe4613d46, 0x8426f4bc, 0x3e1b5fc0, 0x0d5a3c18, 0xd0f6d154, 0x21c7ff5e, 0xeb3f3d6c, 0x9da5edc0,
    0x5d753b81, 0x0d8d53d4, 0x2613f018, 0x4443698d, 0x8ca1edcd, 0x10ed3f4e, 0x789b403a, 0x7b984a4b,
    0x964ebc25, 0x7520ee60, 0x4f4828bc, 0x115c407d, 0x32dd0667, 0xa741715e, 0x1d3f3532, 0x817d1f56,
    0x2f99a552, 0x6b2a5956, 0x8d4f4f05, 0xd23c1e17, 0x98993748, 0x2c92e536, 0x237ebdc3, 0xa762fb43,
    0x32016b71, 0xd0e7cf79, 0x7d35bdd5, 0x53dac3d2, 0x31016b71, 0x7fb8f8ce, 0x9a38c232, 0xefaa42ad,
    0x876b823d, 0x18175347, 0xdb46597d, 0xd2c168da, 0xcd6fe9dc, 0x45272e4e, 0x8d4bca5b, 0xa4043d47,
    0xaab7aa47, 0x202881ae, 0xa4aef160, 0xecd7e6bc, 0x391359ad, 0xd8cc9318, 0xbbeee52e, 0x077067b0,
    0xebd39d62, 0x0cedc547, 0x23d3e15e, 0xa5a81318, 0x179a32c6, 0xe4d3483d, 0x03680905, 0xe8018abc,
    0xdde9ef5b, 0x438b8705, 0xb48224a0, 0xcbd69218, 0x9075795b, 0xc6411c3e, 0x03833f5c, 0xf33f8b5e,
    0x495e464b, 0x83c8e65b, 0xac09cd25, 0xdaabc547, 0x7665a553, 0xc5263718, 0x2fd0c5cd, 0x22224d61,
    0x3e954048, 0xfaa37557, 0x36dbc658, 0xa81453d0, 0x5a941f5d, 0xa598ea60, 0x65384ac6, 0x10aaa545,
    0xaaab795b, 0xdda7024c, 0x0966f4c6, 0x68571c08, 0x8b40ee59, 0x33ac096c, 0x844b4c4b, 0xd392254d,
    0xba4d5a46, 0x63029653, 0xf655f636, 0xbe4c4bb1, 0x45dad036, 0x204bc052, 0x06c3a2b2, 0xf31fba6a,
    0xb21f09b0, 0x540d0751, 0xc7b46a57, 0x6a11795b, 0x3d514045, 0x0318aa6d, 0x30306ec3, 0x5c077432,
    0x259ae46d, 0x82bbd35f, 0xae4222c0, 0x254415d4, 0xbd5f574b, 0xd8fd175e, 0x0a3f38c3, 0x2dce6bb8,
    0xc201d058, 0x17fca5bc, 0xe8453cca, 0xd361f636, 0xa0d9edc0, 0x2f232e4e, 0x134e116c, 0x61ddc058,
    0x05ba7283, 0xe1f7ed5b, 0x040ec452, 0x4b672e4e, 0xe4efa36d, 0x47dca52e, 0xe9332e4e, 0xa3acb992,
    0x24714c90, 0xa8cc8632, 0x26b1ce6d, 0x264e53d4, 0xd3d2718c, 0x225534ad, 0xe289f3a2, 0x87341717,
    0x9255ad4f, 0x184bbb25, 0x885c7abc, 0x3a6e9ac6, 0x1924185e, 0xb73d4c90, 0x946d807a, 0xa0d78e3f,
    0x5a16bb25, 0xcb09795b, 0x8d0de657, 0x630b8b25, 0xe572c6cf, 0x2b3f1118, 0x4242a91f, 0x32990905,
    0x058b0905, 0xe266fc60, 0xbe66c5b0, 0xcc98e46d, 0x698c943e, 0x44bd0cc3, 0x865c7abc, 0x771764d3,
    0x4675d655, 0x354e4826, 0xb67ac152, 0xaeccf285, 0xea625b4e, 0xbcd6031f, 0x5e81eb18, 0x74b347ce,
    0x3ca56ac1, 0x54ee4546, 0x38a8175e, 0xa3c21155, 0x2f01576d, 0x5d7ade50, 0xa003ae48, 0x2bc1d31f,
    0x13f5094c, 0x7ab32648, 0x542e9fd5, 0x53136bc1, 0x7fdf51c0, 0x802197b2, 0xa2d2cc5b, 0x6b5f4bc0,
};

Credits_CTransaction createBitcreditTransaction(const char* publicKeyTo, COutPoint outPoint, int64_t nSubsidy) {
    Credits_CTransaction txNew;
    //Add tx in
	Credits_CTxIn ctxIn;
	ctxIn.prevout = outPoint;
	txNew.vin.push_back(ctxIn);
	//Add tx out
    const std::vector<unsigned char> vchProofOfDepositPublicKey = ParseHex(publicKeyTo);
    CPubKey pubKeyTo(vchProofOfDepositPublicKey);
    CBitcoinAddress adressTo(pubKeyTo.GetID());
    assert(adressTo.IsValid());
    CScript scriptPubKey;
    scriptPubKey.SetDestination(adressTo.Get());
	CTxOut ctxOut(nSubsidy, scriptPubKey);
	txNew.vout.push_back(ctxOut);
    return txNew;
}

Bitcredit_CMainParams::Bitcredit_CMainParams() {
    // The message start string is designed to be unlikely to occur in normal data.
    // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
    // a large 4-byte int at any alignment.
    pchMessageStart[0] = 0xbe;
    pchMessageStart[1] = 0xf9;
    pchMessageStart[2] = 0xd9;
    pchMessageStart[3] = 0xb4;
    addrFileName = "credits_peers.dat";
    vAlertPubKey = ParseHex("041604c6a3b185d784199e836841099a7f84fd15e9ed467350f58d2b1d0db3bce10c9527480dc35dd8415b9d86a9b021dc83d5e11e4ff0738e751a68e7a1c860a");
    nDefaultPort = 9333;
    nRPCPort = 9332;
    nClientVersion = CREDITS_CLIENT_VERSION;
    nAcceptDepthLinkedBitcoinBlock = 10000;
    nDepositLockDepth = 15000;
    bnProofOfWorkLimit = ~uint256(0) >> 32;

    vSubsidyLevels = createMainSubsidyLevels();
    uint64_t nSubsidyLevel = vSubsidyLevels[0].nSubsidyUpdateTo;

    const char * pubKeyCoinbaseTo = "047a5285d161909416706759f6351ae2954d5d420cfda656570d093161ae8cd8ab0d3c26499077f1d9a8c226eb3920d04ea8bf57f5e7fc03eb47948a1a6b1a1f80";
	Credits_CTransaction txCoinbase = createBitcreditTransaction(pubKeyCoinbaseTo, COutPoint(0, -1), nSubsidyLevel);
	txCoinbase.nTxType = TX_TYPE_COINBASE;
	const char* pszTimestampCoinbase = "The Times 5/Mar/2015 Currency wars threaten new catastrophe";
    txCoinbase.vin[0].scriptSig = CScript() << vector<unsigned char>((const unsigned char*)pszTimestampCoinbase, (const unsigned char*)pszTimestampCoinbase + strlen(pszTimestampCoinbase));
    genesis.vtx.push_back(txCoinbase);

    Credits_CTransaction txDeposit = createBitcreditTransaction(pubKeyCoinbaseTo, COutPoint(txCoinbase.GetHash(), 0), nSubsidyLevel);
    txDeposit.nTxType = TX_TYPE_DEPOSIT;
    const char * scriptSigDeposit = "4730440220789140d0d9f223e86a45faf177a37d5a1cdf8359effe68f5f4001cd637053ffd0220422008273df89ed110583fa62cea281f47af5a4c62f08820026c48716a23766f0141047a5285d161909416706759f6351ae2954d5d420cfda656570d093161ae8cd8ab0d3c26499077f1d9a8c226eb3920d04ea8bf57f5e7fc03eb47948a1a6b1a1f80";
    std::vector<unsigned char> vScriptSigDeposit = ParseHex(scriptSigDeposit);
    txDeposit.vin[0].scriptSig = CScript(vScriptSigDeposit.begin(), vScriptSigDeposit.end());

    const char * pubKeyDepositSign = "04feb1860738edb472a8ea1c111de8af4b29da3456de6a5580f476abb09f4374fb5025dad3ab72268e7ed85ec0bb6174b6ce4924184b28fcea6b33c94f0db458a7";
    const std::vector<unsigned char> vchpubKeyDepositSign = ParseHex(pubKeyDepositSign);
    txDeposit.signingKeyId = CPubKey(vchpubKeyDepositSign).GetID();

    genesis.vtx.push_back(txDeposit);

    genesis.hashPrevBlock = 0;
    genesis.nVersion = 1;
    genesis.nTime    = 1425665088;
    genesis.nBits    = 0x1d00ffff;
    genesis.nNonce   = 3319178079;
    genesis.nTotalMonetaryBase = nSubsidyLevel;
    genesis.nTotalDepositBase = nSubsidyLevel;
    genesis.nDepositAmount = nSubsidyLevel;
    genesis.hashLinkedBitcoinBlock   =  uint256("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");

    genesis.RecalcLockHashAndMerkleRoot();

    genesis.vsig.clear();
    const char * cDepositSignature = "1c5179b838d0f7e1e919671a87f528b9d7b1d0ca50fe2686e150553834fa9a293e22378a6c36537153697785db84af14906c5ac92c2bb71b295e87ec4435434ecf";
	const std::vector<unsigned char> vchDepositSignature = ParseHex(cDepositSignature);
    genesis.vsig.push_back(CCompactSignature(vchDepositSignature));
    genesis.hashSigMerkleRoot = genesis.BuildSigMerkleTree();

    hashGenesisBlock = genesis.GetHash();

    assert(hashGenesisBlock == uint256("0x00000000983b1b95f3b177e948e48591c939c17e8764cb72008ee0cf21308a45"));
    //This is a little endian (byte swapped) merkle root
    assert(genesis.hashMerkleRoot == uint256("0x3839ef2433e3e091d1ba0ea97fc9192b43fb08e38f178ef434618ab0673222b4"));
    assert(genesis.hashSigMerkleRoot == uint256("0x2fb2f1637af1544e7949a62cedcf4d2b918fb9bb42a39150dcc5f9943ff7d9f2"));

    vSeeds.push_back(CDNSSeedData("bitcredit-currency.org", "dnsseed.bitcredit-currency.org"));
    vSeeds.push_back(CDNSSeedData("xlre4.org", "seed.xlre4.org"));
    vSeeds.push_back(CDNSSeedData("crenode.org", "seed.crenode.org"));

    base58Prefixes[PUBKEY_ADDRESS] = list_of(0);
    base58Prefixes[SCRIPT_ADDRESS] = list_of(5);
    base58Prefixes[SECRET_KEY] =     list_of(128);
    base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E);
    base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4);

    // Convert the pnSeeds array into usable address objects.
    for (unsigned int i = 0; i < ARRAYLEN(bitcredit_pnSeed); i++)
    {
        // It'll only connect to one or two seed nodes because once it connects,
        // it'll get a pile of addresses with newer timestamps.
        // Seed nodes are given a random 'last seen time' of between one and two
        // weeks ago.
        const int64_t nOneWeek = 7*24*60*60;
        struct in_addr ip;
        memcpy(&ip, &bitcredit_pnSeed[i], sizeof(ip));
        CAddress addr(CService(ip, GetDefaultPort()));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vFixedSeeds.push_back(addr);
    }
}

Bitcoin_CMainParams::Bitcoin_CMainParams() {
    pchMessageStart[0] = 0xf9;
    pchMessageStart[1] = 0xbe;
    pchMessageStart[2] = 0xb4;
    pchMessageStart[3] = 0xd9;
    addrFileName = "bitcoin_peers.dat";
    vAlertPubKey = ParseHex("041604c6a3b185d784199e836841099a7f84fd15e9ed467350f58d2b1d0db3bce10c9527480dc35dd8415b9d86a9b021dc83d5e11e4ff0738e751a68e7a1c860a");
    nDefaultPort = 8333;
    nRPCPort = 8332;
    //Hardcoded linkage to bitcoin "real" version
    nClientVersion = 1000000 * 	0
    							+ 10000 * 	9
    							+ 100 * 		1
    							+ 1 * 			0;
    nAcceptDepthLinkedBitcoinBlock = 10000;
    nDepositLockDepth = 15000;
    bnProofOfWorkLimit = ~uint256(0) >> 32;
    nSubsidyHalvingInterval = 210000;

    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    Bitcoin_CTransaction txNew;
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
    genesis.nBits    = 0x1d00ffff;
    genesis.nNonce   = 2083236893;

    hashGenesisBlock = genesis.GetHash();
    assert(hashGenesisBlock == uint256("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"));
    assert(genesis.hashMerkleRoot == uint256("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

    vSeeds.push_back(CDNSSeedData("bitcoin.sipa.be", "seed.bitcoin.sipa.be"));
    vSeeds.push_back(CDNSSeedData("bluematt.me", "dnsseed.bluematt.me"));
    vSeeds.push_back(CDNSSeedData("dashjr.org", "dnsseed.bitcoin.dashjr.org"));
    vSeeds.push_back(CDNSSeedData("bitcoinstats.com", "seed.bitcoinstats.com"));
    vSeeds.push_back(CDNSSeedData("bitnodes.io", "seed.bitnodes.io"));
    vSeeds.push_back(CDNSSeedData("xf2.org", "bitseed.xf2.org"));

    base58Prefixes[PUBKEY_ADDRESS] = list_of(0);
    base58Prefixes[SCRIPT_ADDRESS] = list_of(5);
    base58Prefixes[SECRET_KEY] =     list_of(128);
    base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E);
    base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4);

    // Convert the pnSeeds array into usable address objects.
    for (unsigned int i = 0; i < ARRAYLEN(bitcoin_pnSeed); i++)
    {
        const int64_t nOneWeek = 7*24*60*60;
        struct in_addr ip;
        memcpy(&ip, &bitcoin_pnSeed[i], sizeof(ip));
        CAddress addr(CService(ip, GetDefaultPort()));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vFixedSeeds.push_back(addr);
    }
}

static Bitcredit_CMainParams bitcredit_mainParams;
static Bitcoin_CMainParams bitcoin_mainParams;


//
// Testnet (v3)
//
class Bitcredit_CTestNetParams : public Bitcredit_CMainParams {
public:
    Bitcredit_CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.

        pchMessageStart[0] = 0x11;
        pchMessageStart[1] = 0x0b;
        pchMessageStart[2] = 0x07;
        pchMessageStart[3] = 0x09;
        vAlertPubKey = ParseHex("04384667400b96afd79e8e8814160f921a6e309a13a3ee789029f254175e0e462b3d705248c00430feac36da45005cccca8b15ef98e54920c74e12f5391cc592fa");
        nDefaultPort = 19333;
        nRPCPort = 19332;
        nAcceptDepthLinkedBitcoinBlock = 100;
        nDepositLockDepth = 200;
        strDataDir = "testnet3";

        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1425665090;
        genesis.nNonce = 282806301;
        genesis.hashLinkedBitcoinBlock =  uint256("0x000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943");

        genesis.RecalcLockHashAndMerkleRoot();

        genesis.vsig.clear();
        const char * cDepositSignature = "1b018a9bacc876b8c1762820662ad1d7ed5125dc0106b863dc72432b1c97fbbec0090bdfa6a94a6ee13f7753a40eeb2e075078e97742091d193d37bd71e8e6f2f7";
    	const std::vector<unsigned char> vchDepositSignature = ParseHex(cDepositSignature);
        genesis.vsig.push_back(CCompactSignature(vchDepositSignature));
        genesis.hashSigMerkleRoot = genesis.BuildSigMerkleTree();

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0000000092999ac46443e5fbdc4510ba3bc7e9826b5bda69fe4e8ba0476951ce"));
        //This is a little endian (byte swapped) merkle root
        assert(genesis.hashMerkleRoot == uint256("0xcf46057fbf34163273f775b705d9d471a19667724ca6e9b7715b394a442d7867"));
        assert(genesis.hashSigMerkleRoot == uint256("0x9e9771cd560287077c228fffed1ee3e1f389f9a292a482797ebf28803d7fc2f4"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("bitcredit-currency.org", "testnet-seed.bitcredit-currency.org"));

        base58Prefixes[PUBKEY_ADDRESS] = list_of(111);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(196);
        base58Prefixes[SECRET_KEY]     = list_of(239);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94);
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
    virtual int FastForwardClaimBitcoinBlockHeight() const { return 349024; }
    virtual uint256 FastForwardClaimBitcoinBlockHash() const { return uint256("0x000000009097317e3fda5d3f5b442cd32de854adddb5c3baa0109946abc04b3b"); }
};
class Bitcoin_CTestNetParams : public Bitcoin_CMainParams {
public:
	Bitcoin_CTestNetParams() {
         pchMessageStart[0] = 0x0b;
         pchMessageStart[1] = 0x11;
         pchMessageStart[2] = 0x09;
         pchMessageStart[3] = 0x07;
         vAlertPubKey = ParseHex("04384667400b96afd79e8e8814160f921a6e309a13a3ee789029f254175e0e462b3d705248c00430feac36da45005cccca8b15ef98e54920c74e12f5391cc592fa");
         nDefaultPort = 18333;
         nRPCPort = 18332;
         nAcceptDepthLinkedBitcoinBlock = 100;
         nDepositLockDepth = 200;
         strDataDir = "testnet3";

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
static Bitcredit_CTestNetParams bitcredit_testNetParams;
static Bitcoin_CTestNetParams bitcoin_testNetParams;


//
// Regression test
//
class Bitcredit_CRegTestParams : public Bitcredit_CTestNetParams {
public:
    Bitcredit_CRegTestParams() {
        pchMessageStart[0] = 0xbf;
        pchMessageStart[1] = 0xfa;
        pchMessageStart[2] = 0xda;
        pchMessageStart[3] = 0xb5;
        vSubsidyLevels = createRegTestSubsidyLevels();
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1425665088;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 11397;
        nDefaultPort = 19444;
        strDataDir = "regtest";

        genesis.RecalcLockHashAndMerkleRoot();

        genesis.vsig.clear();
        const char * cDepositSignature = "1b90fad20c4b467b5ba339dad2bddc2ccd4fddc741704da94d80917471962017f3de9325713104ad770ee8be4e4e2d84c19bb9f3d23b154c63e42c4ad1d44adee5";
    	const std::vector<unsigned char> vchDepositSignature = ParseHex(cDepositSignature);
        genesis.vsig.push_back(CCompactSignature(vchDepositSignature));
        genesis.hashSigMerkleRoot = genesis.BuildSigMerkleTree();

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == uint256("0x0000013459dd9e819a9fff403f00f87c5bc1b8e4259d204054f97ac8d18f0a3c"));
        //This is a little endian (byte swapped) merkle root
        assert(genesis.hashMerkleRoot == uint256("0xbd7842448f2d1f64c88cce9da440d9dc1f0fdcb3fa120f6ef3e0be607ab04d21"));
        assert(genesis.hashSigMerkleRoot == uint256("0xd06996c0f4a7a125d65ae29003e33a917608a9563589013cae1887e0cd06bca3"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
    virtual int FastForwardClaimBitcoinBlockHeight() const { return 0; }
    virtual uint256 FastForwardClaimBitcoinBlockHash() const { return uint256(1); }
};
class Bitcoin_CRegTestParams : public Bitcoin_CTestNetParams {
public:
	Bitcoin_CRegTestParams() {
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nSubsidyHalvingInterval = 150;
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        genesis.nTime = 1296688602;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 2;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        strDataDir = "regtest";
        assert(hashGenesisBlock == uint256("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));

        vSeeds.clear();
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static Bitcredit_CRegTestParams bitcredit_regTestParams;
static Bitcoin_CRegTestParams bitcoin_regTestParams;

static Bitcredit_CMainParams *bitcredit_pCurrentParams = &bitcredit_mainParams;
static Bitcoin_CMainParams *bitcoin_pCurrentParams = &bitcoin_mainParams;

const Bitcredit_CMainParams &Bitcredit_Params() {
    return *bitcredit_pCurrentParams;
}
const Bitcoin_CMainParams &Bitcoin_Params() {
    return *bitcoin_pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            bitcredit_pCurrentParams = &bitcredit_mainParams;
            bitcoin_pCurrentParams = &bitcoin_mainParams;
            break;
        case CChainParams::TESTNET:
            bitcredit_pCurrentParams = &bitcredit_testNetParams;
            bitcoin_pCurrentParams = &bitcoin_testNetParams;
            break;
        case CChainParams::REGTEST:
            bitcredit_pCurrentParams = &bitcredit_regTestParams;
        	//Setting bitcoin to always go for testNetParams when running credits in regtest
            //If not, there would be no bitcoin blockchain to run in parallell with
        	bitcoin_pCurrentParams = &bitcoin_testNetParams;
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

bool FastForwardClaimStateFor(const int nBitcoinBlockHeight, const uint256 nBitcoinBlockHash) {
	if(Bitcredit_Params().FastForwardClaimBitcoinBlockHeight() == 0) {
		return false;
	}
	if(nBitcoinBlockHeight < Bitcredit_Params().FastForwardClaimBitcoinBlockHeight()) {
		return true;
	}
	if(nBitcoinBlockHeight > Bitcredit_Params().FastForwardClaimBitcoinBlockHeight()) {
		return false;
	}
	assert(nBitcoinBlockHash == Bitcredit_Params().FastForwardClaimBitcoinBlockHash());
	return true;
}
