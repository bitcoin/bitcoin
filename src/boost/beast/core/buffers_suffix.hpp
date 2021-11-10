//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_BUFFERS_SUFFIX_HPP
#define BOOST_BEAST_BUFFERS_SUFFIX_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <iterator>
#include <utility>

namespace boost {
namespace beast {

/** Adaptor to progressively trim the front of a <em>BufferSequence</em>.

    This adaptor wraps a buffer sequence to create a new sequence
    which may be incrementally consumed. Bytes consumed are removed
    from the front of the buffer. The underlying memory is not changed,
    instead the adaptor efficiently iterates through a subset of
    the buffers wrapped.

    The wrapped buffer is not modified, a copy is made instead.
    Ownership of the underlying memory is not transferred, the application
    is still responsible for managing its lifetime.

    @tparam BufferSequence The buffer sequence to wrap.

    @par Example

    This function writes the entire contents of a buffer sequence
    to the specified stream.

    @code
    template<class SyncWriteStream, class ConstBufferSequence>
    void send(SyncWriteStream& stream, ConstBufferSequence const& buffers)
    {
        buffers_suffix<ConstBufferSequence> bs{buffers};
        while(buffer_bytes(bs) > 0)
            bs.consume(stream.write_some(bs));
    }
    @endcode
*/
template<class BufferSequence>
class buffers_suffix
{
    using iter_type =
        buffers_iterator_type<BufferSequence>;

    BufferSequence bs_;
    iter_type begin_{};
    std::size_t skip_ = 0;

    template<class Deduced>
    buffers_suffix(Deduced&& other, std::size_t dist)
        : bs_(std::forward<Deduced>(other).bs_)
        , begin_(std::next(
            net::buffer_sequence_begin(bs_),
                dist))
        , skip_(other.skip_)
    {
    }

public:
    /** The type for each element in the list of buffers.

        If <em>BufferSequence</em> meets the requirements of
        <em>MutableBufferSequence</em>, then this type will be
        `net::mutable_buffer`, otherwise this type will be
        `net::const_buffer`.
    */
#if BOOST_BEAST_DOXYGEN
    using value_type = __see_below__;
#else
    using value_type = buffers_type<BufferSequence>;
#endif

#if BOOST_BEAST_DOXYGEN
    /// A bidirectional iterator type that may be used to read elements.
    using const_iterator = __implementation_defined__;

#else
    class const_iterator;

#endif

    /// Constructor
    buffers_suffix();

    /// Copy Constructor
    buffers_suffix(buffers_suffix const&);

    /** Constructor

        A copy of the buffer sequence is made. Ownership of the
        underlying memory is not transferred or copied.
    */
    explicit
    buffers_suffix(BufferSequence const& buffers);

    /** Constructor

        This constructs the buffer sequence in-place from
        a list of arguments.

        @param args Arguments forwarded to the buffers constructor.
    */
    template<class... Args>
    explicit
    buffers_suffix(boost::in_place_init_t, Args&&... args);

    /// Copy Assignment
    buffers_suffix& operator=(buffers_suffix const&);

    /// Get a bidirectional iterator to the first element.
    const_iterator
    begin() const;

    /// Get a bidirectional iterator to one past the last element.
    const_iterator
    end() const;

    /** Remove bytes from the beginning of the sequence.

        @param amount The number of bytes to remove. If this is
        larger than the number of bytes remaining, all the
        bytes remaining are removed.
    */
    void
    consume(std::size_t amount);
};

} // beast
} // boost

#include <boost/beast/core/impl/buffers_suffix.hpp>

#endif
