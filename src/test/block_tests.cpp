#include <boost/test/unit_test.hpp>

#include "main.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE(genesis_block_load)
{
    // Genesis Block:
    // CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
    //   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
    //     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
    //     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
    //   vMerkleTree: 4a5e1e

    // Genesis block
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    CTransaction txNew;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = 50 * COIN;
    txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlock block;
    block.vtx.push_back(txNew);
    block.hashPrevBlock = 0;
    block.hashMerkleRoot = block.BuildMerkleTree();
    block.nVersion = 1;
    block.nTime    = 1231006505;
    block.nBits    = 0x1d00ffff;
    block.nNonce   = 2083236893;

    BOOST_CHECK_MESSAGE(block.hashMerkleRoot == uint256("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"), "Merkle root hash of genesis block incorrect.");
    BOOST_CHECK_MESSAGE(block.GetHash() == hashGenesisBlock, "Genesis block hash incorrect.");
}

BOOST_AUTO_TEST_SUITE_END()
