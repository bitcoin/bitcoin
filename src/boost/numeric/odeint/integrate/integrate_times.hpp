/*
 [auto_generated]
 boost/numeric/odeint/integrate/integrate_times.hpp

 [begin_description]
 Integration of ODEs with observation at user defined points
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011-2015 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_TIMES_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_TIMES_HPP_INCLUDED

#include <boost/type_traits/is_same.hpp>

#include <boost/range.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/integrate/null_observer.hpp>
#include <boost/numeric/odeint/integrate/check_adapter.hpp>
#include <boost/numeric/odeint/integrate/detail/integrate_times.hpp>

namespace boost {
namespace numeric {
namespace odeint {


/*
 * \brief Integrates while calling the observer at the time points given by sequence [times_start, time_end)
 * the two overloads are needed in order to solve the forwarding problem
 */
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer , class StepOverflowChecker >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        TimeIterator times_start , TimeIterator times_end , Time dt ,
        Observer observer , StepOverflowChecker checker )
{
    // unwrap references
    typedef typename odeint::unwrap_reference< Stepper >::type stepper_type;
    typedef typename odeint::unwrap_reference< Observer >::type observer_type;
    typedef typename odeint::unwrap_reference< StepOverflowChecker >::type checker_type;
    typedef typename stepper_type::stepper_category stepper_category;

    // pass on checked stepper and observer
    // checked_stepper/observer use references internally, so passing by value is fine
    return detail::integrate_times(
            checked_stepper<stepper_type, checker_type>(stepper, checker) ,
            system , start_state ,
            times_start , times_end , dt ,
            checked_observer<observer_type, checker_type>(observer, checker),
            stepper_category() );
}

/**
 * \brief Solves the forwarding problem, can be called with Boost.Range as start_state.
 */
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer , class StepOverflowChecker >
size_t integrate_times(
        Stepper stepper , System system , const State &start_state ,
        TimeIterator times_start , TimeIterator times_end , Time dt ,
        Observer observer , StepOverflowChecker checker )
{
    typedef typename odeint::unwrap_reference< Stepper >::type stepper_type;
    typedef typename odeint::unwrap_reference< Observer >::type observer_type;
    typedef typename odeint::unwrap_reference< StepOverflowChecker >::type checker_type;
    typedef typename stepper_type::stepper_category stepper_category;

    stepper_type &st = stepper;
    observer_type &obs = observer;
    checker_type &chk = checker;

    return detail::integrate_times(
            checked_stepper<stepper_type, checker_type>(stepper, checker) ,
            system , start_state ,
            times_start , times_end , dt ,
            checked_observer<observer_type, checker_type>(observer, checker),
            stepper_category() );
}

/**
 * \brief The same function as above, but with the observation times given as range.
 */
template< class Stepper , class System , class State , class TimeRange , class Time , class Observer , class StepOverflowChecker >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        const TimeRange &times , Time dt ,
        Observer observer , StepOverflowChecker checker )
{
    return integrate_times(
            stepper , system , start_state ,
            boost::begin( times ) , boost::end( times ) , dt , observer , checker );
}

/**
 * \brief Solves the forwarding problem, can be called with Boost.Range as start_state.
 */
template< class Stepper , class System , class State , class TimeRange , class Time , class Observer , class StepOverflowChecker >
size_t integrate_times(
        Stepper stepper , System system , const State &start_state ,
        const TimeRange &times , Time dt ,
        Observer observer , StepOverflowChecker checker )
{
    return integrate_times(
            stepper , system , start_state ,
            boost::begin( times ) , boost::end( times ) , dt , observer , checker );
}




/*
* The same functions as above, but without a StepOverflowChecker
*/
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        TimeIterator times_start , TimeIterator times_end , Time dt ,
        Observer observer )
{
    typedef typename odeint::unwrap_reference< Stepper >::type::stepper_category stepper_category;
    // simply don't use checked_* adapters
    return detail::integrate_times(
            stepper , system , start_state ,
            times_start , times_end , dt ,
            observer , stepper_category() );
}

/**
* \brief Solves the forwarding problem, can be called with Boost.Range as start_state.
*/
template< class Stepper , class System , class State , class TimeIterator , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , const State &start_state ,
        TimeIterator times_start , TimeIterator times_end , Time dt ,
        Observer observer )
{
    typedef typename odeint::unwrap_reference< Stepper >::type::stepper_category stepper_category;
    return detail::integrate_times(
            stepper , system , start_state ,
            times_start , times_end , dt ,
            observer , stepper_category() );
}

/**
* \brief The same function as above, but with the observation times given as range.
*/
template< class Stepper , class System , class State , class TimeRange , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , State &start_state ,
        const TimeRange &times , Time dt ,
        Observer observer )
{
    return integrate_times(
            stepper , system , start_state ,
            boost::begin( times ) , boost::end( times ) , dt , observer );
}

/**
* \brief Solves the forwarding problem, can be called with Boost.Range as start_state.
*/
template< class Stepper , class System , class State , class TimeRange , class Time , class Observer >
size_t integrate_times(
        Stepper stepper , System system , const State &start_state ,
        const TimeRange &times , Time dt ,
        Observer observer )
{
    return integrate_times(
            stepper , system , start_state ,
            boost::begin( times ) , boost::end( times ) , dt , observer);
}


/********* DOXYGEN ***********/

    /**
     * \fn size_t integrate_times( Stepper stepper , System system , State &start_state , TimeIterator times_start , TimeIterator times_end , Time dt , Observer observer )
     * \brief Integrates the ODE with observer calls at given time points.
     *
     * Integrates the ODE given by system using the given stepper. This function
     * does observer calls at the subsequent time points given by the range 
     * times_start, times_end. If the stepper has not step size control, the 
     * step size might be reduced occasionally to ensure observer calls exactly
     * at the time points from the given sequence. If the stepper is a 
     * ControlledStepper, the step size is adjusted to meet the error bounds, 
     * but also might be reduced occasionally to ensure correct observer calls.
     * If a DenseOutputStepper is provided, the dense output functionality is
     * used to call the observer at the given times. The end time of the 
     * integration is always *(end_time-1).
     * If a max_step_checker is provided as StepOverflowChecker, a
     * no_progress_error is thrown if too many steps (default: 500) are
     * performed without progress, i.e. in between observer calls. If no
     * checker is provided, no such overflow check is performed.
     *
     * \param stepper The stepper to be used for numerical integration.
     * \param system Function/Functor defining the rhs of the ODE.
     * \param start_state The initial condition x0.
     * \param times_start Iterator to the start time
     * \param times_end Iterator to the end time
     * \param dt The time step between observer calls, _not_ necessarily the 
     * time step of the integration.
     * \param observer Function/Functor called at equidistant time intervals.
     * \param checker [optional] Functor to check for step count overflows, if no
     * checker is provided, no exception is thrown.
     * \return The number of steps performed.
     */



} // namespace odeint
} // namespace numeric
} // namespace boost



#endif // BOOST_NUMERIC_ODEINT_INTEGRATE_INTEGRATE_TIMES_HPP_INCLUDED
