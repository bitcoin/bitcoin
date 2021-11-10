// Boost uuid_io.hpp header file  ----------------------------------------------//

// Copyright 2009 Andy Tompkins.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// Revision History
//  20 Mar 2009 - Initial Revision
//  28 Nov 2009 - disabled deprecated warnings for MSVC

#ifndef BOOST_UUID_IO_HPP
#define BOOST_UUID_IO_HPP

#include <boost/uuid/uuid.hpp>
#include <ios>
#include <ostream>
#include <istream>
#include <boost/io/ios_state.hpp>
#include <locale>
#include <algorithm>

#if defined(_MSC_VER)
#pragma warning(push) // Save warning settings.
#pragma warning(disable : 4996) // Disable deprecated std::ctype<char>::widen, std::copy
#endif

namespace boost {
namespace uuids {

template <typename ch, typename char_traits>
    std::basic_ostream<ch, char_traits>& operator<<(std::basic_ostream<ch, char_traits> &os, uuid const& u)
{
    io::ios_flags_saver flags_saver(os);
    io::basic_ios_fill_saver<ch, char_traits> fill_saver(os);

    const typename std::basic_ostream<ch, char_traits>::sentry ok(os);
    if (ok) {
        const std::streamsize width = os.width(0);
        const std::streamsize uuid_width = 36;
        const std::ios_base::fmtflags flags = os.flags();
        const typename std::basic_ios<ch, char_traits>::char_type fill = os.fill();
        if (flags & (std::ios_base::right | std::ios_base::internal)) {
            for (std::streamsize i=uuid_width; i<width; i++) {
                os << fill;
            }
        }

        os << std::hex << std::right;
        os.fill(os.widen('0'));

        std::size_t i=0;
        for (uuid::const_iterator i_data = u.begin(); i_data!=u.end(); ++i_data, ++i) {
            os.width(2);
            os << static_cast<unsigned int>(*i_data);
            if (i == 3 || i == 5 || i == 7 || i == 9) {
                os << os.widen('-');
            }
        }

        if (flags & std::ios_base::left) {
            for (std::streamsize s=uuid_width; s<width; s++) {
                os << fill;
            }
        }

        os.width(0); //used the width so reset it
    }
    return os;
}

template <typename ch, typename char_traits>
    std::basic_istream<ch, char_traits>& operator>>(std::basic_istream<ch, char_traits> &is, uuid &u)
{
    const typename std::basic_istream<ch, char_traits>::sentry ok(is);
    if (ok) {
        unsigned char data[16];

        typedef std::ctype<ch> ctype_t;
        ctype_t const& ctype = std::use_facet<ctype_t>(is.getloc());

        ch xdigits[16];
        {
            char szdigits[] = "0123456789ABCDEF";
            ctype.widen(szdigits, szdigits+16, xdigits);
        }
        ch*const xdigits_end = xdigits+16;

        ch c;
        for (std::size_t i=0; i<u.size() && is; ++i) {
            is >> c;
            c = ctype.toupper(c);

            ch* f = std::find(xdigits, xdigits_end, c);
            if (f == xdigits_end) {
                is.setstate(std::ios_base::failbit);
                break;
            }

            unsigned char byte = static_cast<unsigned char>(std::distance(&xdigits[0], f));

            is >> c;
            c = ctype.toupper(c);
            f = std::find(xdigits, xdigits_end, c);
            if (f == xdigits_end) {
                is.setstate(std::ios_base::failbit);
                break;
            }

            byte <<= 4;
            byte |= static_cast<unsigned char>(std::distance(&xdigits[0], f));

            data[i] = byte;

            if (is) {
                if (i == 3 || i == 5 || i == 7 || i == 9) {
                    is >> c;
                    if (c != is.widen('-')) is.setstate(std::ios_base::failbit);
                }
            }
        }

        if (is) {
            std::copy(data, data+16, u.begin());
        }
    }
    return is;
}

namespace detail {
inline char to_char(size_t i) {
    if (i <= 9) {
        return static_cast<char>('0' + i);
    } else {
        return static_cast<char>('a' + (i-10));
    }
}

inline wchar_t to_wchar(size_t i) {
    if (i <= 9) {
        return static_cast<wchar_t>(L'0' + i);
    } else {
        return static_cast<wchar_t>(L'a' + (i-10));
    }
}

} // namespace detail

template<class OutputIterator>
OutputIterator to_chars(uuid const& u, OutputIterator out)
{
    std::size_t i=0;
    for (uuid::const_iterator it_data = u.begin(); it_data!=u.end(); ++it_data, ++i) {
        const size_t hi = ((*it_data) >> 4) & 0x0F;
        *out++ = detail::to_char(hi);

        const size_t lo = (*it_data) & 0x0F;
        *out++ = detail::to_char(lo);

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            *out++ = '-';
        }
    }
    return out;
}

inline bool to_chars(uuid const& u, char* first, char* last)
{
    if (last - first < 36) {
        return false;
    }
    to_chars(u, first);
    return true;
}

inline std::string to_string(uuid const& u)
{
    std::string result(36, char());
    // string::data() returns const char* before C++17
    to_chars(u, &result[0]);
    return result;
}

#ifndef BOOST_NO_STD_WSTRING
inline std::wstring to_wstring(uuid const& u)
{
    std::wstring result;
    result.reserve(36);

    std::size_t i=0;
    for (uuid::const_iterator it_data = u.begin(); it_data!=u.end(); ++it_data, ++i) {
        const size_t hi = ((*it_data) >> 4) & 0x0F;
        result += detail::to_wchar(hi);

        const size_t lo = (*it_data) & 0x0F;
        result += detail::to_wchar(lo);

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            result += L'-';
        }
    }
    return result;
}

#endif

}} //namespace boost::uuids

#if defined(_MSC_VER)
#pragma warning(pop) // Restore warnings to previous state.
#endif

#endif // BOOST_UUID_IO_HPP
