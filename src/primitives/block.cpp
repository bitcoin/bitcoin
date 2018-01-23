// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <utilstrencodings.h>
#include <crypto/common.h>

#include <btv_const.h>
#include <crypto/cryptonight.h>

uint256 CBlockHeader::GetHash() const
{
    uint256 ret;
    if (!IsBtvBranched()) // before branch
    {
        return SerializeHash(*this);
    }
    else
    {
        char data[32];
        memset(data, 0, 32);
        cryptonight_hash(data, (const void*)this, 80);

        std::vector<unsigned char> vch;
        for (int i = 0; i < 32; ++i)
        {
            vch.push_back((unsigned char) data[i]);
        }

		uint256 result(vch);
		return result;
    }
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
