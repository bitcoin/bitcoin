/*
 [auto_generated]
 boost/numeric/odeint/integrate/detail/integrate_const.hpp

 [begin_description]
 integrate const implementation
 [end_description]

 Copyright 2012-2015 Mario Mulansky
 Copyright 2012 Christoph Koke
 Copyright 2012 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_CONST_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_INTEGRATE_DETAIL_INTEGRATE_CONST_HPP_INCLUDED

#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/util/unit_helper.hpp>
#include <boost/numeric/odeint/integrate/detail/integrate_adaptive.hpp>

#include <boost/numeric/odeint/util/detail/less_with_sign.hpp>

namespace boost {
namespace numeric {
namespace odeint {
namespace detail {

// forward declaration
template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_adaptive(
        Stepper stepper , System system , State &start_state ,
        Time &start_time , Time end_time , Time &dt ,
        Observer observer , controlled_stepper_tag
);


template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_const(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , stepper_tag
)
{

    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;

    Time time = start_time;
    int step = 0;
    // cast time+dt explicitely in case of expression templates (e.g. multiprecision)
    while( less_eq_with_sign( static_cast<Time>(time+dt) , end_time , dt ) )
    {
        obs( start_state , time );
        st.do_step( system , start_state , time , dt );
        // direct computation of the time avoids error propagation happening when using time += dt
        // we need clumsy type analysis to get boost units working here
        ++step;
        time = start_time + static_cast< typename unit_value_type<Time>::type >(step) * dt;
    }
    obs( start_state , time );

    return step;
}



template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_const(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , controlled_stepper_tag
)
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;

    Time time = start_time;
    const Time time_step = dt;
    int real_steps = 0;
    int step = 0;
    
    while( less_eq_with_sign( static_cast<Time>(time+time_step) , end_time , dt ) )
    {
        obs( start_state , time );
        // integrate_adaptive_checked uses the given checker to throw if an overflow occurs
        real_steps += detail::integrate_adaptive(stepper, system, start_state, time,
                                                 static_cast<Time>(time + time_step), dt,
                                                 null_observer(), controlled_stepper_tag());
        // direct computation of the time avoids error propagation happening when using time += dt
        // we need clumsy type analysis to get boost units working here
        step++;
        time = start_time + static_cast< typename unit_value_type<Time>::type >(step) * time_step;
    }
    obs( start_state , time );

    return real_steps;
}


template< class Stepper , class System , class State , class Time , class Observer >
size_t integrate_const(
        Stepper stepper , System system , State &start_state ,
        Time start_time , Time end_time , Time dt ,
        Observer observer , dense_output_stepper_tag
)
{
    typename odeint::unwrap_reference< Observer >::type &obs = observer;
    typename odeint::unwrap_reference< Stepper >::type &st = stepper;

    Time time = start_time;
    
    st.initialize( start_state , time , dt );
    obs( start_state , time );
    time += dt;

    int obs_step( 1 );
    int real_step( 0 );
    
    while( less_eq_with_sign( static_cast<Time>(time+dt) , end_time , dt ) )
    {
        while( less_eq_with_sign( time , st.current_time() , dt ) )
        {
            st.calc_state( time , start_state );
            obs( start_state , time );
            ++obs_step;
            // direct computation of the time avoids error propagation happening when using time += dt
            // we need clumsy type analysis to get boost units working here
            time = start_time + static_cast< typename unit_value_type<Time>::type >(obs_step) * dt;
        }
        // we have not reached the end, do another real step
        if( less_with_sign( static_cast<Time>(st.current_time()+st.current_time_step()) ,
                            end_time ,
                            st.current_time_step() ) )
        {
            while( less_eq_with_sign( st.current_time() , time , dt ) )
            {
                st.do_step( system );
                ++real_step;
            }
        }
        else if( less_with_sign( st.current_time() , end_time , st.current_time_step() ) )
        { // do the last step ending exactly on the end point
            st.initialize( st.current_state() , st.current_time() , end_time - st.current_time() );
            st.do_step( system );
            ++real_step;
        }
        
    }
    // last observation, if we are still in observation interval
    // might happen due to finite precision problems when computing the the time
    if( less_eq_with_sign( time , end_time , dt ) )
    {
        st.calc_state( time , start_state );
        obs( start_state , time );
    }
    
    return real_step;
}


} } } }

#endif
