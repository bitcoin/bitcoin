// -----------------------------------------------------------
//
//   Copyright (c) 2001-2002 Chuck Allison and Jeremy Siek
//        Copyright (c) 2003-2006, 2008 Gennaro Prota
//             Copyright (c) 2014 Ahmed Charles
//
// Copyright (c) 2014 Glen Joseph Fernandes
// (glenjofe@gmail.com)
//
// Copyright (c) 2014 Riccardo Marcangelo
//             Copyright (c) 2018 Evgeny Shulgin
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// -----------------------------------------------------------

#ifndef BOOST_DYNAMIC_BITSET_DYNAMIC_BITSET_HPP
#define BOOST_DYNAMIC_BITSET_DYNAMIC_BITSET_HPP

#include <assert.h>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <iterator>     // used to implement append(Iter, Iter)
#include <vector>
#include <climits>      // for CHAR_BIT

#include "boost/dynamic_bitset/config.hpp"

#ifndef BOOST_NO_STD_LOCALE
#  include <locale>
#endif

#if defined(BOOST_OLD_IOSTREAMS)
#  include <iostream.h>
#  include <ctype.h> // for isspace
#else
#  include <istream>
#  include <ostream>
#endif

#include "boost/dynamic_bitset_fwd.hpp"
#include "boost/dynamic_bitset/detail/dynamic_bitset.hpp"
#include "boost/dynamic_bitset/detail/lowest_bit.hpp"
#include "boost/move/move.hpp"
#include "boost/limits.hpp"
#include "boost/static_assert.hpp"
#include "boost/core/addressof.hpp"
#include "boost/core/no_exceptions_support.hpp"
#include "boost/throw_exception.hpp"
#include "boost/functional/hash/hash.hpp"


namespace boost {

template <typename Block, typename Allocator>
class dynamic_bitset
{
    // Portability note: member function templates are defined inside
    // this class definition to avoid problems with VC++. Similarly,
    // with the member functions of nested classes.
    //
    // [October 2008: the note above is mostly historical; new versions
    // of VC++ are likely able to digest a more drinking form of the
    // code; but changing it now is probably not worth the risks...]

    BOOST_STATIC_ASSERT((bool)detail::dynamic_bitset_impl::allowed_block_type<Block>::value);
    typedef std::vector<Block, Allocator> buffer_type;

public:
    typedef Block block_type;
    typedef Allocator allocator_type;
    typedef std::size_t size_type;
    typedef typename buffer_type::size_type block_width_type;

    BOOST_STATIC_CONSTANT(block_width_type, bits_per_block = (std::numeric_limits<Block>::digits));
    BOOST_STATIC_CONSTANT(size_type, npos = static_cast<size_type>(-1));


public:

    // A proxy class to simulate lvalues of bit type.
    //
    class reference
    {
        friend class dynamic_bitset<Block, Allocator>;


        // the one and only non-copy ctor
        reference(block_type & b, block_width_type pos)
            :m_block(b),
             m_mask( (assert(pos < bits_per_block),
                      block_type(1) << pos )
                   )
        { }

        void operator&(); // left undefined

    public:

        // copy constructor: compiler generated

        operator bool() const { return (m_block & m_mask) != 0; }
        bool operator~() const { return (m_block & m_mask) == 0; }

        reference& flip() { do_flip(); return *this; }

        reference& operator=(bool x)               { do_assign(x);   return *this; } // for b[i] = x
        reference& operator=(const reference& rhs) { do_assign(rhs); return *this; } // for b[i] = b[j]

        reference& operator|=(bool x) { if  (x) do_set();   return *this; }
        reference& operator&=(bool x) { if (!x) do_reset(); return *this; }
        reference& operator^=(bool x) { if  (x) do_flip();  return *this; }
        reference& operator-=(bool x) { if  (x) do_reset(); return *this; }

     private:
        block_type & m_block;
        const block_type m_mask;

        void do_set() { m_block |= m_mask; }
        void do_reset() { m_block &= ~m_mask; }
        void do_flip() { m_block ^= m_mask; }
        void do_assign(bool x) { x? do_set() : do_reset(); }
    };

    typedef bool const_reference;

    // constructors, etc.
    dynamic_bitset() : m_num_bits(0) {}

    explicit
    dynamic_bitset(const Allocator& alloc);

    explicit
    dynamic_bitset(size_type num_bits, unsigned long value = 0,
               const Allocator& alloc = Allocator());


    // WARNING: you should avoid using this constructor.
    //
    //  A conversion from string is, in most cases, formatting,
    //  and should be performed by using operator>>.
    //
    // NOTE:
    //  Leave the parentheses around std::basic_string<CharT, Traits, Alloc>::npos.
    //  g++ 3.2 requires them and probably the standard will - see core issue 325
    // NOTE 2:
    //  split into two constructors because of bugs in MSVC 6.0sp5 with STLport

    template <typename CharT, typename Traits, typename Alloc>
    dynamic_bitset(const std::basic_string<CharT, Traits, Alloc>& s,
        typename std::basic_string<CharT, Traits, Alloc>::size_type pos,
        typename std::basic_string<CharT, Traits, Alloc>::size_type n,
        size_type num_bits = npos,
        const Allocator& alloc = Allocator())

    :m_bits(alloc),
     m_num_bits(0)
    {
      init_from_string(s, pos, n, num_bits);
    }

    template <typename CharT, typename Traits, typename Alloc>
    explicit
    dynamic_bitset(const std::basic_string<CharT, Traits, Alloc>& s,
      typename std::basic_string<CharT, Traits, Alloc>::size_type pos = 0)

    :m_bits(Allocator()),
     m_num_bits(0)
    {
      init_from_string(s, pos, (std::basic_string<CharT, Traits, Alloc>::npos),
                       npos);
    }

    // The first bit in *first is the least significant bit, and the
    // last bit in the block just before *last is the most significant bit.
    template <typename BlockInputIterator>
    dynamic_bitset(BlockInputIterator first, BlockInputIterator last,
                   const Allocator& alloc = Allocator())

    :m_bits(alloc),
     m_num_bits(0)
    {
        using boost::detail::dynamic_bitset_impl::value_to_type;
        using boost::detail::dynamic_bitset_impl::is_numeric;

        const value_to_type<
            is_numeric<BlockInputIterator>::value> selector;

        dispatch_init(first, last, selector);
    }

    template <typename T>
    void dispatch_init(T num_bits, unsigned long value,
                       detail::dynamic_bitset_impl::value_to_type<true>)
    {
        init_from_unsigned_long(static_cast<size_type>(num_bits), value);
    }

    template <typename T>
    void dispatch_init(T first, T last,
                       detail::dynamic_bitset_impl::value_to_type<false>)
    {
        init_from_block_range(first, last);
    }

    template <typename BlockIter>
    void init_from_block_range(BlockIter first, BlockIter last)
    {
        assert(m_bits.size() == 0);
        m_bits.insert(m_bits.end(), first, last);
        m_num_bits = m_bits.size() * bits_per_block;
    }

    // copy constructor
    dynamic_bitset(const dynamic_bitset& b);

    ~dynamic_bitset();

    void swap(dynamic_bitset& b);
    dynamic_bitset& operator=(const dynamic_bitset& b);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
    dynamic_bitset(dynamic_bitset&& src);
    dynamic_bitset& operator=(dynamic_bitset&& src);
#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

    allocator_type get_allocator() const;

    // size changing operations
    void resize(size_type num_bits, bool value = false);
    void clear();
    void push_back(bool bit);
    void pop_back();
    void append(Block block);

    template <typename BlockInputIterator>
    void m_append(BlockInputIterator first, BlockInputIterator last, std::input_iterator_tag)
    {
        std::vector<Block, Allocator> v(first, last);
        m_append(v.begin(), v.end(), std::random_access_iterator_tag());
    }
    template <typename BlockInputIterator>
    void m_append(BlockInputIterator first, BlockInputIterator last, std::forward_iterator_tag)
    {
        assert(first != last);
        block_width_type r = count_extra_bits();
        std::size_t d = std::distance(first, last);
        m_bits.reserve(num_blocks() + d);
        if (r == 0) {
            for( ; first != last; ++first)
                m_bits.push_back(*first); // could use vector<>::insert()
        }
        else {
            m_highest_block() |= (*first << r);
            do {
                Block b = *first >> (bits_per_block - r);
                ++first;
                m_bits.push_back(b | (first==last? 0 : *first << r));
            } while (first != last);
        }
        m_num_bits += bits_per_block * d;
    }
    template <typename BlockInputIterator>
    void append(BlockInputIterator first, BlockInputIterator last) // strong guarantee
    {
        if (first != last) {
            typename std::iterator_traits<BlockInputIterator>::iterator_category cat;
            m_append(first, last, cat);
        }
    }


    // bitset operations
    dynamic_bitset& operator&=(const dynamic_bitset& b);
    dynamic_bitset& operator|=(const dynamic_bitset& b);
    dynamic_bitset& operator^=(const dynamic_bitset& b);
    dynamic_bitset& operator-=(const dynamic_bitset& b);
    dynamic_bitset& operator<<=(size_type n);
    dynamic_bitset& operator>>=(size_type n);
    dynamic_bitset operator<<(size_type n) const;
    dynamic_bitset operator>>(size_type n) const;

    // basic bit operations
    dynamic_bitset& set(size_type n, size_type len, bool val /* = true */); // default would make it ambiguous
    dynamic_bitset& set(size_type n, bool val = true);
    dynamic_bitset& set();
    dynamic_bitset& reset(size_type n, size_type len);
    dynamic_bitset& reset(size_type n);
    dynamic_bitset& reset();
    dynamic_bitset& flip(size_type n, size_type len);
    dynamic_bitset& flip(size_type n);
    dynamic_bitset& flip();
    bool test(size_type n) const;
    bool test_set(size_type n, bool val = true);
    bool all() const;
    bool any() const;
    bool none() const;
    dynamic_bitset operator~() const;
    size_type count() const BOOST_NOEXCEPT;

    // subscript
    reference operator[](size_type pos) {
        return reference(m_bits[block_index(pos)], bit_index(pos));
    }
    bool operator[](size_type pos) const { return test(pos); }

    unsigned long to_ulong() const;

    size_type size() const BOOST_NOEXCEPT;
    size_type num_blocks() const BOOST_NOEXCEPT;
    size_type max_size() const BOOST_NOEXCEPT;
    bool empty() const BOOST_NOEXCEPT;
    size_type capacity() const BOOST_NOEXCEPT;
    void reserve(size_type num_bits);
    void shrink_to_fit();

    bool is_subset_of(const dynamic_bitset& a) const;
    bool is_proper_subset_of(const dynamic_bitset& a) const;
    bool intersects(const dynamic_bitset & a) const;

    // lookup
    size_type find_first() const;
    size_type find_next(size_type pos) const;


#if !defined BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS
    // lexicographical comparison
    template <typename B, typename A>
    friend bool operator==(const dynamic_bitset<B, A>& a,
                           const dynamic_bitset<B, A>& b);

    template <typename B, typename A>
    friend bool operator<(const dynamic_bitset<B, A>& a,
                          const dynamic_bitset<B, A>& b);

    template <typename B, typename A>
    friend bool oplessthan(const dynamic_bitset<B, A>& a,
                          const dynamic_bitset<B, A>& b);


    template <typename B, typename A, typename BlockOutputIterator>
    friend void to_block_range(const dynamic_bitset<B, A>& b,
                               BlockOutputIterator result);

    template <typename BlockIterator, typename B, typename A>
    friend void from_block_range(BlockIterator first, BlockIterator last,
                                 dynamic_bitset<B, A>& result);


    template <typename CharT, typename Traits, typename B, typename A>
    friend std::basic_istream<CharT, Traits>& operator>>(std::basic_istream<CharT, Traits>& is,
                                                         dynamic_bitset<B, A>& b);

    template <typename B, typename A, typename stringT>
    friend void to_string_helper(const dynamic_bitset<B, A> & b, stringT & s, bool dump_all);

    template <typename B, typename A>
    friend std::size_t hash_value(const dynamic_bitset<B, A>& a);
#endif

public:
    // forward declaration for optional zero-copy serialization support
    class serialize_impl;
    friend class serialize_impl;

private:
    BOOST_STATIC_CONSTANT(block_width_type, ulong_width = std::numeric_limits<unsigned long>::digits);

    dynamic_bitset& range_operation(size_type pos, size_type len,
        Block (*partial_block_operation)(Block, size_type, size_type),
        Block (*full_block_operation)(Block));
    void m_zero_unused_bits();
    bool m_check_invariants() const;

    static bool m_not_empty(Block x){ return x != Block(0); };
    size_type m_do_find_from(size_type first_block) const;

    block_width_type count_extra_bits() const BOOST_NOEXCEPT { return bit_index(size()); }
    static size_type block_index(size_type pos) BOOST_NOEXCEPT { return pos / bits_per_block; }
    static block_width_type bit_index(size_type pos) BOOST_NOEXCEPT { return static_cast<block_width_type>(pos % bits_per_block); }
    static Block bit_mask(size_type pos) BOOST_NOEXCEPT { return Block(1) << bit_index(pos); }
    static Block bit_mask(size_type first, size_type last) BOOST_NOEXCEPT
    {
        Block res = (last == bits_per_block - 1)
            ? detail::dynamic_bitset_impl::max_limit<Block>::value
            : ((Block(1) << (last + 1)) - 1);
        res ^= (Block(1) << first) - 1;
        return res;
    }
    static Block set_block_bits(Block block, size_type first,
        size_type last, bool val) BOOST_NOEXCEPT
    {
        if (val)
            return block | bit_mask(first, last);
        else
            return block & static_cast<Block>(~bit_mask(first, last));
    }

    // Functions for operations on ranges
    inline static Block set_block_partial(Block block, size_type first,
        size_type last) BOOST_NOEXCEPT
    {
        return set_block_bits(block, first, last, true);
    }
    inline static Block set_block_full(Block) BOOST_NOEXCEPT
    {
        return detail::dynamic_bitset_impl::max_limit<Block>::value;
    }
    inline static Block reset_block_partial(Block block, size_type first,
        size_type last) BOOST_NOEXCEPT
    {
        return set_block_bits(block, first, last, false);
    }
    inline static Block reset_block_full(Block) BOOST_NOEXCEPT
    {
        return 0;
    }
    inline static Block flip_block_partial(Block block, size_type first,
        size_type last) BOOST_NOEXCEPT
    {
        return block ^ bit_mask(first, last);
    }
    inline static Block flip_block_full(Block block) BOOST_NOEXCEPT
    {
        return ~block;
    }

    template <typename CharT, typename Traits, typename Alloc>
    void init_from_string(const std::basic_string<CharT, Traits, Alloc>& s,
        typename std::basic_string<CharT, Traits, Alloc>::size_type pos,
        typename std::basic_string<CharT, Traits, Alloc>::size_type n,
        size_type num_bits)
    {
        assert(pos <= s.size());

        typedef typename std::basic_string<CharT, Traits, Alloc> StrT;
        typedef typename StrT::traits_type Tr;

        const typename StrT::size_type rlen = (std::min)(n, s.size() - pos);
        const size_type sz = ( num_bits != npos? num_bits : rlen);
        m_bits.resize(calc_num_blocks(sz));
        m_num_bits = sz;


        BOOST_DYNAMIC_BITSET_CTYPE_FACET(CharT, fac, std::locale());
        const CharT one = BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '1');

        const size_type m = num_bits < rlen ? num_bits : rlen;
        typename StrT::size_type i = 0;
        for( ; i < m; ++i) {

            const CharT c = s[(pos + m - 1) - i];

            assert( Tr::eq(c, one)
                    || Tr::eq(c, BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '0')) );

            if (Tr::eq(c, one))
                set(i);

        }

    }

    void init_from_unsigned_long(size_type num_bits,
                                 unsigned long value/*,
                                 const Allocator& alloc*/)
    {

        assert(m_bits.size() == 0);

        m_bits.resize(calc_num_blocks(num_bits));
        m_num_bits = num_bits;

        typedef unsigned long num_type;
        typedef boost::detail::dynamic_bitset_impl
            ::shifter<num_type, bits_per_block, ulong_width> shifter;

        //if (num_bits == 0)
        //    return;

        // zero out all bits at pos >= num_bits, if any;
        // note that: num_bits == 0 implies value == 0
        if (num_bits < static_cast<size_type>(ulong_width)) {
            const num_type mask = (num_type(1) << num_bits) - 1;
            value &= mask;
        }

        typename buffer_type::iterator it = m_bits.begin();
        for( ; value; shifter::left_shift(value), ++it) {
            *it = static_cast<block_type>(value);
        }

    }



BOOST_DYNAMIC_BITSET_PRIVATE:

    bool m_unchecked_test(size_type pos) const;
    static size_type calc_num_blocks(size_type num_bits);

    Block&        m_highest_block();
    const Block&  m_highest_block() const;

    buffer_type m_bits;
    size_type   m_num_bits;


    class bit_appender;
    friend class bit_appender;
    class bit_appender {
      // helper for stream >>
      // Supplies to the lack of an efficient append at the less
      // significant end: bits are actually appended "at left" but
      // rearranged in the destructor. From the perspective of
      // client code everything works *as if* dynamic_bitset<> had
      // an append_at_right() function (eventually throwing the same
      // exceptions as push_back) except that the function is in fact
      // called bit_appender::do_append().
      //
      dynamic_bitset & bs;
      size_type n;
      Block mask;
      Block * current;

      // not implemented
      bit_appender(const bit_appender &);
      bit_appender & operator=(const bit_appender &);

    public:
        bit_appender(dynamic_bitset & r) : bs(r), n(0), mask(0), current(0) {}
        ~bit_appender() {
            // reverse the order of blocks, shift
            // if needed, and then resize
            //
            std::reverse(bs.m_bits.begin(), bs.m_bits.end());
            const block_width_type offs = bit_index(n);
            if (offs)
                bs >>= (bits_per_block - offs);
            bs.resize(n); // doesn't enlarge, so can't throw
            assert(bs.m_check_invariants());
        }
        inline void do_append(bool value) {

            if (mask == 0) {
                bs.append(Block(0));
                current = &bs.m_highest_block();
                mask = Block(1) << (bits_per_block - 1);
            }

            if(value)
                *current |= mask;

            mask /= 2;
            ++n;
        }
        size_type get_count() const { return n; }
    };

};

#if !defined BOOST_NO_INCLASS_MEMBER_INITIALIZATION

template <typename Block, typename Allocator>
const typename dynamic_bitset<Block, Allocator>::block_width_type
dynamic_bitset<Block, Allocator>::bits_per_block;

template <typename Block, typename Allocator>
const typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::npos;

template <typename Block, typename Allocator>
const typename dynamic_bitset<Block, Allocator>::block_width_type
dynamic_bitset<Block, Allocator>::ulong_width;

#endif

// Global Functions:

// comparison
template <typename Block, typename Allocator>
bool operator!=(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
bool operator<=(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
bool operator>(const dynamic_bitset<Block, Allocator>& a,
               const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
bool operator>=(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b);

// stream operators
#ifdef BOOST_OLD_IOSTREAMS
template <typename Block, typename Allocator>
std::ostream& operator<<(std::ostream& os,
                         const dynamic_bitset<Block, Allocator>& b);

template <typename Block, typename Allocator>
std::istream& operator>>(std::istream& is, dynamic_bitset<Block,Allocator>& b);
#else
template <typename CharT, typename Traits, typename Block, typename Allocator>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const dynamic_bitset<Block, Allocator>& b);

template <typename CharT, typename Traits, typename Block, typename Allocator>
std::basic_istream<CharT, Traits>&
operator>>(std::basic_istream<CharT, Traits>& is,
           dynamic_bitset<Block, Allocator>& b);
#endif

// bitset operations
template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator&(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator|(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator^(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator-(const dynamic_bitset<Block, Allocator>& b1,
          const dynamic_bitset<Block, Allocator>& b2);

// namespace scope swap
template<typename Block, typename Allocator>
void swap(dynamic_bitset<Block, Allocator>& b1,
          dynamic_bitset<Block, Allocator>& b2);


template <typename Block, typename Allocator, typename stringT>
void
to_string(const dynamic_bitset<Block, Allocator>& b, stringT & s);

template <typename Block, typename Allocator, typename BlockOutputIterator>
void
to_block_range(const dynamic_bitset<Block, Allocator>& b,
               BlockOutputIterator result);


template <typename BlockIterator, typename B, typename A>
inline void
from_block_range(BlockIterator first, BlockIterator last,
                 dynamic_bitset<B, A>& result)
{
    // PRE: distance(first, last) <= numblocks()
    std::copy (first, last, result.m_bits.begin());
}

//=============================================================================
// dynamic_bitset implementation


//-----------------------------------------------------------------------------
// constructors, etc.

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>::dynamic_bitset(const Allocator& alloc)
  : m_bits(alloc), m_num_bits(0)
{

}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>::
dynamic_bitset(size_type num_bits, unsigned long value, const Allocator& alloc)
    : m_bits(alloc),
      m_num_bits(0)
{
    init_from_unsigned_long(num_bits, value);
}

// copy constructor
template <typename Block, typename Allocator>
inline dynamic_bitset<Block, Allocator>::
dynamic_bitset(const dynamic_bitset& b)
  : m_bits(b.m_bits), m_num_bits(b.m_num_bits)
{

}

template <typename Block, typename Allocator>
inline dynamic_bitset<Block, Allocator>::
~dynamic_bitset()
{
    assert(m_check_invariants());
}

template <typename Block, typename Allocator>
inline void dynamic_bitset<Block, Allocator>::
swap(dynamic_bitset<Block, Allocator>& b) // no throw
{
    std::swap(m_bits, b.m_bits);
    std::swap(m_num_bits, b.m_num_bits);
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::
operator=(const dynamic_bitset<Block, Allocator>& b)
{
    m_bits = b.m_bits;
    m_num_bits = b.m_num_bits;
    return *this;
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename Block, typename Allocator>
inline dynamic_bitset<Block, Allocator>::
dynamic_bitset(dynamic_bitset<Block, Allocator>&& b)
  : m_bits(boost::move(b.m_bits)), m_num_bits(boost::move(b.m_num_bits))
{
    // Required so that assert(m_check_invariants()); works.
    assert((b.m_bits = buffer_type()).empty());
    b.m_num_bits = 0;
}

template <typename Block, typename Allocator>
inline dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::
operator=(dynamic_bitset<Block, Allocator>&& b)
{
    if (boost::addressof(b) == this) { return *this; }

    m_bits = boost::move(b.m_bits);
    m_num_bits = boost::move(b.m_num_bits);
    // Required so that assert(m_check_invariants()); works.
    assert((b.m_bits = buffer_type()).empty());
    b.m_num_bits = 0;
    return *this;
}

#endif // BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::allocator_type
dynamic_bitset<Block, Allocator>::get_allocator() const
{
    return m_bits.get_allocator();
}

//-----------------------------------------------------------------------------
// size changing operations

template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
resize(size_type num_bits, bool value) // strong guarantee
{

  const size_type old_num_blocks = num_blocks();
  const size_type required_blocks = calc_num_blocks(num_bits);

  const block_type v = value? detail::dynamic_bitset_impl::max_limit<Block>::value : Block(0);

  if (required_blocks != old_num_blocks) {
    m_bits.resize(required_blocks, v); // s.g. (copy)
  }


  // At this point:
  //
  //  - if the buffer was shrunk, we have nothing more to do,
  //    except a call to m_zero_unused_bits()
  //
  //  - if it was enlarged, all the (used) bits in the new blocks have
  //    the correct value, but we have not yet touched those bits, if
  //    any, that were 'unused bits' before enlarging: if value == true,
  //    they must be set.

  if (value && (num_bits > m_num_bits)) {

    const block_width_type extra_bits = count_extra_bits();
    if (extra_bits) {
        assert(old_num_blocks >= 1 && old_num_blocks <= m_bits.size());

        // Set them.
        m_bits[old_num_blocks - 1] |= (v << extra_bits);
    }

  }

  m_num_bits = num_bits;
  m_zero_unused_bits();

}

template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
clear() // no throw
{
  m_bits.clear();
  m_num_bits = 0;
}


template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
push_back(bool bit)
{
  const size_type sz = size();
  resize(sz + 1);
  set(sz, bit);
}

template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
pop_back()
{
  const size_type old_num_blocks = num_blocks();
  const size_type required_blocks = calc_num_blocks(m_num_bits - 1);

  if (required_blocks != old_num_blocks) {
    m_bits.pop_back();
  }

  --m_num_bits;
  m_zero_unused_bits();
}


template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::
append(Block value) // strong guarantee
{
    const block_width_type r = count_extra_bits();

    if (r == 0) {
        // the buffer is empty, or all blocks are filled
        m_bits.push_back(value);
    }
    else {
        m_bits.push_back(value >> (bits_per_block - r));
        m_bits[m_bits.size() - 2] |= (value << r); // m_bits.size() >= 2
    }

    m_num_bits += bits_per_block;
    assert(m_check_invariants());

}


//-----------------------------------------------------------------------------
// bitset operations
template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator&=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < num_blocks(); ++i)
        m_bits[i] &= rhs.m_bits[i];
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator|=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < num_blocks(); ++i)
        m_bits[i] |= rhs.m_bits[i];
    //m_zero_unused_bits();
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator^=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < this->num_blocks(); ++i)
        m_bits[i] ^= rhs.m_bits[i];
    //m_zero_unused_bits();
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator-=(const dynamic_bitset& rhs)
{
    assert(size() == rhs.size());
    for (size_type i = 0; i < num_blocks(); ++i)
        m_bits[i] &= ~rhs.m_bits[i];
    //m_zero_unused_bits();
    return *this;
}

//
// NOTE:
//  Note that the 'if (r != 0)' is crucial to avoid undefined
//  behavior when the left hand operand of >> isn't promoted to a
//  wider type (because rs would be too large).
//
template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::operator<<=(size_type n)
{
    if (n >= m_num_bits)
        return reset();
    //else
    if (n > 0) {

        size_type    const last = num_blocks() - 1;  // num_blocks() is >= 1
        size_type    const div  = n / bits_per_block; // div is <= last
        block_width_type const r = bit_index(n);
        block_type * const b    = &m_bits[0];

        if (r != 0) {

            block_width_type const rs = bits_per_block - r;

            for (size_type i = last-div; i>0; --i) {
                b[i+div] = (b[i] << r) | (b[i-1] >> rs);
            }
            b[div] = b[0] << r;

        }
        else {
            for (size_type i = last-div; i>0; --i) {
                b[i+div] = b[i];
            }
            b[div] = b[0];
        }

        // zero out div blocks at the less significant end
        std::fill_n(m_bits.begin(), div, static_cast<block_type>(0));

        // zero out any 1 bit that flowed into the unused part
        m_zero_unused_bits(); // thanks to Lester Gong

    }

    return *this;


}


//
// NOTE:
//  see the comments to operator <<=
//
template <typename B, typename A>
dynamic_bitset<B, A> & dynamic_bitset<B, A>::operator>>=(size_type n) {
    if (n >= m_num_bits) {
        return reset();
    }
    //else
    if (n>0) {

        size_type  const last  = num_blocks() - 1; // num_blocks() is >= 1
        size_type  const div   = n / bits_per_block;   // div is <= last
        block_width_type const r     = bit_index(n);
        block_type * const b   = &m_bits[0];


        if (r != 0) {

            block_width_type const ls = bits_per_block - r;

            for (size_type i = div; i < last; ++i) {
                b[i-div] = (b[i] >> r) | (b[i+1]  << ls);
            }
            // r bits go to zero
            b[last-div] = b[last] >> r;
        }

        else {
            for (size_type i = div; i <= last; ++i) {
                b[i-div] = b[i];
            }
            // note the '<=': the last iteration 'absorbs'
            // b[last-div] = b[last] >> 0;
        }



        // div blocks are zero filled at the most significant end
        std::fill_n(m_bits.begin() + (num_blocks()-div), div, static_cast<block_type>(0));
    }

    return *this;
}


template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
dynamic_bitset<Block, Allocator>::operator<<(size_type n) const
{
    dynamic_bitset r(*this);
    return r <<= n;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
dynamic_bitset<Block, Allocator>::operator>>(size_type n) const
{
    dynamic_bitset r(*this);
    return r >>= n;
}


//-----------------------------------------------------------------------------
// basic bit operations

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::set(size_type pos,
        size_type len, bool val)
{
    if (val)
        return range_operation(pos, len, set_block_partial, set_block_full);
    else
        return range_operation(pos, len, reset_block_partial, reset_block_full);
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::set(size_type pos, bool val)
{
    assert(pos < m_num_bits);

    if (val)
        m_bits[block_index(pos)] |= bit_mask(pos);
    else
        reset(pos);

    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::set()
{
  std::fill(m_bits.begin(), m_bits.end(), detail::dynamic_bitset_impl::max_limit<Block>::value);
  m_zero_unused_bits();
  return *this;
}

template <typename Block, typename Allocator>
inline dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::reset(size_type pos, size_type len)
{
    return range_operation(pos, len, reset_block_partial, reset_block_full);
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::reset(size_type pos)
{
    assert(pos < m_num_bits);
#if defined __MWERKS__ && BOOST_WORKAROUND(__MWERKS__, <= 0x3003) // 8.x
    // CodeWarrior 8 generates incorrect code when the &=~ is compiled,
    // use the |^ variation instead.. <grafik>
    m_bits[block_index(pos)] |= bit_mask(pos);
    m_bits[block_index(pos)] ^= bit_mask(pos);
#else
    m_bits[block_index(pos)] &= ~bit_mask(pos);
#endif
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::reset()
{
  std::fill(m_bits.begin(), m_bits.end(), Block(0));
  return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::flip(size_type pos, size_type len)
{
    return range_operation(pos, len, flip_block_partial, flip_block_full);
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::flip(size_type pos)
{
    assert(pos < m_num_bits);
    m_bits[block_index(pos)] ^= bit_mask(pos);
    return *this;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>&
dynamic_bitset<Block, Allocator>::flip()
{
    for (size_type i = 0; i < num_blocks(); ++i)
        m_bits[i] = ~m_bits[i];
    m_zero_unused_bits();
    return *this;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::m_unchecked_test(size_type pos) const
{
    return (m_bits[block_index(pos)] & bit_mask(pos)) != 0;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::test(size_type pos) const
{
    assert(pos < m_num_bits);
    return m_unchecked_test(pos);
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::test_set(size_type pos, bool val)
{
    bool const b = test(pos);
    if (b != val) {
        set(pos, val);
    }
    return b;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::all() const
{
    if (empty()) {
        return true;
    }

    const block_width_type extra_bits = count_extra_bits();
    block_type const all_ones = detail::dynamic_bitset_impl::max_limit<Block>::value;

    if (extra_bits == 0) {
        for (size_type i = 0, e = num_blocks(); i < e; ++i) {
            if (m_bits[i] != all_ones) {
                return false;
            }
        }
    } else {
        for (size_type i = 0, e = num_blocks() - 1; i < e; ++i) {
            if (m_bits[i] != all_ones) {
                return false;
            }
        }
        const block_type mask = (block_type(1) << extra_bits) - 1;
        if (m_highest_block() != mask) {
            return false;
        }
    }
    return true;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::any() const
{
    for (size_type i = 0; i < num_blocks(); ++i)
        if (m_bits[i])
            return true;
    return false;
}

template <typename Block, typename Allocator>
inline bool dynamic_bitset<Block, Allocator>::none() const
{
    return !any();
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
dynamic_bitset<Block, Allocator>::operator~() const
{
    dynamic_bitset b(*this);
    b.flip();
    return b;
}

template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::count() const BOOST_NOEXCEPT
{
    using detail::dynamic_bitset_impl::table_width;
    using detail::dynamic_bitset_impl::access_by_bytes;
    using detail::dynamic_bitset_impl::access_by_blocks;
    using detail::dynamic_bitset_impl::value_to_type;

#if BOOST_WORKAROUND(__GNUC__, == 4) && (__GNUC_MINOR__ == 3) && (__GNUC_PATCHLEVEL__ == 3)
    // NOTE: Explicit qualification of "bits_per_block"
    //       breaks compilation on gcc 4.3.3
    enum { no_padding = bits_per_block == CHAR_BIT * sizeof(Block) };
#else
    // NOTE: Explicitly qualifying "bits_per_block" to workaround
    //       regressions of gcc 3.4.x
    enum { no_padding =
        dynamic_bitset<Block, Allocator>::bits_per_block
        == CHAR_BIT * sizeof(Block) };
#endif

    enum { enough_table_width = table_width >= CHAR_BIT };

#if ((defined(BOOST_MSVC) && (BOOST_MSVC >= 1600)) || (defined(__clang__) && defined(__c2__)) || (defined(BOOST_INTEL) && defined(_MSC_VER))) && (defined(_M_IX86) || defined(_M_X64))
    // Windows popcount is effective starting from the unsigned short type
    enum { uneffective_popcount = sizeof(Block) < sizeof(unsigned short) };
#elif defined(BOOST_GCC) || defined(__clang__) || (defined(BOOST_INTEL) && defined(__GNUC__))
    // GCC popcount is effective starting from the unsigned int type
    enum { uneffective_popcount = sizeof(Block) < sizeof(unsigned int) };
#else
    enum { uneffective_popcount = true };
#endif

    enum { mode = (no_padding && enough_table_width && uneffective_popcount)
                          ? access_by_bytes
                          : access_by_blocks };

    return do_count(m_bits.begin(), num_blocks(), Block(0),
                    static_cast<value_to_type<(bool)mode> *>(0));
}


//-----------------------------------------------------------------------------
// conversions


template <typename B, typename A, typename stringT>
void to_string_helper(const dynamic_bitset<B, A> & b, stringT & s,
                      bool dump_all)
{
    typedef typename stringT::traits_type Tr;
    typedef typename stringT::value_type  Ch;

    BOOST_DYNAMIC_BITSET_CTYPE_FACET(Ch, fac, std::locale());
    const Ch zero = BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '0');
    const Ch one  = BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '1');

    // Note that this function may access (when
    // dump_all == true) bits beyond position size() - 1

    typedef typename dynamic_bitset<B, A>::size_type size_type;

    const size_type len = dump_all?
         dynamic_bitset<B, A>::bits_per_block * b.num_blocks():
         b.size();
    s.assign (len, zero);

    for (size_type i = 0; i < len; ++i) {
        if (b.m_unchecked_test(i))
            Tr::assign(s[len - 1 - i], one);

    }

}


// A comment similar to the one about the constructor from
// basic_string can be done here. Thanks to James Kanze for
// making me (Gennaro) realize this important separation of
// concerns issue, as well as many things about i18n.
//
template <typename Block, typename Allocator, typename stringT>
inline void
to_string(const dynamic_bitset<Block, Allocator>& b, stringT& s)
{
    to_string_helper(b, s, false);
}


// Differently from to_string this function dumps out
// every bit of the internal representation (may be
// useful for debugging purposes)
//
template <typename B, typename A, typename stringT>
inline void
dump_to_string(const dynamic_bitset<B, A>& b, stringT& s)
{
    to_string_helper(b, s, true /* =dump_all*/);
}

template <typename Block, typename Allocator, typename BlockOutputIterator>
inline void
to_block_range(const dynamic_bitset<Block, Allocator>& b,
               BlockOutputIterator result)
{
    // note how this copies *all* bits, including the
    // unused ones in the last block (which are zero)
    std::copy(b.m_bits.begin(), b.m_bits.end(), result);
}

template <typename Block, typename Allocator>
unsigned long dynamic_bitset<Block, Allocator>::
to_ulong() const
{

  if (m_num_bits == 0)
      return 0; // convention

  // Check for overflows. This may be a performance burden on very
  // large bitsets but is required by the specification, sorry
  if (find_next(ulong_width - 1) != npos)
    BOOST_THROW_EXCEPTION(std::overflow_error("boost::dynamic_bitset::to_ulong overflow"));


  // Ok, from now on we can be sure there's no "on" bit
  // beyond the "allowed" positions
  typedef unsigned long result_type;

  const size_type maximum_size =
            (std::min)(m_num_bits, static_cast<size_type>(ulong_width));

  const size_type last_block = block_index( maximum_size - 1 );

  assert((last_block * bits_per_block) < static_cast<size_type>(ulong_width));

  result_type result = 0;
  for (size_type i = 0; i <= last_block; ++i) {
    const size_type offset = i * bits_per_block;
    result |= (static_cast<result_type>(m_bits[i]) << offset);
  }

  return result;
}

template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::size() const BOOST_NOEXCEPT
{
    return m_num_bits;
}

template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::num_blocks() const BOOST_NOEXCEPT
{
    return m_bits.size();
}

template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::max_size() const BOOST_NOEXCEPT
{
    // Semantics of vector<>::max_size() aren't very clear
    // (see lib issue 197) and many library implementations
    // simply return dummy values, _unrelated_ to the underlying
    // allocator.
    //
    // Given these problems, I was tempted to not provide this
    // function at all but the user could need it if he provides
    // his own allocator.
    //

    const size_type m = detail::dynamic_bitset_impl::
                        vector_max_size_workaround(m_bits);

    return m <= (size_type(-1)/bits_per_block) ?
        m * bits_per_block :
        size_type(-1);
}

template <typename Block, typename Allocator>
inline bool dynamic_bitset<Block, Allocator>::empty() const BOOST_NOEXCEPT
{
  return size() == 0;
}

template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::capacity() const BOOST_NOEXCEPT
{
    return m_bits.capacity() * bits_per_block;
}

template <typename Block, typename Allocator>
inline void dynamic_bitset<Block, Allocator>::reserve(size_type num_bits)
{
    m_bits.reserve(calc_num_blocks(num_bits));
}

template <typename Block, typename Allocator>
void dynamic_bitset<Block, Allocator>::shrink_to_fit()
{
    if (m_bits.size() < m_bits.capacity()) {
      buffer_type(m_bits).swap(m_bits);
    }
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::
is_subset_of(const dynamic_bitset<Block, Allocator>& a) const
{
    assert(size() == a.size());
    for (size_type i = 0; i < num_blocks(); ++i)
        if (m_bits[i] & ~a.m_bits[i])
            return false;
    return true;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::
is_proper_subset_of(const dynamic_bitset<Block, Allocator>& a) const
{
    assert(size() == a.size());
    assert(num_blocks() == a.num_blocks());

    bool proper = false;
    for (size_type i = 0; i < num_blocks(); ++i) {
        const Block & bt =   m_bits[i];
        const Block & ba = a.m_bits[i];

        if (bt & ~ba)
            return false; // not a subset at all
        if (ba & ~bt)
            proper = true;
    }
    return proper;
}

template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::intersects(const dynamic_bitset & b) const
{
    size_type common_blocks = num_blocks() < b.num_blocks()
                              ? num_blocks() : b.num_blocks();

    for(size_type i = 0; i < common_blocks; ++i) {
        if(m_bits[i] & b.m_bits[i])
            return true;
    }
    return false;
}

// --------------------------------
// lookup

// look for the first bit "on", starting
// from the block with index first_block
//

template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::m_do_find_from(size_type first_block) const
{

    size_type i = std::distance(m_bits.begin(),
        std::find_if(m_bits.begin() + first_block, m_bits.end(), m_not_empty) );

    if (i >= num_blocks())
        return npos; // not found

    return i * bits_per_block + static_cast<size_type>(detail::lowest_bit(m_bits[i]));
}


template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::find_first() const
{
    return m_do_find_from(0);
}


template <typename Block, typename Allocator>
typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::find_next(size_type pos) const
{

    const size_type sz = size();
    if (pos >= (sz-1) || sz == 0)
        return npos;

    ++pos;

    const size_type blk = block_index(pos);
    const block_width_type ind = bit_index(pos);

    // shift bits upto one immediately after current
    const Block fore = m_bits[blk] >> ind;

    return fore?
        pos + static_cast<size_type>(detail::lowest_bit(fore))
        :
        m_do_find_from(blk + 1);

}



//-----------------------------------------------------------------------------
// comparison

template <typename Block, typename Allocator>
bool operator==(const dynamic_bitset<Block, Allocator>& a,
                const dynamic_bitset<Block, Allocator>& b)
{
    return (a.m_num_bits == b.m_num_bits)
           && (a.m_bits == b.m_bits);
}

template <typename Block, typename Allocator>
inline bool operator!=(const dynamic_bitset<Block, Allocator>& a,
                       const dynamic_bitset<Block, Allocator>& b)
{
    return !(a == b);
}

template <typename Block, typename Allocator>
bool operator<(const dynamic_bitset<Block, Allocator>& a,
               const dynamic_bitset<Block, Allocator>& b)
{
//    assert(a.size() == b.size());

    typedef BOOST_DEDUCED_TYPENAME dynamic_bitset<Block, Allocator>::size_type size_type;
    
    size_type asize(a.size());
    size_type bsize(b.size());

    if (!bsize)
        {
        return false;
        }
    else if (!asize)
        {
        return true;
        }
    else if (asize == bsize)
        {
        for (size_type ii = a.num_blocks(); ii > 0; --ii) 
            {
            size_type i = ii-1;
            if (a.m_bits[i] < b.m_bits[i])
                return true;
            else if (a.m_bits[i] > b.m_bits[i])
                return false;
            }
        return false;
        }
    else
        {
        
        size_type leqsize(std::min BOOST_PREVENT_MACRO_SUBSTITUTION(asize,bsize));
    
        for (size_type ii = 0; ii < leqsize; ++ii,--asize,--bsize)
            {
            size_type i = asize-1;
            size_type j = bsize-1;
            if (a[i] < b[j])
                return true;
            else if (a[i] > b[j])
                return false;
            }
        return (a.size() < b.size());
        }
}

template <typename Block, typename Allocator>
bool oplessthan(const dynamic_bitset<Block, Allocator>& a,
               const dynamic_bitset<Block, Allocator>& b)
{
//    assert(a.size() == b.size());

    typedef BOOST_DEDUCED_TYPENAME dynamic_bitset<Block, Allocator>::size_type size_type;
    
    size_type asize(a.num_blocks());
    size_type bsize(b.num_blocks());
    assert(asize == 3);
    assert(bsize == 4);

    if (!bsize)
        {
        return false;
        }
    else if (!asize)
        {
        return true;
        }
    else
        {
        
        size_type leqsize(std::min BOOST_PREVENT_MACRO_SUBSTITUTION(asize,bsize));
        assert(leqsize == 3);
    
        //if (a.size() == 0)
        //  return false;
    
        // Since we are storing the most significant bit
        // at pos == size() - 1, we need to do the comparisons in reverse.
        //
        for (size_type ii = 0; ii < leqsize; ++ii,--asize,--bsize)
            {
            size_type i = asize-1;
            size_type j = bsize-1;
            if (a.m_bits[i] < b.m_bits[j])
                return true;
            else if (a.m_bits[i] > b.m_bits[j])
                return false;
            }
        return (a.num_blocks() < b.num_blocks());
        }
}

template <typename Block, typename Allocator>
inline bool operator<=(const dynamic_bitset<Block, Allocator>& a,
                       const dynamic_bitset<Block, Allocator>& b)
{
    return !(a > b);
}

template <typename Block, typename Allocator>
inline bool operator>(const dynamic_bitset<Block, Allocator>& a,
                      const dynamic_bitset<Block, Allocator>& b)
{
    return b < a;
}

template <typename Block, typename Allocator>
inline bool operator>=(const dynamic_bitset<Block, Allocator>& a,
                       const dynamic_bitset<Block, Allocator>& b)
{
    return !(a < b);
}

//-----------------------------------------------------------------------------
// hash operations

template <typename Block, typename Allocator>
inline std::size_t hash_value(const dynamic_bitset<Block, Allocator>& a)
{
    std::size_t res = hash_value(a.m_num_bits);
    boost::hash_combine(res, a.m_bits);
    return res;
}

//-----------------------------------------------------------------------------
// stream operations

#ifdef BOOST_OLD_IOSTREAMS
template < typename Block, typename Alloc>
std::ostream&
operator<<(std::ostream& os, const dynamic_bitset<Block, Alloc>& b)
{
    // NOTE: since this is aimed at "classic" iostreams, exception
    // masks on the stream are not supported. The library that
    // ships with gcc 2.95 has an exceptions() member function but
    // nothing is actually implemented; not even the class ios::failure.

    using namespace std;

    const ios::iostate ok = ios::goodbit;
    ios::iostate err = ok;

    if (os.opfx()) {

        //try
        typedef typename dynamic_bitset<Block, Alloc>::size_type bitsetsize_type;

        const bitsetsize_type sz = b.size();
        std::streambuf * buf = os.rdbuf();
        size_t npad = os.width() <= 0  // careful: os.width() is signed (and can be < 0)
            || (bitsetsize_type) os.width() <= sz? 0 : os.width() - sz;

        const char fill_char = os.fill();
        const ios::fmtflags adjustfield = os.flags() & ios::adjustfield;

        // if needed fill at left; pad is decresed along the way
        if (adjustfield != ios::left) {
            for (; 0 < npad; --npad)
                if (fill_char != buf->sputc(fill_char)) {
                    err |= ios::failbit;
                    break;
                }
        }

        if (err == ok) {
            // output the bitset
            for (bitsetsize_type i = b.size(); 0 < i; --i) {
                const char dig = b.test(i-1)? '1' : '0';
                if (EOF == buf->sputc(dig)) {
                    err |= ios::failbit;
                    break;
                }
            }
        }

        if (err == ok) {
            // if needed fill at right
            for (; 0 < npad; --npad) {
                if (fill_char != buf->sputc(fill_char)) {
                    err |= ios::failbit;
                    break;
                }
            }
        }

        os.osfx();
        os.width(0);

    } // if opfx

    if(err != ok)
        os.setstate(err); // assume this does NOT throw
    return os;

}
#else

template <typename Ch, typename Tr, typename Block, typename Alloc>
std::basic_ostream<Ch, Tr>&
operator<<(std::basic_ostream<Ch, Tr>& os,
           const dynamic_bitset<Block, Alloc>& b)
{

    using namespace std;

    const ios_base::iostate ok = ios_base::goodbit;
    ios_base::iostate err = ok;

    typename basic_ostream<Ch, Tr>::sentry cerberos(os);
    if (cerberos) {

        BOOST_DYNAMIC_BITSET_CTYPE_FACET(Ch, fac, os.getloc());
        const Ch zero = BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '0');
        const Ch one  = BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '1');

        BOOST_TRY {

            typedef typename dynamic_bitset<Block, Alloc>::size_type bitset_size_type;
            typedef basic_streambuf<Ch, Tr> buffer_type;

            buffer_type * buf = os.rdbuf();
            // careful: os.width() is signed (and can be < 0)
            const bitset_size_type width = (os.width() <= 0) ? 0 : static_cast<bitset_size_type>(os.width());
            streamsize npad = (width <= b.size()) ? 0 : width - b.size();

            const Ch fill_char = os.fill();
            const ios_base::fmtflags adjustfield = os.flags() & ios_base::adjustfield;

            // if needed fill at left; pad is decreased along the way
            if (adjustfield != ios_base::left) {
                for (; 0 < npad; --npad)
                    if (Tr::eq_int_type(Tr::eof(), buf->sputc(fill_char))) {
                          err |= ios_base::failbit;
                          break;
                    }
            }

            if (err == ok) {
                // output the bitset
                for (bitset_size_type i = b.size(); 0 < i; --i) {
                    typename buffer_type::int_type
                        ret = buf->sputc(b.test(i-1)? one : zero);
                    if (Tr::eq_int_type(Tr::eof(), ret)) {
                        err |= ios_base::failbit;
                        break;
                    }
                }
            }

            if (err == ok) {
                // if needed fill at right
                for (; 0 < npad; --npad) {
                    if (Tr::eq_int_type(Tr::eof(), buf->sputc(fill_char))) {
                        err |= ios_base::failbit;
                        break;
                    }
                }
            }


            os.width(0);

        } BOOST_CATCH (...) { // see std 27.6.1.1/4
            bool rethrow = false;
            BOOST_TRY { os.setstate(ios_base::failbit); } BOOST_CATCH (...) { rethrow = true; } BOOST_CATCH_END

            if (rethrow)
                BOOST_RETHROW;
        }
        BOOST_CATCH_END
    }

    if(err != ok)
        os.setstate(err); // may throw exception
    return os;

}
#endif


#ifdef BOOST_OLD_IOSTREAMS

    // A sentry-like class that calls isfx in its destructor.
    // "Necessary" because bit_appender::do_append may throw.
    class pseudo_sentry {
        std::istream & m_r;
        const bool m_ok;
    public:
        explicit pseudo_sentry(std::istream & r) : m_r(r), m_ok(r.ipfx(0)) { }
        ~pseudo_sentry() { m_r.isfx(); }
        operator bool() const { return m_ok; }
    };

template <typename Block, typename Alloc>
std::istream&
operator>>(std::istream& is, dynamic_bitset<Block, Alloc>& b)
{

// Extractor for classic IO streams (libstdc++ < 3.0)
// ----------------------------------------------------//
//  It's assumed that the stream buffer functions, and
//  the stream's setstate() _cannot_ throw.


    typedef dynamic_bitset<Block, Alloc> bitset_type;
    typedef typename bitset_type::size_type size_type;

    std::ios::iostate err = std::ios::goodbit;
    pseudo_sentry cerberos(is); // skips whitespaces
    if(cerberos) {

        b.clear();

        const std::streamsize w = is.width();
        const size_type limit = w > 0 && static_cast<size_type>(w) < b.max_size()
                                                         ? static_cast<size_type>(w) : b.max_size();
        typename bitset_type::bit_appender appender(b);
        std::streambuf * buf = is.rdbuf();
        for(int c = buf->sgetc(); appender.get_count() < limit; c = buf->snextc() ) {

            if (c == EOF) {
                err |= std::ios::eofbit;
                break;
            }
            else if (char(c) != '0' && char(c) != '1')
                break; // non digit character

            else {
                BOOST_TRY {
                    appender.do_append(char(c) == '1');
                }
                BOOST_CATCH(...) {
                    is.setstate(std::ios::failbit); // assume this can't throw
                    BOOST_RETHROW;
                }
                BOOST_CATCH_END
            }

        } // for
    }

    is.width(0);
    if (b.size() == 0)
        err |= std::ios::failbit;
    if (err != std::ios::goodbit)
        is.setstate (err); // may throw

    return is;
}

#else // BOOST_OLD_IOSTREAMS

template <typename Ch, typename Tr, typename Block, typename Alloc>
std::basic_istream<Ch, Tr>&
operator>>(std::basic_istream<Ch, Tr>& is, dynamic_bitset<Block, Alloc>& b)
{

    using namespace std;

    typedef dynamic_bitset<Block, Alloc> bitset_type;
    typedef typename bitset_type::size_type size_type;

    const streamsize w = is.width();
    const size_type limit = 0 < w && static_cast<size_type>(w) < b.max_size()?
                                         static_cast<size_type>(w) : b.max_size();

    ios_base::iostate err = ios_base::goodbit;
    typename basic_istream<Ch, Tr>::sentry cerberos(is); // skips whitespaces
    if(cerberos) {

        // in accordance with prop. resol. of lib DR 303 [last checked 4 Feb 2004]
        BOOST_DYNAMIC_BITSET_CTYPE_FACET(Ch, fac, is.getloc());
        const Ch zero = BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '0');
        const Ch one  = BOOST_DYNAMIC_BITSET_WIDEN_CHAR(fac, '1');

        b.clear();
        BOOST_TRY {
            typename bitset_type::bit_appender appender(b);
            basic_streambuf <Ch, Tr> * buf = is.rdbuf();
            typename Tr::int_type c = buf->sgetc();
            for( ; appender.get_count() < limit; c = buf->snextc() ) {

                if (Tr::eq_int_type(Tr::eof(), c)) {
                    err |= ios_base::eofbit;
                    break;
                }
                else {
                    const Ch to_c = Tr::to_char_type(c);
                    const bool is_one = Tr::eq(to_c, one);

                    if (!is_one && !Tr::eq(to_c, zero))
                        break; // non digit character

                    appender.do_append(is_one);

                }

            } // for
        }
        BOOST_CATCH (...) {
            // catches from stream buf, or from vector:
            //
            // bits_stored bits have been extracted and stored, and
            // either no further character is extractable or we can't
            // append to the underlying vector (out of memory)

            bool rethrow = false;   // see std 27.6.1.1/4
            BOOST_TRY { is.setstate(ios_base::badbit); }
            BOOST_CATCH(...) { rethrow = true; }
            BOOST_CATCH_END

            if (rethrow)
                BOOST_RETHROW;

        }
        BOOST_CATCH_END
    }

    is.width(0);
    if (b.size() == 0 /*|| !cerberos*/)
        err |= ios_base::failbit;
    if (err != ios_base::goodbit)
        is.setstate (err); // may throw

    return is;

}


#endif


//-----------------------------------------------------------------------------
// bitset operations

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator&(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b &= y;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator|(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b |= y;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator^(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b ^= y;
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>
operator-(const dynamic_bitset<Block, Allocator>& x,
          const dynamic_bitset<Block, Allocator>& y)
{
    dynamic_bitset<Block, Allocator> b(x);
    return b -= y;
}

//-----------------------------------------------------------------------------
// namespace scope swap

template<typename Block, typename Allocator>
inline void
swap(dynamic_bitset<Block, Allocator>& left,
     dynamic_bitset<Block, Allocator>& right) // no throw
{
    left.swap(right);
}


//-----------------------------------------------------------------------------
// private (on conforming compilers) member functions


template <typename Block, typename Allocator>
inline typename dynamic_bitset<Block, Allocator>::size_type
dynamic_bitset<Block, Allocator>::calc_num_blocks(size_type num_bits)
{
    return num_bits / bits_per_block
           + static_cast<size_type>( num_bits % bits_per_block != 0 );
}

// gives a reference to the highest block
//
template <typename Block, typename Allocator>
inline Block& dynamic_bitset<Block, Allocator>::m_highest_block()
{
    return const_cast<Block &>
           (static_cast<const dynamic_bitset *>(this)->m_highest_block());
}

// gives a const-reference to the highest block
//
template <typename Block, typename Allocator>
inline const Block& dynamic_bitset<Block, Allocator>::m_highest_block() const
{
    assert(size() > 0 && num_blocks() > 0);
    return m_bits.back();
}

template <typename Block, typename Allocator>
dynamic_bitset<Block, Allocator>& dynamic_bitset<Block, Allocator>::range_operation(
    size_type pos, size_type len,
    Block (*partial_block_operation)(Block, size_type, size_type),
    Block (*full_block_operation)(Block))
{
    assert(pos + len <= m_num_bits);

    // Do nothing in case of zero length
    if (!len)
        return *this;

    // Use an additional asserts in order to detect size_type overflow
    // For example: pos = 10, len = size_type_limit - 2, pos + len = 7
    // In case of overflow, 'pos + len' is always smaller than 'len'
    assert(pos + len >= len);

    // Start and end blocks of the [pos; pos + len - 1] sequence
    const size_type first_block = block_index(pos);
    const size_type last_block = block_index(pos + len - 1);

    const size_type first_bit_index = bit_index(pos);
    const size_type last_bit_index = bit_index(pos + len - 1);

    if (first_block == last_block) {
        // Filling only a sub-block of a block
        m_bits[first_block] = partial_block_operation(m_bits[first_block],
            first_bit_index, last_bit_index);
    } else {
        // Check if the corner blocks won't be fully filled with 'val'
        const size_type first_block_shift = bit_index(pos) ? 1 : 0;
        const size_type last_block_shift = (bit_index(pos + len - 1)
            == bits_per_block - 1) ? 0 : 1;

        // Blocks that will be filled with ~0 or 0 at once
        const size_type first_full_block = first_block + first_block_shift;
        const size_type last_full_block = last_block - last_block_shift;

        for (size_type i = first_full_block; i <= last_full_block; ++i) {
            m_bits[i] = full_block_operation(m_bits[i]);
        }

        // Fill the first block from the 'first' bit index to the end
        if (first_block_shift) {
            m_bits[first_block] = partial_block_operation(m_bits[first_block],
                first_bit_index, bits_per_block - 1);
        }

        // Fill the last block from the start to the 'last' bit index
        if (last_block_shift) {
            m_bits[last_block] = partial_block_operation(m_bits[last_block],
                0, last_bit_index);
        }
    }

    return *this;
}

// If size() is not a multiple of bits_per_block
// then not all the bits in the last block are used.
// This function resets the unused bits (convenient
// for the implementation of many member functions)
//
template <typename Block, typename Allocator>
inline void dynamic_bitset<Block, Allocator>::m_zero_unused_bits()
{
    assert (num_blocks() == calc_num_blocks(m_num_bits));

    // if != 0 this is the number of bits used in the last block
    const block_width_type extra_bits = count_extra_bits();

    if (extra_bits != 0)
        m_highest_block() &= (Block(1) << extra_bits) - 1;
}

// check class invariants
template <typename Block, typename Allocator>
bool dynamic_bitset<Block, Allocator>::m_check_invariants() const
{
    const block_width_type extra_bits = count_extra_bits();
    if (extra_bits > 0) {
        const block_type mask = detail::dynamic_bitset_impl::max_limit<Block>::value << extra_bits;
        if ((m_highest_block() & mask) != 0)
            return false;
    }
    if (m_bits.size() > m_bits.capacity() || num_blocks() != calc_num_blocks(size()))
        return false;

    return true;

}


} // namespace boost

#undef BOOST_BITSET_CHAR

// std::hash support
#if !defined(BOOST_NO_CXX11_HDR_FUNCTIONAL) && !defined(BOOST_DYNAMIC_BITSET_NO_STD_HASH)
#include <functional>
namespace std
{
    template<typename Block, typename Allocator>
    struct hash< boost::dynamic_bitset<Block, Allocator> >
    {
        typedef boost::dynamic_bitset<Block, Allocator> argument_type;
        typedef std::size_t result_type;
        result_type operator()(const argument_type& a) const BOOST_NOEXCEPT
        {
            boost::hash<argument_type> hasher;
            return hasher(a);
        }
    };
}
#endif

#endif // include guard

