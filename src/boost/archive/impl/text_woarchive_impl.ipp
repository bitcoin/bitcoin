/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// text_woarchive_impl.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>
#ifndef BOOST_NO_STD_WSTREAMBUF

#include <cstring>
#include <cstddef> // size_t
#if defined(BOOST_NO_STDC_NAMESPACE) && ! defined(__LIBCOMO__)
namespace std{ 
    using ::strlen;
    using ::size_t; 
} // namespace std
#endif

#include <ostream>

#include <boost/archive/text_woarchive.hpp>

namespace boost {
namespace archive {

//////////////////////////////////////////////////////////////////////
// implementation of woarchive functions
//
template<class Archive>
BOOST_WARCHIVE_DECL void
text_woarchive_impl<Archive>::save(const char *s)
{
    // note: superfluous local variable fixes borland warning
    const std::size_t size = std::strlen(s);
    * this->This() << size;
    this->This()->newtoken();
    while(*s != '\0')
        os.put(os.widen(*s++));
}

template<class Archive>
BOOST_WARCHIVE_DECL void
text_woarchive_impl<Archive>::save(const std::string &s)
{
    const std::size_t size = s.size();
    * this->This() << size;
    this->This()->newtoken();
    const char * cptr = s.data();
    for(std::size_t i = size; i-- > 0;)
        os.put(os.widen(*cptr++));
}

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
template<class Archive>
BOOST_WARCHIVE_DECL void
text_woarchive_impl<Archive>::save(const wchar_t *ws)
{
    const std::size_t size = std::wostream::traits_type::length(ws);
    * this->This() << size;
    this->This()->newtoken();
    os.write(ws, size);
}
#endif

#ifndef BOOST_NO_STD_WSTRING
template<class Archive>
BOOST_WARCHIVE_DECL void
text_woarchive_impl<Archive>::save(const std::wstring &ws)
{
    const std::size_t size = ws.length();
    * this->This() << size;
    this->This()->newtoken();
    os.write(ws.data(), size);
}
#endif

} // namespace archive
} // namespace boost

#endif

