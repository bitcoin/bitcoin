/*
 [auto_generated]
 boost/numeric/odeint/stepper/implicit_euler.hpp

 [begin_description]
 Impementation of the implicit Euler method. Works with ublas::vector as state type.
 [end_description]

 Copyright 2010-2012 Mario Mulansky
 Copyright 2010-2012 Karsten Ahnert
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_IMPLICIT_EULER_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_IMPLICIT_EULER_HPP_INCLUDED


#include <utility>

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/util/ublas_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>

namespace boost {
namespace numeric {
namespace odeint {








template< class ValueType , class Resizer = initially_resizer >
class implicit_euler
{

public:

    typedef ValueType value_type;
    typedef value_type time_type;
    typedef boost::numeric::ublas::vector< value_type > state_type;
    typedef state_wrapper< state_type > wrapped_state_type;
    typedef state_type deriv_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    typedef boost::numeric::ublas::matrix< value_type > matrix_type;
    typedef state_wrapper< matrix_type > wrapped_matrix_type;
    typedef boost::numeric::ublas::permutation_matrix< size_t > pmatrix_type;
    typedef state_wrapper< pmatrix_type > wrapped_pmatrix_type;
    typedef Resizer resizer_type;
    typedef stepper_tag stepper_category;
    typedef implicit_euler< ValueType , Resizer > stepper_type;

    implicit_euler( value_type epsilon = 1E-6 )
    : m_epsilon( epsilon ) 
    { }


    template< class System >
    void do_step( System system , state_type &x , time_type t , time_type dt )
    {
        typedef typename odeint::unwrap_reference< System >::type system_type;
        typedef typename odeint::unwrap_reference< typename system_type::first_type >::type deriv_func_type;
        typedef typename odeint::unwrap_reference< typename system_type::second_type >::type jacobi_func_type;
        system_type &sys = system;
        deriv_func_type &deriv_func = sys.first;
        jacobi_func_type &jacobi_func = sys.second;

        m_resizer.adjust_size( x , detail::bind( &stepper_type::template resize_impl<state_type> , detail::ref( *this ) , detail::_1 ) );

        for( size_t i=0 ; i<x.size() ; ++i )
            m_pm.m_v[i] = i;

        t += dt;

        // apply first Newton step
        deriv_func( x , m_dxdt.m_v , t );

        m_b.m_v = dt * m_dxdt.m_v;

        jacobi_func( x , m_jacobi.m_v  , t );
        m_jacobi.m_v *= dt;
        m_jacobi.m_v -= boost::numeric::ublas::identity_matrix< value_type >( x.size() );

        solve( m_b.m_v , m_jacobi.m_v );

        m_x.m_v = x - m_b.m_v;

        // iterate Newton until some precision is reached
        // ToDo: maybe we should apply only one Newton step -> linear implicit one-step scheme
        while( boost::numeric::ublas::norm_2( m_b.m_v ) > m_epsilon )
        {
            deriv_func( m_x.m_v , m_dxdt.m_v , t );
            m_b.m_v = x - m_x.m_v + dt*m_dxdt.m_v;

            // simplified version, only the first Jacobian is used
            //            jacobi( m_x , m_jacobi , t );
            //            m_jacobi *= dt;
            //            m_jacobi -= boost::numeric::ublas::identity_matrix< value_type >( x.size() );

            solve( m_b.m_v , m_jacobi.m_v );

            m_x.m_v -= m_b.m_v;
        }
        x = m_x.m_v;
    }

    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_impl( x );
    }


private:

    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized = false;
        resized |= adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_x , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_b , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_jacobi , x , typename is_resizeable<matrix_type>::type() );
        resized |= adjust_size_by_resizeability( m_pm , x , typename is_resizeable<pmatrix_type>::type() );
        return resized;
    }


    void solve( state_type &x , matrix_type &m )
    {
        int res = boost::numeric::ublas::lu_factorize( m , m_pm.m_v );
        if( res != 0 ) std::exit(0);
        boost::numeric::ublas::lu_substitute( m , m_pm.m_v , x );
    }

private:

    value_type m_epsilon;
    resizer_type m_resizer;
    wrapped_deriv_type m_dxdt;
    wrapped_state_type m_x;
    wrapped_deriv_type m_b;
    wrapped_matrix_type m_jacobi;
    wrapped_pmatrix_type m_pm;


};


} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_IMPLICIT_EULER_HPP_INCLUDED
