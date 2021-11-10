/*
 [auto_generated]
 boost/numeric/odeint/algebra/detail/extract_value_type.hpp

 [begin_description]
 Extract true value type from complex types (eg. std::complex)
 [end_description]

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_EXTRACT_VALUE_TYPE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_ALGEBRA_DETAIL_EXTRACT_VALUE_TYPE_HPP_INCLUDED

#include <boost/utility.hpp>
#include <boost/mpl/has_xxx.hpp>

BOOST_MPL_HAS_XXX_TRAIT_DEF(value_type)

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

template< typename S , typename Enabler = void >
struct extract_value_type {};

// as long as value_types are defined we go down the value_type chain
// e.g. returning S::value_type::value_type::value_type

template< typename S >
struct extract_value_type<S , typename boost::disable_if< has_value_type<S> >::type >
{
    // no value_type defined, return S
    typedef S type;
};

template< typename S >
struct extract_value_type< S , typename boost::enable_if< has_value_type<S> >::type >
{
    // go down the value_type
    typedef typename extract_value_type< typename S::value_type >::type type;
};

} } } }

#endif
