// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <crypto/common.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <chainparams.h>
#include "global.h"

uint256 CBlockHeader::GetHash(int forceProgram) const
{

    CSHA256 ctx;

    unsigned char* headerData = (unsigned char*)malloc(sizeof(*this));

    unsigned char strPrevBlock[32];
    memcpy(strPrevBlock, hashPrevBlock.data(), 32);
    for (int i = 0; i < 16; i++) {
        unsigned char swap = strPrevBlock[i];
        strPrevBlock[i] = strPrevBlock[31 - i];
        strPrevBlock[31 - i] = swap;
    }

    memcpy(headerData, &nVersion, 4);
    memcpy(headerData + 4, strPrevBlock, 32);
    memcpy(headerData + 36, hashMerkleRoot.data(), 32);
    memcpy(headerData + 68, &nTime, 4);
    memcpy(headerData + 72, &nBits, 4);
    memcpy(headerData + 76, &nNonce, 4);

    std::string strResult = g_hashFunction->calcBlockHeaderHash(nTime, headerData, hashPrevBlock.GetHex(), hashMerkleRoot.GetHex(), forceProgram);
    free(headerData);


    uint256 result;
    result.SetHex(strResult);
    return result;

}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    return s.str();
}
