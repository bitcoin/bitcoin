// Copyright (c) 2015-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_MERKLE_H
#define BITCOIN_CONSENSUS_MERKLE_H

#include <stdint.h>
#include <array>
#include <iterator>
#include <vector>

#include <primitives/transaction.h>
#include <primitives/block.h>
#include <serialize.h>
#include <uint256.h>

uint256 ComputeMerkleRoot(const std::vector<uint256>& leaves, bool* mutated = nullptr);
std::vector<uint256> ComputeMerkleBranch(const std::vector<uint256>& leaves, uint32_t position);
uint256 ComputeMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t position);

/*
 * Has similar API semantics, but produces Merkle roots and validates
 * branches 3x as fast, and without the mutation vulnerability. Cannot
 * be substituted for the non-fast variants because the hash values are
 * different. ComputeFastMerkleBranch returns a pair with the second
 * element being the path used to validate the branch.
 *
 * Because the fast Merkle branch does not do unnecessary hash operations,
 * the path used to validate a branch is derived from but not necessarily
 * the same as the original position in the list. ComputeFastMerkleBranch
 * calculates the path by dropping high-order zeros from the binary
 * representation of the position until the path is the same length or
 * less as the number of Merkle branches.
 *
 * To understand why this works, consider a list of 303 elements from
 * which a fast Merkle tree is constructed, and we request the branch to
 * the 292nd element. The binary encoded positions of the last and
 * desired elements are as follows:
 *
 *   0b 1 0 0 1 0 1 1 1 0 # decimal 302 (zero-indexed)
 *
 *   0b 1 0 0 1 0 0 0 1 1 # decimal 291
 *
 * The root of the Merkle tree has a left branch that contains 2^8 = 256
 * elements, and a right branch that contains the remaining 47. The first
 * level of the right branch contains 2^5 = 32 nodes on the left side, and
 * the remaining 15 nodes on the right side. The next level contains 2^3 =
 * 8 nodes on the left, and the remaining 7 on the right. This pattern
 * repeats on the right hand side of the tree: each layer has the largest
 * remaining power of two on the left, and the residual on the right.
 *
 * Notice specifically that the sizes of the sub-trees correspnd to the
 * set bits in the zero-based index of the final element. For each 1 at,
 * index n, there is a branch with 2^n elements on the left and the
 * remaining amount on the right.
 *
 * So, for an element whose path traverse the right-side of the tree, the
 * intervening levels (e.g. 2^7 and 2^6) are missing. These correspond to
 * zeros in the binary expansion, and they are removed from the path
 * description. However once the path takes a left-turn into the tree (a
 * zero where a one is present in the expansion of the last element), the
 * sub-tree is full and no more 0's can be pruned out.
 *
 * So the path for element 292 becomes:
 *
 *     0b 1 - - 1 - 0 0 1 1 # decimal 291
 *
 *   = 0b 1 1 0 0 1 1
 *
 *   = 51
 */
uint256 ComputeFastMerkleRoot(const std::vector<uint256>& leaves);
std::pair<std::vector<uint256>, uint32_t> ComputeFastMerkleBranch(const std::vector<uint256>& leaves, uint32_t position);
uint256 ComputeFastMerkleRootFromBranch(const uint256& leaf, const std::vector<uint256>& branch, uint32_t path);

/*
 * Compute the Merkle root of the transactions in a block.
 * *mutated is set to true if a duplicated subtree was found.
 */
uint256 BlockMerkleRoot(const CBlock& block, bool* mutated = nullptr);

/*
 * Compute the Merkle root of the witness transactions in a block.
 * *mutated is set to true if a duplicated subtree was found.
 */
uint256 BlockWitnessMerkleRoot(const CBlock& block, bool* mutated = nullptr);

/*
 * Compute the Merkle branch for the tree of transactions in a block, for a
 * given position.
 * This can be verified using ComputeMerkleRootFromBranch.
 */
std::vector<uint256> BlockMerkleBranch(const CBlock& block, uint32_t position);

/*
 * Each link of a Merkle tree can have one of three values in a proof
 * object:
 *
 *   DESCEND: This link connects to another sub-tree, which must be
 *     processed. The root of this sub-tree is the hash value of the
 *     link.
 *
 *   VERIFY: This hash value of this link must be provided at
 *     validation time. Computation of the Merkle root and comparison
 *     with a reference value provides a batch confirmation as to
 *     whether ALL the provided VERIFY hashes are correct.
 *
 *   SKIP: The hash value of this link is provided as part of the
 *     proof.
 */
enum class MerkleLink : unsigned char { DESCEND, VERIFY, SKIP };

/*
 * An internal node can have up to eight different structures, the
 * product of the 3 possible MerkleLink states the left and right
 * branches can have, with the exception of the {SKIP, SKIP} state
 * which would be pruned as a SKIP hash in the parent node.
 *
 * This means nodes can be represented as a 3-bit integer, and packed
 * 8 nodes to each 3 byte sequence. The MerkleNode class uses an
 * unsigned char to represent the unpacked code, whereas the
 * MerkleNodeReference class is used to access a 3-bit code value
 * within a packed representation.
 */
struct MerkleNode
{
    typedef unsigned char code_type;

protected:
    code_type m_code;

    static const std::array<MerkleLink, 8> m_left_from_code;
    static const std::array<MerkleLink, 8> m_right_from_code;

    static code_type _get_code(MerkleLink left, MerkleLink right);

public:
    explicit MerkleNode(MerkleLink left, MerkleLink right) : m_code(_get_code(left, right)) { }

    explicit MerkleNode(code_type code) : m_code(code) { }

    /* Note that a code value of 0 is a {VERIFY, SKIP} node. */
    MerkleNode() : m_code(0) { }

    /* The default behavior is adequate. */
    MerkleNode(const MerkleNode&) = default;
    MerkleNode(MerkleNode&&) = default;
    MerkleNode& operator=(const MerkleNode&) = default;
    MerkleNode& operator=(MerkleNode&&) = default;

    /*
     * Ideally this would perhaps be operator int() and operator=(),
     * however C++ does not let us mark an assingment operator as
     * explicit. This unfortunately defeats many of the protections
     * against bugs that strong typing would give us as any integer or
     * Boolean value could be mistakenly assigned and interpreted as a
     * code, and therefore assignable to a MerkleNode, probably
     * generating a memory access exception if the value is not
     * between 0 and 7.
     */
    inline code_type GetCode() const
      { return m_code; }

    inline MerkleNode& SetCode(code_type code)
    {
        m_code = code;
        return *this;
    }

    /*
     * The getters and setters for the left and right MerkleLinks
     * simply recalculate the code value using tables. The code values
     * line up such that this could be done with arithmetic and
     * shifts, but it is probably of similar efficiency.
     */
    inline MerkleLink GetLeft() const
      { return m_left_from_code[m_code]; }

    inline MerkleNode& SetLeft(MerkleLink left)
    {
        m_code = _get_code(left, m_right_from_code[m_code]);
        return *this;
    }

    inline MerkleLink GetRight() const
      { return m_right_from_code[m_code]; }

    inline MerkleNode& SetRight(MerkleLink right)
    {
        m_code = _get_code(m_left_from_code[m_code], right);
        return *this;
    }

    /* Equality */
    inline bool operator==(MerkleNode other) const
      { return (m_code == other.m_code); }
    inline bool operator!=(MerkleNode other) const
      { return !(*this == other); }

    /* Relational */
    inline bool operator<(MerkleNode other) const
      { return (m_code < other.m_code); }
    inline bool operator<=(MerkleNode other) const
      { return !(other < *this); }
    inline bool operator>=(MerkleNode other) const
      { return !(*this < other); }
    inline bool operator>(MerkleNode other) const
      { return (other < *this); }

    /* Needs access to m_{left,right}_from_code and _get_code() */
    friend struct MerkleNodeReference;
};

/*
 * Now we begin constructing the necessary infrastructure for
 * supporting an STL-like container for packed 3-bit code
 * representations of MerkleNode values. This is parallels the way
 * that std::vector<bool> is specialized, with the added complication
 * of a non-power-of-2 packed size.
 */

/*
 * First we build a "reference" class which is able to address the
 * location of a packed 3-bit value, and to read and write that value
 * without affecting its neighbors.
 *
 * Then we will make use of this MerkleNode reference type to
 * construct an STL-compatible iterator class (technically two, since
 * the class const_iterator is not a const instance of the class
 * iterator, for reasons).
 */
struct MerkleNodeReference
{
    /*
     * Nodes are stored with a tightly packed 3-bit encoding, the
     * code. This allows up to 8 node specifications to fit within 3
     * bytes:
     *
     *    -- Node index
     *   /
     *   00011122 23334445 55666777
     *    byte 0   byte 1   byte 2
     *   76543210 76543210 76543210
     *                            /
     *                Bit Index --
     *
     * A reference to a particular node consists of a pointer to the
     * beginning of this 3 byte sequence, and the index (between 0 and
     * 7) of the node.
     */
    typedef unsigned char base_type;
    typedef unsigned char offset_type;

protected:
    base_type* m_base;
    offset_type m_offset;

    /*
     * The parameterized constructor is protected because MerkleNode
     * references should only ever be created by the friended iterator
     * and container code.
     */
    MerkleNodeReference(base_type* base, offset_type offset) : m_base(base), m_offset(offset) { }

    /*
     * We're emulating a reference, not a pointer, and it doesn't make
     * sense to have a default-constructable reference.
     */
    MerkleNodeReference() = delete;

public:
    /*
     * The default copy constructors are sufficient. Note that these
     * create a new reference object that points to the same packed
     * MerkleNode value.
     */
    MerkleNodeReference(const MerkleNodeReference& other) = default;
    MerkleNodeReference(MerkleNodeReference&& other) = default;

    /*
     * Copy assignment operators are NOT the default behavior:
     * assigning one reference to another copies the underlying
     * values, to make the MerkleNodeReference objects behave like
     * references. It is NOT the same as the copy constructor, which
     * copies the reference itself.
     */
    inline MerkleNodeReference& operator=(const MerkleNodeReference& other)
      { return SetCode(other.GetCode()); }
    inline MerkleNodeReference& operator=(MerkleNodeReference&& other)
      { return SetCode(other.GetCode()); }

public:
    /* Read a 3-bit code value */
    MerkleNode::code_type GetCode() const;

    /* Write a 3-bit code value */
    MerkleNodeReference& SetCode(MerkleNode::code_type code);

    /* Read and write the MerkleLink values individually. */
    inline MerkleLink GetLeft() const
      { return MerkleNode::m_left_from_code[GetCode()]; }
    inline MerkleNodeReference& SetLeft(MerkleLink left)
      { return SetCode(MerkleNode::_get_code(left, GetRight())); }

    MerkleLink GetRight() const
      { return MerkleNode::m_right_from_code[GetCode()]; }
    MerkleNodeReference& SetRight(MerkleLink right)
      { return SetCode(MerkleNode::_get_code(GetLeft(), right)); }

    /* Equality */
    inline bool operator==(const MerkleNodeReference& other) const
      { return (GetCode() == other.GetCode()); }
    inline bool operator==(MerkleNode other) const
      { return (GetCode() == other.GetCode()); }
    inline bool operator!=(const MerkleNodeReference& other) const
      { return (GetCode() != other.GetCode()); }
    inline bool operator!=(MerkleNode other) const
      { return (GetCode() != other.GetCode()); }

    /* Relational */
    inline bool operator<(const MerkleNodeReference& other) const
      { return (GetCode() < other.GetCode()); }
    inline bool operator<(MerkleNode other) const
      { return (GetCode() < other.GetCode()); }
    inline bool operator<=(const MerkleNodeReference& other) const
      { return (GetCode() <= other.GetCode()); }
    inline bool operator<=(MerkleNode other) const
      { return (GetCode() <= other.GetCode()); }
    inline bool operator>=(const MerkleNodeReference& other) const
      { return (GetCode() >= other.GetCode()); }
    inline bool operator>=(MerkleNode other) const
      { return (GetCode() >= other.GetCode()); }
    inline bool operator>(const MerkleNodeReference& other) const
      { return (GetCode() > other.GetCode()); }
    inline bool operator>(MerkleNode other) const
      { return (GetCode() > other.GetCode()); }

    /* Conversion to/from class MerkleNode */
    inline MerkleNodeReference& operator=(const MerkleNode& other)
      { return SetCode(other.GetCode()); }
    inline operator MerkleNode() const
      { return MerkleNode(GetCode()); }

protected:
    /* Needs C(base,offset) and access to m_base and m_offset */
    friend struct MerkleNodeIteratorBase;

    /* Needs C(base,offset) */
    template<class T, class Alloc> friend class std::vector;
};

inline bool operator==(MerkleNode lhs, const MerkleNodeReference& rhs)
  { return (lhs.GetCode() == rhs.GetCode()); }
inline bool operator!=(MerkleNode lhs, const MerkleNodeReference& rhs)
  { return (lhs.GetCode() != rhs.GetCode()); }
inline bool operator<(MerkleNode lhs, const MerkleNodeReference& rhs)
  { return (lhs.GetCode() < rhs.GetCode()); }
inline bool operator<=(MerkleNode lhs, const MerkleNodeReference& rhs)
  { return (lhs.GetCode() <= rhs.GetCode()); }
inline bool operator>=(MerkleNode lhs, const MerkleNodeReference& rhs)
  { return (lhs.GetCode() >= rhs.GetCode()); }
inline bool operator>(MerkleNode lhs, const MerkleNodeReference& rhs)
  { return (lhs.GetCode() > rhs.GetCode()); }

/*
 * Now we construct an STL-compatible iterator object. If you are not
 * familiar with writing STL iterators, this might be difficult to
 * review. I will not explain how this works in detail, but I
 * encourage reviewers to compare this with how std::vector<bool>'s
 * iterators are implemented, as well as available documentation for
 * std::iterator, as necessary.
 *
 * We derive from std::iterator basically just to provide the ability
 * to specialized algorithms based on iterator category tags. All
 * iterator functionality is implemented by this class due to the
 * peculiarities of iterating over packed representations.
 *
 * Note, if you're not aware, that for STL containers const_iterator
 * is not the iterator class with a const qualifier applied. The class
 * MerkleNodeIteratorBase implements the common functionality and the
 * two iterators derive from it.
 */
struct MerkleNodeIteratorBase : public std::iterator<std::random_access_iterator_tag, MerkleNodeReference::base_type>
{
    /*
     * The value extracted from an iterator is a MerkleNode, or more
     * properly a MerkleNodeReference (iterator) or const MerkleNode
     * (const_iterator). The packed array of 3-bit code values only
     * has its values extracted and then converted to MerkleNode as
     * necessary on the fly.
     */
    typedef MerkleNode value_type;
    typedef std::ptrdiff_t difference_type;

protected:
    MerkleNodeReference m_ref;

    /*
     * A pass through initializer used by the derived iterator types,
     * since otherwise m_ref would not be accessible to their
     * constructor member initialization lists.
     */
    MerkleNodeIteratorBase(MerkleNodeReference::base_type* base, MerkleNodeReference::offset_type offset) : m_ref(base, offset) { }

    /*
     * Constructing an iterator from another iterator clones the
     * underlying reference.
     */
    MerkleNodeIteratorBase(const MerkleNodeIteratorBase&) = default;
    MerkleNodeIteratorBase(MerkleNodeIteratorBase&&) = default;

    /*
     * Copy assignment also clones the underlying reference, but a
     * non-default copy assignment is required because the underlying
     * MerkleNodeReference structure uses its own copy assignment
     * operator to emulate reference behavior, but in this context we
     * want what would otherwise be the default behavior of copying
     * the reference itself.
     */
    inline MerkleNodeIteratorBase& operator=(const MerkleNodeIteratorBase& other)
    {
       m_ref.m_base = other.m_ref.m_base;
        m_ref.m_offset = other.m_ref.m_offset;
        return *this;
    }

    inline MerkleNodeIteratorBase& operator=(MerkleNodeIteratorBase&& other)
    {
        m_ref.m_base = other.m_ref.m_base;
        m_ref.m_offset = other.m_ref.m_offset;
        return *this;
    }

public:
    /* Distance */
    difference_type operator-(const MerkleNodeIteratorBase& other) const;

    /*
     * The standard increment, decrement, advancement, etc. operators
     * for both iterator and const_iterator do the same thing and
     * could be defined here, but then they would be returning
     * instances of the base class not the iterator. So look to those
     * definitions in the derived classes below.
     */

    /*
     * Equality
     *
     * Note: Comparing the underlying reference directly with
     *       MerkleNodeReference::operator== and friends would result
     *       in the underlying values being compared, not the memory
     *       addresses.
     */
    inline bool operator==(const MerkleNodeIteratorBase& other) const
    {
        return m_ref.m_base == other.m_ref.m_base
            && m_ref.m_offset == other.m_ref.m_offset;
    }

    inline bool operator!=(const MerkleNodeIteratorBase& other) const
      { return !(*this == other); }

    /* Relational */
    inline bool operator<(const MerkleNodeIteratorBase& other) const
    {
        return m_ref.m_base < other.m_ref.m_base
           || (m_ref.m_base == other.m_ref.m_base && m_ref.m_offset < other.m_ref.m_offset);
    }

    inline bool operator<=(const MerkleNodeIteratorBase& other) const
      { return !(other < *this); }
    inline bool operator>=(const MerkleNodeIteratorBase& other) const
      { return !(*this < other); }
    inline bool operator>(const MerkleNodeIteratorBase& other) const
      { return other < *this; }

protected:
    /*
     * Move to the next 3-bit code value, incrementing m_base (by 3!)
     * only if we've gone past the end of a 3-byte block of 8 code
     * values.
     */
    void _incr();

    /*
     * Move to the prior 3-bit code value, moving m_back back (by 3)
     * if we've gone past the beginning of the 3-byte block of 8 code
     * values it points to.
     */
    void _decr();

    /*
     * Move an arbitrary number of elements forward or backwards. for
     * random access (see related operator-() definition below).
     */
    void _seek(difference_type distance);
};

/*
 * Forward random access iterator, using the _incr(), _decr(), _seek()
 * and operator-() methods of MerkleNodeIteratorBase, which is the
 * important business logic. This class is mostly just API wrappers to
 * provide an API interface close enough to API compatible with STL
 * iterators to be usable with other standard library generics.
 */
struct MerkleNodeIterator : public MerkleNodeIteratorBase
{
    typedef MerkleNodeIterator iterator;
    typedef MerkleNodeReference reference;
    typedef MerkleNodeReference* pointer;

    /*
     * Default constructor makes an unusable iterator, but required so
     * that an iterator variable can declared and initialized
     * separately.
     */
    MerkleNodeIterator() : MerkleNodeIteratorBase(nullptr, 0) { }

    /* Default copy/move constructors and assignment operators are fine. */
    MerkleNodeIterator(const MerkleNodeIterator& other) = default;
    MerkleNodeIterator(MerkleNodeIterator&& other) = default;
    MerkleNodeIterator& operator=(const MerkleNodeIterator& other) = default;
    MerkleNodeIterator& operator=(MerkleNodeIterator&& other) = default;

protected:
    MerkleNodeIterator(MerkleNodeReference::base_type* base, MerkleNodeReference::offset_type offset) : MerkleNodeIteratorBase(base, offset) { }

public:
    /* Dereference */
    inline reference operator*() const
      { return m_ref; }
    inline pointer operator->() const
      { return const_cast<pointer>(&m_ref); }

    /* Advancement */
    inline iterator& operator++()
    {
        _incr();
        return *this;
    }

    inline iterator operator++(int)
    {
        iterator ret(*this);
        _incr();
        return ret;
    }

    inline iterator& operator--()
    {
        _decr();
        return *this;
    }

    inline iterator operator--(int)
    {
        iterator ret(*this);
        _decr();
        return ret;
    }

    /* Random access */
    inline difference_type operator-(const MerkleNodeIterator& other) const
    {
        /*
         * The base class implements this correctly, but since we
         * define another overload of operator-() below, we need to
         * explicitly implement this variant too.
         */
        return MerkleNodeIteratorBase::operator-(other);
    }

    inline iterator& operator+=(difference_type n)
    {
        _seek(n);
        return *this;
    }

    inline iterator& operator-=(difference_type n)
    {
        _seek(-n);
        return *this;
    }

    inline iterator operator+(difference_type n) const
    {
        iterator ret(*this);
        ret._seek(n);
        return ret;
    }

    inline iterator operator-(difference_type n) const
    {
        iterator ret(*this);
        ret._seek(-n);
        return ret;
    }

    inline reference operator[](difference_type n) const
      { return *(*this + n); }

    /* std::vector<Node> specialization uses C(base,offset) */
    template<class T, class Alloc> friend class std::vector;
};

inline MerkleNodeIterator operator+(MerkleNodeIterator::difference_type n, const MerkleNodeIterator& x)
  { return (x + n); }

struct MerkleNodeConstIterator : public MerkleNodeIteratorBase
{
    typedef MerkleNodeConstIterator iterator;
    typedef const MerkleNodeReference reference;
    typedef const MerkleNodeReference* pointer;

    /* Creates an unusable iterator with a sentinal value. */
    MerkleNodeConstIterator() : MerkleNodeIteratorBase(nullptr, 0) { }

    /* Pass-through constructor of the m_ref field. */
    MerkleNodeConstIterator(const MerkleNodeIterator& other) : MerkleNodeIteratorBase(static_cast<const MerkleNodeIteratorBase&>(other)) { }

    /* Default copy/move constructors and assignment operators are fine. */
    MerkleNodeConstIterator(const MerkleNodeConstIterator& other) = default;
    MerkleNodeConstIterator(MerkleNodeConstIterator&& other) = default;
    MerkleNodeConstIterator& operator=(const MerkleNodeConstIterator& other) = default;
    MerkleNodeConstIterator& operator=(MerkleNodeConstIterator&& other) = default;

protected:
    /*
     * const_cast is required (and allowed) because the const
     * qualifier is only dropped to copy its value into m_ref. No API
     * is provided to actually manipulate the underlying value of the
     * reference by this class.
     */
    MerkleNodeConstIterator(const MerkleNodeReference::base_type* base, MerkleNodeReference::offset_type offset) : MerkleNodeIteratorBase(const_cast<MerkleNodeReference::base_type*>(base), offset) { }

public:
  /* Dereference */
    inline reference operator*() const
      { return m_ref; }
    inline pointer operator->() const
      { return &m_ref; }

    /* Advancement */
    inline iterator& operator++()
    {
        _incr();
        return *this;
    }

    inline iterator operator++(int)
    {
        iterator tmp = *this;
        _incr();
        return tmp;
    }

    inline iterator& operator--()
    {
        _decr();
        return *this;
    }

    inline iterator operator--(int)
    {
        iterator tmp = *this;
        _decr();
        return tmp;
    }

    /* Random access */
    inline difference_type operator-(const MerkleNodeConstIterator& other) const
    {
      /*
       * The base class implements this correctly, but since we define
       * another version of operator-() below, we need to explicitly
       * implement this variant too.
       */
      return MerkleNodeIteratorBase::operator-(other);
    }

    inline iterator& operator+=(difference_type n)
    {
        _seek(n);
        return *this;
    }

    inline iterator& operator-=(difference_type n)
    {
        *this += -n;
        return *this;
    }

    inline iterator operator+(difference_type n) const
    {
        iterator tmp = *this;
        return tmp += n;
    }

    inline iterator operator-(difference_type n) const
    {
        iterator tmp = *this;
        return tmp -= n;
    }

    inline reference operator[](difference_type n) const
      { return *(*this + n); }

    /* std::vector<MerkleNode> uses C(base,offset) */
    template<class T, class Alloc> friend class std::vector;
};

inline MerkleNodeConstIterator operator+(MerkleNodeConstIterator::difference_type n, const MerkleNodeConstIterator& x)
  { return x + n; }

/*
 * Now we are ready to define the specialization of std::vector for
 * packed 3-bit MerkleNode values. We use a std::vector<unsigned char>
 * as the underlying container to hold the encoded bytes, with 3
 * packed MerkleNodes per byte. We then provide a std::vector
 * compatible API to return iterators over MerkleNodeReference objects
 * inside this packed array.
 */
namespace std {
template<class Allocator>
class vector<MerkleNode, Allocator>
{
public:
    typedef MerkleNode value_type;
    typedef MerkleNodeReference::base_type base_type;
    typedef typename Allocator::template rebind<value_type>::other allocator_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef MerkleNodeReference reference;
    typedef const MerkleNodeReference const_reference;
    typedef MerkleNodeReference* pointer;
    typedef const MerkleNodeReference* const_pointer;
    typedef MerkleNodeIterator iterator;
    typedef MerkleNodeConstIterator const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

protected:
    /*
     * A std::vector<unsignd char> is what is actually used to store
     * the packed Node representation.
     */
    typedef typename Allocator::template rebind<base_type>::other base_allocator_type;
    typedef std::vector<base_type, base_allocator_type> vch_type;

    /*
     * m_vch.size() is (3 * m_count) / 8, but because of the truncation
     * we can't know from m_vch.size() alone how many nodes are in the
     * tree structure, so the count needs to be stored explicitly.
     */
    vch_type m_vch;
    size_type m_count;

    /* Returns the required size of m_vch to contain count packed Nodes. */
    static inline const typename vch_type::size_type _vch_size(size_type count)
      { return (3 * count + 7) / 8; }

public:
    explicit vector(const Allocator& alloc = Allocator()) : m_vch(static_cast<base_allocator_type>(alloc)), m_count(0) { }

    /*
     * Yes, this doesn't allow specifying the allocator. That is a bug
     * in C++11, fixed in C++14. However we aim for exact
     * compatibility with the version of C++ used by Bitcoin Core,
     * which is still pegged to C++11.
     *
     * Note: Because m_vch is a vector of a primitive type, its values
     *       are value initialized to zero according to the C++11
     *       standard. We don't need to do anything. Note, however,
     *       that in prior versions of the standard the behavior was
     *       different and this implementation will not work with
     *       pre-C++11 compilers.
     */
    explicit vector(size_type count) : m_vch(_vch_size(count)), m_count(count) { }

    vector(size_type count, value_type value, const Allocator& alloc = Allocator()) : m_vch(_vch_size(count), 0, static_cast<base_allocator_type>(alloc)), m_count(count)
    {
        MerkleNode::code_type code = value.GetCode();
        if (code) // Already filled with zeros
            _fill(0, count, code);
    }

    /* Assign constructors */
    template<class InputIt>
    vector(InputIt first, InputIt last, const Allocator& alloc = Allocator()) : m_vch(static_cast<base_allocator_type>(alloc)), m_count(0)
      { insert(begin(), first, last); }

    vector(std::initializer_list<value_type> ilist, const Allocator& alloc = Allocator()) : m_vch(static_cast<base_allocator_type>(alloc)), m_count(0)
      { assign(ilist); }

    /* Copy constructors */
    vector(const vector& other) = default;
    vector(const vector& other, const Allocator& alloc) : m_vch(other.m_vch, static_cast<base_allocator_type>(alloc)), m_count(other.m_count) { }
    vector(vector&& other) = default;
    vector(vector&& other, const Allocator& alloc) : m_vch(other.m_vch, static_cast<base_allocator_type>(alloc)), m_count(other.m_count) { }

    /* Assignment operators */
    vector& operator=(const vector& other) = default;
    vector& operator=(vector&& other) = default;

    inline vector& operator=(std::initializer_list<value_type> ilist)
    {
        assign(ilist);
        return *this;
    }

    /* Equality comparators */
    inline bool operator==(const vector &other) const
      { return ((m_count == other.m_count) && (m_vch == other.m_vch)); }
    inline bool operator!=(const vector &other) const
      { return !(*this == other); }

    /* Relational compariators */
    inline bool operator<(const vector &other) const
      { return ((m_vch < other.m_vch) || ((m_vch == other.m_vch) && (m_count < other.m_count))); }
    inline bool operator<=(const vector &other) const
      { return !(other < *this); }
    inline bool operator>=(const vector &other) const
      { return !(*this < other); }
    inline bool operator>(const vector &other) const
      { return (other < *this); }

    /* Clear & assign methods */
    void assign(size_type count, value_type value)
    {
        clear();
        insert(begin(), count, value);
    }

    template<class InputIt>
    void assign(InputIt first, InputIt last)
    {
        clear();
        insert(begin(), first, last);
    }

    void assign(std::initializer_list<value_type> ilist)
    {
        clear();
        reserve(ilist.size());
        for (auto node : ilist)
            push_back(node);
    }

    allocator_type get_allocator() const
      { return allocator_type(m_vch.get_allocator()); }

    /* Item access */
    reference at(size_type pos)
    {
        if (!(pos < size()))
            throw std::out_of_range("vector<Node>::at out of range");
        return (*this)[pos];
    }

    const_reference at(size_type pos) const
    {
        if (!(pos < size()))
            throw std::out_of_range("vector<Node>::at out of range");
        return (*this)[pos];
    }

    inline reference operator[](size_type pos)
      { return reference(data() + (3 * (pos / 8)), pos % 8); }
    inline const_reference operator[](size_type pos) const
      { return const_reference(const_cast<const_reference::base_type*>(data() + (3 * (pos / 8))), pos % 8); }

    inline reference front()
      { return (*this)[0]; }
    inline const_reference front() const
      { return (*this)[0]; }

    inline reference back()
      { return (*this)[m_count-1]; }
    inline const_reference back() const
      { return (*this)[m_count-1]; }

    inline base_type* data()
      { return m_vch.data(); }
    inline const base_type* data() const
      { return m_vch.data(); }

    /* Iterators */
    inline iterator begin() noexcept
      { return iterator(data(), 0); }
    inline const_iterator begin() const noexcept
      { return const_iterator(data(), 0); }
    inline const_iterator cbegin() const noexcept
      { return begin(); }

    inline iterator end() noexcept
      { return iterator(data() + (3 * (m_count / 8)), m_count % 8); }
    inline const_iterator end() const noexcept
      { return const_iterator(data() + (3 * (m_count / 8)), m_count % 8); }
    inline const_iterator cend() const noexcept
      { return end(); }

    inline reverse_iterator rbegin() noexcept
      { return reverse_iterator(end()); }
    inline const_reverse_iterator rbegin() const noexcept
      { return const_reverse_iterator(end()); }
    inline const_reverse_iterator crbegin() const noexcept
      { return rbegin(); }

    inline reverse_iterator rend() noexcept
      { return reverse_iterator(begin()); }
    inline const_reverse_iterator rend() const noexcept
      { return const_reverse_iterator(begin()); }
    inline const_reverse_iterator crend() const noexcept
      { return rend(); }

    /* Size and capacity */
    inline bool empty() const noexcept
      { return !m_count; }

    inline size_type size() const noexcept
      { return m_count; }

    inline size_type max_size() const noexcept
    {
        /* We must be careful in what we return due to overflow. */
        return std::max(m_vch.max_size(), 8 * m_vch.max_size() / 3);
    }

    inline void reserve(size_type new_cap)
      { m_vch.reserve(_vch_size(new_cap)); }

    inline size_type capacity() const noexcept
    {
        /* Again, careful of overflow, although it is more of a
         * theoretical concern here since such limitations would only
         * be encountered if the vector consumed more than 1/8th of
         * the addressable memory range. */
        return std::max(m_count, 8 * m_vch.capacity() / 3);
    }

    inline void resize(size_type count)
      { resize(count, value_type()); }

    void resize(size_type count, value_type value)
    {
        auto old_count = m_count;
        _resize(count);
        if (old_count < count)
            _fill(old_count, count, value.GetCode());
    }

    inline void shrink_to_fit()
      { m_vch.shrink_to_fit(); }

protected:
    /*
     * Resizes the underlying vector to support the number of packed
     * Nodes specified. Does NOT initialize any newly allocated bytes,
     * except for the unused bits in the last byte when shrinking or
     * the last new byte added, to prevent acquisition of dirty
     * status. It is the responsibility of the caller to initialize
     * any added new MerkleNodes.
     */
    void _resize(size_type count)
    {
        if (count < m_count) {
            /* Clear bits of elements being removed in the new last
             * byte, for the purpose of not introducing dirty
             * status. */
            _fill(count, std::min((count + 7) & ~7, m_count), 0);
        }
        size_type new_vch_size = _vch_size(count);
        m_vch.resize(new_vch_size);
        if (m_count < count) {
            /* Clear the last byte, if a byte is being added, so that
             * none of the extra bits introduce dirty status. */
            if (new_vch_size > _vch_size(m_count)) {
                m_vch.back() = 0;
            }
        }
        m_count = count;
    }

    /*
     * A memmove()-like behavior over the packed elements of this
     * container. The source and the destination are allowed to
     * overlap. Any non-overlap in the source is left with its prior
     * value intact.
     */
    void _move(size_type first, size_type last, size_type dest)
    {
        /* TODO: This could be made much faster by copying chunks at a
         *       time. This far less efficient approach is taken
         *       because it is more obviously correct and _move is not
         *       in the pipeline critical to validation performance. */
        if (dest < first) {
            std::copy(begin()+first, begin()+last, begin()+dest);
        }
        if (first < dest) {
            dest += last - first;
            std::copy_backward(begin()+first, begin()+last, begin()+dest);
        }
    }

    /*
     * A std::fill()-like behavior over a range of the packed elements
     * of this container.
     */
    void _fill(size_type first, size_type last, MerkleNode::code_type value)
    {
        /* TODO: This could be made much faster for long ranges by
         *       precomputing the 3-byte repeating sequence and using
         *       that for long runs. However this method mostly exists
         *       for API compatability with std::vector, and is not
         *       used by Merkle tree manipulation code, which at best
         *       very infrequently needs to fill a range of serialized
         *       MerkleNode code values. */
        std::fill(begin()+first, begin()+last, MerkleNode(value));
    }

public:
    void clear() noexcept
    {
        m_vch.clear();
        m_count = 0;
    }

    iterator insert(const_iterator pos, value_type value)
    {
        difference_type n = pos - cbegin();
        _resize(m_count + 1);
        _move(n, m_count-1, n+1);
        operator[](n) = value;
        return (begin() + n);
    }

    iterator insert(const_iterator pos, size_type count, value_type value)
    {
        difference_type n = pos - cbegin();
        _resize(m_count + count);
        _move(n, m_count-count, n+count);
        _fill(n, n+count, value.GetCode());
        return (begin() + n);
    }

    /*
     * TODO: This implementation should be correct. It's a rather
     *       trivial method to implement. However (1) the lack of easy
     *       non-interactive input iterators in the standard library
     *       makes this difficult to write tests for; and (2) it's not
     *       currently used anyway. Still, for exact compatibility
     *       with the standard container interface this should get
     *       tested and implemented.
     */
    template<class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last, std::input_iterator_tag)
    {
        difference_type n = pos - cbegin();
        for (; first != last; ++first, ++pos)
            pos = insert(pos, *first);
        return (begin() + n);
    }

    template<class InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last, std::forward_iterator_tag)
    {
      auto n = std::distance(cbegin(), pos);
      auto len = std::distance(first, last);
      _resize(m_count + len);
      _move(n, m_count-len, n+len);
      auto pos2 = begin() + n;
      for (; first != last; ++first, ++pos2)
          *pos2 = *first;
      return (begin() + n);
    }

    template<class InputIt>
    inline iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        /* Use iterator tag dispatching to be able to pre-allocate the
         * space when InputIt is a random access iterator, to prevent
         * multiple resizings on a large insert. */
        return insert(pos, first, last, typename iterator_traits<InputIt>::iterator_category());
    }

    inline iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
      { return insert(pos, ilist.begin(), ilist.end()); }

    template<class... Args>
    iterator emplace(const_iterator pos, Args&&... args)
    {
        difference_type n = pos - cbegin();
        _resize(m_count + 1);
        _move(n, m_count-1, n+1);
        auto ret = begin() + n;
        *ret = MerkleNode(args...);
        return ret;
    }

    iterator erase(const_iterator pos)
    {
        difference_type n = pos - cbegin();
        _move(n+1, m_count, n);
        _resize(m_count - 1);
        return (begin() + n);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        auto n = std::distance(cbegin(), first);
        auto len = std::distance(first, last);
        _move(n+len, m_count, n);
        _resize(m_count - len);
        return (begin() + n);
    }

    void push_back(value_type value)
    {
        if (m_vch.size() < _vch_size(m_count+1))
            m_vch.push_back(0);
        (*this)[m_count++] = value;
    }

    template<class... Args>
    void emplace_back(Args&&... args)
    {
        if (m_vch.size() < _vch_size(m_count+1))
            m_vch.push_back(0);
        (*this)[m_count++] = MerkleNode(args...);
    }

    void pop_back()
    {
        (*this)[m_count-1].SetCode(0);
        if (_vch_size(m_count-1) < m_vch.size())
            m_vch.pop_back();
        --m_count;
    }

    void swap(vector& other)
    {
        m_vch.swap(other.m_vch);
        std::swap(m_count, other.m_count);
    }

public:
    base_type dirty() const
    {
        switch (m_count%8) {
            case 0:  return 0;
            case 1:  return m_vch.back() & 0x1f;
            case 2:  return m_vch.back() & 0x03;
            case 3:  return m_vch.back() & 0x7f;
            case 4:  return m_vch.back() & 0x0f;
            case 5:  return m_vch.back() & 0x01;
            case 6:  return m_vch.back() & 0x3f;
            default: return m_vch.back() & 0x07;
        }
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        /* The size of the node array is prefixed by the number of
         * nodes, not the number of bytes which much then be
         * read. _resize() handles conversion between the two. */
        uint64_t count = m_count;
        READWRITE(VARINT(count));
        if (ser_action.ForRead())
            _resize(static_cast<size_type>(count));
        /* The size of the underlying storage vector is always kept
         * exactly equal to the minimum number of bytes necessary to
         * store the number 3-bit packed code values, so we can just
         * read and write it as a plain old data. */
        if (!m_vch.empty()) {
            READWRITE(REF(CFlatData(m_vch)));
        }
    }
};

template<class A> inline bool operator==(const vector<MerkleNode, A> &lhs, const vector<MerkleNode, A> &rhs)
  { return lhs.operator==(rhs); }
template<class A> inline bool operator!=(const vector<MerkleNode, A> &lhs, const vector<MerkleNode, A> &rhs)
  { return lhs.operator!=(rhs); }

template<class A> inline bool operator<(const vector<MerkleNode, A> &lhs, const vector<MerkleNode, A> &rhs)
  { return lhs.operator<(rhs); }
template<class A> inline bool operator<=(const vector<MerkleNode, A> &lhs, const vector<MerkleNode, A> &rhs)
  { return lhs.operator<=(rhs); }
template<class A> inline bool operator>=(const vector<MerkleNode, A> &lhs, const vector<MerkleNode, A> &rhs)
  { return lhs.operator>=(rhs); }
template<class A> inline bool operator>(const vector<MerkleNode, A> &lhs, const vector<MerkleNode, A> &rhs)
  { return lhs.operator>(rhs); }
} // namespace std

template<typename Stream, typename T, typename A>
void Serialize_impl(Stream& os, const std::vector<T, A>& v, const MerkleNode&)
{
    v.Serialize(os);
}

template<typename Stream, typename T, typename A>
void Unserialize_impl(Stream& is, std::vector<T, A>& v, const MerkleNode&)
{
    v.Unserialize(is);
}

/*
 * TraversalPredicate (used below) is a template parameter
 *   representing a callable object or function pointer which supports
 *   the following API:
 *
 * bool operator()(std::vector<Node>::size_t depth, MerkleLink value, bool right)
 *
 * It is called by the traversal routines both to process the contents
 * of the tree (for example, calculate the Merkle root), or report
 * reaching the end of traversal.
 *
 * size_t depth
 *
 *   The depth of the MerkleLink being processed. MerkleLinks of the
 *   root node have a depth of 1. Depth 0 would be the depth of a
 *   single-hash tree, which has no internal nodes.
 *
 * MerkleLink value
 *
 *   The value of the link, one of MerkleLink::DESCEND,
 *   MerkleLink::VERIFY, or MerkleLink::SKIP.
 *
 * bool right
 *
 *   false if this is a left-link, true if this is a right-link.
 *
 * returns bool
 *
 *   false if traversal should continue. true causes traversal to
 *   terminate and an iterator to the node containing the link in
 *   question to be returned to the caller.
 */

/*
 * Does a depth-first traversal of a tree. Starting with the root node
 * first, the left link is passed to pred then advanced, and if it is
 * MerkleLink::DESCEND then its sub-tree is recursively processed,
 * then the right link followed by its sub-tree. Traversal ends the
 * first time any of following conditions hold true:
 *
 *   1. first == last;
 *   2. the entire sub-tree with first as the root node has been
 *      processed; or
 *   3. pred() returns true.
 *
 * Iter first
 *
 *   The root of the subtree to be processed.
 *
 * Iter last
 *
 *   One past the last node to possibly be processed. Traversal will
 *   end earlier if it reaches the end of the subtree, or if
 *   TraversalPredicate returns true.
 *
 * TraversalPredicate pred
 *
 *   A callable object or function pointer that executed for each link
 *   in the tree. It is passed the current depth, the MerkleLink
 *   value, and a Boolean value indicating whether it is the left or
 *   the right link.
 *
 * returns std::pair<Iter, bool>
 *
 *   Returns the iterator pointing to the node where traversal
 *   terminated, and a Boolean value indicating whether it was the
 *   left (false) or right (true) branch that triggered termination,
 *   or {last, incomplete} if termination is due to hitting the end of
 *   the range, where incomplete is a Boolean value indicating whether
 *   termination was due to finishing the subtree (false) or merely
 *   hitting the end of the range (true).
 */
template<class Iter, class TraversalPredicate>
std::pair<Iter, bool> depth_first_traverse(Iter first, Iter last, TraversalPredicate pred)
{
    /* Depth-first traversal uses space linear with respect to the
     * depth of the tree, which is logarithmetic in the case of a
     * balanced tree. What is stored is a path from the root to the
     * node under consideration, and a record of whether it was the
     * left (false) or right (true) branch that was taken. */
    std::vector<std::pair<Iter, bool> > stack;

    for (auto pos = first; pos != last; ++pos) {
        /* Each branch is processed the same. First we check if the
         * branch meets the user-provided termination criteria. Then
         * if it is a MerkleLink::DESCEND we save our position on the
         * stack and "recurse" down the link into the next layer. */
        if (pred(stack.size()+1, pos->GetLeft(), false)) {
            return {pos, false};
        }
        if (pos->GetLeft() == MerkleLink::DESCEND) {
            stack.emplace_back(pos, false);
            continue;
        }

        /* If the left link was MerkleLink::VERIFY or
         * MerkleLink::SKIP, we continue on to the right branch in the
         * same way. */
        if (pred(stack.size()+1, pos->GetRight(), true)) {
            return {pos, true};
        }
        if (pos->GetRight() == MerkleLink::DESCEND) {
            stack.emplace_back(pos, true);
            continue;
        }

        /* After processing a leaf node (neither left nor right
         * branches are MerkleLink:DESCEND) we move up the path,
         * processing the right branches of nodes for which we had
         * descended the left-branch. */
        bool done = false;
        while (!stack.empty() && !done) {
            if (stack.back().second) {
                stack.pop_back();
            } else {
                if (pred(stack.size(), stack.back().first->GetRight(), true)) {
                    return {stack.back().first, true};
                }
                stack.back().second = true;
                if (stack.back().first->GetRight() == MerkleLink::DESCEND) {
                    done = true;
                }
            }
        }

        /* We get to this point only after retreating up the path and
         * hitting an inner node for which the right branch has not
         * been explored (in which case the stack is not empty and we
         * continue), or if we have completed processing the entire
         * subtree, in which case traversal terminates. */
        if (stack.empty())
            return {++pos, false};
    }

    /* The user-provided traversal predicate did not at any point
     * terminate traversal, but we nevertheless hit the end of the
     * traversal range with portions of the subtree still left
     * unexplored. */
    return {last, true};
}

/*
 * A MerkleProof is a transportable structure that contains the
 * information necessary to verify the root of a Merkle tree given N
 * accompanying "verify" hashes. The proof consists of those portions
 * of the tree which can't be pruned, and M "skip" hashes, each of
 * which is either the root hash of a fully pruned subtree, or a leaf
 * value not included in the set of "verify" hashes.
 */
struct MerkleProof
{
    typedef std::vector<MerkleNode> path_type;
    path_type m_path;

    typedef std::vector<uint256> skip_type;
    skip_type m_skip;

    MerkleProof(path_type&& path, skip_type&& skip) : m_path(path), m_skip(skip) { }

    /* Default constructors and assignment operators are fine */
    MerkleProof() = default;
    MerkleProof(const MerkleProof&) = default;
    MerkleProof(MerkleProof&&) = default;
    MerkleProof& operator=(const MerkleProof&) = default;
    MerkleProof& operator=(MerkleProof&&) = default;

    void clear() noexcept;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(REF(m_path));
        /* The standard serialization primitives for a vector involves
         * using the Satoshi-defined CompactSize format, which isn't
         * actually a very nice format to for most purposes when
         * compared with the VarInt encoding developed by Pieter
         * Wuille. Since we have the freedom of defining a new
         * serialization format for MerkleProofs, we choose to
         * explicitly use the latter here. */
        uint64_t skip_size = m_skip.size();
        READWRITE(VARINT(skip_size));
        if (ser_action.ForRead())
            m_skip.resize(static_cast<skip_type::size_type>(skip_size));
        /* Read/write hashes as plain old data. */
        if (!m_skip.empty()) {
            READWRITE(REF(CFlatData(m_skip)));
        }
    }
};

void swap(MerkleProof& lhs, MerkleProof& rhs);

/*
 * A MerkleTree combines a MerkleProof with a vector of "verify" hash
 * values. It also contains methods for re-computing the root hash.
 */
struct MerkleTree
{
    typedef MerkleProof proof_type;
    proof_type m_proof;

    typedef proof_type::skip_type verify_type;
    verify_type m_verify;

    /* Builds a new Merkle tree with the specified left-branch and
     * right-branch, including properly handling the case of left or
     * right being a single hash. */
    MerkleTree(const MerkleTree& left, const MerkleTree& right);

    /* Default constructors and assignment operators are fine */
    MerkleTree() = default;
    MerkleTree(const MerkleTree&) = default;
    MerkleTree(MerkleTree&&) = default;
    MerkleTree& operator=(const MerkleTree&) = default;
    MerkleTree& operator=(MerkleTree&&) = default;

    void clear() noexcept;

    /* Calculates the root hash of the MerkleTree, a process that
     * requires a depth first traverse of the full tree using linear
     * time and logarithmic (depth) space.. */
    uint256 GetHash(bool* invalid = nullptr) const;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(REF(m_proof));
        /* See the note in MerkleProof about CompactSize vs VarInt. */
        uint64_t verify_size = m_verify.size();
        READWRITE(VARINT(verify_size));
        if (ser_action.ForRead())
            m_verify.resize(static_cast<verify_type::size_type>(verify_size));
        /* Read/write hashes as plain old data. */
        if (!m_verify.empty()) {
            READWRITE(REF(CFlatData(m_verify)));
        }
    }
};

void swap(MerkleTree& lhs, MerkleTree& rhs);

#endif // BITCOIN_CONSENSUS_MERKLE_H
