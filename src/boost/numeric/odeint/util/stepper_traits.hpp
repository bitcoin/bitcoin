/*
  [auto_generated]
  boost/numeric/odeint/util/stepper_traits.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2013 Karsten Ahnert
  Copyright 2013 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_UTIL_STEPPER_TRAITS_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_UTIL_STEPPER_TRAITS_HPP_DEFINED

#include <boost/numeric/odeint/util/unwrap_reference.hpp>


namespace boost {
namespace numeric {
namespace odeint {
namespace traits {

template< class Stepper >
struct state_type
{
    typedef typename boost::numeric::odeint::unwrap_reference< Stepper >::type stepper_type;
    typedef typename stepper_type::state_type type;
};

template< class Stepper >
struct time_type
{
    typedef typename boost::numeric::odeint::unwrap_reference< Stepper >::type stepper_type;
    typedef typename stepper_type::time_type type;
};

template< class Stepper >
struct stepper_category
{
    typedef typename boost::numeric::odeint::unwrap_reference< Stepper >::type stepper_type;
    typedef typename stepper_type::stepper_category type;
};

template< class Stepper >
struct value_type
{
    typedef typename boost::numeric::odeint::unwrap_reference< Stepper >::type stepper_type;
    typedef typename stepper_type::value_type type;
};

} // namespace traits
} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_UTIL_STEPPER_TRAITS_HPP_DEFINED
