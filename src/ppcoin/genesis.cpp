// Copyright (c) 2011 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#include "../headers.h"
#include "../db.h"
#include "../net.h"
#include "../init.h"
#include "../main.h"
#include "../util.h"

using namespace std;
using namespace boost;

int main(int argc, char *argv[])
{
    fPrintToConsole = true;
    printf("PPCoin Begin Genesis Block\n");

    // Genesis block
    const char* pszTimestamp = "MarketWatch 07/Nov/2011 Gold tops $1,790 to end at over six-week high";
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
    block.nBits    = 0x1d00ffff;
    block.nTime    = GetAdjustedTime();
    block.nNonce   = 0;

    CBigNum bnTarget;
    bnTarget.SetCompact(block.nBits);

    while (block.GetHash() > bnTarget.getuint256())
    {
        if (block.nNonce % 1048576 == 0)
            printf("n=%dM hash=%s\n", block.nNonce / 1048576,
                   block.GetHash().ToString().c_str());
        block.nTime = GetAdjustedTime();
        block.nNonce++;
    }

    printf("PPCoin Found Genesis Block:\n");
    printf("genesis hash=%s\n", block.GetHash().ToString().c_str());
    printf("merkle  root=%s\n", block.hashMerkleRoot.ToString().c_str());
    block.print();

    printf("PPCoin End Genesis Block\n");
} 
