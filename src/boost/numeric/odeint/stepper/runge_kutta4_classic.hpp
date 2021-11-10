/*
 [auto_generated]
 boost/numeric/odeint/stepper/runge_kutta4_classic.hpp

 [begin_description]
 Implementation for the classical Runge Kutta stepper.
 [end_description]

 Copyright 2010-2013 Karsten Ahnert
 Copyright 2010-2013 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA4_CLASSIC_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA4_CLASSIC_HPP_INCLUDED



#include <boost/numeric/odeint/stepper/base/explicit_stepper_base.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

namespace boost {
namespace numeric {
namespace odeint {

template<
class State ,
class Value = double ,
class Deriv = State ,
class Time = Value ,
class Algebra = typename algebra_dispatcher< State >::algebra_type ,
class Operations = typename operations_dispatcher< State >::operations_type ,
class Resizer = initially_resizer
>
#ifndef DOXYGEN_SKIP
class runge_kutta4_classic
: public explicit_stepper_base<
  runge_kutta4_classic< State , Value , Deriv , Time , Algebra , Operations , Resizer > ,
  4 , State , Value , Deriv , Time , Algebra , Operations , Resizer >
#else
class runge_kutta4_classic : public explicit_stepper_base
#endif
{

public :

    #ifndef DOXYGEN_SKIP
    typedef explicit_stepper_base<
    runge_kutta4_classic< State , Value , Deriv , Time , Algebra , Operations , Resizer > ,
    4 , State , Value , Deriv , Time , Algebra , Operations , Resizer > stepper_base_type;
    #else
    typedef explicit_stepper_base< runge_kutta4_classic< ... > , ... > stepper_base_type;
    #endif

    typedef typename stepper_base_type::state_type state_type;
    typedef typename stepper_base_type::value_type value_type;
    typedef typename stepper_base_type::deriv_type deriv_type;
    typedef typename stepper_base_type::time_type time_type;
    typedef typename stepper_base_type::algebra_type algebra_type;
    typedef typename stepper_base_type::operations_type operations_type;
    typedef typename stepper_base_type::resizer_type resizer_type;

    #ifndef DOXYGEN_SKIP
    typedef typename stepper_base_type::stepper_type stepper_type;
    typedef typename stepper_base_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_base_type::wrapped_deriv_type wrapped_deriv_type;
    #endif // DOXYGEN_SKIP



    runge_kutta4_classic( const algebra_type &algebra = algebra_type() ) : stepper_base_type( algebra )
    { }


    template< class System , class StateIn , class DerivIn , class StateOut >
    void do_step_impl( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
    {
        // ToDo : check if size of in,dxdt,out are equal?

        static const value_type val1 = static_cast< value_type >( 1 );

        m_resizer.adjust_size( in , detail::bind( &stepper_type::template resize_impl< StateIn > , detail::ref( *this ) , detail::_1 ) );

        typename odeint::unwrap_reference< System >::type &sys = system;

        const time_type dh = dt / static_cast< value_type >( 2 );
        const time_type th = t + dh;

        // dt * dxdt = k1
        // m_x_tmp = x + dh*dxdt
        stepper_base_type::m_algebra.for_each3( m_x_tmp.m_v , in , dxdt ,
                typename operations_type::template scale_sum2< value_type , time_type >( val1 , dh ) );


        // dt * m_dxt = k2
        sys( m_x_tmp.m_v , m_dxt.m_v , th );

        // m_x_tmp = x + dh*m_dxt
        stepper_base_type::m_algebra.for_each3( m_x_tmp.m_v , in , m_dxt.m_v ,
                typename operations_type::template scale_sum2< value_type , time_type >( val1 , dh ) );


        // dt * m_dxm = k3
        sys( m_x_tmp.m_v , m_dxm.m_v , th );
        //m_x_tmp = x + dt*m_dxm
        stepper_base_type::m_algebra.for_each3( m_x_tmp.m_v , in , m_dxm.m_v ,
                typename operations_type::template scale_sum2< value_type , time_type >( val1 , dt ) );


        // dt * m_dxh = k4
        sys( m_x_tmp.m_v , m_dxh.m_v , t + dt );

        //x += dt/6 * ( m_dxdt + m_dxt + val2*m_dxm )
        time_type dt6 = dt / static_cast< value_type >( 6 );
        time_type dt3 = dt / static_cast< value_type >( 3 );
        stepper_base_type::m_algebra.for_each6( out , in , dxdt , m_dxt.m_v , m_dxm.m_v , m_dxh.m_v ,
                                             typename operations_type::template scale_sum5< value_type , time_type , time_type , time_type , time_type >( 1.0 , dt6 , dt3 , dt3 , dt6 ) );
        
        // x += dt/6 * m_dxdt + dt/3 * m_dxt )
        // stepper_base_type::m_algebra.for_each4( out , in , dxdt , m_dxt.m_v , 
        //                                         typename operations_type::template scale_sum3< value_type , time_type , time_type >( 1.0 , dt6 , dt3 ) ); 
        // // x += dt/3 * m_dxm + dt/6 * m_dxh )
        // stepper_base_type::m_algebra.for_each4( out , out , m_dxm.m_v , m_dxh.m_v , 
        //                                         typename operations_type::template scale_sum3< value_type , time_type , time_type >( 1.0 , dt3 , dt6 ) ); 

    }

    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_impl( x );
        stepper_base_type::adjust_size( x );
    }

private:

    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized = false;
        resized |= adjust_size_by_resizeability( m_x_tmp , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_dxm , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_dxt , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_dxh , x , typename is_resizeable<deriv_type>::type() );
        return resized;
    }


    resizer_type m_resizer;

    wrapped_deriv_type m_dxt;
    wrapped_deriv_type m_dxm;
    wrapped_deriv_type m_dxh;
    wrapped_state_type m_x_tmp;

};


/********* DOXYGEN *********/

/**
 * \class runge_kutta4_classic
 * \brief The classical Runge-Kutta stepper of fourth order.
 *
 * The Runge-Kutta method of fourth order is one standard method for
 * solving ordinary differential equations and is widely used, see also
 * <a href="http://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods">en.wikipedia.org/wiki/Runge-Kutta_methods</a>
 * The method is explicit and fulfills the Stepper concept. Step size control
 * or continuous output are not provided.  This class implements the method directly, hence the 
 * generic Runge-Kutta algorithm is not used.
 * 
 * This class derives from explicit_stepper_base and inherits its interface via
 * CRTP (current recurring template pattern). For more details see
 * explicit_stepper_base.
 *
 * \tparam State The state type.
 * \tparam Value The value type.
 * \tparam Deriv The type representing the time derivative of the state.
 * \tparam Time The time representing the independent variable - the time.
 * \tparam Algebra The algebra type.
 * \tparam Operations The operations type.
 * \tparam Resizer The resizer policy type.
 */

    /**
     * \fn runge_kutta4_classic::runge_kutta4_classic( const algebra_type &algebra )
     * \brief Constructs the runge_kutta4_classic class. This constructor can be used as a default
     * constructor if the algebra has a default constructor. 
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */


    /**
     * \fn runge_kutta4_classic::do_step_impl( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
     * \brief This method performs one step. The derivative `dxdt` of `in` at the time `t` is passed to the method.
     * The result is updated out of place, hence the input is in `in` and the output in `out`.
     * Access to this step functionality is provided by explicit_stepper_base and 
     * `do_step_impl` should not be called directly.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param dxdt The derivative of x at t.
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dt The step size.
     */

    /**
     * \fn runge_kutta4_classic::adjust_size( const StateType &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA4_CLASSIC_HPP_INCLUDED
