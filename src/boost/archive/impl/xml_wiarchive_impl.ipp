/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// xml_wiarchive_impl.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstring>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::memcpy; 
} //std
#endif

#include <boost/config.hpp> // msvc 6.0 needs this to suppress warnings
#ifndef BOOST_NO_STD_WSTREAMBUF

#include <boost/assert.hpp>
#include <algorithm> // std::copy
#include <boost/detail/workaround.hpp> // Dinkumware and RogueWave
#if BOOST_WORKAROUND(BOOST_DINKUMWARE_STDLIB, == 1)
#include <boost/archive/dinkumware.hpp>
#endif

#include <boost/io/ios_state.hpp>
#include <boost/core/uncaught_exceptions.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/serialization/string.hpp>

#include <boost/archive/basic_xml_archive.hpp>
#include <boost/archive/xml_wiarchive.hpp>

#include <boost/archive/xml_archive_exception.hpp>
#include <boost/archive/iterators/mb_from_wchar.hpp>

#include <boost/archive/detail/utf8_codecvt_facet.hpp>

#include "basic_xml_grammar.hpp"

namespace boost {
namespace archive {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// implemenations of functions specific to wide char archives

namespace { // anonymous

void copy_to_ptr(char * s, const std::wstring & ws){
    std::copy(
        iterators::mb_from_wchar<std::wstring::const_iterator>(
            ws.begin()
        ), 
        iterators::mb_from_wchar<std::wstring::const_iterator>(
            ws.end()
        ), 
        s
    );
    s[ws.size()] = 0;
}

} // anonymous

template<class Archive>
BOOST_WARCHIVE_DECL void
xml_wiarchive_impl<Archive>::load(std::string & s){
    std::wstring ws;
    bool result = gimpl->parse_string(is, ws);
    if(! result)
        boost::serialization::throw_exception(
            xml_archive_exception(xml_archive_exception::xml_archive_parsing_error)
        );
    #if BOOST_WORKAROUND(_RWSTD_VER, BOOST_TESTED_AT(20101))
    if(NULL != s.data())
    #endif
        s.resize(0);
    s.reserve(ws.size());
    std::copy(
        iterators::mb_from_wchar<std::wstring::iterator>(
            ws.begin()
        ), 
        iterators::mb_from_wchar<std::wstring::iterator>(
            ws.end()
        ), 
        std::back_inserter(s)
    );
}

#ifndef BOOST_NO_STD_WSTRING
template<class Archive>
BOOST_WARCHIVE_DECL void
xml_wiarchive_impl<Archive>::load(std::wstring & ws){
    bool result = gimpl->parse_string(is, ws);
    if(! result)
        boost::serialization::throw_exception(
            xml_archive_exception(xml_archive_exception::xml_archive_parsing_error)
        );
}
#endif

template<class Archive>
BOOST_WARCHIVE_DECL void
xml_wiarchive_impl<Archive>::load(char * s){
    std::wstring ws;
    bool result = gimpl->parse_string(is, ws);
    if(! result)
        boost::serialization::throw_exception(
            xml_archive_exception(xml_archive_exception::xml_archive_parsing_error)
        );
    copy_to_ptr(s, ws);
}

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
template<class Archive>
BOOST_WARCHIVE_DECL void
xml_wiarchive_impl<Archive>::load(wchar_t * ws){
    std::wstring twstring;
    bool result = gimpl->parse_string(is, twstring);
    if(! result)
        boost::serialization::throw_exception(
            xml_archive_exception(xml_archive_exception::xml_archive_parsing_error)
        );
    std::memcpy(ws, twstring.c_str(), twstring.size());
    ws[twstring.size()] = L'\0';
}
#endif

template<class Archive>
BOOST_WARCHIVE_DECL void
xml_wiarchive_impl<Archive>::load_override(class_name_type & t){
    const std::wstring & ws = gimpl->rv.class_name;
    if(ws.size() > BOOST_SERIALIZATION_MAX_KEY_SIZE - 1)
        boost::serialization::throw_exception(
            archive_exception(archive_exception::invalid_class_name)
        );
    copy_to_ptr(t, ws);
}

template<class Archive>
BOOST_WARCHIVE_DECL void
xml_wiarchive_impl<Archive>::init(){
    gimpl->init(is);
    this->set_library_version(
        boost::serialization::library_version_type(gimpl->rv.version)
    );
}

template<class Archive>
BOOST_WARCHIVE_DECL
xml_wiarchive_impl<Archive>::xml_wiarchive_impl(
    std::wistream &is_,
    unsigned int flags
) :
    basic_text_iprimitive<std::wistream>(
        is_, 
        true // don't change the codecvt - use the one below
    ),
    basic_xml_iarchive<Archive>(flags),
    gimpl(new xml_wgrammar())
{
    if(0 == (flags & no_codecvt)){
        archive_locale = std::locale(
            is_.getloc(),
            new boost::archive::detail::utf8_codecvt_facet
        );
        // libstdc++ crashes without this
        is_.sync();
        is_.imbue(archive_locale);
    }
}

template<class Archive>
BOOST_WARCHIVE_DECL
xml_wiarchive_impl<Archive>::~xml_wiarchive_impl(){
    if(boost::core::uncaught_exceptions() > 0)
        return;
    if(0 == (this->get_flags() & no_header)){
        gimpl->windup(is);
    }
}

} // namespace archive
} // namespace boost

#endif  // BOOST_NO_STD_WSTREAMBUF
