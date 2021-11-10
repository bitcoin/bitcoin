/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_binary_oprimitive.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream>
#include <cstddef> // NULL
#include <cstring>

#include <boost/config.hpp>

#if defined(BOOST_NO_STDC_NAMESPACE) && ! defined(__LIBCOMO__)
namespace std{ 
    using ::strlen; 
} // namespace std
#endif

#ifndef BOOST_NO_CWCHAR
#include <cwchar>
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::wcslen; }
#endif
#endif

#include <boost/archive/basic_binary_oprimitive.hpp>
#include <boost/core/no_exceptions_support.hpp>

namespace boost {
namespace archive {

//////////////////////////////////////////////////////////////////////
// implementation of basic_binary_oprimitive

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_oprimitive<Archive, Elem, Tr>::init()
{
    // record native sizes of fundamental types
    // this is to permit detection of attempts to pass
    // native binary archives accross incompatible machines.
    // This is not foolproof but its better than nothing.
    this->This()->save(static_cast<unsigned char>(sizeof(int)));
    this->This()->save(static_cast<unsigned char>(sizeof(long)));
    this->This()->save(static_cast<unsigned char>(sizeof(float)));
    this->This()->save(static_cast<unsigned char>(sizeof(double)));
    // for checking endianness
    this->This()->save(int(1));
}

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_oprimitive<Archive, Elem, Tr>::save(const char * s)
{
    std::size_t l = std::strlen(s);
    this->This()->save(l);
    save_binary(s, l);
}

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_oprimitive<Archive, Elem, Tr>::save(const std::string &s)
{
    std::size_t l = static_cast<std::size_t>(s.size());
    this->This()->save(l);
    save_binary(s.data(), l);
}

#ifndef BOOST_NO_CWCHAR
#ifndef BOOST_NO_INTRINSIC_WCHAR_T
template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_oprimitive<Archive, Elem, Tr>::save(const wchar_t * ws)
{
    std::size_t l = std::wcslen(ws);
    this->This()->save(l);
    save_binary(ws, l * sizeof(wchar_t) / sizeof(char));
}
#endif

#ifndef BOOST_NO_STD_WSTRING
template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL void
basic_binary_oprimitive<Archive, Elem, Tr>::save(const std::wstring &ws)
{
    std::size_t l = ws.size();
    this->This()->save(l);
    save_binary(ws.data(), l * sizeof(wchar_t) / sizeof(char));
}
#endif
#endif // BOOST_NO_CWCHAR

template<class Archive, class Elem, class Tr>
BOOST_ARCHIVE_OR_WARCHIVE_DECL
basic_binary_oprimitive<Archive, Elem, Tr>::basic_binary_oprimitive(
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
basic_binary_oprimitive<Archive, Elem, Tr>::~basic_binary_oprimitive(){}

} // namespace archive
} // namespace boost
