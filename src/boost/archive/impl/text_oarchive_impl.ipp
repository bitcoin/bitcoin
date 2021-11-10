/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// text_oarchive_impl.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <string>
#include <boost/config.hpp>
#include <cstddef> // size_t

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::size_t; 
} // namespace std
#endif

#ifndef BOOST_NO_CWCHAR
#include <cwchar>
#ifdef BOOST_NO_STDC_NAMESPACE
namespace std{ using ::wcslen; }
#endif
#endif

#include <boost/archive/text_oarchive.hpp>

namespace boost { 
namespace archive {

//////////////////////////////////////////////////////////////////////
// implementation of basic_text_oprimitive overrides for the combination
// of template parameters used to create a text_oprimitive

template<class Archive>
BOOST_ARCHIVE_DECL void
text_oarchive_impl<Archive>::save(const char * s)
{
    const std::size_t len = std::ostream::traits_type::length(s);
    *this->This() << len;
    this->This()->newtoken();
    os << s;
}

template<class Archive>
BOOST_ARCHIVE_DECL void
text_oarchive_impl<Archive>::save(const std::string &s)
{
    const std::size_t size = s.size();
    *this->This() << size;
    this->This()->newtoken();
    os << s;
}

#ifndef BOOST_NO_CWCHAR
#ifndef BOOST_NO_INTRINSIC_WCHAR_T
template<class Archive>
BOOST_ARCHIVE_DECL void
text_oarchive_impl<Archive>::save(const wchar_t * ws)
{
    const std::size_t l = std::wcslen(ws);
    * this->This() << l;
    this->This()->newtoken();
    os.write((const char *)ws, l * sizeof(wchar_t)/sizeof(char));
}
#endif

#ifndef BOOST_NO_STD_WSTRING
template<class Archive>
BOOST_ARCHIVE_DECL void
text_oarchive_impl<Archive>::save(const std::wstring &ws)
{
    const std::size_t l = ws.size();
    * this->This() << l;
    this->This()->newtoken();
    os.write((const char *)(ws.data()), l * sizeof(wchar_t)/sizeof(char));
}
#endif
#endif // BOOST_NO_CWCHAR

template<class Archive>
BOOST_ARCHIVE_DECL 
text_oarchive_impl<Archive>::text_oarchive_impl(
    std::ostream & os, 
    unsigned int flags
) :
    basic_text_oprimitive<std::ostream>(
        os, 
        0 != (flags & no_codecvt)
    ),
    basic_text_oarchive<Archive>(flags)
{
}

template<class Archive>
BOOST_ARCHIVE_DECL void
text_oarchive_impl<Archive>::save_binary(const void *address, std::size_t count){
    put('\n');
    this->end_preamble();
    #if ! defined(__MWERKS__)
    this->basic_text_oprimitive<std::ostream>::save_binary(
    #else
    this->basic_text_oprimitive::save_binary(
    #endif
        address, 
        count
    );
    this->delimiter = this->eol;
}

} // namespace archive
} // namespace boost

