#ifndef BOOST_ARCHIVE_BASIC_TEXT_IARCHIVE_HPP
#define BOOST_ARCHIVE_BASIC_TEXT_IARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_text_iarchive.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

// archives stored as text - note these ar templated on the basic
// stream templates to accommodate wide (and other?) kind of characters
//
// note the fact that on libraries without wide characters, ostream is
// is not a specialization of basic_ostream which in fact is not defined
// in such cases.   So we can't use basic_istream<IStream::char_type> but rather
// use two template parameters

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/archive/detail/common_iarchive.hpp>

#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace boost {
namespace archive {

namespace detail {
    template<class Archive> class interface_iarchive;
} // namespace detail

/////////////////////////////////////////////////////////////////////////
// class basic_text_iarchive - read serialized objects from a input text stream
template<class Archive>
class BOOST_SYMBOL_VISIBLE basic_text_iarchive :
    public detail::common_iarchive<Archive>
{
#ifdef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
protected:
    #if BOOST_WORKAROUND(BOOST_MSVC, < 1500)
        // for some inexplicable reason insertion of "class" generates compile error
        // on msvc 7.1
        friend detail::interface_iarchive<Archive>;
    #else
        friend class detail::interface_iarchive<Archive>;
    #endif
#endif
    // intermediate level to support override of operators
    // fot templates in the absence of partial function
    // template ordering
    typedef detail::common_iarchive<Archive> detail_common_iarchive;
    template<class T>
    void load_override(T & t){
        this->detail_common_iarchive::load_override(t);
    }
    // text file don't include the optional information
    void load_override(class_id_optional_type & /*t*/){}

    BOOST_ARCHIVE_OR_WARCHIVE_DECL void
    load_override(class_name_type & t);

    BOOST_ARCHIVE_OR_WARCHIVE_DECL void
    init();

    basic_text_iarchive(unsigned int flags) :
        detail::common_iarchive<Archive>(flags)
    {}
    ~basic_text_iarchive() BOOST_OVERRIDE {}
};

} // namespace archive
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#endif // BOOST_ARCHIVE_BASIC_TEXT_IARCHIVE_HPP
