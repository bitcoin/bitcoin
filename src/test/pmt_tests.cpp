// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus/merkle.h"
#include "merkleblock.h"
#include "serialize.h"
#include "streams.h"
#include "uint256.h"
#include "arith_uint256.h"
#include "version.h"
#include "random.h"
#include "test/test_bitcoin.h"

#include <vector>

#include <boost/assign/list_of.hpp>
#include <boost/test/unit_test.hpp>

using namespace std;

class CPartialMerkleTreeTester : public CPartialMerkleTree
{
public:
    // flip one bit in one of the hashes - this should break the authentication
    void Damage() {
        unsigned int n = insecure_rand() % vHash.size();
        int bit = insecure_rand() % 256;
        *(vHash[n].begin() + (bit>>3)) ^= 1<<(bit&7);
    }
};

BOOST_FIXTURE_TEST_SUITE(pmt_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(pmt_test1)
{
    seed_insecure_rand(false);
    static const unsigned int nTxCounts[] = {1, 4, 7, 17, 56, 100, 127, 256, 312, 513, 1000, 4095};

    for (int n = 0; n < 12; n++) {
        unsigned int nTx = nTxCounts[n];

        // build a block with some dummy transactions
        CBlock block;
        for (unsigned int j=0; j<nTx; j++) {
            CMutableTransaction tx;
            tx.nLockTime = j; // actual transaction data doesn't matter; just make the nLockTime's unique
            block.vtx.push_back(CTransaction(tx));
        }

        // calculate actual merkle root and height
        uint256 merkleRoot1 = BlockMerkleRoot(block);
        std::vector<uint256> vTxid(nTx, uint256());
        for (unsigned int j=0; j<nTx; j++)
            vTxid[j] = block.vtx[j].GetHash();
        int nHeight = 1, nTx_ = nTx;
        while (nTx_ > 1) {
            nTx_ = (nTx_+1)/2;
            nHeight++;
        }

        // check with random subsets with inclusion chances 1, 1/2, 1/4, ..., 1/128
        for (int att = 1; att < 15; att++) {
            // build random subset of txid's
            std::vector<bool> vMatch(nTx, false);
            std::vector<uint256> vMatchTxid1;
            for (unsigned int j=0; j<nTx; j++) {
                bool fInclude = (insecure_rand() & ((1 << (att/2)) - 1)) == 0;
                vMatch[j] = fInclude;
                if (fInclude)
                    vMatchTxid1.push_back(vTxid[j]);
            }

            // build the partial merkle tree
            CPartialMerkleTree pmt1(vTxid, vMatch);

            // serialize
            CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
            ss << pmt1;

            // verify CPartialMerkleTree's size guarantees
            unsigned int n = std::min<unsigned int>(nTx, 1 + vMatchTxid1.size()*nHeight);
            BOOST_CHECK(ss.size() <= 10 + (258*n+7)/8);

            // deserialize into a tester copy
            CPartialMerkleTreeTester pmt2;
            ss >> pmt2;

            // extract merkle root and matched txids from copy
            std::vector<uint256> vMatchTxid2;
            uint256 merkleRoot2 = pmt2.ExtractMatches(vMatchTxid2);

            // check that it has the same merkle root as the original, and a valid one
            BOOST_CHECK(merkleRoot1 == merkleRoot2);
            BOOST_CHECK(!merkleRoot2.IsNull());

            // check that it contains the matched transactions (in the same order!)
            BOOST_CHECK(vMatchTxid1 == vMatchTxid2);

            // check that random bit flips break the authentication
            for (int j=0; j<4; j++) {
                CPartialMerkleTreeTester pmt3(pmt2);
                pmt3.Damage();
                std::vector<uint256> vMatchTxid3;
                uint256 merkleRoot3 = pmt3.ExtractMatches(vMatchTxid3);
                BOOST_CHECK(merkleRoot3 != merkleRoot1);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(pmt_malleability)
{
    std::vector<uint256> vTxid = boost::assign::list_of
        (ArithToUint256(1))(ArithToUint256(2))
        (ArithToUint256(3))(ArithToUint256(4))
        (ArithToUint256(5))(ArithToUint256(6))
        (ArithToUint256(7))(ArithToUint256(8))
        (ArithToUint256(9))(ArithToUint256(10))
        (ArithToUint256(9))(ArithToUint256(10));
    std::vector<bool> vMatch = boost::assign::list_of(false)(false)(false)(false)(false)(false)(false)(false)(false)(true)(true)(false);

    CPartialMerkleTree tree(vTxid, vMatch);
    std::vector<uint256> vTxid2;
    BOOST_CHECK(tree.ExtractMatches(vTxid).IsNull());
}

BOOST_AUTO_TEST_SUITE_END()
