//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_HTTP_IMPL_FIELD_IPP
#define BOOST_BEAST_HTTP_IMPL_FIELD_IPP

#include <boost/beast/http/field.hpp>
#include <boost/assert.hpp>
#include <algorithm>
#include <array>
#include <cstring>
#include <ostream>


namespace boost {
namespace beast {
namespace http {

namespace detail {

struct field_table
{
    static
    std::uint32_t
    get_chars(
        unsigned char const* p) noexcept
    {
        // VFALCO memcpy is endian-dependent
        //std::memcpy(&v, p, 4);
        // Compiler should be smart enough to
        // optimize this down to one instruction.
        return
             p[0] |
            (p[1] <<  8) |
            (p[2] << 16) |
            (p[3] << 24);
    }

    using array_type =
        std::array<string_view, 357>;

    // Strings are converted to lowercase
    static
    std::uint32_t
    digest(string_view s)
    {
        std::uint32_t r = 0;
        std::size_t n = s.size();
        auto p = reinterpret_cast<
            unsigned char const*>(s.data());
        // consume N characters at a time
        // VFALCO Can we do 8 on 64-bit systems?
        while(n >= 4)
        {
            auto const v = get_chars(p);
            r = (r * 5 + (
                v | 0x20202020 )); // convert to lower
            p += 4;
            n -= 4;
        }
        // handle remaining characters
        while( n > 0 )
        {
            r = r * 5 + ( *p | 0x20 );
            ++p;
            --n;
        }
        return r;
    }

    // This comparison is case-insensitive, and the
    // strings must contain only valid http field characters.
    static
    bool
    equals(string_view lhs, string_view rhs)
    {
        using Int = std::uint32_t; // VFALCO std::size_t?
        auto n = lhs.size();
        if(n != rhs.size())
            return false;
        auto p1 = reinterpret_cast<
            unsigned char const*>(lhs.data());
        auto p2 = reinterpret_cast<
            unsigned char const*>(rhs.data());
        auto constexpr S = sizeof(Int);
        auto constexpr Mask = static_cast<Int>(
            0xDFDFDFDFDFDFDFDF & ~Int{0});
        for(; n >= S; p1 += S, p2 += S, n -= S)
        {
            Int const v1 = get_chars(p1);
            Int const v2 = get_chars(p2);
            if((v1 ^ v2) & Mask)
                return false;
        }
        for(; n; ++p1, ++p2, --n)
            if(( *p1 ^ *p2) & 0xDF)
                return false;
        return true;
    }

    array_type by_name_;

    enum { N = 5155 };
    unsigned char map_[ N ][ 2 ] = {};

/*
    From:
    
    https://www.iana.org/assignments/message-headers/message-headers.xhtml
*/
    field_table()
        : by_name_({{
// string constants
            "<unknown-field>",
            "A-IM",
            "Accept",
            "Accept-Additions",
            "Accept-Charset",
            "Accept-Datetime",
            "Accept-Encoding",
            "Accept-Features",
            "Accept-Language",
            "Accept-Patch",
            "Accept-Post",
            "Accept-Ranges",
            "Access-Control",
            "Access-Control-Allow-Credentials",
            "Access-Control-Allow-Headers",
            "Access-Control-Allow-Methods",
            "Access-Control-Allow-Origin",
            "Access-Control-Expose-Headers",
            "Access-Control-Max-Age",
            "Access-Control-Request-Headers",
            "Access-Control-Request-Method",
            "Age",
            "Allow",
            "ALPN",
            "Also-Control",
            "Alt-Svc",
            "Alt-Used",
            "Alternate-Recipient",
            "Alternates",
            "Apparently-To",
            "Apply-To-Redirect-Ref",
            "Approved",
            "Archive",
            "Archived-At",
            "Article-Names",
            "Article-Updates",
            "Authentication-Control",
            "Authentication-Info",
            "Authentication-Results",
            "Authorization",
            "Auto-Submitted",
            "Autoforwarded",
            "Autosubmitted",
            "Base",
            "Bcc",
            "Body",
            "C-Ext",
            "C-Man",
            "C-Opt",
            "C-PEP",
            "C-PEP-Info",
            "Cache-Control",
            "CalDAV-Timezones",
            "Cancel-Key",
            "Cancel-Lock",
            "Cc",
            "Close",
            "Comments",
            "Compliance",
            "Connection",
            "Content-Alternative",
            "Content-Base",
            "Content-Description",
            "Content-Disposition",
            "Content-Duration",
            "Content-Encoding",
            "Content-features",
            "Content-ID",
            "Content-Identifier",
            "Content-Language",
            "Content-Length",
            "Content-Location",
            "Content-MD5",
            "Content-Range",
            "Content-Return",
            "Content-Script-Type",
            "Content-Style-Type",
            "Content-Transfer-Encoding",
            "Content-Type",
            "Content-Version",
            "Control",
            "Conversion",
            "Conversion-With-Loss",
            "Cookie",
            "Cookie2",
            "Cost",
            "DASL",
            "Date",
            "Date-Received",
            "DAV",
            "Default-Style",
            "Deferred-Delivery",
            "Delivery-Date",
            "Delta-Base",
            "Depth",
            "Derived-From",
            "Destination",
            "Differential-ID",
            "Digest",
            "Discarded-X400-IPMS-Extensions",
            "Discarded-X400-MTS-Extensions",
            "Disclose-Recipients",
            "Disposition-Notification-Options",
            "Disposition-Notification-To",
            "Distribution",
            "DKIM-Signature",
            "DL-Expansion-History",
            "Downgraded-Bcc",
            "Downgraded-Cc",
            "Downgraded-Disposition-Notification-To",
            "Downgraded-Final-Recipient",
            "Downgraded-From",
            "Downgraded-In-Reply-To",
            "Downgraded-Mail-From",
            "Downgraded-Message-Id",
            "Downgraded-Original-Recipient",
            "Downgraded-Rcpt-To",
            "Downgraded-References",
            "Downgraded-Reply-To",
            "Downgraded-Resent-Bcc",
            "Downgraded-Resent-Cc",
            "Downgraded-Resent-From",
            "Downgraded-Resent-Reply-To",
            "Downgraded-Resent-Sender",
            "Downgraded-Resent-To",
            "Downgraded-Return-Path",
            "Downgraded-Sender",
            "Downgraded-To",
            "EDIINT-Features",
            "Eesst-Version",
            "Encoding",
            "Encrypted",
            "Errors-To",
            "ETag",
            "Expect",
            "Expires",
            "Expiry-Date",
            "Ext",
            "Followup-To",
            "Forwarded",
            "From",
            "Generate-Delivery-Report",
            "GetProfile",
            "Hobareg",
            "Host",
            "HTTP2-Settings",
            "If",
            "If-Match",
            "If-Modified-Since",
            "If-None-Match",
            "If-Range",
            "If-Schedule-Tag-Match",
            "If-Unmodified-Since",
            "IM",
            "Importance",
            "In-Reply-To",
            "Incomplete-Copy",
            "Injection-Date",
            "Injection-Info",
            "Jabber-ID",
            "Keep-Alive",
            "Keywords",
            "Label",
            "Language",
            "Last-Modified",
            "Latest-Delivery-Time",
            "Lines",
            "Link",
            "List-Archive",
            "List-Help",
            "List-ID",
            "List-Owner",
            "List-Post",
            "List-Subscribe",
            "List-Unsubscribe",
            "List-Unsubscribe-Post",
            "Location",
            "Lock-Token",
            "Man",
            "Max-Forwards",
            "Memento-Datetime",
            "Message-Context",
            "Message-ID",
            "Message-Type",
            "Meter",
            "Method-Check",
            "Method-Check-Expires",
            "MIME-Version",
            "MMHS-Acp127-Message-Identifier",
            "MMHS-Authorizing-Users",
            "MMHS-Codress-Message-Indicator",
            "MMHS-Copy-Precedence",
            "MMHS-Exempted-Address",
            "MMHS-Extended-Authorisation-Info",
            "MMHS-Handling-Instructions",
            "MMHS-Message-Instructions",
            "MMHS-Message-Type",
            "MMHS-Originator-PLAD",
            "MMHS-Originator-Reference",
            "MMHS-Other-Recipients-Indicator-CC",
            "MMHS-Other-Recipients-Indicator-To",
            "MMHS-Primary-Precedence",
            "MMHS-Subject-Indicator-Codes",
            "MT-Priority",
            "Negotiate",
            "Newsgroups",
            "NNTP-Posting-Date",
            "NNTP-Posting-Host",
            "Non-Compliance",
            "Obsoletes",
            "Opt",
            "Optional",
            "Optional-WWW-Authenticate",
            "Ordering-Type",
            "Organization",
            "Origin",
            "Original-Encoded-Information-Types",
            "Original-From",
            "Original-Message-ID",
            "Original-Recipient",
            "Original-Sender",
            "Original-Subject",
            "Originator-Return-Address",
            "Overwrite",
            "P3P",
            "Path",
            "PEP",
            "Pep-Info",
            "PICS-Label",
            "Position",
            "Posting-Version",
            "Pragma",
            "Prefer",
            "Preference-Applied",
            "Prevent-NonDelivery-Report",
            "Priority",
            "Privicon",
            "ProfileObject",
            "Protocol",
            "Protocol-Info",
            "Protocol-Query",
            "Protocol-Request",
            "Proxy-Authenticate",
            "Proxy-Authentication-Info",
            "Proxy-Authorization",
            "Proxy-Connection",
            "Proxy-Features",
            "Proxy-Instruction",
            "Public",
            "Public-Key-Pins",
            "Public-Key-Pins-Report-Only",
            "Range",
            "Received",
            "Received-SPF",
            "Redirect-Ref",
            "References",
            "Referer",
            "Referer-Root",
            "Relay-Version",
            "Reply-By",
            "Reply-To",
            "Require-Recipient-Valid-Since",
            "Resent-Bcc",
            "Resent-Cc",
            "Resent-Date",
            "Resent-From",
            "Resent-Message-ID",
            "Resent-Reply-To",
            "Resent-Sender",
            "Resent-To",
            "Resolution-Hint",
            "Resolver-Location",
            "Retry-After",
            "Return-Path",
            "Safe",
            "Schedule-Reply",
            "Schedule-Tag",
            "Sec-Fetch-Dest",
            "Sec-Fetch-Mode",
            "Sec-Fetch-Site",
            "Sec-Fetch-User",
            "Sec-WebSocket-Accept",
            "Sec-WebSocket-Extensions",
            "Sec-WebSocket-Key",
            "Sec-WebSocket-Protocol",
            "Sec-WebSocket-Version",
            "Security-Scheme",
            "See-Also",
            "Sender",
            "Sensitivity",
            "Server",
            "Set-Cookie",
            "Set-Cookie2",
            "SetProfile",
            "SIO-Label",
            "SIO-Label-History",
            "SLUG",
            "SoapAction",
            "Solicitation",
            "Status-URI",
            "Strict-Transport-Security",
            "Subject",
            "SubOK",
            "Subst",
            "Summary",
            "Supersedes",
            "Surrogate-Capability",
            "Surrogate-Control",
            "TCN",
            "TE",
            "Timeout",
            "Title",
            "To",
            "Topic",
            "Trailer",
            "Transfer-Encoding",
            "TTL",
            "UA-Color",
            "UA-Media",
            "UA-Pixels",
            "UA-Resolution",
            "UA-Windowpixels",
            "Upgrade",
            "Urgency",
            "URI",
            "User-Agent",
            "Variant-Vary",
            "Vary",
            "VBR-Info",
            "Version",
            "Via",
            "Want-Digest",
            "Warning",
            "WWW-Authenticate",
            "X-Archived-At",
            "X-Device-Accept",
            "X-Device-Accept-Charset",
            "X-Device-Accept-Encoding",
            "X-Device-Accept-Language",
            "X-Device-User-Agent",
            "X-Frame-Options",
            "X-Mittente",
            "X-PGP-Sig",
            "X-Ricevuta",
            "X-Riferimento-Message-ID",
            "X-TipoRicevuta",
            "X-Trasporto",
            "X-VerificaSicurezza",
            "X400-Content-Identifier",
            "X400-Content-Return",
            "X400-Content-Type",
            "X400-MTS-Identifier",
            "X400-Originator",
            "X400-Received",
            "X400-Recipients",
            "X400-Trace",
            "Xref"
        }})
    {
        for(std::size_t i = 1, n = 256; i < n; ++i)
        {
            auto sv = by_name_[ i ];
            auto h = digest(sv);
            auto j = h % N;
            BOOST_ASSERT(map_[j][0] == 0);
            map_[j][0] = static_cast<unsigned char>(i);
        }

        for(std::size_t i = 256, n = by_name_.size(); i < n; ++i)
        {
            auto sv = by_name_[i];
            auto h = digest(sv);
            auto j = h % N;
            BOOST_ASSERT(map_[j][1] == 0);
            map_[j][1] = static_cast<unsigned char>(i - 255);
        }
    }

    field
    string_to_field(string_view s) const
    {
        auto h = digest(s);
        auto j = h % N;
        int i = map_[j][0];
        string_view s2 = by_name_[i];
        if(i != 0 && equals(s, s2))
            return static_cast<field>(i);
        i = map_[j][1];
        if(i == 0)
            return field::unknown;
        i += 255;
        s2 = by_name_[i];

        if(equals(s, s2))
            return static_cast<field>(i);
        return field::unknown;
    }

    //
    // Deprecated
    //

    using const_iterator =
    array_type::const_iterator; 

    std::size_t
    size() const
    {
        return by_name_.size();
    }

    const_iterator
    begin() const
    {
        return by_name_.begin();
    }

    const_iterator
    end() const
    {
        return by_name_.end();
    }
};

BOOST_BEAST_DECL
field_table const&
get_field_table()
{
    static field_table const tab;
    return tab;
}

BOOST_BEAST_DECL
string_view
to_string(field f)
{
    auto const& v = get_field_table();
    BOOST_ASSERT(static_cast<unsigned>(f) < v.size());
    return v.begin()[static_cast<unsigned>(f)];
}

} // detail

string_view
to_string(field f)
{
    return detail::to_string(f);
}

field
string_to_field(string_view s)
{
    return detail::get_field_table().string_to_field(s);
}

std::ostream&
operator<<(std::ostream& os, field f)
{
    return os << to_string(f);
}

} // http
} // beast
} // boost

#endif
