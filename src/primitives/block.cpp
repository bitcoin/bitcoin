// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "sph_keccak.h"
#include "streams.h"

uint256 CBlockHeader::GetHash() const
{
    if (nTime < 0x57100000)  // 2016 Apr 14 20:39:28 UTC
        return SerializeHash(*this);

    CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
    ss << *this;

    sph_keccak256_context ctx_keccak;
    uint256 hash;

    sph_keccak256_init(&ctx_keccak);
    sph_keccak256(&ctx_keccak, (void*)&*ss.begin(), ss.size());
    sph_keccak256_close(&ctx_keccak, static_cast<void*>(&hash));

    return hash;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=%d, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i].ToString() << "\n";
    }
    return s.str();
}
