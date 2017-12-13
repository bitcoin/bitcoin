// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/merkle.h>
#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(merkle_tests, TestingSetup)

// Older version of the merkle root computation code, for comparison.
static uint256 BlockBuildMerkleTree(const CBlock& block, bool* fMutated, std::vector<uint256>& vMerkleTree)
{
    vMerkleTree.clear();
    vMerkleTree.reserve(block.vtx.size() * 2 + 16); // Safe upper bound for the number of total nodes.
    for (std::vector<CTransactionRef>::const_iterator it(block.vtx.begin()); it != block.vtx.end(); ++it)
        vMerkleTree.push_back((*it)->GetHash());
    int j = 0;
    bool mutated = false;
    for (int nSize = block.vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            if (i2 == i + 1 && i2 + 1 == nSize && vMerkleTree[j+i] == vMerkleTree[j+i2]) {
                // Two identical hashes at the end of the list at a particular level.
                mutated = true;
            }
            vMerkleTree.push_back(Hash(vMerkleTree[j+i].begin(), vMerkleTree[j+i].end(),
                                       vMerkleTree[j+i2].begin(), vMerkleTree[j+i2].end()));
        }
        j += nSize;
    }
    if (fMutated) {
        *fMutated = mutated;
    }
    return (vMerkleTree.empty() ? uint256() : vMerkleTree.back());
}

// Older version of the merkle branch computation code, for comparison.
static std::vector<uint256> BlockGetMerkleBranch(const CBlock& block, const std::vector<uint256>& vMerkleTree, int nIndex)
{
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = block.vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

static inline int ctz(uint32_t i) {
    if (i == 0) return 0;
    int j = 0;
    while (!(i & 1)) {
        j++;
        i >>= 1;
    }
    return j;
}

BOOST_AUTO_TEST_CASE(merkle_test)
{
    for (int i = 0; i < 32; i++) {
        // Try 32 block sizes: all sizes from 0 to 16 inclusive, and then 15 random sizes.
        int ntx = (i <= 16) ? i : 17 + (InsecureRandRange(4000));
        // Try up to 3 mutations.
        for (int mutate = 0; mutate <= 3; mutate++) {
            int duplicate1 = mutate >= 1 ? 1 << ctz(ntx) : 0; // The last how many transactions to duplicate first.
            if (duplicate1 >= ntx) break; // Duplication of the entire tree results in a different root (it adds a level).
            int ntx1 = ntx + duplicate1; // The resulting number of transactions after the first duplication.
            int duplicate2 = mutate >= 2 ? 1 << ctz(ntx1) : 0; // Likewise for the second mutation.
            if (duplicate2 >= ntx1) break;
            int ntx2 = ntx1 + duplicate2;
            int duplicate3 = mutate >= 3 ? 1 << ctz(ntx2) : 0; // And for the third mutation.
            if (duplicate3 >= ntx2) break;
            int ntx3 = ntx2 + duplicate3;
            // Build a block with ntx different transactions.
            CBlock block;
            block.vtx.resize(ntx);
            for (int j = 0; j < ntx; j++) {
                CMutableTransaction mtx;
                mtx.nLockTime = j;
                block.vtx[j] = MakeTransactionRef(std::move(mtx));
            }
            // Compute the root of the block before mutating it.
            bool unmutatedMutated = false;
            uint256 unmutatedRoot = BlockMerkleRoot(block, &unmutatedMutated);
            BOOST_CHECK(unmutatedMutated == false);
            // Optionally mutate by duplicating the last transactions, resulting in the same merkle root.
            block.vtx.resize(ntx3);
            for (int j = 0; j < duplicate1; j++) {
                block.vtx[ntx + j] = block.vtx[ntx + j - duplicate1];
            }
            for (int j = 0; j < duplicate2; j++) {
                block.vtx[ntx1 + j] = block.vtx[ntx1 + j - duplicate2];
            }
            for (int j = 0; j < duplicate3; j++) {
                block.vtx[ntx2 + j] = block.vtx[ntx2 + j - duplicate3];
            }
            // Compute the merkle root and merkle tree using the old mechanism.
            bool oldMutated = false;
            std::vector<uint256> merkleTree;
            uint256 oldRoot = BlockBuildMerkleTree(block, &oldMutated, merkleTree);
            // Compute the merkle root using the new mechanism.
            bool newMutated = false;
            uint256 newRoot = BlockMerkleRoot(block, &newMutated);
            BOOST_CHECK(oldRoot == newRoot);
            BOOST_CHECK(newRoot == unmutatedRoot);
            BOOST_CHECK((newRoot == uint256()) == (ntx == 0));
            BOOST_CHECK(oldMutated == newMutated);
            BOOST_CHECK(newMutated == !!mutate);
            // If no mutation was done (once for every ntx value), try up to 16 branches.
            if (mutate == 0) {
                for (int loop = 0; loop < std::min(ntx, 16); loop++) {
                    // If ntx <= 16, try all branches. Otherwise, try 16 random ones.
                    int mtx = loop;
                    if (ntx > 16) {
                        mtx = InsecureRandRange(ntx);
                    }
                    std::vector<uint256> newBranch = BlockMerkleBranch(block, mtx);
                    std::vector<uint256> oldBranch = BlockGetMerkleBranch(block, merkleTree, mtx);
                    BOOST_CHECK(oldBranch == newBranch);
                    BOOST_CHECK(ComputeMerkleRootFromBranch(block.vtx[mtx]->GetHash(), newBranch, mtx) == oldRoot);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(merkle_link)
{
    BOOST_CHECK(sizeof(MerkleLink) == 1);
}

BOOST_AUTO_TEST_CASE(merkle_node)
{
    BOOST_CHECK(sizeof(MerkleNode) == 1);

    BOOST_CHECK(MerkleNode().GetCode() == 0);

    const MerkleNode by_code[8] = {MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(4), MerkleNode(5), MerkleNode(6), MerkleNode(7)};
    const MerkleNode by_link[8] = {
        MerkleNode(MerkleLink::VERIFY, MerkleLink::SKIP),
        MerkleNode(MerkleLink::VERIFY, MerkleLink::VERIFY),
        MerkleNode(MerkleLink::VERIFY, MerkleLink::DESCEND),
        MerkleNode(MerkleLink::DESCEND, MerkleLink::SKIP),
        MerkleNode(MerkleLink::DESCEND, MerkleLink::VERIFY),
        MerkleNode(MerkleLink::DESCEND, MerkleLink::DESCEND),
        MerkleNode(MerkleLink::SKIP, MerkleLink::VERIFY),
        MerkleNode(MerkleLink::SKIP, MerkleLink::DESCEND),
    };

    for (int i = 0; i <= 7; ++i) {
        BOOST_CHECK(i == by_code[i].GetCode());
        BOOST_CHECK(i == by_link[i].GetCode());
    }

    for (int i = 0; i <= 7; ++i) {
        for (int j = 0; j <= 7; ++j) {
            BOOST_CHECK((i==j) == (by_code[i] == by_link[j]));
            BOOST_CHECK((i!=j) == (by_code[i] != by_link[j]));
            BOOST_CHECK((i<j) == (by_code[i] < by_link[j]));
            BOOST_CHECK((i<=j) == (by_code[i] <= by_link[j]));
            BOOST_CHECK((i>=j) == (by_code[i] >= by_link[j]));
            BOOST_CHECK((i>j) == (by_code[i] > by_link[j]));
        }
    }

    MerkleNode a(0);
    a.SetCode(1);
    BOOST_CHECK(a.GetCode() == 1);
    BOOST_CHECK(a == MerkleNode(1));

    a = MerkleNode(3);
    BOOST_CHECK(a != MerkleNode(1));
    BOOST_CHECK(a.GetCode() == 3);

    for (int i = 0; i <= 7; ++i) {
        MerkleNode n = by_code[i];
        MerkleLink l = n.GetLeft();
        MerkleLink r = n.GetRight();
        BOOST_CHECK(MerkleNode(l,r) == by_link[i]);
        for (int j = 0; j <= 2; ++j) {
          MerkleNode n2(n);
          BOOST_CHECK(n2 == n);
          n2.SetLeft(MerkleLink(j));
          BOOST_CHECK(n2 == MerkleNode(MerkleLink(j),r));
        }
        for (int j = 0; j <= 2; ++j) {
          MerkleNode n3(n);
          BOOST_CHECK(n3 == n);
          n3.SetRight(MerkleLink(j));
          BOOST_CHECK(n3 == MerkleNode(l,MerkleLink(j)));
        }
    }
}

/*
 * To properly test some features requires access to protected members
 * of these classes. In the case of MerkleNode, just some static class
 * members so we write a method to return those.
 */
struct PublicMerkleNode: public MerkleNode
{
    typedef const std::array<MerkleLink, 8> m_link_from_code_type;
    static m_link_from_code_type& m_left_from_code()
      { return MerkleNode::m_left_from_code; }
    static m_link_from_code_type& m_right_from_code()
      { return MerkleNode::m_right_from_code; }
};

/*
 * In the case of MerkleNodeReference we need access to class instance
 * members, so we have a somewhat more involve wrapper that is used
 * for actual MerkleNodeReference instances (and forwards whatever
 * functionality needs to be defined to the base class).
 */
struct PublicMerkleNodeReference: public MerkleNodeReference
{
    PublicMerkleNodeReference(base_type* base, offset_type offset) : MerkleNodeReference(base, offset) { }

    PublicMerkleNodeReference() = delete;

    PublicMerkleNodeReference(const PublicMerkleNodeReference& other) : MerkleNodeReference(other) { }
    PublicMerkleNodeReference(const MerkleNodeReference& other) : MerkleNodeReference(other) { }
    PublicMerkleNodeReference(PublicMerkleNodeReference&& other) : MerkleNodeReference(other) { }
    PublicMerkleNodeReference(MerkleNodeReference&& other) : MerkleNodeReference(other) { }
    inline PublicMerkleNodeReference& operator=(const PublicMerkleNodeReference& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline PublicMerkleNodeReference& operator=(const MerkleNodeReference& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline PublicMerkleNodeReference& operator=(PublicMerkleNodeReference&& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline PublicMerkleNodeReference& operator=(MerkleNodeReference&& other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }

    inline PublicMerkleNodeReference& operator=(MerkleNode other)
      { return static_cast<PublicMerkleNodeReference&>(MerkleNodeReference::operator=(other)); }
    inline operator MerkleNode() const
      { return MerkleNodeReference::operator MerkleNode(); }

    typedef MerkleNodeReference::base_type* m_base_type;
    m_base_type& m_base()
      { return MerkleNodeReference::m_base; }
    const m_base_type& m_base() const
      { return MerkleNodeReference::m_base; }

    typedef MerkleNodeReference::offset_type m_offset_type;
    m_offset_type& m_offset()
      { return MerkleNodeReference::m_offset; }
    const m_offset_type& m_offset() const
      { return MerkleNodeReference::m_offset; }
};

BOOST_AUTO_TEST_CASE(merkle_node_reference)
{
    MerkleNodeReference::base_type v[3] = {0};
    PublicMerkleNodeReference _r[8] = {
        {v, 0}, {v, 1}, {v, 2}, {v, 3},
        {v, 4}, {v, 5}, {v, 6}, {v, 7},
    };
    MerkleNodeReference* r = &_r[0];
    const MerkleNode n[8] = {
        MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3),
        MerkleNode(4), MerkleNode(5), MerkleNode(6), MerkleNode(7),
    };

    PublicMerkleNodeReference a(v, 0);
    BOOST_CHECK(a.m_base() == &v[0]);
    BOOST_CHECK(a.m_offset() == 0);

    for (int i = 0; i <= 7; ++i) {
        BOOST_CHECK(r[i].GetCode() == 0);
        PublicMerkleNodeReference(v, i).SetCode(i);
        BOOST_CHECK_MESSAGE(r[i].GetCode() == i, strprintf("%d", i).c_str());
    }

    BOOST_CHECK(v[0] == static_cast<MerkleNodeReference::base_type>(0x05));
    BOOST_CHECK(v[1] == static_cast<MerkleNodeReference::base_type>(0x39));
    BOOST_CHECK(v[2] == static_cast<MerkleNodeReference::base_type>(0x77));

    for (int i = 0; i <= 7; ++i) {
        BOOST_CHECK(n[i].GetCode() == i);
        BOOST_CHECK(r[i].GetCode() == i);
        BOOST_CHECK(r[i].GetLeft() == PublicMerkleNode::m_left_from_code()[i]);
        BOOST_CHECK(r[i].GetRight() == PublicMerkleNode::m_right_from_code()[i]);
    }

    PublicMerkleNodeReference ref(v, 0);
    PublicMerkleNodeReference ref2(v, 7);

    for (MerkleNode::code_type i = 0; i <= 7; ++i) {
        for (MerkleNode::code_type j = 0; j <= 7; ++j) {
          ref.SetCode(i);
          ref2.SetCode(j);
          MerkleNode node(j);
          BOOST_CHECK((i==j) == (ref == node));
          BOOST_CHECK((j==i) == (node == ref));
          BOOST_CHECK((i==j) == (ref == ref2));
          BOOST_CHECK((i!=j) == (ref != node));
          BOOST_CHECK((j!=i) == (node != ref));
          BOOST_CHECK((i!=j) == (ref != ref2));
          BOOST_CHECK((i<j) == (ref < node));
          BOOST_CHECK((j<i) == (node < ref));
          BOOST_CHECK((i<j) == (ref < ref2));
          BOOST_CHECK((i<=j) == (ref <= node));
          BOOST_CHECK((j<=i) == (node <= ref));
          BOOST_CHECK((i<=j) == (ref <= ref2));
          BOOST_CHECK((i>=j) == (ref >= node));
          BOOST_CHECK((j>=i) == (node >= ref));
          BOOST_CHECK((i>=j) == (ref >= ref2));
          BOOST_CHECK((i>j) == (ref > node));
          BOOST_CHECK((j>i) == (node > ref));
          BOOST_CHECK((i>j) == (ref > ref2));
          MerkleLink new_left = node.GetLeft();
          MerkleLink new_right = node.GetRight();
          if ((new_left == MerkleLink::SKIP) && (ref.GetRight() == MerkleLink::SKIP)) {
              /* Prevent errors due to temporary {SKIP,SKIP} */
              ref.SetRight(MerkleLink::VERIFY);
          }
          ref.SetLeft(new_left);
          BOOST_CHECK(ref.GetLeft() == node.GetLeft());
          if ((ref.GetLeft() == MerkleLink::SKIP) && (new_right == MerkleLink::SKIP)) {
              /* Prevent errors due to temporary {SKIP,SKIP} */
              ref.SetLeft(MerkleLink::VERIFY);
          }
          ref.SetRight(new_right);
          BOOST_CHECK(ref.GetRight() == node.GetRight());
          BOOST_CHECK(ref == node);
          BOOST_CHECK(node == ref);
          ref.SetCode(i);
          BOOST_CHECK((i==j) == (ref == ref2));
          ref2 = ref;
          BOOST_CHECK(ref == ref2);
          ref2 = node;
          BOOST_CHECK(ref2 == node);
          BOOST_CHECK((i==j) == (ref == ref2));
          static_cast<MerkleNode>(ref).SetCode(j);
          BOOST_CHECK((i==j) == (ref == ref2));
        }
    }
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_constructor)
{
    /* explicit vector(const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> def;
    BOOST_CHECK(def.empty());
    BOOST_CHECK(!def.dirty());

    std::allocator<MerkleNode> alloc;
    std::vector<MerkleNode> with_alloc(alloc);
    BOOST_CHECK(with_alloc.get_allocator() == std::allocator<MerkleNode>());
    BOOST_CHECK(with_alloc.get_allocator() == def.get_allocator());

    /* explicit vector(size_type count) */
    std::vector<MerkleNode> three(3);
    BOOST_CHECK(three.size() == 3);
    BOOST_CHECK(three[0] == MerkleNode());
    BOOST_CHECK(three[1] == MerkleNode());
    BOOST_CHECK(three[2] == MerkleNode());

    std::vector<MerkleNode> nine(9);
    BOOST_CHECK(nine.size() == 9);
    BOOST_CHECK(nine.front() == MerkleNode());
    BOOST_CHECK(nine.back() == MerkleNode());

    /* vector(size_type count, value_type value, const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> three_ones(3, MerkleNode(1));
    BOOST_CHECK(three_ones.size() == 3);
    BOOST_CHECK(three_ones[0] == MerkleNode(1));
    BOOST_CHECK(three_ones[1] == MerkleNode(1));
    BOOST_CHECK(three_ones[2] == MerkleNode(1));
    BOOST_CHECK(three.size() == three_ones.size());
    BOOST_CHECK(three != three_ones);

    std::vector<MerkleNode> nine_sevens(9, MerkleNode(7), alloc);
    BOOST_CHECK(nine_sevens.size() == 9);
    BOOST_CHECK(nine_sevens.front() == MerkleNode(7));
    BOOST_CHECK(nine_sevens.back() == MerkleNode(7));
    BOOST_CHECK(nine.size() == nine_sevens.size());
    BOOST_CHECK(nine != nine_sevens);

    /* void assign(size_type count, value_type value) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t.assign(3, MerkleNode(1));
        BOOST_CHECK(t == three_ones);
        std::vector<MerkleNode> t2(three_ones);
        t2.assign(9, MerkleNode(7));
        BOOST_CHECK(t2 == nine_sevens);
    }

    /* template<class InputIt> vector(InputIt first, InputIt last, const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> one_two_three;
    one_two_three.push_back(MerkleNode(1)); BOOST_CHECK(one_two_three[0].GetCode() == 1);
    one_two_three.push_back(MerkleNode(2)); BOOST_CHECK(one_two_three[1].GetCode() == 2);
    one_two_three.push_back(MerkleNode(3)); BOOST_CHECK(one_two_three[2].GetCode() == 3);

    std::list<MerkleNode> l{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    std::vector<MerkleNode> from_list(l.begin(), l.end());
    BOOST_CHECK(from_list == one_two_three);

    std::deque<MerkleNode> q{MerkleNode(3), MerkleNode(2), MerkleNode(1)};
    std::vector<MerkleNode> from_reversed_deque(q.rbegin(), q.rend(), alloc);
    BOOST_CHECK(from_reversed_deque == one_two_three);
    BOOST_CHECK(from_reversed_deque == from_list);

    /* template<class InputIt> void assign(InputIt first, InputIt last) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t.assign(from_list.begin(), from_list.end());
        BOOST_CHECK(t == one_two_three);
        std::vector<MerkleNode> t2;
        t2.assign(q.rbegin(), q.rend());
        BOOST_CHECK(t2 == one_two_three);
    }

    /* vector(std::initializer_list<value_type> ilist, const Allocator& alloc = Allocator()) */
    std::vector<MerkleNode> from_ilist{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    BOOST_CHECK(from_ilist == one_two_three);

    /* vector& operator=(std::initializer_list<value_type> ilist) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t = {MerkleNode(1), MerkleNode(2), MerkleNode(3)};
        BOOST_CHECK(t == one_two_three);
    }

    /* void assign(std::initializer_list<value_type> ilist) */
    {
        std::vector<MerkleNode> t(nine_sevens);
        t.assign({MerkleNode(1), MerkleNode(2), MerkleNode(3)});
        BOOST_CHECK(t == one_two_three);
    }

    /* vector(const vector& other) */
    {
        std::vector<MerkleNode> v123(one_two_three);
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
        BOOST_CHECK(v123 == one_two_three);
    }

    /* vector(const vector& other, const Allocator& alloc) */
    {
        std::vector<MerkleNode> v123(one_two_three, alloc);
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
        BOOST_CHECK(v123 == one_two_three);
    }

    /* vector(vector&& other) */
    {
        std::vector<MerkleNode> v123a(one_two_three);
        BOOST_CHECK(v123a == one_two_three);
        std::vector<MerkleNode> v123b(std::move(v123a));
        BOOST_CHECK(v123b == one_two_three);
    }

    /* vector(vector&& other, const Allocator& alloc) */
    {
        std::vector<MerkleNode> v123a(one_two_three);
        BOOST_CHECK(v123a == one_two_three);
        std::vector<MerkleNode> v123b(std::move(v123a), alloc);
        BOOST_CHECK(v123b == one_two_three);
    }

    /* vector& operator=(const vector& other) */
    {
        std::vector<MerkleNode> v123;
        v123 = one_two_three;
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
        BOOST_CHECK(v123 == one_two_three);
    }

    /* vector& operator=(vector&& other) */
    {
        std::vector<MerkleNode> v123;
        v123 = std::move(one_two_three);
        BOOST_CHECK(v123.size() == 3);
        BOOST_CHECK(v123[0] == MerkleNode(1));
        BOOST_CHECK(v123[1] == MerkleNode(2));
        BOOST_CHECK(v123[2] == MerkleNode(3));
    }
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_relational)
{
    std::vector<MerkleNode> a{MerkleNode(0)};
    std::vector<MerkleNode> b{MerkleNode(0), MerkleNode(1)};

    BOOST_CHECK(!(a==b));
    BOOST_CHECK(a!=b);
    BOOST_CHECK(a<b);
    BOOST_CHECK(a<=b);
    BOOST_CHECK(!(a>=b));
    BOOST_CHECK(!(a>b));

    a.push_back(MerkleNode(1));

    BOOST_CHECK(a==b);
    BOOST_CHECK(!(a!=b));
    BOOST_CHECK(!(a<b));
    BOOST_CHECK(a<=b);
    BOOST_CHECK(a>=b);
    BOOST_CHECK(!(a>b));

    a.push_back(MerkleNode(2));

    BOOST_CHECK(!(a==b));
    BOOST_CHECK(a!=b);
    BOOST_CHECK(!(a<b));
    BOOST_CHECK(!(a<=b));
    BOOST_CHECK(a>=b);
    BOOST_CHECK(a>b);
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_access)
{
    std::vector<MerkleNode> v{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    const std::vector<MerkleNode>& c = v;

    BOOST_CHECK(v == c);

    BOOST_CHECK(v.at(0) == MerkleNode(1));
    BOOST_CHECK(c.at(0) == MerkleNode(1));
    BOOST_CHECK(v.at(1) == MerkleNode(2));
    BOOST_CHECK(v.at(1) == MerkleNode(2));
    BOOST_CHECK(v.at(2) == MerkleNode(3));
    BOOST_CHECK(v.at(2) == MerkleNode(3));

    BOOST_CHECK_THROW(v.at(3), std::out_of_range);
    BOOST_CHECK_THROW(c.at(3), std::out_of_range);

    BOOST_CHECK(v[0] == MerkleNode(1));
    BOOST_CHECK(c[0] == MerkleNode(1));
    BOOST_CHECK(v[1] == MerkleNode(2));
    BOOST_CHECK(c[1] == MerkleNode(2));
    BOOST_CHECK(v[2] == MerkleNode(3));
    BOOST_CHECK(c[2] == MerkleNode(3));

    /* Known to work due to a weirdness of the packed format, used as
     * a check that the access is not bounds checked. */
    BOOST_CHECK(!v.dirty());
    BOOST_CHECK(v[3] == MerkleNode(0));
    BOOST_CHECK(!c.dirty());
    BOOST_CHECK(c[3] == MerkleNode(0));

    BOOST_CHECK(v.front() == MerkleNode(1));
    BOOST_CHECK(c.front() == MerkleNode(1));
    BOOST_CHECK(v.back() == MerkleNode(3));
    BOOST_CHECK(c.back() == MerkleNode(3));

    BOOST_CHECK(v.data()[0] == 0x29);
    BOOST_CHECK(c.data()[0] == 0x29);
    BOOST_CHECK(v.data()[1] == 0x80);
    BOOST_CHECK(c.data()[1] == 0x80);
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_iterator)
{
    std::vector<MerkleNode> v{MerkleNode(1), MerkleNode(2), MerkleNode(3)};
    const std::vector<MerkleNode>& cv = v;

    BOOST_CHECK(v.begin()[0] == MerkleNode(1));
    BOOST_CHECK(v.begin()[1] == MerkleNode(2));
    BOOST_CHECK(v.begin()[2] == MerkleNode(3));
    BOOST_CHECK(*(2 + v.begin()) == MerkleNode(3));
    auto i = v.begin();
    BOOST_CHECK(*i++ == MerkleNode(1));
    BOOST_CHECK(*i++ == MerkleNode(2));
    BOOST_CHECK(*i++ == MerkleNode(3));
    BOOST_CHECK(i-- == v.end());
    BOOST_CHECK(*i-- == MerkleNode(3));
    BOOST_CHECK(*i-- == MerkleNode(2));
    BOOST_CHECK(*i == MerkleNode(1));
    i += 2;
    BOOST_CHECK(*i == MerkleNode(3));
    BOOST_CHECK((i - v.begin()) == 2);
    i -= 2;
    BOOST_CHECK(i == v.begin());
    BOOST_CHECK(std::distance(v.begin(), v.end()) == 3);
    BOOST_CHECK((v.end() - v.begin()) == 3);

    BOOST_CHECK(cv.begin()[0] == MerkleNode(1));
    BOOST_CHECK(cv.begin()[1] == MerkleNode(2));
    BOOST_CHECK(cv.begin()[2] == MerkleNode(3));
    BOOST_CHECK(*(2 + cv.begin()) == MerkleNode(3));
    auto c = cv.begin();
    BOOST_CHECK(*c++ == MerkleNode(1));
    BOOST_CHECK(*c++ == MerkleNode(2));
    BOOST_CHECK(*c++ == MerkleNode(3));
    BOOST_CHECK(c-- == cv.cend());
    BOOST_CHECK(*c-- == MerkleNode(3));
    BOOST_CHECK(*c-- == MerkleNode(2));
    BOOST_CHECK(*c == MerkleNode(1));
    c += 2;
    BOOST_CHECK(*c == MerkleNode(3));
    BOOST_CHECK((c - v.begin()) == 2);
    c -= 2;
    BOOST_CHECK(c == cv.begin());
    BOOST_CHECK(std::distance(cv.begin(), cv.end()) == 3);
    BOOST_CHECK((cv.end() - cv.begin()) == 3);

    BOOST_CHECK(v.cbegin()[0] == MerkleNode(1));
    BOOST_CHECK(v.cbegin()[1] == MerkleNode(2));
    BOOST_CHECK(v.cbegin()[2] == MerkleNode(3));
    BOOST_CHECK(*(2 + v.cbegin()) == MerkleNode(3));
    auto c2 = v.cbegin();
    BOOST_CHECK(*c2++ == MerkleNode(1));
    BOOST_CHECK(*c2++ == MerkleNode(2));
    BOOST_CHECK(*c2++ == MerkleNode(3));
    BOOST_CHECK(c2-- == v.cend());
    BOOST_CHECK(*c2-- == MerkleNode(3));
    BOOST_CHECK(*c2-- == MerkleNode(2));
    BOOST_CHECK(*c2 == MerkleNode(1));
    c2 += 2;
    BOOST_CHECK(*c2 == MerkleNode(3));
    BOOST_CHECK((c2 - v.cbegin()) == 2);
    c2 -= 2;
    BOOST_CHECK(c2 == v.cbegin());
    BOOST_CHECK(std::distance(v.cbegin(), v.cend()) == 3);
    BOOST_CHECK((v.cend() - v.cbegin()) == 3);

    BOOST_CHECK(v.rbegin()[0] == MerkleNode(3));
    BOOST_CHECK(v.rbegin()[1] == MerkleNode(2));
    BOOST_CHECK(v.rbegin()[2] == MerkleNode(1));
    BOOST_CHECK(*(2 + v.rbegin()) == MerkleNode(1));
    auto r = v.rbegin();
    BOOST_CHECK(*r++ == MerkleNode(3));
    BOOST_CHECK(*r++ == MerkleNode(2));
    BOOST_CHECK(*r++ == MerkleNode(1));
    BOOST_CHECK(r-- == v.rend());
    BOOST_CHECK(*r-- == MerkleNode(1));
    BOOST_CHECK(*r-- == MerkleNode(2));
    BOOST_CHECK(*r == MerkleNode(3));
    r += 2;
    BOOST_CHECK(*r == MerkleNode(1));
    BOOST_CHECK((r - v.rbegin()) == 2);
    r -= 2;
    BOOST_CHECK(r == v.rbegin());
    BOOST_CHECK(std::distance(v.rbegin(), v.rend()) == 3);
    BOOST_CHECK((v.rend() - v.rbegin()) == 3);

    BOOST_CHECK(cv.rbegin()[0] == MerkleNode(3));
    BOOST_CHECK(cv.rbegin()[1] == MerkleNode(2));
    BOOST_CHECK(cv.rbegin()[2] == MerkleNode(1));
    BOOST_CHECK(*(2 + cv.rbegin()) == MerkleNode(1));
    auto rc = cv.rbegin();
    BOOST_CHECK(*rc++ == MerkleNode(3));
    BOOST_CHECK(*rc++ == MerkleNode(2));
    BOOST_CHECK(*rc++ == MerkleNode(1));
    BOOST_CHECK(rc-- == cv.rend());
    BOOST_CHECK(*rc-- == MerkleNode(1));
    BOOST_CHECK(*rc-- == MerkleNode(2));
    BOOST_CHECK(*rc == MerkleNode(3));
    rc += 2;
    BOOST_CHECK(*rc == MerkleNode(1));
    BOOST_CHECK((rc - cv.rbegin()) == 2);
    rc -= 2;
    BOOST_CHECK(rc == cv.rbegin());
    BOOST_CHECK(std::distance(cv.rbegin(), cv.rend()) == 3);
    BOOST_CHECK((cv.rend() - cv.rbegin()) == 3);

    BOOST_CHECK(v.crbegin()[0] == MerkleNode(3));
    BOOST_CHECK(v.crbegin()[1] == MerkleNode(2));
    BOOST_CHECK(v.crbegin()[2] == MerkleNode(1));
    BOOST_CHECK(*(2 + v.crbegin()) == MerkleNode(1));
    auto rc2 = v.crbegin();
    BOOST_CHECK(*rc2++ == MerkleNode(3));
    BOOST_CHECK(*rc2++ == MerkleNode(2));
    BOOST_CHECK(*rc2++ == MerkleNode(1));
    BOOST_CHECK(rc2-- == v.crend());
    BOOST_CHECK(*rc2-- == MerkleNode(1));
    BOOST_CHECK(*rc2-- == MerkleNode(2));
    BOOST_CHECK(*rc2 == MerkleNode(3));
    rc2 += 2;
    BOOST_CHECK(*rc2 == MerkleNode(1));
    BOOST_CHECK((rc2 - v.crbegin()) == 2);
    rc2 -= 2;
    BOOST_CHECK(rc2 == v.crbegin());
    BOOST_CHECK(std::distance(v.crbegin(), v.crend()) == 3);
    BOOST_CHECK((v.crend() - v.crbegin()) == 3);
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_capacity)
{
    std::vector<MerkleNode> v;
    BOOST_CHECK(v.empty());
    BOOST_CHECK(v.size() == 0);
    BOOST_CHECK(v.max_size() >= std::vector<unsigned char>().max_size());
    BOOST_CHECK(v.capacity() >= v.size());

    v.push_back(MerkleNode(1));
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 1);
    BOOST_CHECK(v.capacity() >= v.size());

    v.push_back(MerkleNode(2));
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 2);
    BOOST_CHECK(v.capacity() >= v.size());

    v.push_back(MerkleNode(3));
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 3);
    BOOST_CHECK(v.capacity() >= v.size());

    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3)}));

    v.resize(6);
    BOOST_CHECK(!v.empty());
    BOOST_CHECK(v.size() == 6);
    BOOST_CHECK(v.capacity() >= v.size());

    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0)}));

    v.shrink_to_fit();
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0)}));

    v.resize(9, MerkleNode(7));
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0),
                                              MerkleNode(7), MerkleNode(7), MerkleNode(7)}));

    v.shrink_to_fit();
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                              MerkleNode(0), MerkleNode(0), MerkleNode(0),
                                              MerkleNode(7), MerkleNode(7), MerkleNode(7)}));

    v.resize(3);
    BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
}

/*
 * Takes an iterator of any category derived from InputIterator, and
 * exposes an interface restricted to only support the InputIterator
 * API, including the input_iterator_tag. Useful for mocking an
 * InputIterator for testing non-random-access or non-bidirectional
 * code paths.
 */
template<class Iter>
struct mock_input_iterator: public std::iterator<std::input_iterator_tag, typename Iter::reference>
{
    typedef Iter inner_type;
    inner_type m_iter;

    typedef mock_input_iterator iterator;
    typedef typename inner_type::value_type value_type;
    typedef typename inner_type::difference_type difference_type;
    typedef typename inner_type::pointer pointer;
    typedef typename inner_type::reference reference;

    mock_input_iterator() = delete;

    explicit mock_input_iterator(inner_type& iter) : m_iter(iter) { }

    iterator& operator=(inner_type& iter)
    {
        m_iter = iter;
        return *this;
    }

    mock_input_iterator(const iterator&) = default;
    mock_input_iterator(iterator&&) = default;
    iterator& operator=(const iterator&) = default;
    iterator& operator=(iterator&&) = default;

    /* Distance */
    difference_type operator-(const iterator& other) const
      { return (m_iter - other.m_iter); }

    /* Equality */
    inline bool operator==(const iterator& other) const
      { return (m_iter == other.m_iter); }
    inline bool operator!=(const iterator& other) const
      { return (m_iter != other.m_iter); }

    /* Input iterators are not relational comparable */

    /* Dereference */
    inline reference operator*() const
      { return (*m_iter); }
    inline pointer operator->() const
      { return m_iter.operator->(); }

    /* Advancement */
    inline iterator& operator++()
    {
        m_iter.operator++();
        return *this;
    }

    inline iterator& operator++(int _)
      { return iterator(m_iter.operator++(_)); }
};

template<class Iter>
inline mock_input_iterator<Iter> wrap_mock_input_iterator(Iter iter)
  { return mock_input_iterator<Iter>(iter); }

BOOST_AUTO_TEST_CASE(merkle_node_vector_insert)
{
    /* void push_back(value_type value) */
    /* template<class... Args> void emplace_back(Args&&... args) */
    std::vector<MerkleNode> one_two_three;
    one_two_three.push_back(MerkleNode(1));
    one_two_three.emplace_back(static_cast<MerkleNode::code_type>(2));
    one_two_three.emplace_back(MerkleLink::DESCEND, MerkleLink::SKIP);
    BOOST_CHECK(one_two_three.size() == 3);
    BOOST_CHECK(one_two_three[0] == MerkleNode(1));
    BOOST_CHECK(one_two_three[1] == MerkleNode(2));
    BOOST_CHECK(one_two_three[2] == MerkleNode(3));

    /* void clear() */
    {
        std::vector<MerkleNode> v(one_two_three);
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v == one_two_three);
        v.clear();
        BOOST_CHECK(v.empty());
        BOOST_CHECK(one_two_three.size() == 3);
    }

    /* iterator insert(const_iterator pos, value_type value) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.insert(v.begin(), MerkleNode(0));
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v != one_two_three);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        res = v.insert(v.begin()+2, MerkleNode(4));
        BOOST_CHECK(res == (v.begin()+2));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(3)}));
        res = v.insert(v.begin()+4, MerkleNode(5));
        BOOST_CHECK(res == (v.begin()+4));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(3)}));
        res = v.insert(v.begin()+6, MerkleNode(6));
        BOOST_CHECK(res == (v.begin()+6));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(3), MerkleNode(6)}));
        res = v.insert(v.end(), MerkleNode(7));
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+7));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(3), MerkleNode(6), MerkleNode(7)}));
    }

    /* iterator insert(const_iterator pos, size_type count, value_type value) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.insert(v.begin(), 0, MerkleNode(0));
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == one_two_three);
        res = v.insert(v.begin()+1, 1, MerkleNode(4));
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(3)}));
        res = v.insert(v.begin()+3, 2, MerkleNode(5));
        BOOST_CHECK(res == (v.begin()+3));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(5), MerkleNode(3)}));
        res = v.insert(v.begin()+6, 3, MerkleNode(6));
        BOOST_CHECK(res == (v.begin()+6));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(5), MerkleNode(3), MerkleNode(6), MerkleNode(6), MerkleNode(6)}));
        res = v.insert(v.end(), 2, MerkleNode(7));
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+9));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(4), MerkleNode(2), MerkleNode(5), MerkleNode(5), MerkleNode(3), MerkleNode(6), MerkleNode(6), MerkleNode(6), MerkleNode(7), MerkleNode(7)}));
    }

    /* template<class InputIt> iterator insert(const_iterator pos, InputIt first, InputIt last) */
    {
        std::vector<MerkleNode> ones({MerkleNode(1), MerkleNode(1)});
        std::vector<MerkleNode> twos({MerkleNode(2), MerkleNode(2)});
        std::vector<MerkleNode> v;
        BOOST_CHECK(v.empty());
        auto res = v.insert(v.begin(), wrap_mock_input_iterator(ones.begin()), wrap_mock_input_iterator(ones.end()));
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1)}));
        res = v.insert(v.begin()+1, one_two_three.begin(), one_two_three.end());
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1)}));
        res = v.insert(v.end(), twos.begin(), twos.begin() + 1);
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+5));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
        std::vector<MerkleNode> v2(v);
        res = v2.insert(v2.end(), v.begin(), v.end());
        BOOST_CHECK(res != v2.end());
        BOOST_CHECK(res == (v2.begin()+6));
        BOOST_CHECK(v2 == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
        std::vector<MerkleNode> v3(v2);
        res = v3.insert(v3.begin()+1, v2.begin(), v2.end());
        BOOST_CHECK(res == (v3.begin()+1));
        BOOST_CHECK(v3 == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
        res = v3.insert(v3.begin(), one_two_three.begin(), one_two_three.end());
        BOOST_CHECK(res == v3.begin());
        BOOST_CHECK(v3 == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2), MerkleNode(1), MerkleNode(1), MerkleNode(2), MerkleNode(3), MerkleNode(1), MerkleNode(2)}));
    }

    /* iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) */
    {
        std::vector<MerkleNode> v;
        auto res = v.insert(v.begin(), {MerkleNode(1), MerkleNode(1)});
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(1)}));
        res = v.insert(v.begin()+1, {MerkleNode(2), MerkleNode(2)});
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(2), MerkleNode(1)}));
        res = v.insert(v.end(), {MerkleNode(3)});
        BOOST_CHECK(res == (v.begin()+4));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(2), MerkleNode(1), MerkleNode(3)}));
    }

    /* template<class... Args> iterator emplace(const_iterator pos, Args&&... args) */
    {
        std::vector<MerkleNode> v;
        auto res = v.emplace(v.begin());
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0)}));
        res = v.emplace(v.end(), MerkleNode(2));
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(2)}));
        res = v.emplace(res, static_cast<MerkleNode::code_type>(1));
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(2)}));
        res = v.emplace(v.end(), MerkleLink::DESCEND, MerkleLink::SKIP);
        BOOST_CHECK(res != v.end());
        BOOST_CHECK(res == (v.begin()+3));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(0), MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
    }

    /* iterator erase(const_iterator pos) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.erase(v.begin()+1);
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(3)}));
        res = v.erase(v.begin());
        BOOST_CHECK(res == v.begin());
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(3)}));
    }

    /* iterator erase(const_iterator first, const_iterator last) */
    {
        std::vector<MerkleNode> v(one_two_three);
        auto res = v.erase(v.begin()+1, v.end());
        BOOST_CHECK(res == v.end());
        BOOST_CHECK(res == (v.begin()+1));
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1)}));
    }

    /* void pop_back() */
    {
        std::vector<MerkleNode> v;
        v.insert(v.end(), one_two_three.begin(), one_two_three.end());
        v.insert(v.end(), one_two_three.begin(), one_two_three.end());
        v.insert(v.end(), one_two_three.begin(), one_two_three.end());
        BOOST_CHECK(v.size() == 9);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 8);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 7);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 6);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 5);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1), MerkleNode(2)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 4);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3),
                                                  MerkleNode(1)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 3);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2), MerkleNode(3)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 2);
        BOOST_CHECK(v == std::vector<MerkleNode>({MerkleNode(1), MerkleNode(2)}));
        v.pop_back();
        BOOST_CHECK(v.size() == 1);
        BOOST_CHECK(v == std::vector<MerkleNode>(1, MerkleNode(1)));
        v.pop_back();
        BOOST_CHECK(v.empty());
        BOOST_CHECK(v == std::vector<MerkleNode>());
    }

    /* void swap(vector& other) */
    {
        std::vector<MerkleNode> tmp;
        BOOST_CHECK(tmp.empty());
        tmp.swap(one_two_three);
        BOOST_CHECK(tmp.size() == 3);
    }
    BOOST_CHECK(one_two_three.empty());
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_dirty)
{
    static std::size_t k_vch_size[9] =
      {0, 1, 1, 2, 2, 2, 3, 3, 3};

    for (std::size_t i = 1; i <= 8; ++i) {
        std::vector<MerkleNode> v;
        for (std::size_t j = 0; j < i; ++j)
            v.emplace_back(static_cast<MerkleNode::code_type>(j));
        for (std::size_t j = 0; j < (8*k_vch_size[i]); ++j) {
            BOOST_CHECK(!v.dirty());
            v.data()[j/8] ^= (1 << (7-(j%8)));
            BOOST_CHECK((j < (3*i)) == !(v.dirty()));
            v.data()[j/8] ^= (1 << (7-(j%8)));
        }
    }
}

BOOST_AUTO_TEST_CASE(merkle_node_vector_serialize)
{
    typedef std::vector<unsigned char> vch;

    std::vector<MerkleNode> v;
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x00"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(0);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x01\x00"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(1);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x02\x04"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(2);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x03\x05\x00"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(3);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x04\x05\x30"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(4);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x05\x05\x38"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(5);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x06\x05\x39\x40"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(6);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x07\x05\x39\x70"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(7);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x08\x05\x39\x77"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    v.emplace_back(5);
    {
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, v);
        BOOST_CHECK(std::equal(ds.begin(), ds.end(), "\x09\x05\x39\x77\xa0"));
        std::vector<MerkleNode> v2;
        ds >> v2;
        BOOST_CHECK(v == v2);
    }

    {
        auto data = ParseHex("02600239361160903c6695c6804b7157c7bd10013e9ba89b1f954243bc8e3990b08db96632753d6ca30fea890f37fc150eaed8d068acf596acb2251b8fafd72db977d3");
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION, CFlatData(data.data(), data.data()+data.size()));
        MerkleProof proof;
        BOOST_CHECK_MESSAGE(ds[0] == '\x02', HexStr(ds.begin(), ds.end()).c_str());
        BOOST_CHECK_MESSAGE(ds.size() == 67, HexStr(ds.begin(), ds.end()).c_str());
        ds >> proof;
        BOOST_CHECK(ds.empty());
        BOOST_CHECK(proof.m_path.size() == 2);
        BOOST_CHECK(proof.m_path[0] == MerkleNode(MerkleLink::DESCEND, MerkleLink::SKIP));
        BOOST_CHECK(proof.m_path[1] == MerkleNode(MerkleLink::VERIFY, MerkleLink::SKIP));
        BOOST_CHECK(proof.m_skip.size() == 2);
        BOOST_CHECK(proof.m_skip[0] == uint256S("b98db090398ebc4342951f9ba89b3e0110bdc757714b80c695663c9060113639"));
        BOOST_CHECK(proof.m_skip[1] == uint256S("d377b92dd7af8f1b25b2ac96f5ac68d0d8ae0e15fc370f89ea0fa36c3d753266"));
    }
}

BOOST_AUTO_TEST_CASE(fast_merkle_branch)
{
    using std::swap;
    const std::vector<uint256> leaves = {
      (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << 'a').GetHash(),
      (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << 'b').GetHash(),
      (CHashWriter(SER_GETHASH, PROTOCOL_VERSION) << 'c').GetHash(),
    };
    const uint256 root = ComputeFastMerkleRoot(leaves);
    BOOST_CHECK(root == uint256S("0x9cde1ad752292baac9c86e91d0c0e506a3bc0e7f11fd449c8c54bbd3e46d91a1"));
    {
        std::vector<uint256> branch;
        uint32_t path;
        std::tie(branch, path) = ComputeFastMerkleBranch(leaves, 0);
        BOOST_CHECK(path == 0);
        BOOST_CHECK(branch.size() == 2);
        BOOST_CHECK(branch[0] == leaves[1]);
        BOOST_CHECK(branch[1] == leaves[2]);
        BOOST_CHECK(root == ComputeFastMerkleRootFromBranch(leaves[0], branch, path));
    }
    {
        std::vector<uint256> branch;
        uint32_t path;
        std::tie(branch, path) = ComputeFastMerkleBranch(leaves, 1);
        BOOST_CHECK(path == 1);
        BOOST_CHECK(branch.size() == 2);
        BOOST_CHECK(branch[0] == leaves[0]);
        BOOST_CHECK(branch[1] == leaves[2]);
        BOOST_CHECK(root == ComputeFastMerkleRootFromBranch(leaves[1], branch, path));
    }
    {
        std::vector<uint256> branch;
        uint32_t path;
        std::tie(branch, path) = ComputeFastMerkleBranch(leaves, 2);
        BOOST_CHECK(path == 1);
        BOOST_CHECK(branch.size() == 1);
        BOOST_CHECK(branch[0] == uint256S("0xc771140365578d348d7ffc6e04a102ecf3e2eea51177d38fac92f954aebdd1cd"));
        BOOST_CHECK(root == ComputeFastMerkleRootFromBranch(leaves[2], branch, path));
    }
    BOOST_CHECK(ComputeFastMerkleRoot({}) == MerkleTree().GetHash());
    for (int i = 1; i < 35; ++i) {
        std::vector<uint256> leaves;
        leaves.resize(i);
        for (int j = 0; j < i; ++j) {
            leaves.back().begin()[j/8] ^= ((char)1 << (j%8));
        }
        const uint256 root = ComputeFastMerkleRoot(leaves);
        for (int j = 0; j < i; ++j) {
            const auto branch = ComputeFastMerkleBranch(leaves, j);
            BOOST_CHECK(ComputeFastMerkleRootFromBranch(leaves[j], branch.first, branch.second) == root);
            std::vector<MerkleTree> subtrees(i);
            for (int k = 0; k < i; ++k) {
                if (k == j) {
                    subtrees[k].m_verify.push_back(leaves[k]);
                } else {
                    subtrees[k].m_proof.m_skip.push_back(leaves[k]);
                }
            }
            while (subtrees.size() > 1) {
                std::vector<MerkleTree> other;
                for (auto itr = subtrees.begin(); itr != subtrees.end(); ++itr) {
                    auto itr2 = std::next(itr);
                    if (itr2 != subtrees.end()) {
                        other.emplace_back(*(itr++), *itr2);
                    } else {
                        other.emplace_back();
                        swap(other.back(), *itr);
                    }
                }
                swap(subtrees, other);
            }
            BOOST_CHECK(subtrees[0].m_verify.size() == 1);
            bool invalid = false;
            BOOST_CHECK(subtrees[0].GetHash(&invalid) == root);
            BOOST_CHECK(!invalid);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
