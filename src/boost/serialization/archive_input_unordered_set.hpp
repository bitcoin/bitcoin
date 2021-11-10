#ifndef  BOOST_SERIALIZATION_ARCHIVE_INPUT_UNORDERED_SET_HPP
#define BOOST_SERIALIZATION_ARCHIVE_INPUT_UNORDERED_SET_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// archive_input_unordered_set.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
// (C) Copyright 2014 Jim Bell
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <utility>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/detail/stack_constructor.hpp>
#include <boost/move/utility_core.hpp>

namespace boost {
namespace serialization {

namespace stl {

// unordered_set input
template<class Archive, class Container>
struct archive_input_unordered_set
{
    inline void operator()(
        Archive &ar,
        Container &s,
        const unsigned int v
    ){
        typedef typename Container::value_type type;
        detail::stack_construct<Archive, type> t(ar, v);
        // borland fails silently w/o full namespace
        ar >> boost::serialization::make_nvp("item", t.reference());
        std::pair<typename Container::const_iterator, bool> result =
            s.insert(boost::move(t.reference()));
        if(result.second)
            ar.reset_object_address(& (* result.first), & t.reference());
    }
};

// unordered_multiset input
template<class Archive, class Container>
struct archive_input_unordered_multiset
{
    inline void operator()(
        Archive &ar,
        Container &s,
        const unsigned int v
    ){
        typedef typename Container::value_type type;
        detail::stack_construct<Archive, type> t(ar, v);
        ar >> boost::serialization::make_nvp("item", t.reference());
        typename Container::const_iterator result =
            s.insert(boost::move(t.reference()));
        ar.reset_object_address(& (* result), & t.reference());
    }
};

} // stl
} // serialization
} // boost

#endif // BOOST_SERIALIZATION_ARCHIVE_INPUT_UNORDERED_SET_HPP
