//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//
// This is a derivative work, original copyright follows:
//

/*
    Copyright (c) 2015 Orson Peters <orsonpeters@gmail.com>
    
    This software is provided 'as-is', without any express or implied warranty. In no event will the
    authors be held liable for any damages arising from the use of this software.
    
    Permission is granted to anyone to use this software for any purpose, including commercial
    applications, and to alter it and redistribute it freely, subject to the following restrictions:
    
    1. The origin of this software must not be misrepresented; you must not claim that you wrote the
       original software. If you use this software in a product, an acknowledgment in the product
       documentation would be appreciated but is not required.
    
    2. Altered source versions must be plainly marked as such, and must not be misrepresented as
       being the original software.
    
    3. This notice may not be removed or altered from any source distribution.
*/

#ifndef BOOST_BEAST_CORE_DETAIL_CHACHA_HPP
#define BOOST_BEAST_CORE_DETAIL_CHACHA_HPP

#include <cstdint>
#include <limits>

namespace boost {
namespace beast {
namespace detail {

template<std::size_t R>
class chacha
{
    alignas(16) std::uint32_t block_[16];
    std::uint32_t keysetup_[8];
    std::uint64_t ctr_ = 0;
    int idx_ = 16;

    void generate_block()
    {
        std::uint32_t constexpr constants[4] = {
            0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };
        std::uint32_t input[16];
        for (int i = 0; i < 4; ++i)
            input[i] = constants[i];
        for (int i = 0; i < 8; ++i)
            input[4 + i] = keysetup_[i];
        input[12] = (ctr_ / 16) & 0xffffffffu;
        input[13] = (ctr_ / 16) >> 32;
        input[14] = input[15] = 0xdeadbeef; // Could use 128-bit counter.
        for (int i = 0; i < 16; ++i)
            block_[i] = input[i];
        chacha_core();
        for (int i = 0; i < 16; ++i)
            block_[i] += input[i];
    }

    void chacha_core()
    {
        #define BOOST_BEAST_CHACHA_ROTL32(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

        #define BOOST_BEAST_CHACHA_QUARTERROUND(x, a, b, c, d) \
            x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = BOOST_BEAST_CHACHA_ROTL32(x[d], 16); \
            x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = BOOST_BEAST_CHACHA_ROTL32(x[b], 12); \
            x[a] = x[a] + x[b]; x[d] ^= x[a]; x[d] = BOOST_BEAST_CHACHA_ROTL32(x[d],  8); \
            x[c] = x[c] + x[d]; x[b] ^= x[c]; x[b] = BOOST_BEAST_CHACHA_ROTL32(x[b],  7)

        for (unsigned i = 0; i < R; i += 2)
        {
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 0, 4,  8, 12);
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 1, 5,  9, 13);
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 2, 6, 10, 14);
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 3, 7, 11, 15);
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 0, 5, 10, 15);
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 1, 6, 11, 12);
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 2, 7,  8, 13);
            BOOST_BEAST_CHACHA_QUARTERROUND(block_, 3, 4,  9, 14);
        }

        #undef BOOST_BEAST_CHACHA_QUARTERROUND
        #undef BOOST_BEAST_CHACHA_ROTL32
    }

public:
    static constexpr std::size_t state_size = sizeof(chacha::keysetup_);

    using result_type = std::uint32_t;

    chacha(std::uint32_t const* v, std::uint64_t stream)
    {
        for (int i = 0; i < 6; ++i)
            keysetup_[i] = v[i];
        keysetup_[6] = v[6] + (stream & 0xffffffff);
        keysetup_[7] = v[7] + ((stream >> 32) & 0xffffffff);
    }

    std::uint32_t
    operator()()
    {
        if(idx_ == 16)
        {
            idx_ = 0;
            ++ctr_;
            generate_block();
        }
        return block_[idx_++];
    }
};

} // detail
} // beast
} // boost

#endif
