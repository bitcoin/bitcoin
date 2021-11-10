#ifndef BOOST_ARCHIVE_DETAIL_COMMON_IARCHIVE_HPP
#define BOOST_ARCHIVE_DETAIL_COMMON_IARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// common_iarchive.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>

#include <boost/archive/detail/basic_iarchive.hpp>
#include <boost/archive/detail/basic_pointer_iserializer.hpp>
#include <boost/archive/detail/interface_iarchive.hpp>

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace boost {
namespace archive {
namespace detail {

class extended_type_info;

// note: referred to as Curiously Recurring Template Patter (CRTP)
template<class Archive>
class BOOST_SYMBOL_VISIBLE common_iarchive :
    public basic_iarchive,
    public interface_iarchive<Archive>
{
    friend class interface_iarchive<Archive>;
    friend class basic_iarchive;
private:
    void vload(version_type & t) BOOST_OVERRIDE {
        * this->This() >> t;
    }
    void vload(object_id_type & t) BOOST_OVERRIDE {
        * this->This() >> t;
    }
    void vload(class_id_type & t) BOOST_OVERRIDE {
        * this->This() >> t;
    }
    void vload(class_id_optional_type & t) BOOST_OVERRIDE {
        * this->This() >> t;
    }
    void vload(tracking_type & t) BOOST_OVERRIDE {
        * this->This() >> t;
    }
    void vload(class_name_type &s) BOOST_OVERRIDE {
        * this->This() >> s;
    }
protected:
    // default processing - invoke serialization library
    template<class T>
    void load_override(T & t){
        archive::load(* this->This(), t);
    }
    // default implementations of functions which emit start/end tags for
    // archive types that require them.
    void load_start(const char * /*name*/){}
    void load_end(const char * /*name*/){}
    // default archive initialization
    common_iarchive(unsigned int flags = 0) :
        basic_iarchive(flags),
        interface_iarchive<Archive>()
    {}
};

} // namespace detail
} // namespace archive
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#endif // BOOST_ARCHIVE_DETAIL_COMMON_IARCHIVE_HPP
