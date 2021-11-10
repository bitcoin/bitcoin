/*
 [auto_generated]
 boost/numeric/odeint/stepper/adams_moulton.hpp

 [begin_description]
 Implementation of the Adams-Moulton method. This is method is not a real stepper, it is more a helper class
 which computes the corrector step in the Adams-Bashforth-Moulton method.
 [end_description]

 Copyright 2011-2012 Karsten Ahnert
 Copyright 2011-2013 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_MOULTON_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_MOULTON_HPP_INCLUDED


#include <boost/numeric/odeint/util/bind.hpp>

#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4_classic.hpp>

#include <boost/numeric/odeint/stepper/detail/adams_moulton_call_algebra.hpp>
#include <boost/numeric/odeint/stepper/detail/adams_moulton_coefficients.hpp>
#include <boost/numeric/odeint/stepper/detail/rotating_buffer.hpp>




namespace boost {
namespace numeric {
namespace odeint {


/*
 * Static implicit Adams-Moulton multistep-solver without step size control and without dense output.
 */
template<
size_t Steps ,
class State ,
class Value = double ,
class Deriv = State ,
class Time = Value ,
class Algebra = typename algebra_dispatcher< State >::algebra_type ,
class Operations = typename operations_dispatcher< State >::operations_type ,
class Resizer = initially_resizer
>
class adams_moulton
{
private:


public :

    typedef State state_type;
    typedef state_wrapper< state_type > wrapped_state_type;
    typedef Value value_type;
    typedef Deriv deriv_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    typedef Time time_type;
    typedef Algebra algebra_type;
    typedef Operations operations_type;
    typedef Resizer resizer_type;
    typedef stepper_tag stepper_category;

    typedef adams_moulton< Steps , State , Value , Deriv , Time , Algebra , Operations , Resizer > stepper_type;

    static const size_t steps = Steps;

    typedef unsigned short order_type;
    static const order_type order_value = steps + 1;

    typedef detail::rotating_buffer< wrapped_deriv_type , steps > step_storage_type;

    adams_moulton( )
    : m_coefficients() , m_dxdt() , m_resizer() ,
      m_algebra_instance() , m_algebra( m_algebra_instance )
    { }

    adams_moulton( algebra_type &algebra )
    : m_coefficients() , m_dxdt() , m_resizer() ,
      m_algebra_instance() , m_algebra( algebra )
    { }

    adams_moulton& operator=( const adams_moulton &stepper )
    {
        m_dxdt = stepper.m_dxdt;
        m_resizer = stepper.m_resizer;
        m_algebra = stepper.m_algebra;
        return *this;
    }

    order_type order( void ) const { return order_value; }


    /*
     * Version 1 : do_step( system , x , t , dt , buf );
     *
     * solves the forwarding problem
     */
    template< class System , class StateInOut , class StateIn , class ABBuf >
    void do_step( System system , StateInOut &x , StateIn const & pred , time_type t , time_type dt , const ABBuf &buf )
    {
        do_step( system , x , pred , t , x , dt , buf );
    }

    template< class System , class StateInOut , class StateIn , class ABBuf >
    void do_step( System system , const StateInOut &x , StateIn const & pred , time_type t , time_type dt , const ABBuf &buf )
    {
        do_step( system , x , pred , t , x , dt , buf );
    }



    /*
     * Version 2 : do_step( system , in , t , out , dt , buf );
     *
     * solves the forwarding problem
     */
    template< class System , class StateIn , class PredIn , class StateOut , class ABBuf >
    void do_step( System system , const StateIn &in , const PredIn &pred , time_type t , StateOut &out , time_type dt , const ABBuf &buf )
    {
        do_step_impl( system , in , pred , t , out , dt , buf );
    }

    template< class System , class StateIn , class PredIn , class StateOut , class ABBuf >
    void do_step( System system , const StateIn &in , const PredIn &pred , time_type t , const StateOut &out , time_type dt , const ABBuf &buf )
    {
        do_step_impl( system , in , pred , t , out , dt , buf );
    }



    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_impl( x );
    }

    algebra_type& algebra()
    {   return m_algebra; }

    const algebra_type& algebra() const
    {   return m_algebra; }


private:


    template< class System , class StateIn , class PredIn , class StateOut , class ABBuf >
    void do_step_impl( System system , const StateIn &in , const PredIn &pred , time_type t , StateOut &out , time_type dt , const ABBuf &buf )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_resizer.adjust_size( in , detail::bind( &stepper_type::template resize_impl<StateIn> , detail::ref( *this ) , detail::_1 ) );
        sys( pred , m_dxdt.m_v , t );
        detail::adams_moulton_call_algebra< steps , algebra_type , operations_type >()( m_algebra , in , out , m_dxdt.m_v , buf , m_coefficients , dt );
    }


    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
    }


    const detail::adams_moulton_coefficients< value_type , steps > m_coefficients;
    wrapped_deriv_type m_dxdt;
    resizer_type m_resizer;

protected:

    algebra_type m_algebra_instance;
    algebra_type &m_algebra;
};




} // odeint
} // numeric
} // boost



#endif // BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_MOULTON_HPP_INCLUDED
