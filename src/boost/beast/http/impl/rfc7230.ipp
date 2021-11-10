//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_RFC7230_IPP
#define BOOST_BEAST_HTTP_IMPL_RFC7230_IPP

#include <boost/beast/http/rfc7230.hpp>
#include <algorithm>

namespace boost {
namespace beast {
namespace http {


std::string
param_list::const_iterator::
unquote(string_view sr)
{
    std::string s;
    s.reserve(sr.size());
    auto it = sr.begin() + 1;
    auto end = sr.end() - 1;
    while(it != end)
    {
        if(*it == '\\')
            ++it;
        s.push_back(*it);
        ++it;
    }
    return s;
}

void
param_list::const_iterator::
increment()
{
    s_.clear();
    pi_.increment();
    if(pi_.empty())
    {
        pi_.it = pi_.last;
        pi_.first = pi_.last;
    }
    else if(! pi_.v.second.empty() &&
        pi_.v.second.front() == '"')
    {
        s_ = unquote(pi_.v.second);
        pi_.v.second = string_view{
            s_.data(), s_.size()};
    }
}

void
ext_list::const_iterator::
increment()
{
    /*
        ext-list    = *( "," OWS ) ext *( OWS "," [ OWS ext ] )
        ext         = token param-list
        param-list  = *( OWS ";" OWS param )
        param       = token OWS "=" OWS ( token / quoted-string )

        chunked;a=b;i=j;gzip;windowBits=12
        x,y
        ,,,,,chameleon
    */
    auto const err =
        [&]
        {
            it_ = last_;
            first_ = last_;
        };
    auto need_comma = it_ != first_;
    v_.first = {};
    first_ = it_;
    for(;;)
    {
        detail::skip_ows(it_, last_);
        if(it_ == last_)
            return err();
        auto const c = *it_;
        if(detail::is_token_char(c))
        {
            if(need_comma)
                return err();
            auto const p0 = it_;
            for(;;)
            {
                ++it_;
                if(it_ == last_)
                    break;
                if(! detail::is_token_char(*it_))
                    break;
            }
            v_.first = string_view{&*p0,
                static_cast<std::size_t>(it_ - p0)};
			if (it_ == last_)
				return;
            detail::param_iter pi;
            pi.it = it_;
            pi.first = it_;
            pi.last = last_;
            for(;;)
            {
                pi.increment();
                if(pi.empty())
                    break;
            }
            v_.second = param_list{string_view{&*it_,
                static_cast<std::size_t>(pi.it - it_)}};
            it_ = pi.it;
            return;
        }
        if(c != ',')
            return err();
        need_comma = false;
        ++it_;
    }
}

auto
ext_list::
find(string_view const& s) -> const_iterator
{
    return std::find_if(begin(), end(),
        [&s](value_type const& v)
        {
            return beast::iequals(s, v.first);
        });
}

bool
ext_list::
exists(string_view const& s)
{
    return find(s) != end();
}

void
token_list::const_iterator::
increment()
{
    /*
        token-list  = *( "," OWS ) token *( OWS "," [ OWS ext ] )
    */
    auto const err =
        [&]
        {
            it_ = last_;
            first_ = last_;
        };
    auto need_comma = it_ != first_;
    v_ = {};
    first_ = it_;
    for(;;)
    {
        detail::skip_ows(it_, last_);
        if(it_ == last_)
            return err();
        auto const c = *it_;
        if(detail::is_token_char(c))
        {
            if(need_comma)
                return err();
            auto const p0 = it_;
            for(;;)
            {
                ++it_;
                if(it_ == last_)
                    break;
                if(! detail::is_token_char(*it_))
                    break;
            }
            v_ = string_view{&*p0,
                static_cast<std::size_t>(it_ - p0)};
            return;
        }
        if(c != ',')
            return err();
        need_comma = false;
        ++it_;
    }
}

bool
token_list::
exists(string_view const& s)
{
    return std::find_if(begin(), end(),
        [&s](value_type const& v)
        {
            return beast::iequals(s, v);
        }
    ) != end();
}

} // http
} // beast
} // boost

#endif // BOOST_BEAST_HTTP_IMPL_RFC7230_IPP
