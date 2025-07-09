// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MERKLEBLOCK_H
#define BITCOIN_MERKLEBLOCK_H

#include <common/bloom.h>
#include <primitives/block.h>
#include <serialize.h>
#include <uint256.h>

#include <set>
#include <vector>

// Helper functions for serialization.
std::vector<unsigned char> BitsToBytes(const std::vector<bool>& bits);
std::vector<bool> BytesToBits(const std::vector<unsigned char>& bytes);

/** Data structure that represents a partial merkle tree.
 *
 * It represents a subset of the txid's of a known block, in a way that
 * allows recovery of the list of txid's and the merkle root, in an
 * authenticated way.
 *
 * The encoding works as follows: we traverse the tree in depth-first order,
 * storing a bit for each traversed node, signifying whether the node is the
 * parent of at least one matched leaf txid (or a matched txid itself). In
 * case we are at the leaf level, or this bit is 0, its merkle node hash is
 * stored, and its children are not explored further. Otherwise, no hash is
 * stored, but we recurse into both (or the only) child branch. During
 * decoding, the same depth-first traversal is performed, consuming bits and
 * hashes as they written during encoding.
 *
 * The serialization is fixed and provides a hard guarantee about the
 * encoded size:
 *
 *   SIZE <= 10 + ceil(32.25*N)
 *
 * Where N represents the number of leaf nodes of the partial tree. N itself
 * is bounded by:
 *
 *   N <= total_transactions
 *   N <= 1 + matched_transactions*tree_height
 *
 * The serialization format:
 *  - uint32     total_transactions (4 bytes)
 *  - varint     number of hashes   (1-3 bytes)
 *  - uint256[]  hashes in depth-first order (<= 32*N bytes)
 *  - varint     number of bytes of flag bits (1-3 bytes)
 *  - byte[]     flag bits, packed per 8 in a byte, least significant bit first (<= 2*N-1 bits)
 * The size constraints follow from this.
 */
class CPartialMerkleTree
{
protected:
    /** the total number of transactions in the block */
    unsigned int nTransactions;

    /** node-is-parent-of-matched-txid bits */
    std::vector<bool> vBits;

    /** txids and internal hashes */
    std::vector<uint256> vHash;

    /** flag set when encountering invalid data */
    bool fBad;

    /** helper function to efficiently calculate the number of nodes at given height in the merkle tree */
    unsigned int CalcTreeWidth(int height) const {
        return (nTransactions+(1 << height)-1) >> height;
    }

    /** calculate the hash of a node in the merkle tree (at leaf level: the txid's themselves) */
    uint256 CalcHash(int height, unsigned int pos, const std::vector<uint256> &vTxid);

    /** recursive function that traverses tree nodes, storing the data as bits and hashes */
    void TraverseAndBuild(int height, unsigned int pos, const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch);

    /**
     * recursive function that traverses tree nodes, consuming the bits and hashes produced by TraverseAndBuild.
     * it returns the hash of the respective node and its respective index.
     */
    uint256 TraverseAndExtract(int height, unsigned int pos, unsigned int &nBitsUsed, unsigned int &nHashUsed, std::vector<uint256> &vMatch, std::vector<unsigned int> &vnIndex);

public:

    SERIALIZE_METHODS(CPartialMerkleTree, obj)
    {
        READWRITE(obj.nTransactions, obj.vHash);
        std::vector<unsigned char> bytes;
        SER_WRITE(obj, bytes = BitsToBytes(obj.vBits));
        READWRITE(bytes);
        SER_READ(obj, obj.vBits = BytesToBits(bytes));
        SER_READ(obj, obj.fBad = false);
    }

    /** Construct a partial merkle tree from a list of transaction ids, and a mask that selects a subset of them */
    CPartialMerkleTree(const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch);

    CPartialMerkleTree();

    /**
     * extract the matching txid's represented by this partial merkle tree
     * and their respective indices within the partial tree.
     * returns the merkle root, or 0 in case of failure
     */
    uint256 ExtractMatches(std::vector<uint256> &vMatch, std::vector<unsigned int> &vnIndex);

    /** Get number of transactions the merkle proof is indicating for cross-reference with
     * local blockchain knowledge.
     */
    unsigned int GetNumTransactions() const { return nTransactions; };

};


/**
 * Used to relay blocks as header + vector<merkle branch>
 * to filtered nodes.
 *
 * NOTE: The class assumes that the given CBlock has *at least* 1 transaction. If the CBlock has 0 txs, it will hit an assertion.
 */
class CMerkleBlock
{
public:
    /** Public only for unit testing */
    CBlockHeader header;
    CPartialMerkleTree txn;
    /** Only used by subclass WtxidInBlockMerkleProof */
    bool m_prove_gentx;  //!< if m_gentx doesn't commit to a witness tree, this indicates the gentx should be included in the proven tx list
    CTransactionRef m_gentx;
    CPartialMerkleTree m_wtxid_tree;

    /**
     * Public only for unit testing and relay testing (not relayed).
     *
     * Used only when a bloom filter is specified to allow
     * testing the transactions which matched the bloom filter.
     */
    std::vector<std::pair<unsigned int, uint256> > vMatchedTxn;

    /**
     * Create from a CBlock, filtering transactions according to filter
     * Note that this will call IsRelevantAndUpdate on the filter for each transaction,
     * thus the filter will likely be modified.
     */
    CMerkleBlock(const CBlock& block, CBloomFilter& filter) : CMerkleBlock(block, &filter, nullptr) { }

    // Create from a CBlock, matching the txids in the set
    CMerkleBlock(const CBlock& block, const std::set<Txid>& txids, bool prove_witness=false) : CMerkleBlock{block, nullptr, &txids, prove_witness} {}

    CMerkleBlock() {}

    SERIALIZE_METHODS(CMerkleBlock, obj) {
        SER_WRITE(obj, assert(!obj.m_gentx));
        READWRITE(obj.header, obj.txn);
        SER_READ(obj, obj.m_gentx.reset());
        SER_READ(obj, obj.m_wtxid_tree = CPartialMerkleTree());
    }

    template<typename Stream>
    void SerializeWithWitness(Stream& s) { SerializationWithWitnessOps(*this, s, ActionSerialize{}); }
    template<typename Stream>
    void UnserializeWithWitness(Stream& s) { SerializationWithWitnessOps(*this, s, ActionUnserialize{}); }
    template<typename Stream, typename Type, typename Operation>
    static void SerializationWithWitnessOps(Type& obj, Stream& s, Operation ser_action) {
        // versions are negative to be strictly incompatible with block versions
        // -1 indicates a non-witness block, proving both the generation tx and txid(s) in the main merkle tree
        // -2 indicates a witness block, providing the generation tx in the main merkle tree, and txid(s) in the witness tree
        int32_t version;
        SER_WRITE(obj, version = obj.m_wtxid_tree.GetNumTransactions() ? -2 : -1);
        READWRITE(version);

        SER_WRITE(obj, assert(obj.m_gentx));
        READWRITE(obj.header, obj.txn);
        READWRITE(TX_WITH_WITNESS(obj.m_gentx));
        if (version == -1) {
            READWRITE(obj.m_prove_gentx);
        } else {
            READWRITE(obj.m_wtxid_tree);
        }
    }

private:
    // Combined constructor to consolidate code
    CMerkleBlock(const CBlock& block, CBloomFilter* filter, const std::set<Txid>* txids, bool prove_witness=false);
};

#endif // BITCOIN_MERKLEBLOCK_H
