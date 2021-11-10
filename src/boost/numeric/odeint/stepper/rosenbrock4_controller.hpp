/*
 [auto_generated]
 boost/numeric/odeint/stepper/rosenbrock4_controller.hpp

 [begin_description]
 Controller for the Rosenbrock4 method.
 [end_description]

 Copyright 2011-2012 Karsten Ahnert
 Copyright 2011-2012 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_CONTROLLER_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_CONTROLLER_HPP_INCLUDED

#include <boost/config.hpp>
#include <boost/numeric/odeint/util/bind.hpp>

#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/util/copy.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/detail/less_with_sign.hpp>

#include <boost/numeric/odeint/stepper/rosenbrock4.hpp>

namespace boost {
namespace numeric {
namespace odeint {

template< class Stepper >
class rosenbrock4_controller
{
private:


public:

    typedef Stepper stepper_type;
    typedef typename stepper_type::value_type value_type;
    typedef typename stepper_type::state_type state_type;
    typedef typename stepper_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_type::time_type time_type;
    typedef typename stepper_type::deriv_type deriv_type;
    typedef typename stepper_type::wrapped_deriv_type wrapped_deriv_type;
    typedef typename stepper_type::resizer_type resizer_type;
    typedef controlled_stepper_tag stepper_category;

    typedef rosenbrock4_controller< Stepper > controller_type;


    rosenbrock4_controller( value_type atol = 1.0e-6 , value_type rtol = 1.0e-6 ,
                            const stepper_type &stepper = stepper_type() )
        : m_stepper( stepper ) , m_atol( atol ) , m_rtol( rtol ) ,
          m_max_dt( static_cast<time_type>(0) ) ,
          m_first_step( true ) , m_err_old( 0.0 ) , m_dt_old( 0.0 ) ,
          m_last_rejected( false )
    { }

    rosenbrock4_controller( value_type atol, value_type rtol, time_type max_dt,
                            const stepper_type &stepper = stepper_type() )
            : m_stepper( stepper ) , m_atol( atol ) , m_rtol( rtol ) , m_max_dt( max_dt ) ,
              m_first_step( true ) , m_err_old( 0.0 ) , m_dt_old( 0.0 ) ,
              m_last_rejected( false )
    { }

    value_type error( const state_type &x , const state_type &xold , const state_type &xerr )
    {
        BOOST_USING_STD_MAX();
        using std::abs;
        using std::sqrt;
        
        const size_t n = x.size();
        value_type err = 0.0 , sk = 0.0;
        for( size_t i=0 ; i<n ; ++i )
        {
            sk = m_atol + m_rtol * max BOOST_PREVENT_MACRO_SUBSTITUTION ( abs( xold[i] ) , abs( x[i] ) );
            err += xerr[i] * xerr[i] / sk / sk;
        }
        return sqrt( err / value_type( n ) );
    }

    value_type last_error( void ) const
    {
        return m_err_old;
    }




    template< class System >
    boost::numeric::odeint::controlled_step_result
    try_step( System sys , state_type &x , time_type &t , time_type &dt )
    {
        m_xnew_resizer.adjust_size( x , detail::bind( &controller_type::template resize_m_xnew< state_type > , detail::ref( *this ) , detail::_1 ) );
        boost::numeric::odeint::controlled_step_result res = try_step( sys , x , t , m_xnew.m_v , dt );
        if( res == success )
        {
            boost::numeric::odeint::copy( m_xnew.m_v , x );
        }
        return res;
    }


    template< class System >
    boost::numeric::odeint::controlled_step_result
    try_step( System sys , const state_type &x , time_type &t , state_type &xout , time_type &dt )
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
        using std::pow;

        static const value_type safe = 0.9 , fac1 = 5.0 , fac2 = 1.0 / 6.0;

        m_xerr_resizer.adjust_size( x , detail::bind( &controller_type::template resize_m_xerr< state_type > , detail::ref( *this ) , detail::_1 ) );

        m_stepper.do_step( sys , x , t , xout , dt , m_xerr.m_v );
        value_type err = error( xout , x , m_xerr.m_v );

        value_type fac = max BOOST_PREVENT_MACRO_SUBSTITUTION (
            fac2 , min BOOST_PREVENT_MACRO_SUBSTITUTION (
                fac1 ,
                static_cast< value_type >( pow( err , 0.25 ) / safe ) ) );
        value_type dt_new = dt / fac;
        if ( err <= 1.0 )
        {
            if( m_first_step )
            {
                m_first_step = false;
            }
            else
            {
                value_type fac_pred = ( m_dt_old / dt ) * pow( err * err / m_err_old , 0.25 ) / safe;
                fac_pred = max BOOST_PREVENT_MACRO_SUBSTITUTION (
                    fac2 , min BOOST_PREVENT_MACRO_SUBSTITUTION ( fac1 , fac_pred ) );
                fac = max BOOST_PREVENT_MACRO_SUBSTITUTION ( fac , fac_pred );
                dt_new = dt / fac;
            }

            m_dt_old = dt;
            m_err_old = max BOOST_PREVENT_MACRO_SUBSTITUTION ( static_cast< value_type >( 0.01 ) , err );
            if( m_last_rejected )
                dt_new = ( dt >= 0.0 ?
                min BOOST_PREVENT_MACRO_SUBSTITUTION ( dt_new , dt ) :
                max BOOST_PREVENT_MACRO_SUBSTITUTION ( dt_new , dt ) );
            t += dt;
            // limit step size to max_dt
            if( m_max_dt != static_cast<time_type>(0) )
            {
                dt = detail::min_abs(m_max_dt, dt_new);
            } else {
                dt = dt_new;
            }
            m_last_rejected = false;
            return success;
        }
        else
        {
            dt = dt_new;
            m_last_rejected = true;
            return fail;
        }
    }


    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_m_xerr( x );
        resize_m_xnew( x );
    }



    stepper_type& stepper( void )
    {
        return m_stepper;
    }

    const stepper_type& stepper( void ) const
    {
        return m_stepper;
    }




protected:

    template< class StateIn >
    bool resize_m_xerr( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_xerr , x , typename is_resizeable<state_type>::type() );
    }

    template< class StateIn >
    bool resize_m_xnew( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_xnew , x , typename is_resizeable<state_type>::type() );
    }


    stepper_type m_stepper;
    resizer_type m_xerr_resizer;
    resizer_type m_xnew_resizer;
    wrapped_state_type m_xerr;
    wrapped_state_type m_xnew;
    value_type m_atol , m_rtol;
    time_type m_max_dt;
    bool m_first_step;
    value_type m_err_old , m_dt_old;
    bool m_last_rejected;
};






} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_CONTROLLER_HPP_INCLUDED
