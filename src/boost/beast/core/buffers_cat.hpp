//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_BUFFERS_CAT_HPP
#define BOOST_BEAST_BUFFERS_CAT_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/detail/tuple.hpp>
#include <boost/beast/core/detail/type_traits.hpp>

namespace boost {
namespace beast {

/** A buffer sequence representing a concatenation of buffer sequences.
    @see buffers_cat
*/
template<class... Buffers>
class buffers_cat_view
{
    detail::tuple<Buffers...> bn_;

public:
    /** The type of buffer returned when dereferencing an iterator.
        If every buffer sequence in the view is a <em>MutableBufferSequence</em>,
        then `value_type` will be `net::mutable_buffer`.
        Otherwise, `value_type` will be `net::const_buffer`.
    */
#if BOOST_BEAST_DOXYGEN
    using value_type = __see_below__;
#else
    using value_type = buffers_type<Buffers...>;
#endif

    /// The type of iterator used by the concatenated sequence
    class const_iterator;

    /// Copy Constructor
    buffers_cat_view(buffers_cat_view const&) = default;

    /// Copy Assignment
    buffers_cat_view& operator=(buffers_cat_view const&) = default;

    /** Constructor
        @param buffers The list of buffer sequences to concatenate.
        Copies of the arguments will be maintained for the lifetime
        of the concatenated sequence; however, the ownership of the
        memory buffers themselves is not transferred.
    */
    explicit
    buffers_cat_view(Buffers const&... buffers);

    /// Returns an iterator to the first buffer in the sequence
    const_iterator
    begin() const;

    /// Returns an iterator to one past the last buffer in the sequence
    const_iterator
    end() const;
};

/** Concatenate 1 or more buffer sequences.

    This function returns a constant or mutable buffer sequence which,
    when iterated, efficiently concatenates the input buffer sequences.
    Copies of the arguments passed will be made; however, the returned
    object does not take ownership of the underlying memory. The
    application is still responsible for managing the lifetime of the
    referenced memory.
    @param buffers The list of buffer sequences to concatenate.
    @return A new buffer sequence that represents the concatenation of
    the input buffer sequences. This buffer sequence will be a
    <em>MutableBufferSequence</em> if each of the passed buffer sequences is
    also a <em>MutableBufferSequence</em>; otherwise the returned buffer
    sequence will be a <em>ConstBufferSequence</em>.
    @see buffers_cat_view
*/
#if BOOST_BEAST_DOXYGEN
template<class... BufferSequence>
buffers_cat_view<BufferSequence...>
buffers_cat(BufferSequence const&... buffers)
#else
template<class B1, class... Bn>
buffers_cat_view<B1, Bn...>
buffers_cat(B1 const& b1, Bn const&... bn)
#endif
{
    static_assert(
        is_const_buffer_sequence<B1, Bn...>::value,
        "BufferSequence type requirements not met");
    return buffers_cat_view<B1, Bn...>{b1, bn...};
}

} // beast
} // boost

#include <boost/beast/core/impl/buffers_cat.hpp>

#endif
