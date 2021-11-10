#ifndef BOOST_ARCHIVE_BASIC_ARCHIVE_HPP
#define BOOST_ARCHIVE_BASIC_ARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_archive.hpp:

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.
#include <cstring> // count
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/integer_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/serialization/library_version_type.hpp>

#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

namespace boost {
namespace archive {

#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4244 4267 )
#endif

BOOST_ARCHIVE_DECL boost::serialization::library_version_type
BOOST_ARCHIVE_VERSION();

// create alias in boost::archive for older user code.
typedef boost::serialization::library_version_type library_version_type;

class version_type {
private:
    typedef uint_least32_t base_type;
    base_type t;
public:
    // should be private - but MPI fails if it's not!!!
    version_type(): t(0) {}
    explicit version_type(const unsigned int & t_) : t(t_){
        BOOST_ASSERT(t_ <= boost::integer_traits<base_type>::const_max);
    }
    version_type(const version_type & t_) :
        t(t_.t)
    {}
    version_type & operator=(const version_type & rhs){
        t = rhs.t;
        return *this;
    }
    // used for text output
    operator base_type () const {
        return t;
    }
    // used for text intput
    operator base_type  & (){
        return t;
    }
    bool operator==(const version_type & rhs) const {
        return t == rhs.t;
    }
    bool operator<(const version_type & rhs) const {
        return t < rhs.t;
    }
};

class class_id_type {
private:
    typedef int_least16_t base_type;
    base_type t;
public:
    // should be private - but then can't use BOOST_STRONG_TYPE below
    class_id_type() : t(0) {}
    explicit class_id_type(const int t_) : t(t_){
        BOOST_ASSERT(t_ <= boost::integer_traits<base_type>::const_max);
    }
    explicit class_id_type(const std::size_t t_) : t(t_){
 //       BOOST_ASSERT(t_ <= boost::integer_traits<base_type>::const_max);
    }
    class_id_type(const class_id_type & t_) :
        t(t_.t)
    {}
    class_id_type & operator=(const class_id_type & rhs){
        t = rhs.t;
        return *this;
    }

    // used for text output
    operator base_type () const {
        return t;
    }
    // used for text input
    operator base_type &() {
        return t;
    }
    bool operator==(const class_id_type & rhs) const {
        return t == rhs.t;
    }
    bool operator<(const class_id_type & rhs) const {
        return t < rhs.t;
    }
};

#define BOOST_SERIALIZATION_NULL_POINTER_TAG boost::archive::class_id_type(-1)

class object_id_type {
private:
    typedef uint_least32_t base_type;
    base_type t;
public:
    object_id_type(): t(0) {}
    // note: presumes that size_t >= unsigned int.
    // use explicit cast to silence useless warning
    explicit object_id_type(const std::size_t & t_) : t(static_cast<base_type>(t_)){
        // make quadruple sure that we haven't lost any real integer
        // precision
        BOOST_ASSERT(t_ <= boost::integer_traits<base_type>::const_max);
    }
    object_id_type(const object_id_type & t_) :
        t(t_.t)
    {}
    object_id_type & operator=(const object_id_type & rhs){
        t = rhs.t;
        return *this;
    }
    // used for text output
    operator base_type () const {
        return t;
    }
    // used for text input
    operator base_type & () {
        return t;
    }
    bool operator==(const object_id_type & rhs) const {
        return t == rhs.t;
    }
    bool operator<(const object_id_type & rhs) const {
        return t < rhs.t;
    }
};

#if defined(_MSC_VER)
#pragma warning( pop )
#endif

struct tracking_type {
    bool t;
    explicit tracking_type(const bool t_ = false)
        : t(t_)
    {}
    tracking_type(const tracking_type & t_)
        : t(t_.t)
    {}
    operator bool () const {
        return t;
    }
    operator bool & () {
        return t;
    }
    tracking_type & operator=(const bool t_){
        t = t_;
        return *this;
    }
    bool operator==(const tracking_type & rhs) const {
        return t == rhs.t;
    }
    bool operator==(const bool & rhs) const {
        return t == rhs;
    }
    tracking_type & operator=(const tracking_type & rhs){
        t = rhs.t;
        return *this;
    }
};

struct class_name_type :
    private boost::noncopyable
{
    char *t;
    operator const char * & () const {
        return const_cast<const char * &>(t);
    }
    operator char * () {
        return t;
    }
    std::size_t size() const {
        return std::strlen(t);
    }
    explicit class_name_type(const char *key_)
    : t(const_cast<char *>(key_)){}
    explicit class_name_type(char *key_)
    : t(key_){}
    class_name_type & operator=(const class_name_type & rhs){
        t = rhs.t;
        return *this;
    }
};

enum archive_flags {
    no_header = 1,  // suppress archive header info
    no_codecvt = 2,  // suppress alteration of codecvt facet
    no_xml_tag_checking = 4,   // suppress checking of xml tags
    no_tracking = 8,           // suppress ALL tracking
    flags_last = 8
};

BOOST_ARCHIVE_DECL const char *
BOOST_ARCHIVE_SIGNATURE();

/* NOTE : Warning  : Warning : Warning : Warning : Warning
 * If any of these are changed to different sized types,
 * binary_iarchive won't be able to read older archives
 * unless you rev the library version and include conditional
 * code based on the library version.  There is nothing
 * inherently wrong in doing this - but you have to be super
 * careful because it's easy to get wrong and start breaking
 * old archives !!!
 */

#define BOOST_ARCHIVE_STRONG_TYPEDEF(T, D)         \
    class D : public T {                           \
    public:                                        \
        explicit D(const T tt) : T(tt){}           \
    };                                             \
/**/

BOOST_ARCHIVE_STRONG_TYPEDEF(class_id_type, class_id_reference_type)
BOOST_ARCHIVE_STRONG_TYPEDEF(class_id_type, class_id_optional_type)
BOOST_ARCHIVE_STRONG_TYPEDEF(object_id_type, object_reference_type)

}// namespace archive
}// namespace boost

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#include <boost/serialization/level.hpp>

// set implementation level to primitive for all types
// used internally by the serialization library

BOOST_CLASS_IMPLEMENTATION(boost::serialization::library_version_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::version_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::class_id_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::class_id_reference_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::class_id_optional_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::class_name_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::object_id_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::object_reference_type, primitive_type)
BOOST_CLASS_IMPLEMENTATION(boost::archive::tracking_type, primitive_type)

#include <boost/serialization/is_bitwise_serializable.hpp>

// set types used internally by the serialization library
// to be bitwise serializable

BOOST_IS_BITWISE_SERIALIZABLE(boost::serialization::library_version_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::version_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::class_id_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::class_id_reference_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::class_id_optional_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::class_name_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::object_id_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::object_reference_type)
BOOST_IS_BITWISE_SERIALIZABLE(boost::archive::tracking_type)

#endif //BOOST_ARCHIVE_BASIC_ARCHIVE_HPP
