/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// xml_oarchive_impl.ipp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <ostream>
#include <iomanip>
#include <algorithm> // std::copy
#include <string>

#include <cstring> // strlen
#include <boost/config.hpp> // msvc 6.0 needs this to suppress warnings
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{ 
    using ::strlen; 
} // namespace std
#endif

#include <boost/core/uncaught_exceptions.hpp>
#include <boost/archive/iterators/xml_escape.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>

#ifndef BOOST_NO_CWCHAR
#include <boost/archive/wcslen.hpp>
#include <boost/archive/iterators/mb_from_wchar.hpp>
#endif

namespace boost {
namespace archive {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// implemenations of functions specific to char archives

// wide char stuff used by char archives
#ifndef BOOST_NO_CWCHAR
// copy chars to output escaping to xml and translating wide chars to mb chars
template<class InputIterator>
void save_iterator(std::ostream &os, InputIterator begin, InputIterator end){
    typedef boost::archive::iterators::mb_from_wchar<
        boost::archive::iterators::xml_escape<InputIterator>
    > translator;
    std::copy(
        translator(begin),
        translator(end),
        boost::archive::iterators::ostream_iterator<char>(os)
    );
}

#ifndef BOOST_NO_STD_WSTRING
template<class Archive>
BOOST_ARCHIVE_DECL void
xml_oarchive_impl<Archive>::save(const std::wstring & ws){
//  at least one library doesn't typedef value_type for strings
//  so rather than using string directly make a pointer iterator out of it
//    save_iterator(os, ws.data(), ws.data() + std::wcslen(ws.data()));
    save_iterator(os, ws.data(), ws.data() + ws.size());
}
#endif

#ifndef BOOST_NO_INTRINSIC_WCHAR_T
template<class Archive>
BOOST_ARCHIVE_DECL void
xml_oarchive_impl<Archive>::save(const wchar_t * ws){
    save_iterator(os, ws, ws + std::wcslen(ws));
}
#endif

#endif // BOOST_NO_CWCHAR

template<class Archive>
BOOST_ARCHIVE_DECL void
xml_oarchive_impl<Archive>::save(const std::string & s){
//  at least one library doesn't typedef value_type for strings
//  so rather than using string directly make a pointer iterator out of it
    typedef boost::archive::iterators::xml_escape<
        const char * 
    > xml_escape_translator;
    std::copy(
        xml_escape_translator(s.data()),
        xml_escape_translator(s.data()+ s.size()),
        boost::archive::iterators::ostream_iterator<char>(os)
    );
}

template<class Archive>
BOOST_ARCHIVE_DECL void
xml_oarchive_impl<Archive>::save(const char * s){
    typedef boost::archive::iterators::xml_escape<
        const char * 
    > xml_escape_translator;
    std::copy(
        xml_escape_translator(s),
        xml_escape_translator(s + std::strlen(s)),
        boost::archive::iterators::ostream_iterator<char>(os)
    );
}

template<class Archive>
BOOST_ARCHIVE_DECL
xml_oarchive_impl<Archive>::xml_oarchive_impl(
    std::ostream & os_, 
    unsigned int flags
) : 
    basic_text_oprimitive<std::ostream>(
        os_,
        0 != (flags & no_codecvt)
    ),
    basic_xml_oarchive<Archive>(flags)
{}

template<class Archive>
BOOST_ARCHIVE_DECL void
xml_oarchive_impl<Archive>::save_binary(const void *address, std::size_t count){
    this->end_preamble();
    #if ! defined(__MWERKS__)
    this->basic_text_oprimitive<std::ostream>::save_binary(
    #else
    this->basic_text_oprimitive::save_binary(
    #endif
        address, 
        count
    );
    this->indent_next = true;
}

template<class Archive>
BOOST_ARCHIVE_DECL
xml_oarchive_impl<Archive>::~xml_oarchive_impl(){
    if(boost::core::uncaught_exceptions() > 0)
        return;
    if(0 == (this->get_flags() & no_header))
        this->windup();
}

} // namespace archive
} // namespace boost
