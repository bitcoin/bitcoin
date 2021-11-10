#ifndef BOOST_ARCHIVE_ITERATORS_BASE64_FROM_BINARY_HPP
#define BOOST_ARCHIVE_ITERATORS_BASE64_FROM_BINARY_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// base64_from_binary.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/assert.hpp>

#include <cstddef> // size_t
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::size_t;
} // namespace std
#endif

#include <boost/iterator/transform_iterator.hpp>
#include <boost/archive/iterators/dataflow_exception.hpp>

namespace boost {
namespace archive {
namespace iterators {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// convert binary integers to base64 characters

namespace detail {

template<class CharType>
struct from_6_bit {
    typedef CharType result_type;
    CharType operator()(CharType t) const{
        static const char * lookup_table =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "+/";
        BOOST_ASSERT(t < 64);
        return lookup_table[static_cast<size_t>(t)];
    }
};

} // namespace detail

// note: what we would like to do is
// template<class Base, class CharType = typename Base::value_type>
//  typedef transform_iterator<
//      from_6_bit<CharType>,
//      transform_width<Base, 6, sizeof(Base::value_type) * 8, CharType>
//  > base64_from_binary;
// but C++ won't accept this.  Rather than using a "type generator" and
// using a different syntax, make a derivation which should be equivalent.
//
// Another issue addressed here is that the transform_iterator doesn't have
// a templated constructor.  This makes it incompatible with the dataflow
// ideal.  This is also addressed here.

//template<class Base, class CharType = typename Base::value_type>
template<
    class Base,
    class CharType = typename boost::iterator_value<Base>::type
>
class base64_from_binary :
    public transform_iterator<
        detail::from_6_bit<CharType>,
        Base
    >
{
    friend class boost::iterator_core_access;
    typedef transform_iterator<
        typename detail::from_6_bit<CharType>,
        Base
    > super_t;

public:
    // make composible buy using templated constructor
    template<class T>
    base64_from_binary(T start) :
        super_t(
            Base(static_cast< T >(start)),
            detail::from_6_bit<CharType>()
        )
    {}
    // intel 7.1 doesn't like default copy constructor
    base64_from_binary(const base64_from_binary & rhs) :
        super_t(
            Base(rhs.base_reference()),
            detail::from_6_bit<CharType>()
        )
    {}
//    base64_from_binary(){};
};

} // namespace iterators
} // namespace archive
} // namespace boost

#endif // BOOST_ARCHIVE_ITERATORS_BASE64_FROM_BINARY_HPP
