//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_CONTAINER_DYNAMIC_BITSET_HPP
#define BOOST_COMPUTE_CONTAINER_DYNAMIC_BITSET_HPP

#include <boost/compute/lambda.hpp>
#include <boost/compute/algorithm/any_of.hpp>
#include <boost/compute/algorithm/fill.hpp>
#include <boost/compute/algorithm/transform_reduce.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/functional/integer.hpp>
#include <boost/compute/types/fundamental.hpp>

namespace boost {
namespace compute {

/// \class dynamic_bitset
/// \brief The dynamic_bitset class contains a resizable bit array.
///
/// For example, to create a dynamic-bitset with space for 1000 bits on the
/// device:
/// \code
/// boost::compute::dynamic_bitset<> bits(1000, queue);
/// \endcode
///
/// The Boost.Compute \c dynamic_bitset class provides a STL-like API and is
/// modeled after the \c boost::dynamic_bitset class from Boost.
///
/// \see \ref vector "vector<T>"
template<class Block = ulong_, class Alloc = buffer_allocator<Block> >
class dynamic_bitset
{
public:
    typedef Block block_type;
    typedef Alloc allocator_type;
    typedef vector<Block, Alloc> container_type;
    typedef typename container_type::size_type size_type;

    BOOST_STATIC_CONSTANT(size_type, bits_per_block = sizeof(block_type) * CHAR_BIT);
    BOOST_STATIC_CONSTANT(size_type, npos = static_cast<size_type>(-1));

    /// Creates a new dynamic bitset with storage for \p size bits. Initializes
    /// all bits to zero.
    dynamic_bitset(size_type size, command_queue &queue)
        : m_bits(size / sizeof(block_type), queue.get_context()),
          m_size(size)
    {
        // initialize all bits to zero
        reset(queue);
    }

    /// Creates a new dynamic bitset as a copy of \p other.
    dynamic_bitset(const dynamic_bitset &other)
        : m_bits(other.m_bits),
          m_size(other.m_size)
    {
    }

    /// Copies the data from \p other to \c *this.
    dynamic_bitset& operator=(const dynamic_bitset &other)
    {
        if(this != &other){
            m_bits = other.m_bits;
            m_size = other.m_size;
        }

        return *this;
    }

    /// Destroys the dynamic bitset.
    ~dynamic_bitset()
    {
    }

    /// Returns the size of the dynamic bitset.
    size_type size() const
    {
        return m_size;
    }

    /// Returns the number of blocks to store the bits in the dynamic bitset.
    size_type num_blocks() const
    {
        return m_bits.size();
    }

    /// Returns the maximum possible size for the dynamic bitset.
    size_type max_size() const
    {
        return m_bits.max_size() * bits_per_block;
    }

    /// Returns \c true if the dynamic bitset is empty (i.e. \c size() == \c 0).
    bool empty() const
    {
        return size() == 0;
    }

    /// Returns the number of set bits (i.e. '1') in the bitset.
    size_type count(command_queue &queue) const
    {
        ulong_ count = 0;
        transform_reduce(
            m_bits.begin(),
            m_bits.end(),
            &count,
            popcount<block_type>(),
            plus<ulong_>(),
            queue
        );
        return static_cast<size_type>(count);
    }

    /// Resizes the bitset to contain \p num_bits. If the new size is greater
    /// than the current size the new bits are set to zero.
    void resize(size_type num_bits, command_queue &queue)
    {
        // resize bits
        const size_type current_block_count = m_bits.size();
        m_bits.resize(num_bits * bits_per_block, queue);

        // fill new block with zeros (if new blocks were added)
        const size_type new_block_count = m_bits.size();
        if(new_block_count > current_block_count){
            fill_n(
                m_bits.begin() + current_block_count,
                new_block_count - current_block_count,
                block_type(0),
                queue
            );
        }

        // store new size
        m_size = num_bits;
    }

    /// Sets the bit at position \p n to \c true.
    void set(size_type n, command_queue &queue)
    {
        set(n, true, queue);
    }

    /// Sets the bit at position \p n to \p value.
    void set(size_type n, bool value, command_queue &queue)
    {
        const size_type bit = n % bits_per_block;
        const size_type block = n / bits_per_block;

        // load current block
        block_type block_value;
        copy_n(m_bits.begin() + block, 1, &block_value, queue);

        // update block value
        if(value){
            block_value |= (size_type(1) << bit);
        }
        else {
            block_value &= ~(size_type(1) << bit);
        }

        // store new block
        copy_n(&block_value, 1, m_bits.begin() + block, queue);
    }

    /// Returns \c true if the bit at position \p n is set (i.e. '1').
    bool test(size_type n, command_queue &queue)
    {
        const size_type bit = n % (sizeof(block_type) * CHAR_BIT);
        const size_type block = n / (sizeof(block_type) * CHAR_BIT);

        block_type block_value;
        copy_n(m_bits.begin() + block, 1, &block_value, queue);

        return block_value & (size_type(1) << bit);
    }

    /// Flips the value of the bit at position \p n.
    void flip(size_type n, command_queue &queue)
    {
        set(n, !test(n, queue), queue);
    }

    /// Returns \c true if any bit in the bitset is set (i.e. '1').
    bool any(command_queue &queue) const
    {
        return any_of(
            m_bits.begin(), m_bits.end(), lambda::_1 != block_type(0), queue
        );
    }

    /// Returns \c true if all of the bits in the bitset are set to zero.
    bool none(command_queue &queue) const
    {
        return !any(queue);
    }

    /// Sets all of the bits in the bitset to zero.
    void reset(command_queue &queue)
    {
        fill(m_bits.begin(), m_bits.end(), block_type(0), queue);
    }

    /// Sets the bit at position \p n to zero.
    void reset(size_type n, command_queue &queue)
    {
        set(n, false, queue);
    }

    /// Empties the bitset (e.g. \c resize(0)).
    void clear()
    {
        m_bits.clear();
    }

    /// Returns the allocator used to allocate storage for the bitset.
    allocator_type get_allocator() const
    {
        return m_bits.get_allocator();
    }

private:
    container_type m_bits;
    size_type m_size;
};

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_CONTAINER_DYNAMIC_BITSET_HPP
