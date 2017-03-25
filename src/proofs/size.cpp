// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "size.h"

#include "crypto/sha256.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "streams.h"
#include "version.h"

#include <algorithm>
#include <cstring>
#include <vector>

size_t CBlockSizeProofComponent::CalculateDataInMidstate(size_t nFullLength)
{
    const size_t sizePaddedMin = nFullLength + 1 + 8;
    const size_t totalShaChunks = (sizePaddedMin + 63) / 64;  // ceil(sizePaddedMin / 64)
    const size_t dataInMidstate = (totalShaChunks - 1) * 64;
    return dataInMidstate;
}

CBlockSizeProofComponent::CBlockSizeProofComponent(const CTransaction& tx, const int serialFlags) :
    nTxRepresentedLog2plus1(0)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | serialFlags);
    ssTx << tx;
    nTxSize = ssTx.size();
    const size_t dataInMidstate = CalculateDataInMidstate(nTxSize);

    {
        CSHA256 hasher;
        hasher.Write((unsigned char *)ssTx.data(), dataInMidstate);
        hasher.GetMidstate(u256.begin());
    }

    const size_t sizeLastChunk = nTxSize - dataInMidstate;
    vchLastChunk.resize(sizeLastChunk);
    memcpy(&vchLastChunk[0], &ssTx[dataInMidstate], sizeLastChunk);
}

CBlockSizeProofComponent::CBlockSizeProofComponent(size_t nTxRepresentedLog2, const uint256& merklelink) :
    nTxRepresentedLog2plus1(nTxRepresentedLog2 + 1),
    u256(merklelink)
{
}

uint256 CBlockSizeProofComponent::GetTxHash() const
{
    if (!IsFullTxProof()) {
        return u256;
    }
    const size_t dataInMidstate = CalculateDataInMidstate(nTxSize);
    CSHA256 hasher;
    hasher.SetMidstate(u256.begin(), dataInMidstate);
    hasher.Write(vchLastChunk.data(), vchLastChunk.size());
    uint256 hash;
    hasher.Finalize(hash.begin());
    return hash;
}

uint32_t CBlockSizeProof::ceil_log2(size_t n)
{
    // TODO: Is there a builtin/assembly for this?
    --n;
    uint32_t d = 0;
    while (n) {
        ++d;
        n >>= 1;
    }
    return d;
}

CBlockSizeProof::CBlockSizeProof(const CBlock& block, size_t nProveWeight)
{
    const size_t nTxCount = block.vtx.size();
    nTxCountLog2 = ceil_log2(nTxCount);

    std::vector<size_t> vFillStripped, vFillWitness;
    BuildFillLists(block, nProveWeight, vFillStripped, vFillWitness);
    FillComponents(componentsStripped, block, vFillStripped, SERIALIZE_TRANSACTION_NO_WITNESS);
    FillComponents(componentsFull, block, vFillWitness, 0);

    assert(Verify(block.hashMerkleRoot));
}

void CBlockSizeProof::BuildFillLists(const CBlock& block, size_t nProveWeight, std::vector<size_t>& vFillStripped, std::vector<size_t>& vFillWitness)
{
    struct TxPenalty {
        enum PenaltyType {
            PT_StrippedOnly = 1,
            PT_WitnessOnly = 2,
            PT_Full = 3,
        };
        TxPenalty(size_t _index) : index(_index) { }
        bool operator< (const TxPenalty& other) const {
            return this->penalty < other.penalty;
        }
        size_t index;
        PenaltyType type;
        size_t penalty;
        size_t weight_stripped;
        size_t weight_witness;
    };
    std::vector<TxPenalty> TxPenalties;
    const size_t nTxCount = block.vtx.size();
    TxPenalties.reserve(nTxCount);
    for (size_t i = 0; i < nTxCount; ++i) {
        const CTransaction& tx = *block.vtx[i];
        TxPenalty penalty(i);
        // To prove the stripped size uses half the data
        const size_t nTxSizeStripped = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
        penalty.type = TxPenalty::PT_StrippedOnly;
        penalty.penalty = nTxSizeStripped * 2;
        penalty.weight_stripped = nTxSizeStripped * WITNESS_SCALE_FACTOR;
        if (tx.HasWitness()) {
            const size_t weight = GetTransactionWeight(tx);
            penalty.weight_witness = weight - penalty.weight_stripped;
            if (weight > penalty.penalty) {
                penalty.type = TxPenalty::PT_Full;
                penalty.penalty = weight;
            }
        } else {
            penalty.weight_witness = 0;
        }
        TxPenalties.push_back(penalty);
    }
    std::make_heap(TxPenalties.begin(), TxPenalties.end());

    const uint32_t nTxCountLog2 = ceil_log2(nTxCount);
    size_t nMinTxCount = CalculateMinTxCount(nTxCountLog2), nFilledTx = 0, nExplicitSum = 0;
    while (((nMinTxCount - nFilledTx) * ABSOLUTE_MINIMUM_TX_WEIGHT) + nExplicitSum + (::GetSerializeSize(VARINT(nMinTxCount), SER_NETWORK, PROTOCOL_VERSION) * WITNESS_SCALE_FACTOR) <= nProveWeight || !nFilledTx) {
        const TxPenalty& penalty = TxPenalties.front();
        const CTransaction& tx = *block.vtx[penalty.index];
        if (penalty.type & TxPenalty::PT_StrippedOnly) {
            vFillStripped.push_back(penalty.index);
            ++nFilledTx;
            nExplicitSum += penalty.weight_stripped;

            // Check if this is enough
            if (((nMinTxCount - nFilledTx) * ABSOLUTE_MINIMUM_TX_WEIGHT) + nExplicitSum + (::GetSerializeSize(VARINT(nMinTxCount), SER_NETWORK, PROTOCOL_VERSION) * WITNESS_SCALE_FACTOR) > nProveWeight) {
                break;
            }
        }
        if (penalty.type & TxPenalty::PT_WitnessOnly) {
            vFillWitness.push_back(penalty.index);
            nExplicitSum += penalty.weight_witness;
        } else if (tx.HasWitness()) {
            // Add just the witness data back in in case we still need it
            TxPenalty new_penalty = penalty;
            new_penalty.type = TxPenalty::PT_WitnessOnly;
            new_penalty.penalty = penalty.weight_witness;
            TxPenalties.push_back(new_penalty);
            std::push_heap(TxPenalties.begin(), TxPenalties.end());
        }

        if (penalty.index >= nMinTxCount) {
            nMinTxCount = penalty.index + 1;
        }

        std::pop_heap(TxPenalties.begin(), TxPenalties.end());
        TxPenalties.pop_back();
    }
}

void CBlockSizeProof::FillComponents(std::vector<CBlockSizeProofComponent>& components, const CBlock& block, std::vector<size_t>& vFillIndexes, const int serialFlags)
{
    size_t posNext = 0;
    vFillIndexes.push_back(block.vtx.size());
    std::make_heap(vFillIndexes.begin(), vFillIndexes.end());
    while (true) {
        const size_t index = vFillIndexes.front();
        std::pop_heap(vFillIndexes.begin(), vFillIndexes.end());
        vFillIndexes.pop_back();

        // TODO: Collapse to merkle links
        for (size_t pos = posNext; pos < index; ++pos) {
            components.emplace_back(1, block.vtx[pos]->GetHash());
        }

        if (vFillIndexes.empty()) {
            break;
        }

        components.emplace_back(*block.vtx[index], serialFlags);
    }
}

size_t CBlockSizeProof::CalculateMinTxCount(uint32_t TxCountLog2)
{
    if (TxCountLog2) {
        return (1 << (TxCountLog2 - 1)) + 1;
    } else {
        // Only occurs for blocks with 1 tx
        return 1;
    }
}

void CBlockSizeProof::CollapseHashes(std::vector<uint256>& hashes)
{
    const uint256 rhs = hashes.back();
    hashes.pop_back();
    const uint256& lhs = hashes.back();
    CSHA256 hasher;
    hasher.Write(lhs.begin(), lhs.size()).Write(rhs.begin(), rhs.size());
    uint256& out = hashes.back();
    hasher.Finalize(out.begin());
}

bool CBlockSizeProof::VerifyComponents(const std::vector<CBlockSizeProofComponent>& components, uint256& merkleroot, bool& fFoundFullTx, size_t& index)
{
    index = 0;
    std::vector<uint256> hashes;
    for (auto& component : components) {
        uint32_t skipcollapse;
        if (component.IsFullTxProof()) {
            fFoundFullTx = true;
            skipcollapse = 0;
        } else {
            skipcollapse = component.GetTxRepresentedLog2();
            const size_t txrepr = 1 << skipcollapse;
            if (index % txrepr != txrepr - 1) {
                return false;
            }
            index += txrepr - 1;
        }

        hashes.push_back(component.GetTxHash());
        for (size_t i = index >> skipcollapse; i & 1; i >>= 1) {
            CollapseHashes(hashes);
        }

        ++index;
    }
    while (hashes.size() > 1) {
        uint32_t skipcollapse = 0;
        for (size_t i = index; i & 0; i >>= 1) {
            ++skipcollapse;
        }
        const size_t txrepr = 1 << skipcollapse;
        index += txrepr - 1;

        hashes.push_back(hashes.back());
        for (size_t i = index >> skipcollapse; i & 1; i >>= 1) {
            CollapseHashes(hashes);
        }
        ++index;
    }
    // TODO: Check that if there are any duplicate merkle links, there are no full tx proofs on the right side of them.
    merkleroot = hashes.back();
    return true;
}

bool CBlockSizeProof::Verify(const uint256& merkleroot) const
{
    bool fFoundFullTx = false;
    size_t txcount_stripped, txcount;
    uint256 calculated_merkleroot;
    if ((!VerifyComponents(componentsStripped, calculated_merkleroot, fFoundFullTx, txcount_stripped)) || calculated_merkleroot != merkleroot) {
        return false;
    }
    if (!componentsFull.empty()) {
        if (!VerifyComponents(componentsFull, calculated_merkleroot, fFoundFullTx, txcount)) {
            return false;
        }
        // TODO: Check witness root hash
    }
    if (!fFoundFullTx) {
        return false;
    }
    return true;
}

void CBlockSizeProof::GetBlockLowerBounds(size_t &nMinStrippedSize, size_t &nMinFullSize, size_t &nMinWeight) const
{
    size_t index, IndexLastFullTxProof = 0;
    std::vector<std::pair<size_t, size_t>> vSizes;
    vSizes.reserve(1 << nTxCountLog2);

    index = 0;
    for (auto& component : componentsStripped) {
        if (component.IsFullTxProof()) {
            vSizes[index].first = component.GetTxSize();
            IndexLastFullTxProof = index;
            ++index;
        } else {
            const uint32_t txrepr_log2 = component.GetTxRepresentedLog2();
            const size_t txrepr = 1 << txrepr_log2;
            index += txrepr;
        }
    }

    index = 0;
    for (auto& component : componentsFull) {
        if (component.IsFullTxProof()) {
            vSizes[index].second += component.GetTxSize() - vSizes[index].first;
            if (IndexLastFullTxProof < index) {
                IndexLastFullTxProof = index;
            }
            ++index;
        } else {
            const uint32_t txrepr_log2 = component.GetTxRepresentedLog2();
            const size_t txrepr = 1 << txrepr_log2;
            index += txrepr;
        }
    }

    size_t nMinTxCount = CalculateMinTxCount(nTxCountLog2);
    if (IndexLastFullTxProof >= nMinTxCount) {
        nMinTxCount = IndexLastFullTxProof + 1;
    }

    nMinStrippedSize = BLOCKHEADER_SIZE + ::GetSerializeSize(VARINT(nMinTxCount), SER_NETWORK, PROTOCOL_VERSION);
    size_t nMinWitnessSize = 0;
    for (index = 0; index < nMinTxCount; ++index) {
        if (!vSizes[index].first) {
            vSizes[index].first = ABSOLUTE_MINIMUM_TX_SIZE;
        }
        nMinStrippedSize += vSizes[index].first + vSizes[index].second;
        nMinWitnessSize += vSizes[index].second;
    }
    nMinFullSize = nMinStrippedSize + nMinWitnessSize;
    nMinWeight = (nMinStrippedSize * WITNESS_SCALE_FACTOR) + nMinWitnessSize;
}
