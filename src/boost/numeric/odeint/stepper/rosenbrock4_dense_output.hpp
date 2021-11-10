/*
 [auto_generated]
 boost/numeric/odeint/stepper/rosenbrock4_dense_output.hpp

 [begin_description]
 Dense output for Rosenbrock 4.
 [end_description]

 Copyright 2011-2012 Karsten Ahnert
 Copyright 2011-2015 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_DENSE_OUTPUT_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_DENSE_OUTPUT_HPP_INCLUDED


#include <utility>

#include <boost/numeric/odeint/util/bind.hpp>

#include <boost/numeric/odeint/stepper/rosenbrock4_controller.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>

#include <boost/numeric/odeint/integrate/max_step_checker.hpp>


namespace boost {
namespace numeric {
namespace odeint {

template< class ControlledStepper >
class rosenbrock4_dense_output
{

public:

    typedef ControlledStepper controlled_stepper_type;
    typedef typename unwrap_reference< controlled_stepper_type >::type unwrapped_controlled_stepper_type;
    typedef typename unwrapped_controlled_stepper_type::stepper_type stepper_type;
    typedef typename stepper_type::value_type value_type;
    typedef typename stepper_type::state_type state_type;
    typedef typename stepper_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_type::time_type time_type;
    typedef typename stepper_type::deriv_type deriv_type;
    typedef typename stepper_type::wrapped_deriv_type wrapped_deriv_type;
    typedef typename stepper_type::resizer_type resizer_type;
    typedef dense_output_stepper_tag stepper_category;

    typedef rosenbrock4_dense_output< ControlledStepper > dense_output_stepper_type;

    rosenbrock4_dense_output( const controlled_stepper_type &stepper = controlled_stepper_type() )
    : m_stepper( stepper ) , 
      m_x1() , m_x2() , 
      m_current_state_x1( true ) ,
      m_t() , m_t_old() , m_dt()
    {
    }



    template< class StateType >
    void initialize( const StateType &x0 , time_type t0 , time_type dt0 )
    {
        m_resizer.adjust_size( x0 , detail::bind( &dense_output_stepper_type::template resize_impl< StateType > , detail::ref( *this ) , detail::_1 ) );
        get_current_state() = x0;
        m_t = t0;
        m_dt = dt0;
    }

    template< class System >
    std::pair< time_type , time_type > do_step( System system )
    {
        unwrapped_controlled_stepper_type &stepper = m_stepper;
        failed_step_checker fail_checker;  // to throw a runtime_error if step size adjustment fails
        controlled_step_result res = fail;
        m_t_old = m_t;
        do
        {
            res = stepper.try_step( system , get_current_state() , m_t , get_old_state() , m_dt );
            fail_checker();  // check for overflow of failed steps
        }
        while( res == fail );
        stepper.stepper().prepare_dense_output();
        this->toggle_current_state();
        return std::make_pair( m_t_old , m_t );
    }


    /*
     * The two overloads are needed in order to solve the forwarding problem.
     */
    template< class StateOut >
    void calc_state( time_type t , StateOut &x )
    {
        unwrapped_controlled_stepper_type &stepper = m_stepper;
        stepper.stepper().calc_state( t , x , get_old_state() , m_t_old , get_current_state() , m_t );
    }

    template< class StateOut >
    void calc_state( time_type t , const StateOut &x )
    {
        unwrapped_controlled_stepper_type &stepper = m_stepper;
        stepper.stepper().calc_state( t , x , get_old_state() , m_t_old , get_current_state() , m_t );
    }


    template< class StateType >
    void adjust_size( const StateType &x )
    {
        unwrapped_controlled_stepper_type &stepper = m_stepper;
        stepper.adjust_size( x );
        resize_impl( x );
    }




    const state_type& current_state( void ) const
    {
        return get_current_state();
    }

    time_type current_time( void ) const
    {
        return m_t;
    }

    const state_type& previous_state( void ) const
    {
        return get_old_state();
    }

    time_type previous_time( void ) const
    {
        return m_t_old;
    }

    time_type current_time_step( void ) const
    {
        return m_dt;
    }




private:

    state_type& get_current_state( void )
    {
        return m_current_state_x1 ? m_x1.m_v : m_x2.m_v ;
    }
    
    const state_type& get_current_state( void ) const
    {
        return m_current_state_x1 ? m_x1.m_v : m_x2.m_v ;
    }
    
    state_type& get_old_state( void )
    {
        return m_current_state_x1 ? m_x2.m_v : m_x1.m_v ;
    }
    
    const state_type& get_old_state( void ) const
    {
        return m_current_state_x1 ? m_x2.m_v : m_x1.m_v ;
    }

    void toggle_current_state( void )
    {
        m_current_state_x1 = ! m_current_state_x1;
    }


    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized = false;
        resized |= adjust_size_by_resizeability( m_x1 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_x2 , x , typename is_resizeable<state_type>::type() );
        return resized;
    }


    controlled_stepper_type m_stepper;
    resizer_type m_resizer;
    wrapped_state_type m_x1 , m_x2;
    bool m_current_state_x1;
    time_type m_t , m_t_old , m_dt;
};



} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_DENSE_OUTPUT_HPP_INCLUDED
