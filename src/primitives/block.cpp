// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <util/strencodings.h>
#include <crypto/common.h>

#include <atomic>

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

CBlockHeader::CBlockHeader(const CBlockHeader& header)
    : nVersion{header.nVersion},
      hashPrevBlock{header.hashPrevBlock},
      hashMerkleRoot{header.hashMerkleRoot},
      nTime{header.nTime},
      nBits{header.nBits},
      nNonce{header.nNonce}
{
}

CBlock::CBlock(const CBlock& block)
    : CBlockHeader{static_cast<CBlockHeader>(block)},
      vtx{block.vtx}
{
    std::atomic_store(&m_checked, std::atomic_load(&block.m_checked));
}

CBlock& CBlock::operator=(const CBlock& block)
{
    *(static_cast<CBlockHeader*>(this)) = static_cast<CBlockHeader>(block);
    vtx = block.vtx;
    std::atomic_store(&m_checked, std::atomic_load(&block.m_checked));
    return *this;
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
