//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_BUFFERS_TO_STRING_HPP
#define BOOST_BEAST_BUFFERS_TO_STRING_HPP

#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/buffer_traits.hpp>
#include <boost/beast/core/buffers_range.hpp>
#include <boost/asio/buffer.hpp>
#include <string>

namespace boost {
namespace beast {

/** Return a string representing the contents of a buffer sequence.

    This function returns a string representing an entire buffer
    sequence. Nulls and unprintable characters in the buffer
    sequence are inserted to the resulting string as-is. No
    character conversions are performed.

    @param buffers The buffer sequence to convert

    @par Example

    This function writes a buffer sequence converted to a string
    to `std::cout`.

    @code
    template<class ConstBufferSequence>
    void print(ConstBufferSequence const& buffers)
    {
        std::cout << buffers_to_string(buffers) << std::endl;
    }
    @endcode
*/
template<class ConstBufferSequence>
std::string
buffers_to_string(ConstBufferSequence const& buffers)
{
    static_assert(
        net::is_const_buffer_sequence<ConstBufferSequence>::value,
        "ConstBufferSequence type requirements not met");
    std::string result;
    result.reserve(buffer_bytes(buffers));
    for(auto const buffer : buffers_range_ref(buffers))
        result.append(static_cast<char const*>(
            buffer.data()), buffer.size());
    return result;
}

} // beast
} // boost

#endif
