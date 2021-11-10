#ifndef BOOST_ARCHIVE_BASIC_XML_GRAMMAR_HPP
#define BOOST_ARCHIVE_BASIC_XML_GRAMMAR_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_xml_grammar.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// this module is derived from simplexml.cpp - an example shipped as part of
// the spirit parser.  This example contains the following notice:
/*=============================================================================
    simplexml.cpp

    Spirit V1.3
    URL: http://spirit.sourceforge.net/

    Copyright (c) 2001, Daniel C. Nuffer

    This software is provided 'as-is', without any express or implied
    warranty. In no event will the copyright holder be held liable for
    any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute
    it freely, subject to the following restrictions:

    1.  The origin of this software must not be misrepresented; you must
        not claim that you wrote the original software. If you use this
        software in a product, an acknowledgment in the product documentation
        would be appreciated but is not required.

    2.  Altered source versions must be plainly marked as such, and must
        not be misrepresented as being the original software.

    3.  This notice may not be removed or altered from any source
        distribution.
=============================================================================*/
#include <string>

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/spirit/include/classic_rule.hpp>
#include <boost/spirit/include/classic_chset.hpp>

#include <boost/archive/basic_archive.hpp>
#include <boost/serialization/tracking.hpp>
#include <boost/serialization/version.hpp>

namespace boost {
namespace archive {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// XML grammar parsing

template<class CharType>
class BOOST_SYMBOL_VISIBLE basic_xml_grammar {
public:
    // The following is not necessary according to DR45, but at least
    // one compiler (Compaq C++ 6.5 in strict_ansi mode) chokes otherwise.
    struct return_values;
    friend struct return_values;

private:
    typedef typename std::basic_istream<CharType> IStream;
    typedef typename std::basic_string<CharType> StringType;
    typedef typename boost::spirit::classic::chset<CharType> chset_t;
    typedef typename boost::spirit::classic::chlit<CharType> chlit_t;
    typedef typename boost::spirit::classic::scanner<
        typename  std::basic_string<CharType>::iterator
    > scanner_t;
    typedef typename boost::spirit::classic::rule<scanner_t> rule_t;
    // Start grammar definition
    rule_t
        Reference,
        Eq,
        STag,
        ETag,
        LetterOrUnderscoreOrColon,
        AttValue,
        CharRef1,
        CharRef2,
        CharRef,
        AmpRef,
        LTRef,
        GTRef,
        AposRef,
        QuoteRef,
        CharData,
        CharDataChars,
        content,
        AmpName,
        LTName,
        GTName,
        ClassNameChar,
        ClassName,
        Name,
        XMLDecl,
        XMLDeclChars,
        DocTypeDecl,
        DocTypeDeclChars,
        ClassIDAttribute,
        ObjectIDAttribute,
        ClassNameAttribute,
        TrackingAttribute,
        VersionAttribute,
        UnusedAttribute,
        Attribute,
        SignatureAttribute,
        SerializationWrapper,
        NameHead,
        NameTail,
        AttributeList,
        S;

    // XML Character classes
    chset_t
        BaseChar,
        Ideographic,
        Char,
        Letter,
        Digit,
        CombiningChar,
        Extender,
        Sch,
        NameChar;

    void init_chset();

    bool my_parse(
        IStream & is,
        const rule_t &rule_,
        const CharType delimiter = L'>'
    ) const ;
public:
    struct return_values {
        StringType object_name;
        StringType contents;
        //class_id_type class_id;
        int_least16_t class_id;
        //object_id_type object_id;
        uint_least32_t object_id;
        //version_type version;
        unsigned int version;
        tracking_type tracking_level;
        StringType class_name;
        return_values() :
            version(0),
            tracking_level(false)
        {}
    } rv;
    bool parse_start_tag(IStream & is) /*const*/;
    bool parse_end_tag(IStream & is) const;
    bool parse_string(IStream & is, StringType & s) /*const*/;
    void init(IStream & is);
    bool windup(IStream & is);
    basic_xml_grammar();
};

} // namespace archive
} // namespace boost

#endif // BOOST_ARCHIVE_BASIC_XML_GRAMMAR_HPP
