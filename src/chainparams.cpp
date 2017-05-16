// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"
#include "utilmoneystr.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"
#include "chainparamsimport.h"

int64_t CChainParams::GetProofOfStakeReward(const CBlockIndex *pindexPrev, int64_t nFees) const
{
    int64_t nSubsidy;
    

    nSubsidy = (pindexPrev->nMoneySupply / COIN) * GetCoinYearReward() / (365 * 24 * (60 * 60 / nTargetSpacing));
    
    if (fDebug && GetBoolArg("-printcreation", false))
        LogPrintf("GetProofOfStakeReward(): create=%s\n", FormatMoney(nSubsidy).c_str());

    return nSubsidy + nFees;
};

bool CChainParams::CheckImportCoinbase(int nHeight, uint256 &hash) const
{
    for (auto &cth : Params().vImportedCoinbaseTxns)
    {
        if (cth.nHeight != nHeight)
            continue;
        
        if (hash == cth.hash)
            return true;
        return error("%s - Hash mismatch at height %d: %s, expect %s.", __func__, nHeight, hash.ToString(), cth.hash.ToString());
    };
    
    return error("%s - Unknown height.", __func__);
};

uint32_t CChainParams::GetStakeMinAge(int nHeight) const
{
    // StakeMinAge is not checked directly, nStakeMinConfirmations is checked in CheckProofOfStake
    if ((uint32_t)nHeight <= nStakeMinConfirmations) // smooth start for the chain. 
        return nHeight * nTargetSpacing;
    return nStakeMinAge;
};

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
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

std::pair<const char*, CAmount> regTestOutputs[15] = {
    std::make_pair("585c2b3914d9ee51f8e710304e386531c3abcc82", 10000 * COIN),
    std::make_pair("c33f3603ce7c46b423536f0434155dad8ee2aa1f", 10000 * COIN),
    std::make_pair("72d83540ed1dcf28bfaca3fa2ed77100c2808825", 10000 * COIN),
    std::make_pair("69e4cc4c219d8971a253cd5db69a0c99c4a5659d", 10000 * COIN),
    std::make_pair("eab5ed88d97e50c87615a015771e220ab0a0991a", 10000 * COIN),
    std::make_pair("119668a93761a34a4ba1c065794b26733975904f", 10000 * COIN),
    std::make_pair("6da49762a4402d199d41d5778fcb69de19abbe9f", 10000 * COIN),
    std::make_pair("27974d10ff5ba65052be7461d89ef2185acbe411", 10000 * COIN),
    std::make_pair("89ea3129b8dbf1238b20a50211d50d462a988f61", 10000 * COIN),
    std::make_pair("3baab5b42a409b7c6848a95dfd06ff792511d561", 10000 * COIN),
    
    std::make_pair("649b801848cc0c32993fb39927654969a5af27b0", 5000 * COIN),
    std::make_pair("d669de30fa30c3e64a0303cb13df12391a2f7256", 5000 * COIN),
    std::make_pair("f0c0e3ebe4a1334ed6a5e9c1e069ef425c529934", 5000 * COIN),
    std::make_pair("27189afe71ca423856de5f17538a069f22385422", 5000 * COIN),
    std::make_pair("0e7f6fe0c4a5a6a9bfd18f7effdd5898b1f40b80", 5000 * COIN),
};

static CBlock CreateGenesisBlockRegTest(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    
    CMutableTransaction txNew;
    txNew.nVersion = PARTICL_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);
    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;
    
    size_t nOutputs = 15;
    
    txNew.vpout.resize(nOutputs);
    
    for (size_t k = 0; k < nOutputs; ++k)
    {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = regTestOutputs[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(regTestOutputs[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    };
    
    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = PARTICL_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);
    
    return genesis;
}

const size_t nGenesisOutputs = 32;
std::pair<const char*, CAmount> genesisOutputs[nGenesisOutputs] = {
    std::make_pair("9a9231e5c0a554a685db12aecee333c7cf046013", 20000 * COIN),
    std::make_pair("94f5c1e7ac73e1b89fb6fd18d3f3e373ef29f22c", 7500 * COIN),
    std::make_pair("6e070085c66ad7eca2e2f28f2ab7373247c62b88", 7500 * COIN),
    std::make_pair("6747608f079d1c3f403112786a7276f2ced7cb13", 7500 * COIN),
    std::make_pair("779ba0f403e8c37cea0a8146a5cda3278ab769b7", 7500 * COIN),
    
    std::make_pair("b15dd479bf821822da7266f4cfcc05f721867710", 20000 * COIN),
    std::make_pair("a8d3e21a00bd62dc87145c86e781ab2ccc820100", 7500 * COIN),
    std::make_pair("abe9da9dd90b8b10b7ad54e0f0d57f06dfdb1dc9", 7500 * COIN),
    std::make_pair("28ae24813c29d41bb40c3d98f575fe1ff39fa1c8", 7500 * COIN),
    std::make_pair("2ef75d74fb829945dc590bb5c94a398f59e94b42", 7500 * COIN),
    
    std::make_pair("41eac8f22825e177c5652e6b3ec203410b74d8cf", 20000 * COIN),
    std::make_pair("ae58bcafcb7b314a329f470f6e233b011882ab18", 7500 * COIN),
    std::make_pair("4668b80445a43f42353cff1a28dbf879fd56ea4c", 7500 * COIN),
    std::make_pair("82285d1ed2466a01e3bd02458457bbc38d9ae054", 7500 * COIN),
    std::make_pair("76577cfa41dee36e9e0e77e797312c7ffbdb28f8", 7500 * COIN),
    
    std::make_pair("6d403ec086b10f274f809534b8c0b346bc0d67be", 20000 * COIN),
    std::make_pair("1a152fbcd5b2bc13aa5d4c69520c4b6222f12c4d", 7500 * COIN),
    std::make_pair("6fb9ed5d8d27b708464f36bb578f6ea3e3b9e0db", 7500 * COIN),
    std::make_pair("9188cbd49aaed91d3ff14dd6f128d8e9087cbc81", 7500 * COIN),
    std::make_pair("e306a3dd4e35a864ea14bcb1df46a44599554cb4", 7500 * COIN),
    
    std::make_pair("a0315fcc94465c1171621e95dae1c893f6260cd6", 20000 * COIN),
    std::make_pair("f2554bb0d11a097d90f44e171fa1ea3fe9589fd6", 7500 * COIN),
    std::make_pair("0403bd751b8c1a2bd8eb3b32dc7ed70cacaf1e28", 7500 * COIN),
    std::make_pair("ee578062b47417b0b44b2ce0ebb63fe73d77d51f", 7500 * COIN),
    std::make_pair("a107f606d54803401ebf9040886e3de683a36e9f", 7500 * COIN),
    
    std::make_pair("d080dbed1a3ff44ad332fb29a5e83befbf72a86a", 10000 * COIN),
    std::make_pair("d19a7fc0134502c51a2ff7ead665d37599402111", 10000 * COIN),
    std::make_pair("ca81e51bc7656a865c5e2a920bd3e38d7977e3d8", 6000 * COIN),
    std::make_pair("9cacfee0ebdd5160c4058010227efff20fdd3b7e", 6000 * COIN),
    std::make_pair("db79c38f850e73e6d871fbbec5a40af5176393a3", 6000 * COIN),
    std::make_pair("b3fd2dc0f3bf2cd03bdd51d91412f11f5eef8a40", 6000 * COIN),
    std::make_pair("7d9327e6f6542fe826ff50a457a7d1bcb8db1765", 6000 * COIN),
};

std::pair<const char*, CAmount> genesisOutputsTestnet[nGenesisOutputs] = {
    std::make_pair("16e1bc3d320d7cd8ee647d8f316c8ba14ebdc6e0", 20000 * COIN),
    std::make_pair("6041c6b843fd7321f8ee8d060464ed2f96cebfa9", 7500 * COIN),
    std::make_pair("7b8ee78e56af2bde5be75b0ea4dd9ab32183c9a8", 7500 * COIN),
    std::make_pair("09c43d9f284b1156322c80f11db3a645cc583c96", 7500 * COIN),
    std::make_pair("6c17afb3a913c8ebe3e65edadcc46f3cfc203f5c", 7500 * COIN),
    
    std::make_pair("ae726404c67380ff5e825f05f7a18b116dad52f5", 20000 * COIN),
    std::make_pair("f946b760809b53dac4848aadb8af5df68e80c863", 7500 * COIN),
    std::make_pair("ea89d369a88d17ec8332a01eb3c5bd8ba7c091f3", 7500 * COIN),
    std::make_pair("79cc005375e6ce47ee5409e526bc735735533ace", 7500 * COIN),
    std::make_pair("20d3c48848871222cec3d04f8776a80940b0130c", 7500 * COIN),
    
    std::make_pair("464d8028cd25b1c03630b29326235e46c45a0fce", 20000 * COIN),
    std::make_pair("8c8c41acd8b3e92ed8afd027af1ac1a01757b324", 7500 * COIN),
    std::make_pair("cde11c1fa83f4222728260695db973301b07cf43", 7500 * COIN),
    std::make_pair("5b93cfead42f3bfc035ed7acc63db596c5e44e89", 7500 * COIN),
    std::make_pair("7f9e5e8933c25faf9614126bfde15a96a1914ee7", 7500 * COIN),
    
    std::make_pair("2903e9dab211039a813c95755b38c0c2e64d9d08", 20000 * COIN),
    std::make_pair("3b6b7f6a69ac1b5ff149ed4c6de17825fc84f219", 7500 * COIN),
    std::make_pair("c6e7c1a5a7700754970c483355cb3fea4969eebe", 7500 * COIN),
    std::make_pair("28dc2c7e688f3dc58b6095d972d40237e2006a50", 7500 * COIN),
    std::make_pair("a05fc446cdc06625d593991de5556a33a713c723", 7500 * COIN),
    
    std::make_pair("79e2c3e58e5c3e0cd119ea900cd5df59e9dd5544", 20000 * COIN),
    std::make_pair("1bef2aeaca5645976705ed7bff7330fa78b1b68a", 7500 * COIN),
    std::make_pair("2dbcfd69622c37590f2ebe7a7c9d01f419a7244f", 7500 * COIN),
    std::make_pair("7411a25eaf494fe7af6289790632a242450465d1", 7500 * COIN),
    std::make_pair("018b583fa822209faf401bf1a50082c8e042d3ca", 7500 * COIN),
    
    std::make_pair("12e04025d07661e641493cb6109f2db48b3a3071", 10000 * COIN),
    std::make_pair("ccd7191dae450081be70337b0047ca82b268c286", 10000 * COIN),
    std::make_pair("fa97fa3c2fa6f7644ee4f5b775ff5288616a235b", 6000 * COIN),
    std::make_pair("cf9afe0a99eec9fc19eef45149f62e77d6b91f51", 6000 * COIN),
    std::make_pair("dc0adbadb3bc83895b86312928fff5ed882617e0", 6000 * COIN),
    std::make_pair("a0720a81483dd42459470abaa33c455282ce45b0", 6000 * COIN),
    std::make_pair("e292f976dcb35fad81073db7ad22d70ab217017b", 6000 * COIN),
};

static CBlock CreateGenesisBlockTestNet(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    
    CMutableTransaction txNew;
    txNew.nVersion = PARTICL_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);
    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;
    
    txNew.vpout.resize(nGenesisOutputs);
    for (size_t k = 0; k < nGenesisOutputs; ++k)
    {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = genesisOutputsTestnet[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(genesisOutputsTestnet[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    };
    
    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = PARTICL_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);
    
    return genesis;
}

static CBlock CreateGenesisBlockMainNet(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    
    CMutableTransaction txNew;
    txNew.nVersion = PARTICL_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);
    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;
    
    txNew.vpout.resize(nGenesisOutputs);
    for (size_t k = 0; k < nGenesisOutputs; ++k)
    {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = genesisOutputs[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(genesisOutputs[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    };
    
    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = PARTICL_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);
    
    return genesis;
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
        consensus.nBlockTargetSpacing = 2 * 60;
        
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256S("0x000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8");
        consensus.BIP65Height = 388381; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 363725; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        
        consensus.powLimit = uint256S("000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        
        
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 2 * 60; // 2 minutes
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
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        //consensus.defaultAssumeValid = uint256S("0x0000000000000000030abc968e1bd635736e880b946085c93152969b9a81a6e2"); //447235

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xfb;
        pchMessageStart[1] = 0xf2;
        pchMessageStart[2] = 0xef;
        pchMessageStart[3] = 0xb4;
        nDefaultPort = 51738;
        nBIP44ID = 0x8000002C;
        
        nModifierInterval = 10 * 60;    // 10 minutes
        nStakeMinConfirmations = 225;   // 225 * 2 minutes
        nTargetSpacing = 120;           // 2 minutes
        nTargetTimespan = 24 * 60;      // 24 mins
        nStakeMinAge = (nStakeMinConfirmations+1) * nTargetSpacing;
        
        AddImportHashesMain(vImportedCoinbaseTxns);
        SetLastImportHeight();
        
        nPruneAfterHeight = 100000;
        
        genesis = CreateGenesisBlockMainNet(1489325366, 34910, 0x1f00ffff);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0000be6337182d22a23622828c8e234933e0dd9f6b9b9faafbd449abbb1b0abf"));
        assert(genesis.hashMerkleRoot == uint256S("0xfa78a0476dc886dd4c8ebba34176f5561945c1c27e113c1989e0aca7f8f6de8d"));
        assert(genesis.hashWitnessMerkleRoot == uint256S("0x59c9ac2d5430e0ce2cc4b9848e956fcad208322169f77529bc509960fb9f2e33"));

        // Note that of those with the service bits flag, most only support a subset of possible options
        vSeeds.push_back(CDNSSeedData("mainnet.particl.io",  "mainnet.particl.io", true));
        

        base58Prefixes[PUBKEY_ADDRESS]     = std::vector<unsigned char>(1,56); // P
        base58Prefixes[SCRIPT_ADDRESS]     = std::vector<unsigned char>(1,60);
        base58Prefixes[SECRET_KEY]         = std::vector<unsigned char>(1,108);
        base58Prefixes[EXT_PUBLIC_KEY]     = boost::assign::list_of(0x69)(0x6e)(0x82)(0xd1).convert_to_container<std::vector<unsigned char> >(); // PPAR
        base58Prefixes[EXT_SECRET_KEY]     = boost::assign::list_of(0x8f)(0x1d)(0xae)(0xb8).convert_to_container<std::vector<unsigned char> >(); // XPAR
        base58Prefixes[STEALTH_ADDRESS]    = std::vector<unsigned char>(1,20);
        base58Prefixes[EXT_KEY_HASH]       = std::vector<unsigned char>(1,75); // X
        base58Prefixes[EXT_ACC_HASH]       = std::vector<unsigned char>(1,23); // A
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >(); // xpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >(); // xprv
        base58Prefixes[EXT_PUBLIC_KEY_SDC] = boost::assign::list_of(0xEE)(0x80)(0x28)(0x6A).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY_SDC] = boost::assign::list_of(0xEE)(0x80)(0x31)(0xE8).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        
        /*
        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 11111, uint256S("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d"))
            ( 33333, uint256S("0x000000002dd5588a74784eaa7ab0507a18ad16a236e7b1ce69f00d7ddfb5d0a6"))
            ( 74000, uint256S("0x0000000000573993a3c9e41ce34471c079dcf5f52a0e824a81e7f953b8661a20"))
            (105000, uint256S("0x00000000000291ce28027faea320c8d2b054b2e0fe44a773f3eefb151d6bdc97"))
            (134444, uint256S("0x00000000000005b12ffd4cd315cd34ffd4a594f430ac814c91184a0d42d2b0fe"))
            (168000, uint256S("0x000000000000099e61ea72015e79632f216fe6cb33d7899acb35b75c8303b763"))
            (193000, uint256S("0x000000000000059f452a5f7340de6682a977387c17010ff6e6c3bd83ca8b1317"))
            (210000, uint256S("0x000000000000048b95347e83192f69cf0366076336c639f9b7228e9ba171342e"))
            (216116, uint256S("0x00000000000001b4f4b433e81ee46494af945cf96014816a4e2370f11b23df4e"))
            (225430, uint256S("0x00000000000001c108384350f74090433e7fcf79a606b8e797f065b130575932"))
            (250000, uint256S("0x000000000000003887df1f29024b06fc2200b55f8af8f35453d7be294df2d214"))
            (279000, uint256S("0x0000000000000001ae8c72a0b0c301f67e3afca10e819efa9041e458e9bd7e40"))
            (295000, uint256S("0x00000000000000004d9b4ef50f0f9d686fd69db2e03af35a100370c64632a983"))
        };
        
        chainTxData = ChainTxData{
            // Data as of block 00000000000000000166d612d5595e2b1cd88d71d695fc580af64d8da8658c23 (height 446482).
            1483472411, // * UNIX timestamp of last known number of transactions
            184495391,  // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            3.2         // * estimated number of transactions per second after that timestamp
        };
        */
    }
    
    void SetOld()
    {
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        
        genesis = CreateGenesisBlock(1231006505, 2083236893, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
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
        consensus.nBlockTargetSpacing = 2 * 60;
        
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 21111;
        consensus.BIP34Hash = uint256S("0x0000000023b3a96d3484e5abb3755c413e7d41500f8e2a5c3f0dd01299cd8ef8");
        consensus.BIP65Height = 581885; // 00000000007f6655f22f98e72ed80d8b06dc761d5da09df0fa1dc4be4f861eb6
        consensus.BIP66Height = 330776; // 000000002104c8c45e99a8853285a3b592602a3ccde2b832481da85e9e4ba182
        consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
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
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1462060800; // May 1st 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1493596800; // May 1st 2017

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        //consensus.defaultAssumeValid = uint256S("0x000000000871ee6842d3648317ccc8a435eb8cc3c2429aee94faff9ba26b05a0"); //1043841
        
        pchMessageStart[0] = 0x08;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x05;
        pchMessageStart[3] = 0x0b;
        nDefaultPort = 51938;
        nBIP44ID = 0x80000001;
        
        nModifierInterval = 10 * 60;    // 10 minutes
        nStakeMinConfirmations = 225;   // 225 * 2 minutes
        nTargetSpacing = 120;           // 2 minutes
        nTargetTimespan = 24 * 60;      // 24 mins
        nStakeMinAge = (nStakeMinConfirmations+1) * nTargetSpacing;
        
        
        AddImportHashesTest(vImportedCoinbaseTxns);
        SetLastImportHeight();
        
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlockTestNet(1492356413, 86023, 0x1f00ffff);
        consensus.hashGenesisBlock = genesis.GetHash();
        
        assert(consensus.hashGenesisBlock == uint256S("0x00007cd5202e8dab3c384b2390e27b89db47647cd973c0913004b9b368cd1162"));
        assert(genesis.hashMerkleRoot == uint256S("0x1617bd7f2f93d11d065cd655440cd05080b11e06b31e95a9b08bde0f4ad34d9b"));
        assert(genesis.hashWitnessMerkleRoot == uint256S("0x05b596687297bf123fd70bf6229678b1eac163e47793579900a72217d22616aa"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.push_back(CDNSSeedData("testnet-seed.particl.io",  "testnet-seed.particl.io", true));
        
        /* TODO: add new seeds
        vSeeds.push_back(CDNSSeedData("main.shadow.cash",  "seed.shadow.cash"));
        vSeeds.push_back(CDNSSeedData("seed2.shadow.cash", "seed2.shadow.cash"));
        vSeeds.push_back(CDNSSeedData("seed3.shadow.cash", "seed3.shadow.cash"));
        vSeeds.push_back(CDNSSeedData("seed4.shadow.cash", "seed4.shadow.cash"));
        vSeeds.push_back(CDNSSeedData("shadowproject.io",  "seed.shadowproject.io"));
        vSeeds.push_back(CDNSSeedData("shadowchain.info",  "seed.shadowchain.info"));
        */

        base58Prefixes[PUBKEY_ADDRESS]     = std::vector<unsigned char>(1,118); // p
        base58Prefixes[SCRIPT_ADDRESS]     = std::vector<unsigned char>(1,122);
        base58Prefixes[SECRET_KEY]         = std::vector<unsigned char>(1,46);
        
        
        base58Prefixes[EXT_PUBLIC_KEY]     = boost::assign::list_of(0xe1)(0x42)(0x78)(0x00).convert_to_container<std::vector<unsigned char> >(); // ppar
        base58Prefixes[EXT_SECRET_KEY]     = boost::assign::list_of(0x04)(0x88)(0x94)(0x78).convert_to_container<std::vector<unsigned char> >(); // xpar
        base58Prefixes[STEALTH_ADDRESS]    = std::vector<unsigned char>(1,21); // T
        base58Prefixes[EXT_KEY_HASH]       = std::vector<unsigned char>(1,137); // x
        base58Prefixes[EXT_ACC_HASH]       = std::vector<unsigned char>(1,83);  // a
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >(); // tpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >(); // tprv
        base58Prefixes[EXT_PUBLIC_KEY_SDC] = boost::assign::list_of(0x76)(0xC0)(0xFD)(0xFB).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY_SDC] = boost::assign::list_of(0x76)(0xC1)(0x07)(0x7A).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;


        /*
        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 546, uint256S("000000002a936ca763904c3c35fce2f3556c559c0214345d31b1bcebf76acb70")),
        };

        chainTxData = ChainTxData{
            // Data as of block 00000000c2872f8f8a8935c8e3c5862be9038c97d4de2cf37ed496991166928a (height 1063660)
            1483546230,
            12834668,
            0.15
        };
        */

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
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
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

        pchMessageStart[0] = 0x09;
        pchMessageStart[1] = 0x12;
        pchMessageStart[2] = 0x06;
        pchMessageStart[3] = 0x0c;
        nDefaultPort = 11938;
        nBIP44ID = 0x80000001;
        
        
        nModifierInterval = 2 * 60;     // 2 minutes
        nStakeMinConfirmations = 12;
        nTargetSpacing = 5;             // 5 seconds
        nTargetTimespan = 16 * 60;      // 16 mins
        nStakeMinAge = (nStakeMinConfirmations+1) * nTargetSpacing;
        
        SetLastImportHeight();
        
        nPruneAfterHeight = 1000;
        
        genesis = CreateGenesisBlockRegTest(1487714923, 1, 0x207fffff);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x233844b5044fc3e9e4b3d948b963fb8e92921ab7820f9b5aece9e3fc8b10fc13"));
        assert(genesis.hashMerkleRoot == uint256S("0x125c3ec540ddf44b76bfa682a7f769516968bb861bb3193d08c93e2cdba979c8"));
        assert(genesis.hashWitnessMerkleRoot == uint256S("0xa1e9e44ea880f3865042cc8dba519c2c472a01f0c7caed97d9e94734eba25d9d"));
        
        
        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"))
        };
        
        base58Prefixes[PUBKEY_ADDRESS]     = std::vector<unsigned char>(1,118); // p
        base58Prefixes[SCRIPT_ADDRESS]     = std::vector<unsigned char>(1,122);
        base58Prefixes[SECRET_KEY]         = std::vector<unsigned char>(1,46);
        base58Prefixes[EXT_PUBLIC_KEY]     = boost::assign::list_of(0xe1)(0x42)(0x78)(0x00).convert_to_container<std::vector<unsigned char> >(); // ppar
        base58Prefixes[EXT_SECRET_KEY]     = boost::assign::list_of(0x04)(0x88)(0x94)(0x78).convert_to_container<std::vector<unsigned char> >(); // xpar
        base58Prefixes[STEALTH_ADDRESS]    = std::vector<unsigned char>(1,21); // T
        base58Prefixes[EXT_KEY_HASH]       = std::vector<unsigned char>(1,137); // x
        base58Prefixes[EXT_ACC_HASH]       = std::vector<unsigned char>(1,83);  // a
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >(); // tpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >(); // tprv
        base58Prefixes[EXT_PUBLIC_KEY_SDC] = boost::assign::list_of(0x76)(0xC0)(0xFD)(0xFB).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY_SDC] = boost::assign::list_of(0x76)(0xC1)(0x07)(0x7A).convert_to_container<std::vector<unsigned char> >();

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
    }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    
    void SetOld()
    {
        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
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

void ResetParams(bool fParticlModeIn)
{
    // Hack to pass old unit tests
    mainParams = CMainParams();
    regTestParams = CRegTestParams();
    if (!fParticlModeIn)
    {
        mainParams.SetOld();
        regTestParams.SetOld();
    };
};

/**
 * Mutable handle to regtest params
 */
CChainParams &RegtestParams()
{
    return regTestParams;
};

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
