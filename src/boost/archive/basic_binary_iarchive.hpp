#ifndef BOOST_ARCHIVE_BASIC_BINARY_IARCHIVE_HPP
#define BOOST_ARCHIVE_BASIC_BINARY_IARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_binary_iarchive.hpp
//
// archives stored as native binary - this should be the fastest way
// to archive the state of a group of obects.  It makes no attempt to
// convert to any canonical form.

// IN GENERAL, ARCHIVES CREATED WITH THIS CLASS WILL NOT BE READABLE
// ON PLATFORM APART FROM THE ONE THEY ARE CREATED ON

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/archive/basic_archive.hpp>
#include <boost/archive/detail/common_iarchive.hpp>
#include <boost/serialization/collection_size_type.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/library_version_type.hpp>
#include <boost/serialization/item_version_type.hpp>
#include <boost/integer_traits.hpp>

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

namespace boost {
namespace archive {

namespace detail {
    template<class Archive> class interface_iarchive;
} // namespace detail

/////////////////////////////////////////////////////////////////////////
// class basic_binary_iarchive - read serialized objects from a input binary stream
template<class Archive>
class BOOST_SYMBOL_VISIBLE basic_binary_iarchive :
    public detail::common_iarchive<Archive>
{
#ifdef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
protected:
    #if BOOST_WORKAROUND(BOOST_MSVC, < 1500)
        // for some inexplicable reason insertion of "class" generates compile erro
        // on msvc 7.1
        friend detail::interface_iarchive<Archive>;
    #else
        friend class detail::interface_iarchive<Archive>;
    #endif
#endif
    // intermediate level to support override of operators
    // fot templates in the absence of partial function
    // template ordering. If we get here pass to base class
    // note extra nonsense to sneak it pass the borland compiers
    typedef detail::common_iarchive<Archive> detail_common_iarchive;
    template<class T>
    void load_override(T & t){
      this->detail_common_iarchive::load_override(t);
    }

    // include these to trap a change in binary format which
    // isn't specifically handled
    // upto 32K classes
    BOOST_STATIC_ASSERT(sizeof(class_id_type) == sizeof(int_least16_t));
    BOOST_STATIC_ASSERT(sizeof(class_id_reference_type) == sizeof(int_least16_t));
    // upto 2G objects
    BOOST_STATIC_ASSERT(sizeof(object_id_type) == sizeof(uint_least32_t));
    BOOST_STATIC_ASSERT(sizeof(object_reference_type) == sizeof(uint_least32_t));

    // binary files don't include the optional information
    void load_override(class_id_optional_type & /* t */){}

    void load_override(tracking_type & t, int /*version*/){
        boost::serialization::library_version_type lv = this->get_library_version();
        if(boost::serialization::library_version_type(6) < lv){
            int_least8_t x=0;
            * this->This() >> x;
            t = boost::archive::tracking_type(x);
        }
        else{
            bool x=0;
            * this->This() >> x;
            t = boost::archive::tracking_type(x);
        }
    }
    void load_override(class_id_type & t){
        boost::serialization::library_version_type lv = this->get_library_version();
        /*
         * library versions:
         *   boost 1.39 -> 5
         *   boost 1.43 -> 7
         *   boost 1.47 -> 9
         *
         *
         * 1) in boost 1.43 and inferior, class_id_type is always a 16bit value, with no check on the library version
         *   --> this means all archives with version v <= 7 are written with a 16bit class_id_type
         * 2) in boost 1.44 this load_override has disappeared (and thus boost 1.44 is not backward compatible at all !!)
         * 3) recent boosts reintroduced load_override with a test on the version :
         *     - v > 7 : this->detail_common_iarchive::load_override(t, version)
         *     - v > 6 : 16bit
         *     - other : 32bit
         *   --> which is obviously incorrect, see point 1
         *
         * the fix here decodes class_id_type on 16bit for all v <= 7, which seems to be the correct behaviour ...
         */
        if(boost::serialization::library_version_type (7) < lv){
            this->detail_common_iarchive::load_override(t);
        }
        else{
            int_least16_t x=0;
            * this->This() >> x;
            t = boost::archive::class_id_type(x);
        }
    }
    void load_override(class_id_reference_type & t){
        load_override(static_cast<class_id_type &>(t));
    }

    void load_override(version_type & t){
        boost::serialization::library_version_type  lv = this->get_library_version();
        if(boost::serialization::library_version_type(7) < lv){
            this->detail_common_iarchive::load_override(t);
        }
        else
        if(boost::serialization::library_version_type(6) < lv){
            uint_least8_t x=0;
            * this->This() >> x;
            t = boost::archive::version_type(x);
        }
        else
        if(boost::serialization::library_version_type(5) < lv){
            uint_least16_t x=0;
            * this->This() >> x;
            t = boost::archive::version_type(x);
        }
        else
        if(boost::serialization::library_version_type(2) < lv){
            // upto 255 versions
            unsigned char x=0;
            * this->This() >> x;
            t = version_type(x);
        }
        else{
            unsigned int x=0;
            * this->This() >> x;
            t = boost::archive::version_type(x);
        }
    }

    void load_override(boost::serialization::item_version_type & t){
        boost::serialization::library_version_type lv = this->get_library_version();
//        if(boost::serialization::library_version_type(7) < lvt){
        if(boost::serialization::library_version_type(6) < lv){
            this->detail_common_iarchive::load_override(t);
        }
        else
        if(boost::serialization::library_version_type(6) < lv){
            uint_least16_t x=0;
            * this->This() >> x;
            t = boost::serialization::item_version_type(x);
        }
        else{
            unsigned int x=0;
            * this->This() >> x;
            t = boost::serialization::item_version_type(x);
        }
    }

    void load_override(serialization::collection_size_type & t){
        if(boost::serialization::library_version_type(5) < this->get_library_version()){
            this->detail_common_iarchive::load_override(t);
        }
        else{
            unsigned int x=0;
            * this->This() >> x;
            t = serialization::collection_size_type(x);
        }
    }

    BOOST_ARCHIVE_OR_WARCHIVE_DECL void
    load_override(class_name_type & t);
    BOOST_ARCHIVE_OR_WARCHIVE_DECL void
    init();

    basic_binary_iarchive(unsigned int flags) :
        detail::common_iarchive<Archive>(flags)
    {}
};

} // namespace archive
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#endif // BOOST_ARCHIVE_BASIC_BINARY_IARCHIVE_HPP
