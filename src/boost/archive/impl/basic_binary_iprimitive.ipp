/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_binary_iprimitive.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/assert.hpp>
#include <cstddef> // size_t, NULL
#include <cstring> // memcpy

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::size_t;
    using ::memcpy;
} // namespace std
#endif

#include <boost/serialization/throw_exception.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/archive/basic_binary_iprimitive.hpp> 

namespace boost {
namespace archive {

//////////////////////////////////////////////////////////////////////
// implementation of basic_binary_iprimitive

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_iprimitive<Archive, Elem, Tr>::init()
{
    // Detect  attempts to pass native binary archives across
    // incompatible platforms. This is not fool proof but its
    // better than nothing.
    unsigned char size;
    this->This()->load(size);
    if(sizeof(int) != size)
        boost::serialization::throw_exception(
            archive_exception(
                archive_exception::incompatible_native_format,
                "size of int"
            )
        );
    this->This()->load(size);
    if(sizeof(long) != size)
        boost::serialization::throw_exception(
            archive_exception(
                archive_exception::incompatible_native_format,
                "size of long"
            )
        );
    this->This()->load(size);
    if(sizeof(float) != size)
        boost::serialization::throw_exception(
            archive_exception(
                archive_exception::incompatible_native_format,
                "size of float"
            )
        );
    this->This()->load(size);
    if(sizeof(double) != size)
        boost::serialization::throw_exception(
            archive_exception(
                archive_exception::incompatible_native_format,
                "size of double"
            )
        );

    // for checking endian
    int i;
    this->This()->load(i);
    if(1 != i)
        boost::serialization::throw_exception(
            archive_exception(
                archive_exception::incompatible_native_format,
                "endian setting"
            )
        );
}

#ifndef BOOST_NO_CWCHAR
#ifndef BOOST_NO_INTRINSIC_WCHAR_T
template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_iprimitive<Archive, Elem, Tr>::load(wchar_t * ws)
{
    std::size_t l; // number of wchar_t !!!
    this->This()->load(l);
    load_binary(ws, l * sizeof(wchar_t) / sizeof(char));
    ws[l] = L'\0';
}
#endif
#endif

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_iprimitive<Archive, Elem, Tr>::load(std::string & s)
{
    std::size_t l;
    this->This()->load(l);
    // borland de-allocator fixup
    #if BOOST_WORKAROUND(_RWSTD_VER, BOOST_TESTED_AT(20101))
    if(NULL != s.data())
    #endif
        s.resize(l);
    // note breaking a rule here - could be a problem on some platform
    if(0 < l)
        load_binary(&(*s.begin()), l);
}

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_iprimitive<Archive, Elem, Tr>::load(char * s)
{
    std::size_t l;
    this->This()->load(l);
    load_binary(s, l);
    s[l] = '\0';
}

#ifndef BOOST_NO_STD_WSTRING
template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_iprimitive<Archive, Elem, Tr>::load(std::wstring & ws)
{
    std::size_t l;
    this->This()->load(l);
    // borland de-allocator fixup
    #if BOOST_WORKAROUND(_RWSTD_VER, BOOST_TESTED_AT(20101))
    if(NULL != ws.data())
    #endif
        ws.resize(l);
    // note breaking a rule here - is could be a problem on some platform
    load_binary(const_cast<wchar_t *>(ws.data()), l * sizeof(wchar_t) / sizeof(char));
}
#endif

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL
basic_binary_iprimitive<Archive, Elem, Tr>::basic_binary_iprimitive(
    std::basic_streambuf<Elem, Tr> & sb, 
    bool no_codecvt
) :
#ifndef BOOST_NO_STD_LOCALE
    m_sb(sb),
    codecvt_null_facet(1),
    locale_saver(m_sb),
    archive_locale(sb.getloc(), & codecvt_null_facet)
{
    if(! no_codecvt){
        m_sb.pubsync();
        m_sb.pubimbue(archive_locale);
    }
}
#else
    m_sb(sb)
{}
#endif

// scoped_ptr requires that g be a complete type at time of
// destruction so define destructor here rather than in the header
template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL
basic_binary_iprimitive<Archive, Elem, Tr>::~basic_binary_iprimitive(){}

} // namespace archive
} // namespace boost
