#ifndef BOOST_ARCHIVE_POLYMORPHIC_OARCHIVE_HPP
#define BOOST_ARCHIVE_POLYMORPHIC_OARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_oarchive.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstddef> // size_t
#include <climits> // ULONG_MAX
#include <string>

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

#include <boost/cstdint.hpp>
#include <boost/archive/detail/oserializer.hpp>
#include <boost/archive/detail/interface_oarchive.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/archive/detail/register_archive.hpp>

#include <boost/archive/detail/decl.hpp>
#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

namespace boost {
namespace serialization {
    class extended_type_info;
} // namespace serialization
namespace archive {
namespace detail {
    class basic_oarchive;
    class basic_oserializer;
}

class polymorphic_oarchive;

class BOOST_SYMBOL_VISIBLE polymorphic_oarchive_impl :
    public detail::interface_oarchive<polymorphic_oarchive>
{
#ifdef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
    friend class detail::interface_oarchive<polymorphic_oarchive>;
    friend class save_access;
#endif
    // primitive types the only ones permitted by polymorphic archives
    virtual void save(const bool t) = 0;

    virtual void save(const char t) = 0;
    virtual void save(const signed char t) = 0;
    virtual void save(const unsigned char t) = 0;
    #ifndef BOOST_NO_CWCHAR
    #ifndef BOOST_NO_INTRINSIC_WCHAR_T
    virtual void save(const wchar_t t) = 0;
    #endif
    #endif
    virtual void save(const short t) = 0;
    virtual void save(const unsigned short t) = 0;
    virtual void save(const int t) = 0;
    virtual void save(const unsigned int t) = 0;
    virtual void save(const long t) = 0;
    virtual void save(const unsigned long t) = 0;

    #if defined(BOOST_HAS_LONG_LONG)
    virtual void save(const boost::long_long_type t) = 0;
    virtual void save(const boost::ulong_long_type t) = 0;
    #elif defined(BOOST_HAS_MS_INT64)
    virtual void save(const __int64 t) = 0;
    virtual void save(const unsigned __int64 t) = 0;
    #endif

    virtual void save(const float t) = 0;
    virtual void save(const double t) = 0;

    // string types are treated as primitives
    virtual void save(const std::string & t) = 0;
    #ifndef BOOST_NO_STD_WSTRING
    virtual void save(const std::wstring & t) = 0;
    #endif

    virtual void save_null_pointer() = 0;
    // used for xml and other tagged formats
    virtual void save_start(const char * name) = 0;
    virtual void save_end(const char * name) = 0;
    virtual void register_basic_serializer(const detail::basic_oserializer & bos) = 0;
    virtual detail::helper_collection & get_helper_collection() = 0;

    virtual void end_preamble() = 0;

    // msvc and borland won't automatically pass these to the base class so
    // make it explicit here
    template<class T>
    void save_override(T & t)
    {
        archive::save(* this->This(), t);
    }
    // special treatment for name-value pairs.
    template<class T>
    void save_override(
            const ::boost::serialization::nvp< T > & t
        ){
        save_start(t.name());
        archive::save(* this->This(), t.const_value());
        save_end(t.name());
    }
protected:
    virtual ~polymorphic_oarchive_impl() {}
public:
    // utility functions implemented by all legal archives
    virtual unsigned int get_flags() const = 0;
    virtual boost::serialization::library_version_type get_library_version() const = 0;
    virtual void save_binary(const void * t, std::size_t size) = 0;

    virtual void save_object(
        const void *x,
        const detail::basic_oserializer & bos
    ) = 0;
    virtual void save_pointer(
        const void * t,
        const detail::basic_pointer_oserializer * bpos_ptr
    ) = 0;
};

// note: preserve naming symmetry
class BOOST_SYMBOL_VISIBLE polymorphic_oarchive :
    public polymorphic_oarchive_impl
{
public:
    ~polymorphic_oarchive() BOOST_OVERRIDE {}
};

} // namespace archive
} // namespace boost

// required by export
BOOST_SERIALIZATION_REGISTER_ARCHIVE(boost::archive::polymorphic_oarchive)

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#endif // BOOST_ARCHIVE_POLYMORPHIC_OARCHIVE_HPP
