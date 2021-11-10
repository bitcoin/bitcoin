//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_FLAT_STATIC_BUFFER_HPP
#define BOOST_BEAST_FLAT_STATIC_BUFFER_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/asio/buffer.hpp>
#include <algorithm>
#include <cstddef>
#include <cstring>

namespace boost {
namespace beast {

/** A dynamic buffer using a fixed size internal buffer.

    A dynamic buffer encapsulates memory storage that may be
    automatically resized as required, where the memory is
    divided into two regions: readable bytes followed by
    writable bytes. These memory regions are internal to
    the dynamic buffer, but direct access to the elements
    is provided to permit them to be efficiently used with
    I/O operations.

    Objects of this type meet the requirements of <em>DynamicBuffer</em>
    and have the following additional properties:

    @li A mutable buffer sequence representing the readable
    bytes is returned by @ref data when `this` is non-const.

    @li Buffer sequences representing the readable and writable
    bytes, returned by @ref data and @ref prepare, will have
    length one.

    @li Ownership of the underlying storage belongs to the
    derived class.

    @note Variables are usually declared using the template class
    @ref flat_static_buffer; however, to reduce the number of template
    instantiations, objects should be passed `flat_static_buffer_base&`.

    @see flat_static_buffer
*/
class flat_static_buffer_base
{
    char* begin_;
    char* in_;
    char* out_;
    char* last_;
    char* end_;

    flat_static_buffer_base(
        flat_static_buffer_base const& other) = delete;
    flat_static_buffer_base& operator=(
        flat_static_buffer_base const&) = delete;

public:
    /** Constructor

        This creates a dynamic buffer using the provided storage area.

        @param p A pointer to valid storage of at least `n` bytes.

        @param n The number of valid bytes pointed to by `p`.
    */
    flat_static_buffer_base(
        void* p, std::size_t n) noexcept
    {
        reset(p, n);
    }

    /** Clear the readable and writable bytes to zero.

        This function causes the readable and writable bytes
        to become empty. The capacity is not changed.

        Buffer sequences previously obtained using @ref data or
        @ref prepare become invalid.

        @esafe

        No-throw guarantee.
    */
    BOOST_BEAST_DECL
    void
    clear() noexcept;

    //--------------------------------------------------------------------------

    /// The ConstBufferSequence used to represent the readable bytes.
    using const_buffers_type = net::const_buffer;

    /// The MutableBufferSequence used to represent the writable bytes.
    using mutable_buffers_type = net::mutable_buffer;

    /// Returns the number of readable bytes.
    std::size_t
    size() const noexcept
    {
        return out_ - in_;
    }

    /// Return the maximum number of bytes, both readable and writable, that can ever be held.
    std::size_t
    max_size() const noexcept
    {
        return dist(begin_, end_);
    }

    /// Return the maximum number of bytes, both readable and writable, that can be held without requiring an allocation.
    std::size_t
    capacity() const noexcept
    {
        return max_size();
    }

    /// Returns a constant buffer sequence representing the readable bytes
    const_buffers_type
    data() const noexcept
    {
        return {in_, dist(in_, out_)};
    }

    /// Returns a constant buffer sequence representing the readable bytes
    const_buffers_type
    cdata() const noexcept
    {
        return data();
    }

    /// Returns a mutable buffer sequence representing the readable bytes
    mutable_buffers_type
    data() noexcept
    {
        return {in_, dist(in_, out_)};
    }

    /** Returns a mutable buffer sequence representing writable bytes.
    
        Returns a mutable buffer sequence representing the writable
        bytes containing exactly `n` bytes of storage.

        All buffers sequences previously obtained using
        @ref data or @ref prepare are invalidated.

        @param n The desired number of bytes in the returned buffer
        sequence.

        @throws std::length_error if `size() + n` exceeds `max_size()`.

        @esafe

        Strong guarantee.
    */
    BOOST_BEAST_DECL
    mutable_buffers_type
    prepare(std::size_t n);

    /** Append writable bytes to the readable bytes.

        Appends n bytes from the start of the writable bytes to the
        end of the readable bytes. The remainder of the writable bytes
        are discarded. If n is greater than the number of writable
        bytes, all writable bytes are appended to the readable bytes.

        All buffers sequences previously obtained using
        @ref data or @ref prepare are invalidated.

        @param n The number of bytes to append. If this number
        is greater than the number of writable bytes, all
        writable bytes are appended.

        @esafe

        No-throw guarantee.
    */
    void
    commit(std::size_t n) noexcept
    {
        out_ += (std::min<std::size_t>)(n, last_ - out_);
    }

    /** Remove bytes from beginning of the readable bytes.

        Removes n bytes from the beginning of the readable bytes.

        All buffers sequences previously obtained using
        @ref data or @ref prepare are invalidated.

        @param n The number of bytes to remove. If this number
        is greater than the number of readable bytes, all
        readable bytes are removed.

        @esafe

        No-throw guarantee.
    */
    BOOST_BEAST_DECL
    void
    consume(std::size_t n) noexcept;

protected:
    /** Constructor

        The buffer will be in an undefined state. It is necessary
        for the derived class to call @ref reset with a pointer
        and size in order to initialize the object.
    */
    flat_static_buffer_base() = default;

    /** Reset the pointed-to buffer.

        This function resets the internal state to the buffer provided.
        All input and output sequences are invalidated. This function
        allows the derived class to construct its members before
        initializing the static buffer.

        @param p A pointer to valid storage of at least `n` bytes.

        @param n The number of valid bytes pointed to by `p`.

        @esafe

        No-throw guarantee.
    */
    BOOST_BEAST_DECL
    void
    reset(void* p, std::size_t n) noexcept;

private:
    static
    std::size_t
    dist(char const* first, char const* last) noexcept
    {
        return static_cast<std::size_t>(last - first);
    }
};

//------------------------------------------------------------------------------

/** A <em>DynamicBuffer</em> with a fixed size internal buffer.

    Buffer sequences returned by @ref data and @ref prepare
    will always be of length one.
    This implements a dynamic buffer using no memory allocations.

    @tparam N The number of bytes in the internal buffer.

    @note To reduce the number of template instantiations when passing
    objects of this type in a deduced context, the signature of the
    receiving function should use @ref flat_static_buffer_base instead.

    @see flat_static_buffer_base
*/
template<std::size_t N>
class flat_static_buffer : public flat_static_buffer_base
{
    char buf_[N];

public:
    /// Constructor
    flat_static_buffer(flat_static_buffer const&);

    /// Constructor
    flat_static_buffer()
        : flat_static_buffer_base(buf_, N)
    {
    }

    /// Assignment
    flat_static_buffer& operator=(flat_static_buffer const&);

    /// Returns the @ref flat_static_buffer_base portion of this object
    flat_static_buffer_base&
    base()
    {
        return *this;
    }

    /// Returns the @ref flat_static_buffer_base portion of this object
    flat_static_buffer_base const&
    base() const
    {
        return *this;
    }

    /// Return the maximum sum of the input and output sequence sizes.
    std::size_t constexpr
    max_size() const
    {
        return N;
    }

    /// Return the maximum sum of input and output sizes that can be held without an allocation.
    std::size_t constexpr
    capacity() const
    {
        return N;
    }
};

} // beast
} // boost

#include <boost/beast/core/impl/flat_static_buffer.hpp>
#ifdef BOOST_BEAST_HEADER_ONLY
#include <boost/beast/core/impl/flat_static_buffer.ipp>
#endif

#endif