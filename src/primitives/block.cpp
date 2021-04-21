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


uint256 CBlockHeader::GetHash() const
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
    //memcpy(headerData + 4, hashPrevBlock.data(), 32);
    memcpy(headerData + 4, strPrevBlock, 32);
    memcpy(headerData + 36, hashMerkleRoot.data(), 32);
    memcpy(headerData + 68, &nTime, 4);
    memcpy(headerData + 72, &nBits, 4);
    memcpy(headerData + 76, &nNonce, 4);

    /*
    printf("\n");
    for (int i = 0; i < 80; i++)
        printf("%02X", headerData[i]);
    printf("\n");
    */


    uint256 resultLE;
    ctx.Write(headerData, 80);
    ctx.Finalize(resultLE.begin());
    uint256 resultBE;
    for (int i = 0; i < 32; i++)
        resultBE.data()[i] = resultLE.data()[31 - i];

    free(headerData);

    return resultBE;
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
