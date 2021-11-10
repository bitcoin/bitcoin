#ifndef BOOST_ARCHIVE_CODECVT_NULL_HPP
#define BOOST_ARCHIVE_CODECVT_NULL_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// codecvt_null.hpp:

// (C) Copyright 2004 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <locale>
#include <cstddef> // NULL, size_t
#ifndef BOOST_NO_CWCHAR
#include <cwchar>   // for mbstate_t
#endif
#include <boost/config.hpp>
#include <boost/serialization/force_include.hpp>
#include <boost/archive/detail/auto_link_archive.hpp>
//#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std {
// For STLport on WinCE, BOOST_NO_STDC_NAMESPACE can get defined if STLport is putting symbols in its own namespace.
// In the case of codecvt, however, this does not mean that codecvt is in the global namespace (it will be in STLport's namespace)
#  if !defined(__SGI_STL_PORT) && !defined(_STLPORT_VERSION)
    using ::codecvt;
#  endif
    using ::mbstate_t;
    using ::size_t;
} // namespace
#endif

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace boost {
namespace archive {

template<class Ch>
class codecvt_null;

template<>
class codecvt_null<char> : public std::codecvt<char, char, std::mbstate_t>
{
    bool do_always_noconv() const throw() BOOST_OVERRIDE {
        return true;
    }
public:
    explicit codecvt_null(std::size_t no_locale_manage = 0) :
        std::codecvt<char, char, std::mbstate_t>(no_locale_manage)
    {}
    ~codecvt_null() BOOST_OVERRIDE {}
};

template<>
class BOOST_SYMBOL_VISIBLE codecvt_null<wchar_t> :
    public std::codecvt<wchar_t, char, std::mbstate_t>
{
    BOOST_SYMBOL_EXPORT std::codecvt_base::result
    do_out(
        std::mbstate_t & state,
        const wchar_t * first1,
        const wchar_t * last1,
        const wchar_t * & next1,
        char * first2,
        char * last2,
        char * & next2
    ) const BOOST_OVERRIDE;

    BOOST_SYMBOL_EXPORT std::codecvt_base::result
    do_in(
        std::mbstate_t & state,
        const char * first1,
        const char * last1,
        const char * & next1,
        wchar_t * first2,
        wchar_t * last2,
        wchar_t * & next2
    ) const BOOST_OVERRIDE;

    BOOST_SYMBOL_EXPORT int do_encoding( ) const throw( ) BOOST_OVERRIDE {
        return sizeof(wchar_t) / sizeof(char);
    }

    BOOST_SYMBOL_EXPORT bool do_always_noconv() const throw() BOOST_OVERRIDE {
        return false;
    }

    BOOST_SYMBOL_EXPORT int do_max_length( ) const throw( ) BOOST_OVERRIDE {
        return do_encoding();
    }
public:
    BOOST_SYMBOL_EXPORT explicit codecvt_null(std::size_t no_locale_manage = 0);

    BOOST_SYMBOL_EXPORT ~codecvt_null() BOOST_OVERRIDE ;
};

} // namespace archive
} // namespace boost

#ifdef BOOST_MSVC
#  pragma warning(pop)
#endif
//#include <boost/archive/detail/abi_suffix.hpp> // pop pragmas

#endif //BOOST_ARCHIVE_CODECVT_NULL_HPP
