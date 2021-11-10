//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_UTF8_CHECKER_IPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_UTF8_CHECKER_IPP

#include <boost/beast/websocket/detail/utf8_checker.hpp>

#include <boost/assert.hpp>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

void
utf8_checker::
reset()
{
    need_ = 0;
    p_ = cp_;
}

bool
utf8_checker::
finish()
{
    auto const success = need_ == 0;
    reset();
    return success;
}

bool
utf8_checker::
write(std::uint8_t const* in, std::size_t size)
{
    auto const valid =
        [](std::uint8_t const*& p)
        {
            if(p[0] < 128)
            {
                ++p;
                return true;
            }
            if((p[0] & 0xe0) == 0xc0)
            {
                if( (p[1] & 0xc0) != 0x80 ||
                    (p[0] & 0x1e) == 0)  // overlong
                    return false;
                p += 2;
                return true;
            }
            if((p[0] & 0xf0) == 0xe0)
            {
                if(    (p[1] & 0xc0) != 0x80
                    || (p[2] & 0xc0) != 0x80
                    || (p[0] == 0xe0 && (p[1] & 0x20) == 0) // overlong
                    || (p[0] == 0xed && (p[1] & 0x20) == 0x20) // surrogate
                    //|| (p[0] == 0xef && p[1] == 0xbf && (p[2] & 0xfe) == 0xbe) // U+FFFE or U+FFFF
                    )
                    return false;
                p += 3;
                return true;
            }
            if((p[0] & 0xf8) == 0xf0)
            {
                if(    (p[0] & 0x07) >= 0x05 // invalid F5...FF characters
                    || (p[1] & 0xc0) != 0x80
                    || (p[2] & 0xc0) != 0x80
                    || (p[3] & 0xc0) != 0x80
                    || (p[0] == 0xf0 && (p[1] & 0x30) == 0) // overlong
                    || (p[0] == 0xf4 && p[1] > 0x8f) || p[0] > 0xf4 // > U+10FFFF
                    )
                    return false;
                p += 4;
                return true;
            }
            return false;
        };
    auto const fail_fast =
        [&]()
        {
            if(cp_[0] < 128)
            {
                return false;
            }

            const auto& p = cp_; // alias, only to keep this code similar to valid() above
            const auto known_only = p_ - cp_;
            if (known_only == 1)
            {
                if((p[0] & 0xe0) == 0xc0)
                {
                    return ((p[0] & 0x1e) == 0);  // overlong
                }
                if((p[0] & 0xf0) == 0xe0)
                {
                    return false;
                }
                if((p[0] & 0xf8) == 0xf0)
                {
                    return ((p[0] & 0x07) >= 0x05);  // invalid F5...FF characters
                }
            }
            else if (known_only == 2)
            {
                if((p[0] & 0xe0) == 0xc0)
                {
                    return ((p[1] & 0xc0) != 0x80 ||
                            (p[0] & 0x1e) == 0);  // overlong
                }
                if((p[0] & 0xf0) == 0xe0)
                {
                    return (  (p[1] & 0xc0) != 0x80
                           || (p[0] == 0xe0 && (p[1] & 0x20) == 0) // overlong
                           || (p[0] == 0xed && (p[1] & 0x20) == 0x20)); // surrogate
                }
                if((p[0] & 0xf8) == 0xf0)
                {
                    return (  (p[0] & 0x07) >= 0x05 // invalid F5...FF characters
                           || (p[1] & 0xc0) != 0x80
                           || (p[0] == 0xf0 && (p[1] & 0x30) == 0) // overlong
                           || (p[0] == 0xf4 && p[1] > 0x8f) || p[0] > 0xf4); // > U+10FFFF
                }
            }
            else if (known_only == 3)
            {
                if((p[0] & 0xe0) == 0xc0)
                {
                    return (  (p[1] & 0xc0) != 0x80
                           || (p[0] & 0x1e) == 0);  // overlong
                }
                if((p[0] & 0xf0) == 0xe0)
                {
                    return (  (p[1] & 0xc0) != 0x80
                           || (p[2] & 0xc0) != 0x80
                           || (p[0] == 0xe0 && (p[1] & 0x20) == 0) // overlong
                           || (p[0] == 0xed && (p[1] & 0x20) == 0x20)); // surrogate
                           //|| (p[0] == 0xef && p[1] == 0xbf && (p[2] & 0xfe) == 0xbe) // U+FFFE or U+FFFF
                }
                if((p[0] & 0xf8) == 0xf0)
                {
                    return (  (p[0] & 0x07) >= 0x05 // invalid F5...FF characters
                           || (p[1] & 0xc0) != 0x80
                           || (p[2] & 0xc0) != 0x80
                           || (p[0] == 0xf0 && (p[1] & 0x30) == 0) // overlong
                           || (p[0] == 0xf4 && p[1] > 0x8f) || p[0] > 0xf4); // > U+10FFFF
                }
            }
            return true;
        };
    auto const needed =
        [](std::uint8_t const v)
        {
            if(v < 128)
                return 1;
            if(v < 192)
                return 0;
            if(v < 224)
                return 2;
            if(v < 240)
                return 3;
            if(v < 248)
                return 4;
            return 0;
        };

    auto const end = in + size;

    // Finish up any incomplete code point
    if(need_ > 0)
    {
        // Calculate what we have
        auto n = (std::min)(size, need_);
        size -= n;
        need_ -= n;

        // Add characters to the code point
        while(n--)
            *p_++ = *in++;
        BOOST_ASSERT(p_ <= cp_ + 4);

        // Still incomplete?
        if(need_ > 0)
        {
            // Incomplete code point
            BOOST_ASSERT(in == end);

            // Do partial validation on the incomplete
            // code point, this is called "Fail fast"
            // in Autobahn|Testsuite parlance.
            return ! fail_fast();
        }

        // Complete code point, validate it
        std::uint8_t const* p = &cp_[0];
        if(! valid(p))
            return false;
        p_ = cp_;
    }

    if(size <= sizeof(std::size_t))
        goto slow;

    // Align `in` to sizeof(std::size_t) boundary
    {
        auto const in0 = in;
        auto last = reinterpret_cast<std::uint8_t const*>(
            ((reinterpret_cast<std::uintptr_t>(in) + sizeof(std::size_t) - 1) /
                sizeof(std::size_t)) * sizeof(std::size_t));

        // Check one character at a time for low-ASCII
        while(in < last)
        {
            if(*in & 0x80)
            {
                // Not low-ASCII so switch to slow loop
                size = size - (in - in0);
                goto slow;
            }
            ++in;
        }
        size = size - (in - in0);
    }

    // Fast loop: Process 4 or 8 low-ASCII characters at a time
    {
        auto const in0 = in;
        auto last = in + size - 7;
        auto constexpr mask = static_cast<
            std::size_t>(0x8080808080808080 & ~std::size_t{0});
        while(in < last)
        {
#if 0
            std::size_t temp;
            std::memcpy(&temp, in, sizeof(temp));
            if((temp & mask) != 0)
#else
            // Technically UB but works on all known platforms
            if((*reinterpret_cast<std::size_t const*>(in) & mask) != 0)
#endif
            {
                size = size - (in - in0);
                goto slow;
            }
            in += sizeof(std::size_t);
        }
        // There's at least one more full code point left
        last += 4;
        while(in < last)
            if(! valid(in))
                return false;
        goto tail;
    }

slow:
    // Slow loop: Full validation on one code point at a time
    {
        auto last = in + size - 3;
        while(in < last)
            if(! valid(in))
                return false;
    }

tail:
    // Handle the remaining bytes. The last
    // characters could split a code point so
    // we save the partial code point for later.
    //
    // On entry to the loop, `in` points to the
    // beginning of a code point.
    //
    for(;;)
    {
        // Number of chars left
        auto n = end - in;
        if(! n)
            break;

        // Chars we need to finish this code point
        auto const need = needed(*in);
        if(need == 0)
            return false;
        if(need <= n)
        {
            // Check a whole code point
            if(! valid(in))
                return false;
        }
        else
        {
            // Calculate how many chars we need
            // to finish this partial code point
            need_ = need - n;

            // Save the partial code point
            while(n--)
                *p_++ = *in++;
            BOOST_ASSERT(in == end);
            BOOST_ASSERT(p_ <= cp_ + 4);

            // Do partial validation on the incomplete
            // code point, this is called "Fail fast"
            // in Autobahn|Testsuite parlance.
            return ! fail_fast();
        }
    }
    return true;
}

bool
check_utf8(char const* p, std::size_t n)
{
    utf8_checker c;
    if(! c.write(reinterpret_cast<const uint8_t*>(p), n))
        return false;
    return c.finish();
}

} // detail
} // websocket
} // beast
} // boost

#endif // BOOST_BEAST_WEBSOCKET_DETAIL_UTF8_CHECKER_IPP
