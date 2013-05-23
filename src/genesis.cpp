// Copyright (c) 2013 The Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#include "init.h"
#include "main.h"
#include "util.h"
#include "prime.h"

using namespace std;
using namespace boost;

int main(int argc, char *argv[])
{
    fPrintToConsole = true;
    GeneratePrimeTable();
    printf("Primecoin Begin Genesis Block\n");

    // Genesis block
    const char* pszDedication = "Sunny King - dedicated to Satoshi Nakamoto and all who have fought for the freedom of mankind";
    CTransaction txNew;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 0 << CBigNum(99) << vector<unsigned char>((const unsigned char*)pszDedication, (const unsigned char*)pszDedication + strlen(pszDedication));
    txNew.vout[0].nValue = 0;
    txNew.vout[0].scriptPubKey = CScript();
    CBlock block;
    block.vtx.push_back(txNew);
    block.hashPrevBlock = 0;
    block.hashMerkleRoot = block.BuildMerkleTree();
    block.nBits    = (4u << nFractionalBits);
    block.nTime    = GetAdjustedTime();
    block.nNonce   = 0;

    CBigNum bnTarget;
    bnTarget.SetCompact(block.nBits);
    unsigned int nTried = 0;
    bool fNewBlock = true;

    while (true)
    {
        unsigned int p = 7;
        CBigNum bnPrimorial;
        Primorial(p, bnPrimorial);
        unsigned int nProbableChainLength;
        unsigned int nPrimesHit;
        unsigned int nTests;
        if (MineProbablePrimeChain(block, bnPrimorial, fNewBlock, nTried, nProbableChainLength, nTests, nPrimesHit))
        {
            printf("target=%s length=%s multiplier=%u hash=%s\n", TargetToString(block.nBits).c_str(), TargetToString(nProbableChainLength).c_str(), nTried, block.GetHash().ToString().c_str());
            break;
        }
        if (fNewBlock)
            block.nNonce++;
    }

    printf("Primecoin Found Genesis Block:\n");
    printf("genesis hash=%s\n", block.GetHash().ToString().c_str());
    printf("merkle  root=%s\n", block.hashMerkleRoot.ToString().c_str());
    block.print();

    printf("Primecoin End Genesis Block\n");
}
