/*
 [auto_generated]
 boost/numeric/odeint/stepper/runge_kutta_dopri5.hpp

 [begin_description]
 Implementation of the Dormand-Prince 5(4) method. This stepper can also be used with the dense-output controlled stepper.
 [end_description]

 Copyright 2010-2013 Karsten Ahnert
 Copyright 2010-2013 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA_DOPRI5_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA_DOPRI5_HPP_INCLUDED


#include <boost/numeric/odeint/util/bind.hpp>

#include <boost/numeric/odeint/stepper/base/explicit_error_stepper_fsal_base.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>
#include <boost/numeric/odeint/util/same_instance.hpp>

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
class runge_kutta_dopri5
#ifndef DOXYGEN_SKIP
: public explicit_error_stepper_fsal_base<
  runge_kutta_dopri5< State , Value , Deriv , Time , Algebra , Operations , Resizer > ,
  5 , 5 , 4 , State , Value , Deriv , Time , Algebra , Operations , Resizer >
#else
: public explicit_error_stepper_fsal_base
#endif
{

public :

    #ifndef DOXYGEN_SKIP
    typedef explicit_error_stepper_fsal_base<
    runge_kutta_dopri5< State , Value , Deriv , Time , Algebra , Operations , Resizer > ,
    5 , 5 , 4 , State , Value , Deriv , Time , Algebra , Operations , Resizer > stepper_base_type;
    #else
    typedef explicit_error_stepper_fsal_base< runge_kutta_dopri5< ... > , ... > stepper_base_type;
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


    runge_kutta_dopri5( const algebra_type &algebra = algebra_type() ) : stepper_base_type( algebra )
    { }


    template< class System , class StateIn , class DerivIn , class StateOut , class DerivOut >
    void do_step_impl( System system , const StateIn &in , const DerivIn &dxdt_in , time_type t ,
            StateOut &out , DerivOut &dxdt_out , time_type dt )
    {
        const value_type a2 = static_cast<value_type> ( 1 ) / static_cast<value_type>( 5 );
        const value_type a3 = static_cast<value_type> ( 3 ) / static_cast<value_type> ( 10 );
        const value_type a4 = static_cast<value_type> ( 4 ) / static_cast<value_type> ( 5 );
        const value_type a5 = static_cast<value_type> ( 8 )/static_cast<value_type> ( 9 );

        const value_type b21 = static_cast<value_type> ( 1 ) / static_cast<value_type> ( 5 );

        const value_type b31 = static_cast<value_type> ( 3 ) / static_cast<value_type>( 40 );
        const value_type b32 = static_cast<value_type> ( 9 ) / static_cast<value_type>( 40 );

        const value_type b41 = static_cast<value_type> ( 44 ) / static_cast<value_type> ( 45 );
        const value_type b42 = static_cast<value_type> ( -56 ) / static_cast<value_type> ( 15 );
        const value_type b43 = static_cast<value_type> ( 32 ) / static_cast<value_type> ( 9 );

        const value_type b51 = static_cast<value_type> ( 19372 ) / static_cast<value_type>( 6561 );
        const value_type b52 = static_cast<value_type> ( -25360 ) / static_cast<value_type> ( 2187 );
        const value_type b53 = static_cast<value_type> ( 64448 ) / static_cast<value_type>( 6561 );
        const value_type b54 = static_cast<value_type> ( -212 ) / static_cast<value_type>( 729 );

        const value_type b61 = static_cast<value_type> ( 9017 ) / static_cast<value_type>( 3168 );
        const value_type b62 = static_cast<value_type> ( -355 ) / static_cast<value_type>( 33 );
        const value_type b63 = static_cast<value_type> ( 46732 ) / static_cast<value_type>( 5247 );
        const value_type b64 = static_cast<value_type> ( 49 ) / static_cast<value_type>( 176 );
        const value_type b65 = static_cast<value_type> ( -5103 ) / static_cast<value_type>( 18656 );

        const value_type c1 = static_cast<value_type> ( 35 ) / static_cast<value_type>( 384 );
        const value_type c3 = static_cast<value_type> ( 500 ) / static_cast<value_type>( 1113 );
        const value_type c4 = static_cast<value_type> ( 125 ) / static_cast<value_type>( 192 );
        const value_type c5 = static_cast<value_type> ( -2187 ) / static_cast<value_type>( 6784 );
        const value_type c6 = static_cast<value_type> ( 11 ) / static_cast<value_type>( 84 );

        typename odeint::unwrap_reference< System >::type &sys = system;

        m_k_x_tmp_resizer.adjust_size( in , detail::bind( &stepper_type::template resize_k_x_tmp_impl<StateIn> , detail::ref( *this ) , detail::_1 ) );

        //m_x_tmp = x + dt*b21*dxdt
        stepper_base_type::m_algebra.for_each3( m_x_tmp.m_v , in , dxdt_in ,
                typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , dt*b21 ) );

        sys( m_x_tmp.m_v , m_k2.m_v , t + dt*a2 );
        // m_x_tmp = x + dt*b31*dxdt + dt*b32*m_k2
        stepper_base_type::m_algebra.for_each4( m_x_tmp.m_v , in , dxdt_in , m_k2.m_v ,
                typename operations_type::template scale_sum3< value_type , time_type , time_type >( 1.0 , dt*b31 , dt*b32 ));

        sys( m_x_tmp.m_v , m_k3.m_v , t + dt*a3 );
        // m_x_tmp = x + dt * (b41*dxdt + b42*m_k2 + b43*m_k3)
        stepper_base_type::m_algebra.for_each5( m_x_tmp.m_v , in , dxdt_in , m_k2.m_v , m_k3.m_v ,
                typename operations_type::template scale_sum4< value_type , time_type , time_type , time_type >( 1.0 , dt*b41 , dt*b42 , dt*b43 ));

        sys( m_x_tmp.m_v, m_k4.m_v , t + dt*a4 );
        stepper_base_type::m_algebra.for_each6( m_x_tmp.m_v , in , dxdt_in , m_k2.m_v , m_k3.m_v , m_k4.m_v ,
                typename operations_type::template scale_sum5< value_type , time_type , time_type , time_type , time_type >( 1.0 , dt*b51 , dt*b52 , dt*b53 , dt*b54 ));

        sys( m_x_tmp.m_v , m_k5.m_v , t + dt*a5 );
        stepper_base_type::m_algebra.for_each7( m_x_tmp.m_v , in , dxdt_in , m_k2.m_v , m_k3.m_v , m_k4.m_v , m_k5.m_v ,
                typename operations_type::template scale_sum6< value_type , time_type , time_type , time_type , time_type , time_type >( 1.0 , dt*b61 , dt*b62 , dt*b63 , dt*b64 , dt*b65 ));

        sys( m_x_tmp.m_v , m_k6.m_v , t + dt );
        stepper_base_type::m_algebra.for_each7( out , in , dxdt_in , m_k3.m_v , m_k4.m_v , m_k5.m_v , m_k6.m_v ,
                typename operations_type::template scale_sum6< value_type , time_type , time_type , time_type , time_type , time_type >( 1.0 , dt*c1 , dt*c3 , dt*c4 , dt*c5 , dt*c6 ));

        // the new derivative
        sys( out , dxdt_out , t + dt );
    }



    template< class System , class StateIn , class DerivIn , class StateOut , class DerivOut , class Err >
    void do_step_impl( System system , const StateIn &in , const DerivIn &dxdt_in , time_type t ,
            StateOut &out , DerivOut &dxdt_out , time_type dt , Err &xerr )
    {
        const value_type c1 = static_cast<value_type> ( 35 ) / static_cast<value_type>( 384 );
        const value_type c3 = static_cast<value_type> ( 500 ) / static_cast<value_type>( 1113 );
        const value_type c4 = static_cast<value_type> ( 125 ) / static_cast<value_type>( 192 );
        const value_type c5 = static_cast<value_type> ( -2187 ) / static_cast<value_type>( 6784 );
        const value_type c6 = static_cast<value_type> ( 11 ) / static_cast<value_type>( 84 );

        const value_type dc1 = c1 - static_cast<value_type> ( 5179 ) / static_cast<value_type>( 57600 );
        const value_type dc3 = c3 - static_cast<value_type> ( 7571 ) / static_cast<value_type>( 16695 );
        const value_type dc4 = c4 - static_cast<value_type> ( 393 ) / static_cast<value_type>( 640 );
        const value_type dc5 = c5 - static_cast<value_type> ( -92097 ) / static_cast<value_type>( 339200 );
        const value_type dc6 = c6 - static_cast<value_type> ( 187 ) / static_cast<value_type>( 2100 );
        const value_type dc7 = static_cast<value_type>( -1 ) / static_cast<value_type> ( 40 );

        /* ToDo: copy only if &dxdt_in == &dxdt_out ? */
        if( same_instance( dxdt_in , dxdt_out ) )
        {
            m_dxdt_tmp_resizer.adjust_size( in , detail::bind( &stepper_type::template resize_dxdt_tmp_impl<StateIn> , detail::ref( *this ) , detail::_1 ) );
            boost::numeric::odeint::copy( dxdt_in , m_dxdt_tmp.m_v );
            do_step_impl( system , in , dxdt_in , t , out , dxdt_out , dt );
            //error estimate
            stepper_base_type::m_algebra.for_each7( xerr , m_dxdt_tmp.m_v , m_k3.m_v , m_k4.m_v , m_k5.m_v , m_k6.m_v , dxdt_out ,
                                                    typename operations_type::template scale_sum6< time_type , time_type , time_type , time_type , time_type , time_type >( dt*dc1 , dt*dc3 , dt*dc4 , dt*dc5 , dt*dc6 , dt*dc7 ) );

        }
        else
        {
            do_step_impl( system , in , dxdt_in , t , out , dxdt_out , dt );
            //error estimate
            stepper_base_type::m_algebra.for_each7( xerr , dxdt_in , m_k3.m_v , m_k4.m_v , m_k5.m_v , m_k6.m_v , dxdt_out ,
                                                    typename operations_type::template scale_sum6< time_type , time_type , time_type , time_type , time_type , time_type >( dt*dc1 , dt*dc3 , dt*dc4 , dt*dc5 , dt*dc6 , dt*dc7 ) );
        
        }

    }


    /*
     * Calculates Dense-Output for Dopri5
     *
     * See Hairer, Norsett, Wanner: Solving Ordinary Differential Equations, Nonstiff Problems. I, p.191/192
     *
     * y(t+theta) = y(t) + h * sum_i^7 b_i(theta) * k_i
     *
     * A = theta^2 * ( 3 - 2 theta )
     * B = theta^2 * ( theta - 1 )
     * C = theta^2 * ( theta - 1 )^2
     * D = theta   * ( theta - 1 )^2
     *
     * b_1( theta ) = A * b_1 - C * X1( theta ) + D
     * b_2( theta ) = 0
     * b_3( theta ) = A * b_3 + C * X3( theta )
     * b_4( theta ) = A * b_4 - C * X4( theta )
     * b_5( theta ) = A * b_5 + C * X5( theta )
     * b_6( theta ) = A * b_6 - C * X6( theta )
     * b_7( theta ) = B + C * X7( theta )
     *
     * An alternative Method is described in:
     *
     * www-m2.ma.tum.de/homepages/simeon/numerik3/kap3.ps
     */
    template< class StateOut , class StateIn1 , class DerivIn1 , class StateIn2 , class DerivIn2 >
    void calc_state( time_type t , StateOut &x ,
                     const StateIn1 &x_old , const DerivIn1 &deriv_old , time_type t_old ,
                     const StateIn2 & /* x_new */ , const DerivIn2 &deriv_new , time_type t_new ) const
    {
        const value_type b1 = static_cast<value_type> ( 35 ) / static_cast<value_type>( 384 );
        const value_type b3 = static_cast<value_type> ( 500 ) / static_cast<value_type>( 1113 );
        const value_type b4 = static_cast<value_type> ( 125 ) / static_cast<value_type>( 192 );
        const value_type b5 = static_cast<value_type> ( -2187 ) / static_cast<value_type>( 6784 );
        const value_type b6 = static_cast<value_type> ( 11 ) / static_cast<value_type>( 84 );

        const time_type dt = ( t_new - t_old );
        const value_type theta = ( t - t_old ) / dt;
        const value_type X1 = static_cast< value_type >( 5 ) * ( static_cast< value_type >( 2558722523LL ) - static_cast< value_type >( 31403016 ) * theta ) / static_cast< value_type >( 11282082432LL );
        const value_type X3 = static_cast< value_type >( 100 ) * ( static_cast< value_type >( 882725551 ) - static_cast< value_type >( 15701508 ) * theta ) / static_cast< value_type >( 32700410799LL );
        const value_type X4 = static_cast< value_type >( 25 ) * ( static_cast< value_type >( 443332067 ) - static_cast< value_type >( 31403016 ) * theta ) / static_cast< value_type >( 1880347072LL ) ;
        const value_type X5 = static_cast< value_type >( 32805 ) * ( static_cast< value_type >( 23143187 ) - static_cast< value_type >( 3489224 ) * theta ) / static_cast< value_type >( 199316789632LL );
        const value_type X6 = static_cast< value_type >( 55 ) * ( static_cast< value_type >( 29972135 ) - static_cast< value_type >( 7076736 ) * theta ) / static_cast< value_type >( 822651844 );
        const value_type X7 = static_cast< value_type >( 10 ) * ( static_cast< value_type >( 7414447 ) - static_cast< value_type >( 829305 ) * theta ) / static_cast< value_type >( 29380423 );

        const value_type theta_m_1 = theta - static_cast< value_type >( 1 );
        const value_type theta_sq = theta * theta;
        const value_type A = theta_sq * ( static_cast< value_type >( 3 ) - static_cast< value_type >( 2 ) * theta );
        const value_type B = theta_sq * theta_m_1;
        const value_type C = theta_sq * theta_m_1 * theta_m_1;
        const value_type D = theta * theta_m_1 * theta_m_1;

        const value_type b1_theta = A * b1 - C * X1 + D;
        const value_type b3_theta = A * b3 + C * X3;
        const value_type b4_theta = A * b4 - C * X4;
        const value_type b5_theta = A * b5 + C * X5;
        const value_type b6_theta = A * b6 - C * X6;
        const value_type b7_theta = B + C * X7;

        // const state_type &k1 = *m_old_deriv;
        // const state_type &k3 = dopri5().m_k3;
        // const state_type &k4 = dopri5().m_k4;
        // const state_type &k5 = dopri5().m_k5;
        // const state_type &k6 = dopri5().m_k6;
        // const state_type &k7 = *m_current_deriv;

        stepper_base_type::m_algebra.for_each8( x , x_old , deriv_old , m_k3.m_v , m_k4.m_v , m_k5.m_v , m_k6.m_v , deriv_new ,
                typename operations_type::template scale_sum7< value_type , time_type , time_type , time_type , time_type , time_type , time_type >( 1.0 , dt * b1_theta , dt * b3_theta , dt * b4_theta , dt * b5_theta , dt * b6_theta , dt * b7_theta ) );
    }


    template< class StateIn >
    void adjust_size( const StateIn &x )
    {
        resize_k_x_tmp_impl( x );
        resize_dxdt_tmp_impl( x );
        stepper_base_type::adjust_size( x );
    }
    

private:

    template< class StateIn >
    bool resize_k_x_tmp_impl( const StateIn &x )
    {
        bool resized = false;
        resized |= adjust_size_by_resizeability( m_x_tmp , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_k2 , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_k3 , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_k4 , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_k5 , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_k6 , x , typename is_resizeable<deriv_type>::type() );
        return resized;
    }

    template< class StateIn >
    bool resize_dxdt_tmp_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdt_tmp , x , typename is_resizeable<deriv_type>::type() );
    }
        


    wrapped_state_type m_x_tmp;
    wrapped_deriv_type m_k2 , m_k3 , m_k4 , m_k5 , m_k6 ;
    wrapped_deriv_type m_dxdt_tmp;
    resizer_type m_k_x_tmp_resizer;
    resizer_type m_dxdt_tmp_resizer;
};



/************* DOXYGEN ************/
/**
 * \class runge_kutta_dopri5
 * \brief The Runge-Kutta Dormand-Prince 5 method.
 *
 * The Runge-Kutta Dormand-Prince 5 method is a very popular method for solving ODEs, see
 * <a href=""></a>.
 * The method is explicit and fulfills the Error Stepper concept. Step size control
 * is provided but continuous output is available which make this method favourable for many applications. 
 * 
 * This class derives from explicit_error_stepper_fsal_base and inherits its interface via CRTP (current recurring
 * template pattern). The method possesses the FSAL (first-same-as-last) property. See
 * explicit_error_stepper_fsal_base for more details.
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
     * \fn runge_kutta_dopri5::runge_kutta_dopri5( const algebra_type &algebra )
     * \brief Constructs the runge_kutta_dopri5 class. This constructor can be used as a default
     * constructor if the algebra has a default constructor.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */

    /**
     * \fn runge_kutta_dopri5::do_step_impl( System system , const StateIn &in , const DerivIn &dxdt_in , time_type t , StateOut &out , DerivOut &dxdt_out , time_type dt )
     * \brief This method performs one step. The derivative `dxdt_in` of `in` at the time `t` is passed to the
     * method. The result is updated out-of-place, hence the input is in `in` and the output in `out`. Furthermore,
     * the derivative is update out-of-place, hence the input is assumed to be in `dxdt_in` and the output in
     * `dxdt_out`. 
     * Access to this step functionality is provided by explicit_error_stepper_fsal_base and 
     * `do_step_impl` should not be called directly.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param dxdt_in The derivative of x at t. dxdt_in is not modified by this method
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dxdt_out The result of the new derivative at time t+dt.
     * \param dt The step size.
     */

    /**
     * \fn runge_kutta_dopri5::do_step_impl( System system , const StateIn &in , const DerivIn &dxdt_in , time_type t , StateOut &out , DerivOut &dxdt_out , time_type dt , Err &xerr )
     * \brief This method performs one step. The derivative `dxdt_in` of `in` at the time `t` is passed to the
     * method. The result is updated out-of-place, hence the input is in `in` and the output in `out`. Furthermore,
     * the derivative is update out-of-place, hence the input is assumed to be in `dxdt_in` and the output in
     * `dxdt_out`. 
     * Access to this step functionality is provided by explicit_error_stepper_fsal_base and 
     * `do_step_impl` should not be called directly.
     * An estimation of the error is calculated.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param dxdt_in The derivative of x at t. dxdt_in is not modified by this method
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dxdt_out The result of the new derivative at time t+dt.
     * \param dt The step size.
     * \param xerr An estimation of the error.
     */

    /**
     * \fn runge_kutta_dopri5::calc_state( time_type t , StateOut &x , const StateIn1 &x_old , const DerivIn1 &deriv_old , time_type t_old , const StateIn2 &  , const DerivIn2 &deriv_new , time_type t_new ) const
     * \brief This method is used for continuous output and it calculates the state `x` at a time `t` from the 
     * knowledge of two states `old_state` and `current_state` at time points `t_old` and `t_new`. It also uses
     * internal variables to calculate the result. Hence this method must be called after two successful `do_step`
     * calls.
     */

    /**
     * \fn runge_kutta_dopri5::adjust_size( const StateIn &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA_DOPRI5_HPP_INCLUDED
