#ifndef BOOST_ARCHIVE_DETAIL_POLYMORPHIC_IARCHIVE_ROUTE_HPP
#define BOOST_ARCHIVE_DETAIL_POLYMORPHIC_IARCHIVE_ROUTE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// polymorphic_iarchive_route.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <string>
#include <ostream>
#include <cstddef>

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

#include <boost/cstdint.hpp>
#include <boost/integer_traits.hpp>
#include <boost/archive/polymorphic_iarchive.hpp>
#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

namespace boost {
namespace serialization {
    class extended_type_info;
} // namespace serialization
namespace archive {
namespace detail{

class basic_iserializer;
class basic_pointer_iserializer;

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

template<class ArchiveImplementation>
class polymorphic_iarchive_route :
    public polymorphic_iarchive,
    // note: gcc dynamic cross cast fails if the the derivation below is
    // not public.  I think this is a mistake.
    public /*protected*/ ArchiveImplementation
{
private:
    // these are used by the serialization library.
    void load_object(
        void *t,
        const basic_iserializer & bis
    ) BOOST_OVERRIDE {
        ArchiveImplementation::load_object(t, bis);
    }
    const basic_pointer_iserializer * load_pointer(
        void * & t,
        const basic_pointer_iserializer * bpis_ptr,
        const basic_pointer_iserializer * (*finder)(
            const boost::serialization::extended_type_info & type
        )
    ) BOOST_OVERRIDE {
        return ArchiveImplementation::load_pointer(t, bpis_ptr, finder);
    }
    void set_library_version(boost::serialization::library_version_type archive_library_version) BOOST_OVERRIDE {
        ArchiveImplementation::set_library_version(archive_library_version);
    }
    boost::serialization::library_version_type get_library_version() const BOOST_OVERRIDE {
        return ArchiveImplementation::get_library_version();
    }
    unsigned int get_flags() const BOOST_OVERRIDE {
        return ArchiveImplementation::get_flags();
    }
    void delete_created_pointers() BOOST_OVERRIDE {
        ArchiveImplementation::delete_created_pointers();
    }
    void reset_object_address(
        const void * new_address,
        const void * old_address
    ) BOOST_OVERRIDE {
        ArchiveImplementation::reset_object_address(new_address, old_address);
    }
    void load_binary(void * t, std::size_t size) BOOST_OVERRIDE {
        ArchiveImplementation::load_binary(t, size);
    }
    // primitive types the only ones permitted by polymorphic archives
    void load(bool & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(char & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(signed char & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(unsigned char & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    #ifndef BOOST_NO_CWCHAR
    #ifndef BOOST_NO_INTRINSIC_WCHAR_T
    void load(wchar_t & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    #endif
    #endif
    void load(short & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(unsigned short & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(int & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(unsigned int & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(long & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(unsigned long & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    #if defined(BOOST_HAS_LONG_LONG)
    void load(boost::long_long_type & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(boost::ulong_long_type & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    #elif defined(BOOST_HAS_MS_INT64)
    void load(__int64 & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(unsigned __int64 & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    #endif
    void load(float & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(double & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    void load(std::string & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    #ifndef BOOST_NO_STD_WSTRING
    void load(std::wstring & t) BOOST_OVERRIDE {
        ArchiveImplementation::load(t);
    }
    #endif
    // used for xml and other tagged formats default does nothing
    void load_start(const char * name) BOOST_OVERRIDE {
        ArchiveImplementation::load_start(name);
    }
    void load_end(const char * name) BOOST_OVERRIDE {
        ArchiveImplementation::load_end(name);
    }
    void register_basic_serializer(const basic_iserializer & bis) BOOST_OVERRIDE {
        ArchiveImplementation::register_basic_serializer(bis);
    }
    helper_collection &
    get_helper_collection() BOOST_OVERRIDE {
        return ArchiveImplementation::get_helper_collection();
    }
public:
    // this can't be inherited because they appear in multiple
    // parents
    typedef mpl::bool_<true> is_loading;
    typedef mpl::bool_<false> is_saving;
    // the >> operator
    template<class T>
    polymorphic_iarchive & operator>>(T & t){
        return polymorphic_iarchive::operator>>(t);
    }
    // the & operator
    template<class T>
    polymorphic_iarchive & operator&(T & t){
        return polymorphic_iarchive::operator&(t);
    }
    // register type function
    template<class T>
    const basic_pointer_iserializer *
    register_type(T * t = NULL){
        return ArchiveImplementation::register_type(t);
    }
    // all current archives take a stream as constructor argument
    template <class _Elem, class _Tr>
    polymorphic_iarchive_route(
        std::basic_istream<_Elem, _Tr> & is,
        unsigned int flags = 0
    ) :
        ArchiveImplementation(is, flags)
    {}
    ~polymorphic_iarchive_route() BOOST_OVERRIDE {}
};

} // namespace detail
} // namespace archive
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#endif // BOOST_ARCHIVE_DETAIL_POLYMORPHIC_IARCHIVE_DISPATCH_HPP
