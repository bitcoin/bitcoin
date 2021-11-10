/*
 [auto_generated]
 boost/numeric/odeint/stepper/generation/generation_runge_kutta_dopri5.hpp

 [begin_description]
 Enable the factory functions for the controller and the dense output of the Runge-Kutta-Dormand-Prince5 method.
 [end_description]

 Copyright 2011 Karsten Ahnert
 Copyright 2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_RUNGE_KUTTA_DOPRI5_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_RUNGE_KUTTA_DOPRI5_HPP_INCLUDED

#include <boost/numeric/odeint/stepper/controlled_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/dense_output_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta_dopri5.hpp>
#include <boost/numeric/odeint/stepper/generation/make_controlled.hpp>
#include <boost/numeric/odeint/stepper/generation/make_dense_output.hpp>

namespace boost {
namespace numeric {
namespace odeint {


template< class State , class Value , class Deriv , class Time , class Algebra , class Operations , class Resize >
struct get_controller< runge_kutta_dopri5< State , Value , Deriv , Time , Algebra , Operations , Resize > >
{
    typedef runge_kutta_dopri5< State , Value , Deriv , Time , Algebra , Operations , Resize > stepper_type;
    typedef controlled_runge_kutta< stepper_type > type;
};


template< class State , class Value , class Deriv , class Time , class Algebra , class Operations , class Resize >
struct get_dense_output< runge_kutta_dopri5< State , Value , Deriv , Time , Algebra , Operations , Resize > >
{
    typedef runge_kutta_dopri5< State , Value , Deriv , Time , Algebra , Operations , Resize > stepper_type;
    typedef controlled_runge_kutta< stepper_type > controller_type;
    typedef dense_output_runge_kutta< controller_type > type;
};




} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_RUNGE_KUTTA_DOPRI5_HPP_INCLUDED
