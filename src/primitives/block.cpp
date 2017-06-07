// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "pow/cuckoo_cycle/cuckoo_cycle.h"

bool CBlockHeader::found_solution(const powa::pow& p, powa::challenge_ref c, powa::solution_ref s)
{
    // copy solution into cycles
    vEdges.resize(s->params.size() / 4);
    memcpy(&vEdges[0], &s->params[0], s->params.size());
    // call it a day; we may lose some solutions for this nonce but whatevs
    solver = nullptr;
    return true;
}

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

std::vector<uint8_t> CBlockHeader::GetData() const
{
    std::vector<uint8_t> vhdr;
    vhdr.resize(80);
    uint8_t* hp = vhdr.data();
    *(int32_t*)hp = nVersion;                   hp += 4;    // 4
    memcpy(hp, hashPrevBlock.begin(), 32);      hp += 32;   // 36
    memcpy(hp, hashMerkleRoot.begin(), 32);     hp += 32;   // 68
    *(uint32_t*)hp = nTime;                     hp += 4;    // 72
    *(uint32_t*)hp = nBits;                     hp += 4;    // 76
    *(uint32_t*)hp = nNonce;                                // 80
    return vhdr;
}

bool CBlockHeader::CheckProofOfWork(bool searchCycle, bool background)
{
    auto challenge = powa::cuckoo_cycle::cc_challenge_ref(new powa::cuckoo_cycle::cc_challenge(28, 28, 128, GetData()));
    printf("CheckProofOfWork :: ");
    for (int i = 0; i < 80; i++) printf("%02x", challenge->params[i]);
    printf("\n");

    if (searchCycle) {
        vEdges.clear();
        solver = std::shared_ptr<powa::cuckoo_cycle::cuckoo_cycle>(new powa::cuckoo_cycle::cuckoo_cycle(challenge, powa::callback_ref(new powa::callback_proxy(this)), false));
        if (background) {
            solver->solve(1, true, 1);
            return false;
        }
        if (!solver->solve()) return false; // no sols found
    }

    if (vEdges.size() < 28) vEdges.resize(28);
    std::vector<uint8_t> data;
    data.resize(vEdges.size() * sizeof(uint32_t));
    memcpy(&data[0], &vEdges[0], vEdges.size() * sizeof(uint32_t));
    printf("data  = "); for (int i = 0; i < 4; i++) printf("%02x", data[i]); printf("...\n");
    printf("cycle = %08x, ...\n", vEdges[0]);
    uint32_t a = *(uint32_t*)&data[0];
    assert(a == vEdges[0]);

    auto solution = powa::solution(data);
    return powa::cuckoo_cycle::cuckoo_cycle(challenge, nullptr, false).is_valid(solution);
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
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i]->ToString() << "\n";
    }
    return s.str();
}

int64_t GetBlockWeight(const CBlock& block)
{
    // This implements the weight = (stripped_size * 4) + witness_size formula,
    // using only serialization with and without witness data. As witness_size
    // is equal to total_size - stripped_size, this formula is identical to:
    // weight = (stripped_size * 3) + total_size.
    return ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
}
