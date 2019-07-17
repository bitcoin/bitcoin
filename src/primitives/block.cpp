// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>

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

bool CBlock::GetWitnessCommitmentSection(const uint8_t header[4], std::vector<uint8_t>& result) const
{
    int cidx = GetWitnessCommitmentIndex();
    if (cidx == -1) return false;
    auto script = vtx.at(0)->vout.at(cidx).scriptPubKey;
    opcodetype opcode;
    CScript::const_iterator pc = script.begin();
    ++pc; // move beyond initial OP_RETURN
    while (script.GetOp(pc, opcode, result)) {
        if (result.size() > 3 && !memcmp(result.data(), header, 4)) {
            result.erase(result.begin(), result.begin() + 4);
            return true;
        }
    }
    result.clear();
    return false;
}

bool CBlock::SetWitnessCommitmentSection(CMutableTransaction& mtx, const uint8_t header[4], const std::vector<uint8_t>& data)
{
    int cidx = GetWitnessCommitmentIndex(mtx);
    if (cidx == -1) return false;

    CScript result;
    std::vector<uint8_t> pushdata;
    auto script = mtx.vout[cidx].scriptPubKey;
    opcodetype opcode;
    CScript::const_iterator pc = script.begin();
    result.push_back(*pc++);
    bool found = false;
    while (script.GetOp(pc, opcode, pushdata)) {
        if (pushdata.size() > 0) {
            if (pushdata.size() > 3 && !memcmp(pushdata.data(), header, 4)) {
                // replace pushdata
                found = true;
                pushdata.erase(pushdata.begin() + 4, pushdata.end());
                pushdata.insert(pushdata.end(), data.begin(), data.end());
            }
            result << pushdata;
        } else {
            result << opcode;
        }
    }
    if (!found) {
        // append section as it did not exist
        pushdata.clear();
        pushdata.insert(pushdata.end(), header, header + 4);
        pushdata.insert(pushdata.end(), data.begin(), data.end());
        result << pushdata;
    }
    mtx.vout[cidx].scriptPubKey = result;
    return true;
}

bool CBlock::SetWitnessCommitmentSection(const uint8_t header[4], const std::vector<uint8_t>& data)
{
    auto mtx = CMutableTransaction(*vtx[0]);
    if (!SetWitnessCommitmentSection(mtx, header, data)) return false;
    vtx[0] = std::make_shared<CTransaction>(mtx);
    return true;
}
