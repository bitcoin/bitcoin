/*
  [auto_generated]
  boost/numeric/odeint/stepper/bulirsch_stoer.hpp

  [begin_description]
  Implementation of the Burlish-Stoer method. As described in
  Ernst Hairer, Syvert Paul Norsett, Gerhard Wanner
  Solving Ordinary Differential Equations I. Nonstiff Problems.
  Springer Series in Comput. Mathematics, Vol. 8, Springer-Verlag 1987, Second revised edition 1993.
  [end_description]

  Copyright 2011-2013 Mario Mulansky
  Copyright 2011-2013 Karsten Ahnert
  Copyright 2012 Christoph Koke

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_BULIRSCH_STOER_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_BULIRSCH_STOER_HPP_INCLUDED


#include <iostream>

#include <algorithm>

#include <boost/config.hpp> // for min/max guidelines

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>

#include <boost/numeric/odeint/stepper/controlled_runge_kutta.hpp>
#include <boost/numeric/odeint/stepper/modified_midpoint.hpp>
#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>
#include <boost/numeric/odeint/util/unit_helper.hpp>
#include <boost/numeric/odeint/util/detail/less_with_sign.hpp>

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
class bulirsch_stoer {

public:

    typedef State state_type;
    typedef Value value_type;
    typedef Deriv deriv_type;
    typedef Time time_type;
    typedef Algebra algebra_type;
    typedef Operations operations_type;
    typedef Resizer resizer_type;
#ifndef DOXYGEN_SKIP
    typedef state_wrapper< state_type > wrapped_state_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    typedef controlled_stepper_tag stepper_category;

    typedef bulirsch_stoer< State , Value , Deriv , Time , Algebra , Operations , Resizer > controlled_error_bs_type;

    typedef typename inverse_time< time_type >::type inv_time_type;

    typedef std::vector< value_type > value_vector;
    typedef std::vector< time_type > time_vector;
    typedef std::vector< inv_time_type > inv_time_vector;  //should be 1/time_type for boost.units
    typedef std::vector< value_vector > value_matrix;
    typedef std::vector< size_t > int_vector;
    typedef std::vector< wrapped_state_type > state_table_type;
#endif //DOXYGEN_SKIP
    const static size_t m_k_max = 8;

    bulirsch_stoer(
        value_type eps_abs = 1E-6 , value_type eps_rel = 1E-6 ,
        value_type factor_x = 1.0 , value_type factor_dxdt = 1.0 ,
        time_type max_dt = static_cast<time_type>(0))
        : m_error_checker( eps_abs , eps_rel , factor_x, factor_dxdt ) , m_midpoint() ,
          m_last_step_rejected( false ) , m_first( true ) ,
          m_max_dt(max_dt) ,
          m_interval_sequence( m_k_max+1 ) ,
          m_coeff( m_k_max+1 ) ,
          m_cost( m_k_max+1 ) ,
          m_facmin_table( m_k_max+1 ) ,
          m_table( m_k_max ) ,
          STEPFAC1( 0.65 ) , STEPFAC2( 0.94 ) , STEPFAC3( 0.02 ) , STEPFAC4( 4.0 ) , KFAC1( 0.8 ) , KFAC2( 0.9 )
    {
        BOOST_USING_STD_MIN();
        BOOST_USING_STD_MAX();
        /* initialize sequence of stage numbers and work */
        for( unsigned short i = 0; i < m_k_max+1; i++ )
        {
            m_interval_sequence[i] = 2 * (i+1);
            if( i == 0 )
                m_cost[i] = m_interval_sequence[i];
            else
                m_cost[i] = m_cost[i-1] + m_interval_sequence[i];
            m_coeff[i].resize(i);
            m_facmin_table[i] = pow BOOST_PREVENT_MACRO_SUBSTITUTION( STEPFAC3 , static_cast< value_type >(1) / static_cast< value_type >( 2*i+1 ) );
            for( size_t k = 0 ; k < i ; ++k  )
            {
                const value_type r = static_cast< value_type >( m_interval_sequence[i] ) / static_cast< value_type >( m_interval_sequence[k] );
                m_coeff[i][k] = 1.0 / ( r*r - static_cast< value_type >( 1.0 ) ); // coefficients for extrapolation
            }
        }
        reset();
    }


    /*
     * Version 1 : try_step( sys , x , t , dt )
     *
     * The overloads are needed to solve the forwarding problem
     */
    template< class System , class StateInOut >
    controlled_step_result try_step( System system , StateInOut &x , time_type &t , time_type &dt )
    {
        return try_step_v1( system , x , t, dt );
    }

    /**
     * \brief Second version to solve the forwarding problem, can be used with Boost.Range as StateInOut.
     */
    template< class System , class StateInOut >
    controlled_step_result try_step( System system , const StateInOut &x , time_type &t , time_type &dt )
    {
        return try_step_v1( system , x , t, dt );
    }

    /*
     * Version 2 : try_step( sys , x , dxdt , t , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System , class StateInOut , class DerivIn >
    controlled_step_result try_step( System system , StateInOut &x , const DerivIn &dxdt , time_type &t , time_type &dt )
    {
        m_xnew_resizer.adjust_size( x , detail::bind( &controlled_error_bs_type::template resize_m_xnew< StateInOut > , detail::ref( *this ) , detail::_1 ) );
        controlled_step_result res = try_step( system , x , dxdt , t , m_xnew.m_v , dt );
        if( res == success )
        {
            boost::numeric::odeint::copy( m_xnew.m_v , x );
        }
        return res;
    }

    /*
     * Version 3 : try_step( sys , in , t , out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System , class StateIn , class StateOut >
    typename boost::disable_if< boost::is_same< StateIn , time_type > , controlled_step_result >::type
    try_step( System system , const StateIn &in , time_type &t , StateOut &out , time_type &dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_dxdt_resizer.adjust_size( in , detail::bind( &controlled_error_bs_type::template resize_m_dxdt< StateIn > , detail::ref( *this ) , detail::_1 ) );
        sys( in , m_dxdt.m_v , t );
        return try_step( system , in , m_dxdt.m_v , t , out , dt );
    }


    /*
     * Full version : try_step( sys , in , dxdt_in , t , out , dt )
     *
     * contains the actual implementation
     */
    template< class System , class StateIn , class DerivIn , class StateOut >
    controlled_step_result try_step( System system , const StateIn &in , const DerivIn &dxdt , time_type &t , StateOut &out , time_type &dt )
    {
        if( m_max_dt != static_cast<time_type>(0) && detail::less_with_sign(m_max_dt, dt, dt) )
        {
            // given step size is bigger then max_dt
            // set limit and return fail
            dt = m_max_dt;
            return fail;
        }

        BOOST_USING_STD_MIN();
        BOOST_USING_STD_MAX();

        static const value_type val1( 1.0 );

        if( m_resizer.adjust_size( in , detail::bind( &controlled_error_bs_type::template resize_impl< StateIn > , detail::ref( *this ) , detail::_1 ) ) )
        {
            reset(); // system resized -> reset
        }

        if( dt != m_dt_last )
        {
            reset(); // step size changed from outside -> reset
        }

        bool reject( true );

        time_vector h_opt( m_k_max+1 );
        inv_time_vector work( m_k_max+1 );

        time_type new_h = dt;

        /* m_current_k_opt is the estimated current optimal stage number */
        for( size_t k = 0 ; k <= m_current_k_opt+1 ; k++ )
        {
            /* the stage counts are stored in m_interval_sequence */
            m_midpoint.set_steps( m_interval_sequence[k] );
            if( k == 0 )
            {
                m_midpoint.do_step( system , in , dxdt , t , out , dt );
                /* the first step, nothing more to do */
            }
            else
            {
                m_midpoint.do_step( system , in , dxdt , t , m_table[k-1].m_v , dt );
                extrapolate( k , m_table , m_coeff , out );
                // get error estimate
                m_algebra.for_each3( m_err.m_v , out , m_table[0].m_v ,
                                     typename operations_type::template scale_sum2< value_type , value_type >( val1 , -val1 ) );
                const value_type error = m_error_checker.error( m_algebra , in , dxdt , m_err.m_v , dt );
                h_opt[k] = calc_h_opt( dt , error , k );
                work[k] = static_cast<value_type>( m_cost[k] ) / h_opt[k];

                if( (k == m_current_k_opt-1) || m_first )
                { // convergence before k_opt ?
                    if( error < 1.0 )
                    {
                        //convergence
                        reject = false;
                        if( (work[k] < KFAC2*work[k-1]) || (m_current_k_opt <= 2) )
                        {
                            // leave order as is (except we were in first round)
                            m_current_k_opt = min BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<int>(m_k_max)-1 , max BOOST_PREVENT_MACRO_SUBSTITUTION( 2 , static_cast<int>(k)+1 ) );
                            new_h = h_opt[k];
                            new_h *= static_cast<value_type>( m_cost[k+1] ) / static_cast<value_type>( m_cost[k] );
                        } else {
                            m_current_k_opt = min BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<int>(m_k_max)-1 , max BOOST_PREVENT_MACRO_SUBSTITUTION( 2 , static_cast<int>(k) ) );
                            new_h = h_opt[k];
                        }
                        break;
                    }
                    else if( should_reject( error , k ) && !m_first )
                    {
                        reject = true;
                        new_h = h_opt[k];
                        break;
                    }
                }
                if( k == m_current_k_opt )
                { // convergence at k_opt ?
                    if( error < 1.0 )
                    {
                        //convergence
                        reject = false;
                        if( (work[k-1] < KFAC2*work[k]) )
                        {
                            m_current_k_opt = max BOOST_PREVENT_MACRO_SUBSTITUTION( 2 , static_cast<int>(m_current_k_opt)-1 );
                            new_h = h_opt[m_current_k_opt];
                        }
                        else if( (work[k] < KFAC2*work[k-1]) && !m_last_step_rejected )
                        {
                            m_current_k_opt = min BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<int>(m_k_max-1) , static_cast<int>(m_current_k_opt)+1 );
                            new_h = h_opt[k];
                            new_h *= static_cast<value_type>(m_cost[m_current_k_opt])/static_cast<value_type>(m_cost[k]);
                        } else
                            new_h = h_opt[m_current_k_opt];
                        break;
                    }
                    else if( should_reject( error , k ) )
                    {
                        reject = true;
                        new_h = h_opt[m_current_k_opt];
                        break;
                    }
                }
                if( k == m_current_k_opt+1 )
                { // convergence at k_opt+1 ?
                    if( error < 1.0 )
                    {   //convergence
                        reject = false;
                        if( work[k-2] < KFAC2*work[k-1] )
                            m_current_k_opt = max BOOST_PREVENT_MACRO_SUBSTITUTION( 2 , static_cast<int>(m_current_k_opt)-1 );
                        if( (work[k] < KFAC2*work[m_current_k_opt]) && !m_last_step_rejected )
                            m_current_k_opt = min BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<int>(m_k_max)-1 , static_cast<int>(k) );
                        new_h = h_opt[m_current_k_opt];
                    } else
                    {
                        reject = true;
                        new_h = h_opt[m_current_k_opt];
                    }
                    break;
                }
            }
        }

        if( !reject )
        {
            t += dt;
        }

        if( !m_last_step_rejected || boost::numeric::odeint::detail::less_with_sign(new_h, dt, dt) )
        {
            // limit step size
            if( m_max_dt != static_cast<time_type>(0) )
            {
                new_h = detail::min_abs(m_max_dt, new_h);
            }
            m_dt_last = new_h;
            dt = new_h;
        }

        m_last_step_rejected = reject;
        m_first = false;

        if( reject )
            return fail;
        else
            return success;
    }

    /** \brief Resets the internal state of the stepper */
    void reset()
    {
        m_first = true;
        m_last_step_rejected = false;
        // crude estimate of optimal order
        m_current_k_opt = 4;
        /* no calculation because log10 might not exist for value_type!
        const value_type logfact( -log10( max BOOST_PREVENT_MACRO_SUBSTITUTION( eps_rel , static_cast< value_type >(1.0E-12) ) ) * 0.6 + 0.5 );
        m_current_k_opt = max BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<value_type>( 1 ) , min BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<value_type>( m_k_max-1 ) , logfact ));
        */
    }


    /* Resizer methods */

    template< class StateIn >
    void adjust_size( const StateIn &x )
    {
        resize_m_dxdt( x );
        resize_m_xnew( x );
        resize_impl( x );
        m_midpoint.adjust_size( x );
    }


private:

    template< class StateIn >
    bool resize_m_dxdt( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
    }

    template< class StateIn >
    bool resize_m_xnew( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_xnew , x , typename is_resizeable<state_type>::type() );
    }

    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized( false );
        for( size_t i = 0 ; i < m_k_max ; ++i )
            resized |= adjust_size_by_resizeability( m_table[i] , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_err , x , typename is_resizeable<state_type>::type() );
        return resized;
    }


    template< class System , class StateInOut >
    controlled_step_result try_step_v1( System system , StateInOut &x , time_type &t , time_type &dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_dxdt_resizer.adjust_size( x , detail::bind( &controlled_error_bs_type::template resize_m_dxdt< StateInOut > , detail::ref( *this ) , detail::_1 ) );
        sys( x , m_dxdt.m_v ,t );
        return try_step( system , x , m_dxdt.m_v , t , dt );
    }


    template< class StateInOut >
    void extrapolate( size_t k , state_table_type &table , const value_matrix &coeff , StateInOut &xest )
    /* polynomial extrapolation, see http://www.nr.com/webnotes/nr3web21.pdf
       uses the obtained intermediate results to extrapolate to dt->0 
    */
    {
        static const value_type val1 = static_cast< value_type >( 1.0 );
        for( int j=k-1 ; j>0 ; --j )
        {
            m_algebra.for_each3( table[j-1].m_v , table[j].m_v , table[j-1].m_v ,
                                 typename operations_type::template scale_sum2< value_type , value_type >( val1 + coeff[k][j] , -coeff[k][j] ) );
        }
        m_algebra.for_each3( xest , table[0].m_v , xest ,
                             typename operations_type::template scale_sum2< value_type , value_type >( val1 + coeff[k][0] , -coeff[k][0]) );
    }

    time_type calc_h_opt( time_type h , value_type error , size_t k ) const
    /* calculates the optimal step size for a given error and stage number */
    {
        BOOST_USING_STD_MIN();
        BOOST_USING_STD_MAX();
        using std::pow;
        value_type expo( 1.0/(2*k+1) );
        value_type facmin = m_facmin_table[k];
        value_type fac;
        if (error == 0.0)
            fac=1.0/facmin;
        else
        {
            fac = STEPFAC2 / pow BOOST_PREVENT_MACRO_SUBSTITUTION( error / STEPFAC1 , expo );
            fac = max BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<value_type>(facmin/STEPFAC4) , min BOOST_PREVENT_MACRO_SUBSTITUTION( static_cast<value_type>(1.0/facmin) , fac ) );
        }
        return h*fac;
    }

    controlled_step_result set_k_opt( size_t k , const inv_time_vector &work , const time_vector &h_opt , time_type &dt )
    /* calculates the optimal stage number */
    {
        if( k == 1 )
        {
            m_current_k_opt = 2;
            return success;
        }
        if( (work[k-1] < KFAC1*work[k]) || (k == m_k_max) )
        {   // order decrease
            m_current_k_opt = k-1;
            dt = h_opt[ m_current_k_opt ];
            return success;
        }
        else if( (work[k] < KFAC2*work[k-1]) || m_last_step_rejected || (k == m_k_max-1) )
        {   // same order - also do this if last step got rejected
            m_current_k_opt = k;
            dt = h_opt[ m_current_k_opt ];
            return success;
        }
        else
        {   // order increase - only if last step was not rejected
            m_current_k_opt = k+1;
            dt = h_opt[ m_current_k_opt-1 ] * m_cost[ m_current_k_opt ] / m_cost[ m_current_k_opt-1 ] ;
            return success;
        }
    }

    bool in_convergence_window( size_t k ) const
    {
        if( (k == m_current_k_opt-1) && !m_last_step_rejected )
            return true; // decrease stepsize only if last step was not rejected
        return ( (k == m_current_k_opt) || (k == m_current_k_opt+1) );
    }

    bool should_reject( value_type error , size_t k ) const
    {
        if( k == m_current_k_opt-1 )
        {
            const value_type d = m_interval_sequence[m_current_k_opt] * m_interval_sequence[m_current_k_opt+1] /
                (m_interval_sequence[0]*m_interval_sequence[0]);
            //step will fail, criterion 17.3.17 in NR
            return ( error > d*d );
        }
        else if( k == m_current_k_opt )
        {
            const value_type d = m_interval_sequence[m_current_k_opt] / m_interval_sequence[0];
            return ( error > d*d );
        } else
            return error > 1.0;
    }

    default_error_checker< value_type, algebra_type , operations_type > m_error_checker;
    modified_midpoint< state_type , value_type , deriv_type , time_type , algebra_type , operations_type , resizer_type > m_midpoint;

    bool m_last_step_rejected;
    bool m_first;

    time_type m_dt_last;
    time_type m_t_last;
    time_type m_max_dt;

    size_t m_current_k_opt;

    algebra_type m_algebra;

    resizer_type m_dxdt_resizer;
    resizer_type m_xnew_resizer;
    resizer_type m_resizer;

    wrapped_state_type m_xnew;
    wrapped_state_type m_err;
    wrapped_deriv_type m_dxdt;

    int_vector m_interval_sequence; // stores the successive interval counts
    value_matrix m_coeff;
    int_vector m_cost; // costs for interval count
    value_vector m_facmin_table; // for precomputed facmin to save pow calls

    state_table_type m_table; // sequence of states for extrapolation

    value_type STEPFAC1 , STEPFAC2 , STEPFAC3 , STEPFAC4 , KFAC1 , KFAC2;
};


/******** DOXYGEN ********/
/**
 * \class bulirsch_stoer
 * \brief The Bulirsch-Stoer algorithm.
 * 
 * The Bulirsch-Stoer is a controlled stepper that adjusts both step size
 * and order of the method. The algorithm uses the modified midpoint and
 * a polynomial extrapolation compute the solution.
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
     * \fn bulirsch_stoer::bulirsch_stoer( value_type eps_abs , value_type eps_rel , value_type factor_x , value_type factor_dxdt )
     * \brief Constructs the bulirsch_stoer class, including initialization of 
     * the error bounds.
     *
     * \param eps_abs Absolute tolerance level.
     * \param eps_rel Relative tolerance level.
     * \param factor_x Factor for the weight of the state.
     * \param factor_dxdt Factor for the weight of the derivative.
     */

    /**
     * \fn bulirsch_stoer::try_step( System system , StateInOut &x , time_type &t , time_type &dt )
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed. Also, the internal order of the stepper is adjusted if required.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. 
     * It must fulfill the Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */

    /**
     * \fn bulirsch_stoer::try_step( System system , StateInOut &x , const DerivIn &dxdt , time_type &t , time_type &dt )
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed. Also, the internal order of the stepper is adjusted if required.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. 
     * It must fulfill the Simple System concept.
     * \param x The state of the ODE which should be solved. Overwritten if 
     * the step is successful.
     * \param dxdt The derivative of state.
     * \param t The value of the time. Updated if the step is successful.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */

    /**
     * \fn bulirsch_stoer::try_step( System system , const StateIn &in , time_type &t , StateOut &out , time_type &dt )
     * \brief Tries to perform one step.
     *
     * \note This method is disabled if state_type=time_type to avoid ambiguity.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed. Also, the internal order of the stepper is adjusted if required.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. 
     * It must fulfill the Simple System concept.
     * \param in The state of the ODE which should be solved.
     * \param t The value of the time. Updated if the step is successful.
     * \param out Used to store the result of the step.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */


    /**
     * \fn bulirsch_stoer::try_step( System system , const StateIn &in , const DerivIn &dxdt , time_type &t , StateOut &out , time_type &dt )
     * \brief Tries to perform one step.
     *
     * This method tries to do one step with step size dt. If the error estimate
     * is to large, the step is rejected and the method returns fail and the 
     * step size dt is reduced. If the error estimate is acceptably small, the
     * step is performed, success is returned and dt might be increased to make 
     * the steps as large as possible. This method also updates t if a step is
     * performed. Also, the internal order of the stepper is adjusted if required.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. 
     * It must fulfill the Simple System concept.
     * \param in The state of the ODE which should be solved.
     * \param dxdt The derivative of state.
     * \param t The value of the time. Updated if the step is successful.
     * \param out Used to store the result of the step.
     * \param dt The step size. Updated.
     * \return success if the step was accepted, fail otherwise.
     */


    /**
     * \fn bulirsch_stoer::adjust_size( const StateIn &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

}
}
}

#endif // BOOST_NUMERIC_ODEINT_STEPPER_BULIRSCH_STOER_HPP_INCLUDED
