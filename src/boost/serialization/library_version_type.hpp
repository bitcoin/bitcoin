#ifndef BOOST_SERIALIZATION_LIBRARY_VERSION_TYPE_HPP
#define BOOST_SERIALIZATION_LIBRARY_VERSION_TYPE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// library_version_type.hpp:

// (C) Copyright 2002-2020 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.
#include <cstring> // count
#include <boost/cstdint.hpp> // uint_least16_t
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/integer_traits.hpp>

namespace boost {
namespace serialization {

#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4244 4267 )
#endif

/* NOTE : Warning  : Warning : Warning : Warning : Warning
 * Don't ever changes this.  If you do, they previously created
 * binary archives won't be readable !!!
 */
class library_version_type {
private:
    typedef uint_least16_t base_type;
    base_type t;
public:
    library_version_type(): t(0) {}
    explicit library_version_type(const unsigned int & t_) : t(t_){
        BOOST_ASSERT(t_ <= boost::integer_traits<base_type>::const_max);
    }
    library_version_type(const library_version_type & t_) :
        t(t_.t)
    {}
    library_version_type & operator=(const library_version_type & rhs){
        t = rhs.t;
        return *this;
    }
    // used for text output
    operator base_type () const {
        return t;
    }
    // used for text input
    operator base_type & (){
        return t;
    }
    bool operator==(const library_version_type & rhs) const {
        return t == rhs.t;
    }
    bool operator<(const library_version_type & rhs) const {
        return t < rhs.t;
    }
};

#if defined(_MSC_VER)
#pragma warning( pop )
#endif

} // serialization
} // boost

#endif // BOOST_SERIALIZATION_LIBRARY_VERSION_TYPE_HPP
