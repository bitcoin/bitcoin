//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_PRNG_IPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_PRNG_IPP

#include <boost/beast/websocket/detail/prng.hpp>
#include <boost/beast/core/detail/chacha.hpp>
#include <boost/beast/core/detail/pcg.hpp>
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <random>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

//------------------------------------------------------------------------------

std::uint32_t const*
prng_seed(std::seed_seq* ss)
{
    struct data
    {
        std::uint32_t v[8];

        explicit
        data(std::seed_seq* pss)
        {
            if(! pss)
            {
                std::random_device g;
                std::seed_seq ss{
                    g(), g(), g(), g(),
                    g(), g(), g(), g()};
                ss.generate(v, v+8);
            }
            else
            {
                pss->generate(v, v+8);
            }
        }
    };
    static data const d(ss);
    return d.v;
}

//------------------------------------------------------------------------------

inline
std::uint32_t
make_nonce()
{
    static std::atomic<std::uint32_t> nonce{0};
    return ++nonce;
}

inline
beast::detail::pcg make_pcg()
{
    auto const pv = prng_seed();
    return beast::detail::pcg{
        ((static_cast<std::uint64_t>(pv[0])<<32)+pv[1]) ^
        ((static_cast<std::uint64_t>(pv[2])<<32)+pv[3]) ^
        ((static_cast<std::uint64_t>(pv[4])<<32)+pv[5]) ^
        ((static_cast<std::uint64_t>(pv[6])<<32)+pv[7]), make_nonce()};
}

#ifdef BOOST_NO_CXX11_THREAD_LOCAL

inline
std::uint32_t
secure_generate()
{
    struct generator
    {
        std::uint32_t operator()()
        {
            std::lock_guard<std::mutex> guard{mtx};
            return gen();
        }

        beast::detail::chacha<20> gen;
        std::mutex mtx;
    };
    static generator gen{beast::detail::chacha<20>{prng_seed(), make_nonce()}};
    return gen();
}

inline
std::uint32_t
fast_generate()
{
    struct generator
    {
        std::uint32_t operator()()
        {
            std::lock_guard<std::mutex> guard{mtx};
            return gen();
        }

        beast::detail::pcg gen;
        std::mutex mtx;
    };
    static generator gen{make_pcg()};
    return gen();
}

#else

inline
std::uint32_t
secure_generate()
{
    thread_local static beast::detail::chacha<20> gen{prng_seed(), make_nonce()};
    return gen();
}

inline
std::uint32_t
fast_generate()
{
    thread_local static beast::detail::pcg gen{make_pcg()};
    return gen();
}

#endif

generator
make_prng(bool secure)
{
    if (secure)
        return &secure_generate;
    else
        return &fast_generate;
}

} // detail
} // websocket
} // beast
} // boost

#endif
