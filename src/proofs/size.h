// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PROOFS_SIZE_H
#define BITCOIN_PROOFS_SIZE_H

#include "consensus/consensus.h"
#include "primitives/transaction.h"
#include "serialize.h"
#include "uint256.h"

#include <vector>

class CBlock;

class CBlockSizeProofComponent
{
private:
    size_t nTxRepresentedLog2plus1;
    uint256 u256;  // Either merkle-link hash, or SHA256 midstate
    size_t nTxSize;
    std::vector<unsigned char> vchLastChunk;

public:
    CBlockSizeProofComponent() : nTxRepresentedLog2plus1(0), nTxSize(0) {}
    CBlockSizeProofComponent(const CTransaction&, int serialFlags);
    CBlockSizeProofComponent(size_t TxRepresentedLog2, const uint256&);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(VARINT(nTxRepresentedLog2plus1));
        READWRITE(u256);
        if (nTxRepresentedLog2plus1) {
            vchLastChunk.clear();
        } else {
            READWRITE(VARINT(nTxSize));
            READWRITE(vchLastChunk);
        }
    }

    bool IsValid() const {
        return nTxRepresentedLog2plus1 || nTxSize;
    }

    bool IsFullTxProof() const {
        return !nTxRepresentedLog2plus1;
    }

    size_t GetTxRepresentedLog2() const {
        assert(nTxRepresentedLog2plus1);
        return nTxRepresentedLog2plus1 - 1;
    }

    size_t GetTxRepresented() const {
        return 1 << GetTxRepresentedLog2();
    }

    size_t GetTxSize() const {
        assert(!nTxRepresentedLog2plus1);
        return nTxSize;
    }

    uint256 GetTxHash() const;

private:
    static size_t CalculateDataInMidstate(size_t nFullLength);
};

class CBlockSizeProof {
private:
    uint32_t nTxCountLog2;
    std::vector<CBlockSizeProofComponent> componentsStripped;
    std::vector<CBlockSizeProofComponent> componentsFull;

public:
    static const size_t BLOCKHEADER_SIZE = 80;
    static const size_t ABSOLUTE_MINIMUM_TX_SIZE = 60;
    static const uint64_t ABSOLUTE_MINIMUM_TX_WEIGHT = ABSOLUTE_MINIMUM_TX_SIZE * WITNESS_SCALE_FACTOR;

    CBlockSizeProof() : nTxCountLog2(0) { }
    CBlockSizeProof(const CBlock&, size_t nProveWeight = MAX_BLOCK_WEIGHT);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(VARINT(nTxCountLog2));
        READWRITE(componentsStripped);
        READWRITE(componentsFull);
    }

    static size_t CalculateMinTxCount(uint32_t nTxCountLog2);
    bool Verify(const uint256& merkleroot, const uint256 *witness_hash, const std::vector<unsigned char> *witness_nonce) const;
    void GetBlockLowerBounds(size_t &nMinStrippedSize, size_t &nMinFullSize, size_t &nMinWeight) const;

private:
    static uint32_t ceil_log2(size_t n);
    static void BuildFillLists(const CBlock&, size_t nProveWeight, std::vector<size_t>& vFillStripped, std::vector<size_t>& vFillWitness);
    void FillComponents(std::vector<CBlockSizeProofComponent>& components, const CBlock&, std::vector<size_t>& vFillIndexes, int serialFlags);
    static bool VerifyComponents(const std::vector<CBlockSizeProofComponent>& components, uint256& out_merkleroot, bool& inout_fFoundFullTx, size_t& out_txcount);
    static void CollapseHashes(std::vector<uint256>& hashes);
    static size_t GetBlockMinSizeForComponents(const std::vector<CBlockSizeProofComponent>&);
};

#endif // BITCOIN_PROOFS_SIZE_H
