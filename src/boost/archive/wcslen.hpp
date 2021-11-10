#ifndef BOOST_ARCHIVE_WCSLEN_HPP
#define BOOST_ARCHIVE_WCSLEN_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// wcslen.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // size_t
#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

#ifndef BOOST_NO_CWCHAR

// a couple of libraries which include wchar_t don't include
// wcslen

#if defined(BOOST_DINKUMWARE_STDLIB) && BOOST_DINKUMWARE_STDLIB < 306 \
|| defined(__LIBCOMO__)

namespace std {
inline std::size_t wcslen(const wchar_t * ws)
{
    const wchar_t * eows = ws;
    while(* eows != 0)
        ++eows;
    return eows - ws;
}
} // namespace std

#else

#ifndef BOOST_NO_CWCHAR
#include <cwchar>
#endif
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::wcslen; }
#endif

#endif // wcslen

#endif //BOOST_NO_CWCHAR

#endif //BOOST_ARCHIVE_WCSLEN_HPP
