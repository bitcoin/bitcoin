#ifndef BOOST_ARCHIVE_DETAIL_BASIC_ISERIALIZER_HPP
#define BOOST_ARCHIVE_DETAIL_BASIC_ISERIALIZER_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// basic_iserializer.hpp: extenstion of type_info required for serialization.

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <cstdlib> // NULL
#include <boost/config.hpp>

#include <boost/archive/basic_archive.hpp>
#include <boost/archive/detail/decl.hpp>
#include <boost/archive/detail/basic_serializer.hpp>
#include <boost/archive/detail/auto_link_archive.hpp>
#include <boost/archive/detail/abi_prefix.hpp> // must be the last header

#ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4511 4512)
#endif

namespace boost {
namespace serialization {
    class extended_type_info;
} // namespace serialization

// forward declarations
namespace archive {
namespace detail {

class basic_iarchive;
class basic_pointer_iserializer;

class BOOST_SYMBOL_VISIBLE basic_iserializer :
    public basic_serializer
{
private:
    basic_pointer_iserializer *m_bpis;
protected:
    explicit BOOST_ARCHIVE_DECL basic_iserializer(
        const boost::serialization::extended_type_info & type
    );
    virtual BOOST_ARCHIVE_DECL ~basic_iserializer();
public:
    bool serialized_as_pointer() const {
        return m_bpis != NULL;
    }
    void set_bpis(basic_pointer_iserializer *bpis){
        m_bpis = bpis;
    }
    const basic_pointer_iserializer * get_bpis_ptr() const {
        return m_bpis;
    }
    virtual void load_object_data(
        basic_iarchive & ar,
        void *x,
        const unsigned int file_version
    ) const = 0;
    // returns true if class_info should be saved
    virtual bool class_info() const = 0 ;
    // returns true if objects should be tracked
    virtual bool tracking(const unsigned int) const = 0 ;
    // returns class version
    virtual version_type version() const = 0 ;
    // returns true if this class is polymorphic
    virtual bool is_polymorphic() const = 0;
    virtual void destroy(/*const*/ void *address) const = 0 ;
};

} // namespae detail
} // namespace archive
} // namespace boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#include <boost/archive/detail/abi_suffix.hpp> // pops abi_suffix.hpp pragmas

#endif // BOOST_ARCHIVE_DETAIL_BASIC_ISERIALIZER_HPP
