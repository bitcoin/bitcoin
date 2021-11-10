// Boost.TypeErasure library
//
// Copyright 2011 Steven Watanabe
//
// Distributed under the Boost Software License Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// $Id$

#ifndef BOOST_TYPE_ERASURE_CONCEPT_OF_HPP_INCLUDED
#define BOOST_TYPE_ERASURE_CONCEPT_OF_HPP_INCLUDED

#include <boost/config.hpp>

namespace boost {
namespace type_erasure {

#ifndef BOOST_TYPE_ERASURE_DOXYGEN

template<class Concept, class T>
class any;

template<class Concept, class T>
class param;

#endif

/**
 * A metafunction returning the concept corresponding
 * to an @ref any.  It will also work for all bases
 * of @ref any, so it can be applied to the @c Base
 * parameter of @ref concept_interface.
 */
template<class T>
struct concept_of
{
#ifdef BOOST_TYPE_ERASURE_DOXYGEN
    typedef detail::unspecified type;
#else
    typedef typename ::boost::type_erasure::concept_of<
        typename T::_boost_type_erasure_derived_type
    >::type type;
#endif
};

/** INTERNAL ONLY */
template<class Concept, class T>
struct concept_of< ::boost::type_erasure::any<Concept, T> >
{
    typedef Concept type;
};

/** INTERNAL ONLY */
template<class Concept, class T>
struct concept_of< ::boost::type_erasure::param<Concept, T> >
{
    typedef Concept type;
};

#ifndef BOOST_NO_CXX11_TEMPLATE_ALIASES

template<class T>
using concept_of_t = typename ::boost::type_erasure::concept_of<T>::type;

#endif

}
}

#endif
