//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_FIELDS_IPP
#define BOOST_BEAST_HTTP_IMPL_FIELDS_IPP

#include <boost/beast/http/fields.hpp>

namespace boost {
namespace beast {
namespace http {
namespace detail {

// `basic_fields` assumes that `std::size_t` is larger than `uint16_t`, so we
// verify it explicitly here, so that users that use split compilation don't
// need to pay the (fairly small) price for this sanity check
BOOST_STATIC_ASSERT((std::numeric_limits<std::size_t>::max)() >=
    (std::numeric_limits<std::uint32_t>::max)());

// Filter a token list
//
inline
void
filter_token_list(
    beast::detail::temporary_buffer& s,
    string_view value,
    iequals_predicate const& pred)
{
    token_list te{value};
    auto it = te.begin();
    auto last = te.end();
    if(it == last)
        return;
    while(pred(*it))
        if(++it == last)
            return;
    s.append(*it);
    while(++it != last)
    {
        if(! pred(*it))
        {
            s.append(", ", *it);
        }
    }
}

void
filter_token_list_last(
    beast::detail::temporary_buffer& s,
    string_view value,
    iequals_predicate const& pred)
{
    token_list te{value};
    if(te.begin() != te.end())
    {
        auto it = te.begin();
        auto next = std::next(it);
        if(next == te.end())
        {
            if(! pred(*it))
                s.append(*it);
            return;
        }
        s.append(*it);
        for(;;)
        {
            it = next;
            next = std::next(it);
            if(next == te.end())
            {
                if(! pred(*it))
                {
                    s.append(", ", *it);
                }
                return;
            }
            s.append(", ", *it);
        }
    }
}

void
keep_alive_impl(
    beast::detail::temporary_buffer& s, string_view value,
    unsigned version, bool keep_alive)
{
    if(version < 11)
    {
        if(keep_alive)
        {
            // remove close
            filter_token_list(s, value, iequals_predicate{"close", {}});
            // add keep-alive
            if(s.empty())
                s.append("keep-alive");
            else if(! token_list{value}.exists("keep-alive"))
                s.append(", keep-alive");
        }
        else
        {
            // remove close and keep-alive
            filter_token_list(s, value,
                iequals_predicate{"close", "keep-alive"});
        }
    }
    else
    {
        if(keep_alive)
        {
            // remove close and keep-alive
            filter_token_list(s, value,
                iequals_predicate{"close", "keep-alive"});
        }
        else
        {
            // remove keep-alive
            filter_token_list(s, value, iequals_predicate{"keep-alive", {}});
            // add close
            if(s.empty())
                s.append("close");
            else if(! token_list{value}.exists("close"))
                s.append(", close");
        }
    }
}

} // detail
} // http
} // beast
} // boost

#endif // BOOST_BEAST_HTTP_IMPL_FIELDS_IPP
