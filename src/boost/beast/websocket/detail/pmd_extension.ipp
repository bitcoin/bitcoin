//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_WEBSOCKET_DETAIL_PMD_EXTENSION_IPP
#define BOOST_BEAST_WEBSOCKET_DETAIL_PMD_EXTENSION_IPP

#include <boost/beast/websocket/detail/pmd_extension.hpp>

namespace boost {
namespace beast {
namespace websocket {
namespace detail {

int
parse_bits(string_view s)
{
    if(s.size() == 0)
        return -1;
    if(s.size() > 2)
        return -1;
    if(s[0] < '1' || s[0] > '9')
        return -1;
    unsigned i = 0;
    for(auto c : s)
    {
        if(c < '0' || c > '9')
            return -1;
        auto const i0 = i;
        i = 10 * i + (c - '0');
        if(i < i0)
            return -1;
    }
    return static_cast<int>(i);
}

// Parse permessage-deflate request fields
//
void
pmd_read_impl(pmd_offer& offer, http::ext_list const& list)
{
    offer.accept = false;
    offer.server_max_window_bits= 0;
    offer.client_max_window_bits = 0;
    offer.server_no_context_takeover = false;
    offer.client_no_context_takeover = false;

    for(auto const& ext : list)
    {
        if(beast::iequals(ext.first, "permessage-deflate"))
        {
            for(auto const& param : ext.second)
            {
                if(beast::iequals(param.first,
                    "server_max_window_bits"))
                {
                    if(offer.server_max_window_bits != 0)
                    {
                        // The negotiation offer contains multiple
                        // extension parameters with the same name.
                        //
                        return; // MUST decline
                    }
                    if(param.second.empty())
                    {
                        // The negotiation offer extension
                        // parameter is missing the value.
                        //
                        return; // MUST decline
                    }
                    offer.server_max_window_bits =
                        parse_bits(param.second);
                    if( offer.server_max_window_bits < 8 ||
                        offer.server_max_window_bits > 15)
                    {
                        // The negotiation offer contains an
                        // extension parameter with an invalid value.
                        //
                        return; // MUST decline
                    }
                }
                else if(beast::iequals(param.first,
                    "client_max_window_bits"))
                {
                    if(offer.client_max_window_bits != 0)
                    {
                        // The negotiation offer contains multiple
                        // extension parameters with the same name.
                        //
                        return; // MUST decline
                    }
                    if(! param.second.empty())
                    {
                        offer.client_max_window_bits =
                            parse_bits(param.second);
                        if( offer.client_max_window_bits < 8 ||
                            offer.client_max_window_bits > 15)
                        {
                            // The negotiation offer contains an
                            // extension parameter with an invalid value.
                            //
                            return; // MUST decline
                        }
                    }
                    else
                    {
                        offer.client_max_window_bits = -1;
                    }
                }
                else if(beast::iequals(param.first,
                    "server_no_context_takeover"))
                {
                    if(offer.server_no_context_takeover)
                    {
                        // The negotiation offer contains multiple
                        // extension parameters with the same name.
                        //
                        return; // MUST decline
                    }
                    if(! param.second.empty())
                    {
                        // The negotiation offer contains an
                        // extension parameter with an invalid value.
                        //
                        return; // MUST decline
                    }
                    offer.server_no_context_takeover = true;
                }
                else if(beast::iequals(param.first,
                    "client_no_context_takeover"))
                {
                    if(offer.client_no_context_takeover)
                    {
                        // The negotiation offer contains multiple
                        // extension parameters with the same name.
                        //
                        return; // MUST decline
                    }
                    if(! param.second.empty())
                    {
                        // The negotiation offer contains an
                        // extension parameter with an invalid value.
                        //
                        return; // MUST decline
                    }
                    offer.client_no_context_takeover = true;
                }
                else
                {
                    // The negotiation offer contains an extension
                    // parameter not defined for use in an offer.
                    //
                    return; // MUST decline
                }
            }
            offer.accept = true;
            return;
        }
    }
}

static_string<512>
pmd_write_impl(pmd_offer const& offer)
{
    static_string<512> s = "permessage-deflate";
    if(offer.server_max_window_bits != 0)
    {
        if(offer.server_max_window_bits != -1)
        {
            s += "; server_max_window_bits=";
            s += to_static_string(
                offer.server_max_window_bits);
        }
        else
        {
            s += "; server_max_window_bits";
        }
    }
    if(offer.client_max_window_bits != 0)
    {
        if(offer.client_max_window_bits != -1)
        {
            s += "; client_max_window_bits=";
            s += to_static_string(
                offer.client_max_window_bits);
        }
        else
        {
            s += "; client_max_window_bits";
        }
    }
    if(offer.server_no_context_takeover)
    {
        s += "; server_no_context_takeover";
    }
    if(offer.client_no_context_takeover)
    {
        s += "; client_no_context_takeover";
    }

    return s;
}

static_string<512>
pmd_negotiate_impl(
    pmd_offer& config,
    pmd_offer const& offer,
    permessage_deflate const& o)
{
    static_string<512> s = "permessage-deflate";

    config.server_no_context_takeover =
        offer.server_no_context_takeover ||
            o.server_no_context_takeover;
    if(config.server_no_context_takeover)
        s += "; server_no_context_takeover";

    config.client_no_context_takeover =
        o.client_no_context_takeover ||
            offer.client_no_context_takeover;
    if(config.client_no_context_takeover)
        s += "; client_no_context_takeover";

    if(offer.server_max_window_bits != 0)
        config.server_max_window_bits = (std::min)(
            offer.server_max_window_bits,
                o.server_max_window_bits);
    else
        config.server_max_window_bits =
            o.server_max_window_bits;
    if(config.server_max_window_bits < 15)
    {
        // ZLib's deflateInit silently treats 8 as
        // 9 due to a bug, so prevent 8 from being used.
        //
        if(config.server_max_window_bits < 9)
            config.server_max_window_bits = 9;

        s += "; server_max_window_bits=";
        s += to_static_string(
            config.server_max_window_bits);
    }

    switch(offer.client_max_window_bits)
    {
    case -1:
        // extension parameter is present with no value
        config.client_max_window_bits =
            o.client_max_window_bits;
        if(config.client_max_window_bits < 15)
        {
            s += "; client_max_window_bits=";
            s += to_static_string(
                config.client_max_window_bits);
        }
        break;

    case 0:
        /*  extension parameter is absent.

            If a received extension negotiation offer doesn't have the
            "client_max_window_bits" extension parameter, the corresponding
            extension negotiation response to the offer MUST NOT include the
            "client_max_window_bits" extension parameter.
        */
        if(o.client_max_window_bits == 15)
            config.client_max_window_bits = 15;
        else
            config.accept = false;
        break;

    default:
        // extension parameter has value in [8..15]
        config.client_max_window_bits = (std::min)(
            o.client_max_window_bits,
                offer.client_max_window_bits);
        s += "; client_max_window_bits=";
        s += to_static_string(
            config.client_max_window_bits);
        break;
    }

    return s;
}

void
pmd_normalize(pmd_offer& offer)
{
    if(offer.accept)
    {
        if( offer.server_max_window_bits == 0)
            offer.server_max_window_bits = 15;

        if( offer.client_max_window_bits ==  0 ||
            offer.client_max_window_bits == -1)
            offer.client_max_window_bits = 15;
    }
}

} // detail
} // websocket
} // beast
} // boost

#endif // BOOST_BEAST_WEBSOCKET_DETAIL_PMD_EXTENSION_IPP
