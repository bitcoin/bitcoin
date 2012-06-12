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
    txNew.vout[0].SetEmpty();
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
        if ((block.nNonce >> 20) << 20 == block.nNonce)
        {
            if (block.vtx[0].nTime + 7200 < GetAdjustedTime() + 60)
                block.vtx[0].nTime = GetAdjustedTime();
            block.nTime = GetAdjustedTime();
            printf("n=%dM hash=%s\n", block.nNonce >> 20,
                   block.GetHash().ToString().c_str());
        }
        block.nNonce++;
    }

    printf("PPCoin Found Genesis Block:\n");
    printf("genesis hash=%s\n", block.GetHash().ToString().c_str());
    printf("merkle  root=%s\n", block.hashMerkleRoot.ToString().c_str());
    block.print();

    printf("PPCoin End Genesis Block\n");
}
