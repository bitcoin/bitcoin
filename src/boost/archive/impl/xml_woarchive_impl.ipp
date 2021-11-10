/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// xml_woarchive_impl.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>
#ifndef BOOST_NO_STD_WSTREAMBUF

#include <ostream>
#include <string>
#include <algorithm> // std::copy
#include <locale>

#include <cstring> // strlen
#include <cstdlib> // mbtowc
#ifndef BOOST_NO_CWCHAR
#include <cwchar>  // wcslen
#endif

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::strlen; 
    #if ! defined(BOOST_NO_INTRINSIC_WCHAR_T)
        using ::mbtowc; 
        using ::wcslen;
    #endif
} // namespace std
#endif

#include <boost/core/uncaught_exceptions.hpp>

#include <boost/archive/xml_woarchive.hpp>
#include <boost/archive/detail/utf8_codecvt_facet.hpp>

#include <boost/serialization/throw_exception.hpp>

#include <boost/archive/iterators/xml_escape.hpp>
#include <boost/archive/iterators/wchar_from_mb.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/dataflow_exception.hpp>

namespace boost {
namespace archive {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// implemenations of functions specific to wide char archives

// copy chars to output escaping to xml and widening characters as we go
template<class InputIterator>
void save_iterator(std::wostream &os, InputIterator begin, InputIterator end){
    typedef iterators::wchar_from_mb<
        iterators::xml_escape<InputIterator>
    > xmbtows;
    std::copy(
        xmbtows(begin),
        xmbtows(end),
        boost::archive::iterators::ostream_iterator<wchar_t>(os)
    );
}

template<class Archive>
BOOST_WARCHIVE_DECL void
xml_woarchive_impl<Archive>::save(const std::string & s){
    // note: we don't use s.begin() and s.end() because dinkumware
    // doesn't have string::value_type defined. So use a wrapper
    // around these values to implement the definitions.
    const char * begin = s.data();
    const char * end = begin + s.size();
    save_iterator(os, begin, end);
}

#ifndef BOOST_NO_STD_WSTRING
template<class Archive>
BOOST_WARCHIVE_DECL void
xml_woarchive_impl<Archive>::save(const std::wstring & ws){
#if 0
    typedef iterators::xml_escape<std::wstring::const_iterator> xmbtows;
    std::copy(
        xmbtows(ws.begin()),
        xmbtows(ws.end()),
        boost::archive::iterators::ostream_iterator<wchar_t>(os)
    );
#endif
    typedef iterators::xml_escape<const wchar_t *> xmbtows;
    std::copy(
        xmbtows(ws.data()),
        xmbtows(ws.data() + ws.size()),
        boost::archive::iterators::ostream_iterator<wchar_t>(os)
    );
}
#endif //BOOST_NO_STD_WSTRING

template<class Archive>
BOOST_WARCHIVE_DECL void
xml_woarchive_impl<Archive>::save(const char * s){
   save_iterator(os, s, s + std::strlen(s));
}

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
template<class Archive>
BOOST_WARCHIVE_DECL void
xml_woarchive_impl<Archive>::save(const wchar_t * ws){
    typedef iterators::xml_escape<const wchar_t *> xmbtows;
    std::copy(
        xmbtows(ws),
        xmbtows(ws + std::wcslen(ws)),
        boost::archive::iterators::ostream_iterator<wchar_t>(os)
    );
}
#endif

template<class Archive>
BOOST_WARCHIVE_DECL
xml_woarchive_impl<Archive>::xml_woarchive_impl(
    std::wostream & os_,
    unsigned int flags
) :
    basic_text_oprimitive<std::wostream>(
        os_,
        true // don't change the codecvt - use the one below
    ),
    basic_xml_oarchive<Archive>(flags)
{
    if(0 == (flags & no_codecvt)){
        archive_locale = std::locale(
            os_.getloc(),
            new boost::archive::detail::utf8_codecvt_facet
        );
        os_.flush();
        os_.imbue(archive_locale);
    }
}

template<class Archive>
BOOST_WARCHIVE_DECL
xml_woarchive_impl<Archive>::~xml_woarchive_impl(){
    if(boost::core::uncaught_exceptions() > 0)
        return;
    if(0 == (this->get_flags() & no_header)){
        os << L"</boost_serialization>";
    }
}

template<class Archive>
BOOST_WARCHIVE_DECL void
xml_woarchive_impl<Archive>::save_binary(
    const void *address,
    std::size_t count
){
    this->end_preamble();
    #if ! defined(__MWERKS__)
    this->basic_text_oprimitive<std::wostream>::save_binary(
    #else
    this->basic_text_oprimitive::save_binary(
    #endif
        address, 
        count
    );
    this->indent_next = true;
}

} // namespace archive
} // namespace boost

#endif //BOOST_NO_STD_WSTREAMBUF
