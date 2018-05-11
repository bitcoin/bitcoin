// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <hash.h>
#include <utilstrencodings.h>

/*     WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given time
       is odd, the last one is duplicated before computing the next level (which
       is unusual in Merkle trees). This results in certain sequences of
       transactions leading to the same merkle root. For example, these two
       trees:

                    A               A
                  /  \            /   \
                B     C         B       C
               / \    |        / \     / \
              D   E   F       D   E   F   F
             / \ / \ / \     / \ / \ / \ / \
             1 2 3 4 5 6     1 2 3 4 5 6 5 6

       for transaction lists [1,2,3,4,5,6] and [1,2,3,4,5,6,5,6] (where 5 and
       6 are repeated) result in the same root hash A (because the hash of both
       of (F) and (F,F) is C).

       The vulnerability results from being able to send a block with such a
       transaction list, with the same merkle root, and the same block hash as
       the original without duplication, resulting in failed validation. If the
       receiving node proceeds to mark that block as permanently invalid
       however, it will fail to accept further unmodified (and thus potentially
       valid) versions of the same block. We defend against this by detecting
       the case where we would hash two identical hashes at the end of the list
       together, and treating that identically to the block having an invalid
       merkle root. Assuming no double-SHA256 collisions, this will detect all
       known ways of changing the transactions without affecting the merkle
       root.
*/

typedef enum {
    MERKLE_COMPUTATION_MUTABLE = 0x1,
    MERKLE_COMPUTATION_FAST    = 0x2
} merklecomputationopts;

static void MerkleHash_Hash256(uint256& parent, const uint256& left, const uint256& right) {
    CHash256().Write(left.begin(), 32).Write(right.begin(), 32).Finalize(parent.begin());
}

/* Calculated by using standard FIPS-180 SHA-256 to hash the first 512
 * bits of the fractional part of the square root of 2 then extracting
 * the midstate. This initial data, expressed in hexadecimal notation
 * is:
 *
 *   6a09e667f3bcc908 b2fb1366ea957d3e 3adec17512775099 da2f590b0667322a
 *   95f9060875714587 5163fcdfb907b672 1ee950bc8738f694 f0090e6c7bf44ed1
 */
static unsigned char _MidstateIV[32] =
    { 0x91, 0x06, 0x6c, 0x2b, 0x97, 0x5c, 0xc8, 0x32,
      0xe7, 0x6c, 0xd4, 0x01, 0x68, 0x21, 0x8d, 0x36,
      0x18, 0xd0, 0x9b, 0xe1, 0x9a, 0x7b, 0xff, 0xc0,
      0xec, 0x1d, 0xcc, 0xf0, 0x8f, 0x77, 0x5b, 0xbd };
static void MerkleHash_Sha256Midstate(uint256& parent, const uint256& left, const uint256& right) {
    CSHA256(_MidstateIV).Write(left.begin(), 32).Write(right.begin(), 32).Midstate(parent.begin(), NULL, NULL);
}

/* This implements a constant-space merkle root/path calculator, limited to 2^32 leaves. */
static void MerkleComputation(const std::vector<uint256>& leaves, uint256* proot, bool* pmutated, uint32_t branchpos, std::vector<uint256>* pbranch, merklecomputationopts flags) {
    if (pbranch) pbranch->clear();
    if (leaves.size() == 0) {
        if (pmutated) *pmutated = false;
        if (proot) *proot = uint256();
        return;
    }
    bool is_mutable = flags & MERKLE_COMPUTATION_MUTABLE;
    auto MerkleHash = MerkleHash_Hash256;
    if (flags & MERKLE_COMPUTATION_FAST) {
        MerkleHash = MerkleHash_Sha256Midstate;
    }
    bool mutated = false;
    // count is the number of leaves processed so far.
    uint32_t count = 0;
    // inner is an array of eagerly computed subtree hashes, indexed by tree
    // level (0 being the leaves).
    // For example, when count is 25 (11001 in binary), inner[4] is the hash of
    // the first 16 leaves, inner[3] of the next 8 leaves, and inner[0] equal to
    // the last leaf. The other inner entries are undefined.
    uint256 inner[32];
    // Which position in inner is a hash that depends on the matching leaf.
    int matchlevel = -1;
    // First process all leaves into 'inner' values.
    while (count < leaves.size()) {
        uint256 h = leaves[count];
        bool matchh = count == branchpos;
        count++;
        int level;
        // For each of the lower bits in count that are 0, do 1 step. Each
        // corresponds to an inner value that existed before processing the
        // current leaf, and each needs a hash to combine it.
        for (level = 0; !(count & (((uint32_t)1) << level)); level++) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            mutated |= (inner[level] == h);
            MerkleHash(h, inner[level], h);
        }
        // Store the resulting hash at inner position level.
        inner[level] = h;
        if (matchh) {
            matchlevel = level;
        }
    }
    // Do a final 'sweep' over the rightmost branch of the tree to process
    // odd levels, and reduce everything to a single top value.
    // Level is the level (counted from the bottom) up to which we've sweeped.
    int level = 0;
    // As long as bit number level in count is zero, skip it. It means there
    // is nothing left at this level.
    while (!(count & (((uint32_t)1) << level))) {
        level++;
    }
    uint256 h = inner[level];
    bool matchh = matchlevel == level;
    while (count != (((uint32_t)1) << level)) {
        // If we reach this point, h is an inner value that is not the top.
        // We combine it with itself (Bitcoin's special rule for odd levels in
        // the tree) to produce a higher level one.
        if (is_mutable && pbranch && matchh) {
            pbranch->push_back(h);
        }
        if (is_mutable) {
            MerkleHash(h, h, h);
        }
        // Increment count to the value it would have if two entries at this
        // level had existed.
        count += (((uint32_t)1) << level);
        level++;
        // And propagate the result upwards accordingly.
        while (!(count & (((uint32_t)1) << level))) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            MerkleHash(h, inner[level], h);
            level++;
        }
    }
    // Return result.
    if (pmutated) *pmutated = mutated;
    if (proot) *proot = h;
}

uint256 ComputeMerkleRoot(const std::vector<uint256>& leaves, bool* mutated) {
    uint256 hash;
    MerkleComputation(leaves, &hash, mutated, -1, nullptr, MERKLE_COMPUTATION_MUTABLE);
    return hash;
}

std::vector<uint256> ComputeMerkleBranch(const std::vector<uint256>& leaves, uint32_t position) {
    std::vector<uint256> ret;
    MerkleComputation(leaves, nullptr, nullptr, position, &ret, MERKLE_COMPUTATION_MUTABLE);
    return ret;
}

uint256 ComputeMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& vMerkleBranch, uint32_t nIndex) {
    uint256 hash = leaf;
    for (std::vector<uint256>::const_iterator it = vMerkleBranch.begin(); it != vMerkleBranch.end(); ++it) {
        if (nIndex & 1) {
            hash = Hash(BEGIN(*it), END(*it), BEGIN(hash), END(hash));
        } else {
            hash = Hash(BEGIN(hash), END(hash), BEGIN(*it), END(*it));
        }
        nIndex >>= 1;
    }
    return hash;
}

uint256 ComputeFastMerkleRoot(const std::vector<uint256>& leaves) {
    uint256 hash;
    if (leaves.empty()) {
        hash = CHashWriter(SER_GETHASH, PROTOCOL_VERSION).GetHash();
    } else {
        MerkleComputation(leaves, &hash, nullptr, -1, nullptr, MERKLE_COMPUTATION_FAST);
    }
    return hash;
}

std::pair<std::vector<uint256>, uint32_t> ComputeFastMerkleBranch(const std::vector<uint256>& leaves, uint32_t position) {
    std::vector<uint256> branch;
    MerkleComputation(leaves, nullptr, nullptr, position, &branch, MERKLE_COMPUTATION_FAST);
    std::size_t max = 0;
    for (int i = 0; i < 32; ++i)
        if (position & ((uint32_t)1)<<i)
            max = i + 1;
    uint32_t path = position;
    while (max > branch.size()) {
        int i;
        for (i = max-1; i >= 0; --i)
            if (!(path & ((uint32_t)1)<<i))
                break;
        if (i < 0) // Should never happen
            return {};
        path = ((path & ~((((uint32_t)1)<<(i+1))-1))>>1)
             |  (path &  ((((uint32_t)1)<< i   )-1));
        --max;
    }
    return {branch, path};
}

uint256 ComputeFastMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t path) {
    uint256 hash = leaf;
    for (std::vector<uint256>::const_iterator it = branch.begin(); it != branch.end(); ++it) {
        if (path & 1) {
            MerkleHash_Sha256Midstate(hash, *it, hash);
        } else {
            MerkleHash_Sha256Midstate(hash, hash, *it);
        }
        path >>= 1;
    }
    return hash;
}

uint256 BlockMerkleRoot(const CBlock& block, bool* mutated)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetHash();
    }
    return ComputeMerkleRoot(leaves, mutated);
}

uint256 BlockWitnessMerkleRoot(const CBlock& block, bool* mutated)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    leaves[0].SetNull(); // The witness hash of the coinbase is 0.
    for (size_t s = 1; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetWitnessHash();
    }
    return ComputeMerkleRoot(leaves, mutated);
}

std::vector<uint256> BlockMerkleBranch(const CBlock& block, uint32_t position)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    for (size_t s = 0; s < block.vtx.size(); s++) {
        leaves[s] = block.vtx[s]->GetHash();
    }
    return ComputeMerkleBranch(leaves, position);
}

/*
 * The {SKIP, SKIP} entry is missing on purpose. Not only does this
 * make the number of possible states a nicely packable power of 2,
 * but excluding that fully prunable state means that any given fully
 * expanded tree and set of verify hashes has one and only one proof
 * encoding -- the serialized tree with all {SKIP, SKIP} nodes
 * recursively pruned.
 */
const std::array<MerkleLink, 8> MerkleNode::m_left_from_code {{
    MerkleLink::VERIFY,  MerkleLink::VERIFY,  MerkleLink::VERIFY,
    MerkleLink::DESCEND, MerkleLink::DESCEND, MerkleLink::DESCEND,
      /* No SKIP */      MerkleLink::SKIP,    MerkleLink::SKIP }};

const std::array<MerkleLink, 8> MerkleNode::m_right_from_code {{
    MerkleLink::SKIP, MerkleLink::VERIFY, MerkleLink::DESCEND,
    MerkleLink::SKIP, MerkleLink::VERIFY, MerkleLink::DESCEND,
      /* No SKIP */   MerkleLink::VERIFY, MerkleLink::DESCEND }};

MerkleNode::code_type MerkleNode::_get_code(MerkleLink left, MerkleLink right)
{
    /*
     * Returns the 3-bit code for a given combination of left and
     * right link values in an internal node.
     */
    code_type code = std::numeric_limits<code_type>::max();
    /* Write out a table of Code values to see why this works :) */
    switch (left)
    {
        case MerkleLink::DESCEND: code = 5; break;
        case MerkleLink::VERIFY:  code = 2; break;
        case MerkleLink::SKIP:    code = 7; break;
    }
    switch (right)
    {
        case MerkleLink::SKIP:    --code; // No break!
        case MerkleLink::VERIFY:  --code; break;
        case MerkleLink::DESCEND:         break;
    }
    return code;
}

MerkleNode::code_type MerkleNodeReference::GetCode() const
{
    /*
     * Belts and suspenders: m_offset should never be anything outside
     * the range [0, 7], so the assignment to max should be undone by
     * the switch that follows. But just in case we'll favor failing
     * in a way that is maximally likely to be detectable when the
     * code is later used.
     */
    MerkleNode::code_type code = std::numeric_limits<MerkleNode::code_type>::max();
    assert((m_offset >= 0) && (m_offset <= 7));
    switch (m_offset)
    {
        /*
         * Use the diagram in the class definition to work out that
         * these magic constant values are correct.
         */
        case 0: code =   m_base[0] >> 5;  break;
        case 1: code =   m_base[0] >> 2;  break;
        case 2: code =  (m_base[0] << 1)
                     | ((m_base[1] >> 7) & 1); break;
        case 3: code =   m_base[1] >> 4;  break;
        case 4: code =   m_base[1] >> 1;  break;
        case 5: code =  (m_base[1] << 2)
                     | ((m_base[2] >> 6) & 3); break;
        case 6: code =   m_base[2] >> 3;       break;
        case 7: code =   m_base[2];            break;
    }
    return code & 7;
}

MerkleNodeReference& MerkleNodeReference::SetCode(MerkleNode::code_type code)
{
    switch (m_offset)
    {
        /*
         * Again, check the diagram in the class definition to see
         * where these magic constant shift and mask values arise
         * from.
         */
        case 0: m_base[0] = (m_base[0] & 0x1f) |  (code      << 5); break;
        case 1: m_base[0] = (m_base[0] & 0xe3) |  (code      << 2); break;
        case 2: m_base[0] = (m_base[0] & 0xfc) |  (code      >> 1);
                m_base[1] = (m_base[1] & 0x7f) | ((code & 1) << 7); break;
        case 3: m_base[1] = (m_base[1] & 0x8f) |  (code      << 4); break;
        case 4: m_base[1] = (m_base[1] & 0xf1) |  (code      << 1); break;
        case 5: m_base[1] = (m_base[1] & 0xfe) |  (code      >> 2);
                m_base[2] = (m_base[2] & 0x3f) | ((code & 3) << 6); break;
        case 6: m_base[2] = (m_base[2] & 0xc7) |  (code      << 3); break;
        case 7: m_base[2] = (m_base[2] & 0xf8) |   code;            break;
    }
    return *this;
}

void MerkleNodeIteratorBase::_incr()
{
    if (m_ref.m_offset++ == 7) {
        m_ref.m_offset = 0;
        m_ref.m_base += 3;
    }
}

void MerkleNodeIteratorBase::_decr()
{
    if (m_ref.m_offset-- == 0) {
        m_ref.m_offset = 7;
        m_ref.m_base -= 3;
    }
}

void MerkleNodeIteratorBase::_seek(MerkleNodeIteratorBase::difference_type distance)
{
    difference_type bits = distance + m_ref.m_offset;
    m_ref.m_base += 3 * (bits / 8);
    bits = bits % 8;
    if (bits < 0) {
        bits += 8;
        m_ref.m_base -= 3;
    }
    m_ref.m_offset = static_cast<MerkleNodeReference::offset_type>(bits);
}

MerkleNodeIteratorBase::difference_type MerkleNodeIteratorBase::operator-(const MerkleNodeIteratorBase& other) const
{
    /*
     * Compare with the version of _seek implemented above. The
     * following property should hold true:
     *
     *   A._seek(B-A) == B
     *
     * That is to say, the difference between two iterators is the
     * value which needs to be passed to _seek() to move from one to
     * the other.
     */
    return 8 * (m_ref.m_base - other.m_ref.m_base) / 3 + m_ref.m_offset - other.m_ref.m_offset;
}

void MerkleProof::clear() noexcept
{
    m_path.clear();
    m_skip.clear();
}

void swap(MerkleProof& lhs, MerkleProof& rhs)
{
    using std::swap;
    swap(lhs.m_path, rhs.m_path);
    swap(lhs.m_skip, rhs.m_skip);
}

MerkleTree::MerkleTree(const MerkleTree& left, const MerkleTree& right)
{
    /* Handle the special case of both left and right being fully
     * pruned, which also results in a fully pruned super-tree.. */
    if (left.m_proof.m_path.empty() && left.m_proof.m_skip.size()==1 && left.m_verify.empty() &&
        right.m_proof.m_path.empty() && right.m_proof.m_skip.size()==1 && right.m_verify.empty())
    {
        m_proof.m_skip.resize(1);
        MerkleHash_Sha256Midstate(m_proof.m_skip[0], left.m_proof.m_skip[0], right.m_proof.m_skip[0]);
        return;
    }

    /* We assume a well-formed, non-empty MerkleTree for both passed
     * in subtrees, in which if there are no internal nodes than
     * either m_skip XOR m_verify must have a single hash. Otherwise
     * the result of what follows will be an invalid MerkleTree. */
    m_proof.m_path.emplace_back(MerkleLink::DESCEND, MerkleLink::DESCEND);

    if (left.m_proof.m_path.empty())
        m_proof.m_path.front().SetLeft(left.m_proof.m_skip.empty()? MerkleLink::VERIFY: MerkleLink::SKIP);
    m_proof.m_path.insert(m_proof.m_path.end(), left.m_proof.m_path.begin(), left.m_proof.m_path.end());
    m_proof.m_skip.insert(m_proof.m_skip.end(), left.m_proof.m_skip.begin(), left.m_proof.m_skip.end());
    m_verify.insert(m_verify.end(), left.m_verify.begin(), left.m_verify.end());

    if (right.m_proof.m_path.empty())
        m_proof.m_path.front().SetRight(right.m_proof.m_skip.empty()? MerkleLink::VERIFY: MerkleLink::SKIP);
    m_proof.m_path.insert(m_proof.m_path.end(), right.m_proof.m_path.begin(), right.m_proof.m_path.end());
    m_proof.m_skip.insert(m_proof.m_skip.end(), right.m_proof.m_skip.begin(), right.m_proof.m_skip.end());
    m_verify.insert(m_verify.end(), right.m_verify.begin(), right.m_verify.end());
}

void MerkleTree::clear() noexcept
{
    m_proof.clear();
    m_verify.clear();
}

void swap(MerkleTree& lhs, MerkleTree& rhs)
{
    using std::swap;
    swap(lhs.m_proof, rhs.m_proof);
    swap(lhs.m_verify, rhs.m_verify);
}

uint256 MerkleTree::GetHash(bool* invalid) const
{
    std::vector<std::pair<bool, uint256> > stack(2);
    auto verify_pos = m_verify.begin();
    auto verify_last = m_verify.end();
    auto skip_pos = m_proof.m_skip.begin();
    auto skip_last = m_proof.m_skip.end();

    auto visitor = [&stack, &verify_pos, &verify_last, &skip_pos, &skip_last](std::size_t depth, MerkleLink value, bool right) -> bool
    {
        const uint256* new_hash = nullptr;
        switch(value) {
            case MerkleLink::DESCEND:
                stack.emplace_back();
                return false;

            case MerkleLink::VERIFY:
                if (verify_pos == verify_last) // read past end of verify hashes list
                    return true;
                new_hash = &(verify_pos++)[0];
                break;

            case MerkleLink::SKIP:
                if (skip_pos == skip_last) // read past end of skip hashes list
                    return true;
                new_hash = &(skip_pos++)[0];
                break;
        }

        uint256 tmp;
        while (stack.back().first) {
            MerkleHash_Sha256Midstate(tmp, stack.back().second, *new_hash);
            new_hash = &tmp;
            stack.pop_back();
        }

        stack.back().first = true;
        stack.back().second = *new_hash;
        return false;
    };

    // As a special case, an empty proof with no verify hashes results
    // in the unsalted hash of empty string. Although this requires
    // extra work in this implementation to support, it provides
    // continuous semantics to the meaning of the MERKLEBLOCKVERIFY
    // opcode, which might potentially reduce the number of code paths
    // in some scripts.
    if (m_proof.m_path.empty() && m_verify.empty() && m_proof.m_skip.empty()) {
        if (invalid) {
            *invalid = false;
        }
        return CHashWriter(SER_GETHASH, PROTOCOL_VERSION).GetHash();
    }

    // Except for the special case of a 0-node, 0-verify, 0-skip tree,
    // it is always the case (for any binary tree), that the number of
    // leaf nodes (verify + skip) is one more than the number of
    // internal nodes in the tree.
    if ((m_verify.size() + m_proof.m_skip.size()) != (m_proof.m_path.size() + 1)) {
        if (invalid) {
            *invalid = true;
        }
        return uint256();
    }

    // If there are NO nodes in the tree, then this is the degenerate
    // case of a single hash, in either the verify or skip set.
    if (m_proof.m_path.empty()) {
        if (invalid) {
            *invalid = false;
        }
        if (!m_verify.empty()) {
            return m_verify[0];
        } else {
            return m_proof.m_skip[0];
        }
    }

    auto res = depth_first_traverse(m_proof.m_path.begin(), m_proof.m_path.end(), visitor);

    if (res.first != m_proof.m_path.end() // m_proof.m_path has "extra" Nodes (after end of tree)
        || res.second                     // m_proof.m_path has insufficient Nodes (tree not finished)
        || stack.size() != 1              // expected one root hash...
        || !stack.back().first)           // ...and an actual hash, not a placeholder
    {
        if (invalid)
            *invalid = true;
        return uint256();
    }

    if (invalid) {
        *invalid = (verify_pos != verify_last  // did not use all verify hashes
                    || skip_pos != skip_last); // did not use all skip hashes
    }

    return stack.back().second;
}
