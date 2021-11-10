//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_CORE_DETAIL_FLAT_STREAM_HPP
#define BOOST_BEAST_CORE_DETAIL_FLAT_STREAM_HPP

#include <boost/beast/core/buffer_traits.hpp>
#include <boost/asio/buffer.hpp>
#include <cstdlib>

namespace boost {
namespace beast {
namespace detail {

class flat_stream_base
{
public:
    // Largest buffer size we will flatten.
    // 16KB is the upper limit on reasonably sized HTTP messages.
    static std::size_t constexpr max_size = 16 * 1024;

    // Largest stack we will use to flatten
    static std::size_t constexpr max_stack = 8 * 1024;

    struct flatten_result
    {
        std::size_t size;
        bool flatten;
    };

    // calculates the flatten settings for a buffer sequence
    template<class BufferSequence>
    static
    flatten_result
    flatten(
        BufferSequence const& buffers, std::size_t limit)
    {
        flatten_result result{0, false};
        auto first = net::buffer_sequence_begin(buffers);
        auto last = net::buffer_sequence_end(buffers);
        if(first != last)
        {
            result.size = buffer_bytes(*first);
            if(result.size < limit)
            {
                auto it = first;
                auto prev = first;
                while(++it != last)
                {
                    auto const n = buffer_bytes(*it);
                    if(result.size + n > limit)
                        break;
                    result.size += n;
                    prev = it;
                }
                result.flatten = prev != first;
            }
        }
        return result;
    }
};

} // detail
} // beast
} // boost

#endif
