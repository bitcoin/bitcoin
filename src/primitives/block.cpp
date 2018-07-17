// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <crypto/common.h>

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
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

// peercoin: entropy bit for stake modifier if chosen by modifier
unsigned int CBlockHeader::GetStakeEntropyBit() const
{
    unsigned int nEntropyBit = 0;
    if (IsProtocolV04(nTime))
    {
        nEntropyBit = ((GetHash().Get64()) & 1llu);// last bit of block hash
        if (fDebug && GetBoolArg("-printstakemodifier"))
            printf("GetStakeEntropyBit(v0.4+): nTime=%u hashBlock=%s entropybit=%d\n", nTime, GetHash().ToString().c_str(), nEntropyBit);
    }
    else
    {
        // old protocol for entropy bit pre v0.4
        uint160 hashSig = Hash160(vchBlockSig);
        if (fDebug && GetBoolArg("-printstakemodifier"))
            printf("GetStakeEntropyBit(v0.3): nTime=%u hashSig=%s", nTime, hashSig.ToString().c_str());
        hashSig >>= 159; // take the first bit of the hash
        nEntropyBit = hashSig.Get64();
        if (fDebug && GetBoolArg("-printstakemodifier"))
            printf(" entropybit=%d\n", nEntropyBit);
    }
    return nEntropyBit;
}
