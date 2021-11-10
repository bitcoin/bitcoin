/*
 [auto_generated]
 boost/numeric/odeint/stepper/generation/generation_dense_output_runge_kutta.hpp

 [begin_description]
 Specialization of the controller factory for the dense_output_runge_kutta class.
 [end_description]

 Copyright 2011-2012 Karsten Ahnert
 Copyright 2011-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_DENSE_OUTPUT_RUNGE_KUTTA_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_DENSE_OUTPUT_RUNGE_KUTTA_HPP_INCLUDED

#include <boost/numeric/odeint/stepper/controlled_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/dense_output_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/generation/make_dense_output.hpp>

namespace boost {
namespace numeric {
namespace odeint {

// controller factory for controlled_runge_kutta
template< class Stepper >
struct dense_output_factory< Stepper , dense_output_runge_kutta< controlled_runge_kutta< Stepper > > >
{
    typedef Stepper stepper_type;
    typedef controlled_runge_kutta< stepper_type > controller_type;
    typedef typename controller_type::error_checker_type error_checker_type;
    typedef typename controller_type::step_adjuster_type step_adjuster_type;
    typedef typename stepper_type::value_type value_type;
    typedef typename stepper_type::time_type time_type;
    typedef dense_output_runge_kutta< controller_type > dense_output_type;

    dense_output_type operator()( value_type abs_error , value_type rel_error , const stepper_type &stepper )
    {
        return dense_output_type( controller_type( error_checker_type( abs_error , rel_error ) ,
                                                   step_adjuster_type() , stepper ) );
    }

    dense_output_type operator()( value_type abs_error , value_type rel_error ,
                                  time_type max_dt , const stepper_type &stepper )
    {
        return dense_output_type(
                controller_type( error_checker_type( abs_error , rel_error) ,
                                 step_adjuster_type( max_dt ) , stepper ) );
    }
};





} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_GENERATION_GENERATION_DENSE_OUTPUT_RUNGE_KUTTA_HPP_INCLUDED
