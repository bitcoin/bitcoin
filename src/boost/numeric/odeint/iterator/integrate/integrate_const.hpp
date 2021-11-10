/*
 [auto_generated]
 boost/numeric/odeint/integrate/integrate_const.hpp

 [begin_description]
 Constant integration of ODEs, meaning that the state of the ODE is observed on constant time intervals.
 The routines makes full use of adaptive and dense-output methods.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_CONST_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_CONST_HPP_INCLUDED

#include <boost/type_traits/is_same.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/iterator/integrate/null_observer.hpp>
#include <boost/numeric/odeint/iterator/integrate/detail/integrate_const.hpp>
#include <boost/numeric/odeint/iterator/integrate/detail/integrate_adaptive.hpp>

namespace boost {
namespace numeric {
namespace odeint {





/*
 * Integrates with constant time step dt.
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_const(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer
)
{
    typedef typename odeint::unwrap_reference< Stepper >::type::stepper_category stepper_category;
    // we want to get as fast as possible to the end
    if( boost::is_same< null_observer , Observer >::value )
    {
        return detail::integrate_adaptive(
                stepper , system , start_state ,
                start_time , end_time  , dt ,
                observer , stepper_category() );
    }
    else
    {
        return detail::integrate_const( stepper , system , start_state , 
                                        start_time , end_time , dt ,
                                        observer , stepper_category() );
      }
}

/**
 * \brief Second version to solve the forwarding problem, 
 * can be called with Boost.Range as start_state.
 */
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_const(
        Stepper stepper , System system , const State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer
)
{
    typedef typename odeint::unwrap_reference< Stepper >::type::stepper_category stepper_category;
    // we want to get as fast as possible to the end
    if( boost::is_same< null_observer , Observer >::value )
    {
        return detail::integrate_adaptive(
                stepper , system , start_state ,
                start_time , end_time  , dt ,
                observer , stepper_category() );
    }
    else
    {
        return detail::integrate_const( stepper , system , start_state , 
                                        start_time , end_time , dt ,
                                        observer , stepper_category() );
    }
}





/**
 * \brief integrate_const without observer calls
 */
template< class Stepper , class System , class State , class Time >
size_t integrate_const(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt
)
{
    return integrate_const( stepper , system , start_state , start_time , end_time , dt , null_observer() );
}

/**
 * \brief Second version to solve the forwarding problem,
 * can be called with Boost.Range as start_state.
 */
template< class Stepper , class System , class State , class Time >
size_t integrate_const(
        Stepper stepper , System system , const State &start_state ,
        Time start_time , Time end_time , Time dt
)
{
    return integrate_const( stepper , system , start_state , start_time , end_time , dt , null_observer() );
}






/********* DOXYGEN *********/
    /**
     * \fn integrate_const( Stepper stepper , System system , State &start_state , Time start_time , Time end_time , Time dt , Observer observer )
     * \brief Integrates the ODE with constant step size.
     *
     * Integrates the ODE defined by system using the given stepper.
     * This method ensures that the observer is called at constant intervals dt.
     * If the Stepper is a normal stepper without step size control, dt is also
     * used for the numerical scheme. If a ControlledStepper is provided, the 
     * algorithm might reduce the step size to meet the error bounds, but it is 
     * ensured that the observer is always called at equidistant time points
     * t0 + n*dt. If a DenseOutputStepper is used, the step size also may vary
     * and the dense output is used to call the observer at equidistant time
     * points.
     *
     * \param stepper The stepper to be used for numerical integration.
     * \param system Function/Functor defining the rhs of the ODE.
     * \param start_state The initial condition x0.
     * \param start_time The initial time t0.
     * \param end_time The final integration time tend.
     * \param dt The time step between observer calls, _not_ necessarily the 
     * time step of the integration.
     * \param observer Function/Functor called at equidistant time intervals.
     * \return The number of steps performed.
     */

} // namespace odeint
} // namespace numeric
} // namespace boost



#endif // BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_CONST_HPP_INCLUDED
