/*
 [auto_generated]
 boost/numeric/odeint/stepper/rosenbrock4.hpp

 [begin_description]
 Implementation of the Rosenbrock 4 method for solving stiff ODEs. Note, that a
 controller and a dense-output stepper exist for this method,
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011-2012 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_HPP_INCLUDED


#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>

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


/*
 * ToDo:
 *
 * 2. Interfacing for odeint, check if controlled_error_stepper can be used
 * 3. dense output
 */



template< class Value >
struct default_rosenbrock_coefficients
{
    typedef Value value_type;
    typedef unsigned short order_type;

    default_rosenbrock_coefficients( void )
    : gamma ( static_cast< value_type >( 0.25 ) ) ,
      d1 ( static_cast< value_type >( 0.25 ) ) ,
      d2 ( static_cast< value_type >( -0.1043 ) ) ,
      d3 ( static_cast< value_type >( 0.1035 ) ) ,
      d4 ( static_cast< value_type >( 0.3620000000000023e-01 ) ) ,
      c2 ( static_cast< value_type >( 0.386 ) ) ,
      c3 ( static_cast< value_type >( 0.21 ) ) ,
      c4 ( static_cast< value_type >( 0.63 ) ) ,
      c21 ( static_cast< value_type >( -0.5668800000000000e+01 ) ) ,
      a21 ( static_cast< value_type >( 0.1544000000000000e+01 ) ) ,
      c31 ( static_cast< value_type >( -0.2430093356833875e+01 ) ) ,
      c32 ( static_cast< value_type >( -0.2063599157091915e+00 ) ) ,
      a31 ( static_cast< value_type >( 0.9466785280815826e+00 ) ) ,
      a32 ( static_cast< value_type >( 0.2557011698983284e+00 ) ) ,
      c41 ( static_cast< value_type >( -0.1073529058151375e+00 ) ) ,
      c42 ( static_cast< value_type >( -0.9594562251023355e+01 ) ) ,
      c43 ( static_cast< value_type >( -0.2047028614809616e+02 ) ) ,
      a41 ( static_cast< value_type >( 0.3314825187068521e+01 ) ) ,
      a42 ( static_cast< value_type >( 0.2896124015972201e+01 ) ) ,
      a43 ( static_cast< value_type >( 0.9986419139977817e+00 ) ) ,
      c51 ( static_cast< value_type >( 0.7496443313967647e+01 ) ) ,
      c52 ( static_cast< value_type >( -0.1024680431464352e+02 ) ) ,
      c53 ( static_cast< value_type >( -0.3399990352819905e+02 ) ) ,
      c54 ( static_cast< value_type >(  0.1170890893206160e+02 ) ) ,
      a51 ( static_cast< value_type >( 0.1221224509226641e+01 ) ) ,
      a52 ( static_cast< value_type >( 0.6019134481288629e+01 ) ) ,
      a53 ( static_cast< value_type >( 0.1253708332932087e+02 ) ) ,
      a54 ( static_cast< value_type >( -0.6878860361058950e+00 ) ) ,
      c61 ( static_cast< value_type >( 0.8083246795921522e+01 ) ) ,
      c62 ( static_cast< value_type >( -0.7981132988064893e+01 ) ) ,
      c63 ( static_cast< value_type >( -0.3152159432874371e+02 ) ) ,
      c64 ( static_cast< value_type >( 0.1631930543123136e+02 ) ) ,
      c65 ( static_cast< value_type >( -0.6058818238834054e+01 ) ) ,
      d21 ( static_cast< value_type >( 0.1012623508344586e+02 ) ) ,
      d22 ( static_cast< value_type >( -0.7487995877610167e+01 ) ) ,
      d23 ( static_cast< value_type >( -0.3480091861555747e+02 ) ) ,
      d24 ( static_cast< value_type >( -0.7992771707568823e+01 ) ) ,
      d25 ( static_cast< value_type >( 0.1025137723295662e+01 ) ) ,
      d31 ( static_cast< value_type >( -0.6762803392801253e+00 ) ) ,
      d32 ( static_cast< value_type >( 0.6087714651680015e+01 ) ) ,
      d33 ( static_cast< value_type >( 0.1643084320892478e+02 ) ) ,
      d34 ( static_cast< value_type >( 0.2476722511418386e+02 ) ) ,
      d35 ( static_cast< value_type >( -0.6594389125716872e+01 ) )
    {}

    const value_type gamma;
    const value_type d1 , d2 , d3 , d4;
    const value_type c2 , c3 , c4;
    const value_type c21 ;
    const value_type a21;
    const value_type c31 , c32;
    const value_type a31 , a32;
    const value_type c41 , c42 , c43;
    const value_type a41 , a42 , a43;
    const value_type c51 , c52 , c53 , c54;
    const value_type a51 , a52 , a53 , a54;
    const value_type c61 , c62 , c63 , c64 , c65;
    const value_type d21 , d22 , d23 , d24 , d25;
    const value_type d31 , d32 , d33 , d34 , d35;

    static const order_type stepper_order = 4;
    static const order_type error_order = 3;
};



template< class Value , class Coefficients = default_rosenbrock_coefficients< Value > , class Resizer = initially_resizer >
class rosenbrock4
{
private:

public:

    typedef Value value_type;
    typedef boost::numeric::ublas::vector< value_type > state_type;
    typedef state_type deriv_type;
    typedef value_type time_type;
    typedef boost::numeric::ublas::matrix< value_type > matrix_type;
    typedef boost::numeric::ublas::permutation_matrix< size_t > pmatrix_type;
    typedef Resizer resizer_type;
    typedef Coefficients rosenbrock_coefficients;
    typedef stepper_tag stepper_category;
    typedef unsigned short order_type;

    typedef state_wrapper< state_type > wrapped_state_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    typedef state_wrapper< matrix_type > wrapped_matrix_type;
    typedef state_wrapper< pmatrix_type > wrapped_pmatrix_type;

    typedef rosenbrock4< Value , Coefficients , Resizer > stepper_type;

    const static order_type stepper_order = rosenbrock_coefficients::stepper_order;
    const static order_type error_order = rosenbrock_coefficients::error_order;

    rosenbrock4( void )
    : m_resizer() , m_x_err_resizer() ,
      m_jac() , m_pm() ,
      m_dfdt() , m_dxdt() , m_dxdtnew() ,
      m_g1() , m_g2() , m_g3() , m_g4() , m_g5() ,
      m_cont3() , m_cont4() , m_xtmp() , m_x_err() ,
      m_coef()
    { }


    order_type order() const { return stepper_order; } 

    template< class System >
    void do_step( System system , const state_type &x , time_type t , state_type &xout , time_type dt , state_type &xerr )
    {
        // get the system and jacobi function
        typedef typename odeint::unwrap_reference< System >::type system_type;
        typedef typename odeint::unwrap_reference< typename system_type::first_type >::type deriv_func_type;
        typedef typename odeint::unwrap_reference< typename system_type::second_type >::type jacobi_func_type;
        system_type &sys = system;
        deriv_func_type &deriv_func = sys.first;
        jacobi_func_type &jacobi_func = sys.second;

        const size_t n = x.size();

        m_resizer.adjust_size( x , detail::bind( &stepper_type::template resize_impl<state_type> , detail::ref( *this ) , detail::_1 ) );

        for( size_t i=0 ; i<n ; ++i )
            m_pm.m_v( i ) = i;

        deriv_func( x , m_dxdt.m_v , t );
        jacobi_func( x , m_jac.m_v , t , m_dfdt.m_v );

        m_jac.m_v *= -1.0;
        m_jac.m_v += 1.0 / m_coef.gamma / dt * boost::numeric::ublas::identity_matrix< value_type >( n );
        boost::numeric::ublas::lu_factorize( m_jac.m_v , m_pm.m_v );

        for( size_t i=0 ; i<n ; ++i )
            m_g1.m_v[i] = m_dxdt.m_v[i] + dt * m_coef.d1 * m_dfdt.m_v[i];
        boost::numeric::ublas::lu_substitute( m_jac.m_v , m_pm.m_v , m_g1.m_v );


        for( size_t i=0 ; i<n ; ++i )
            m_xtmp.m_v[i] = x[i] + m_coef.a21 * m_g1.m_v[i];
        deriv_func( m_xtmp.m_v , m_dxdtnew.m_v , t + m_coef.c2 * dt );
        for( size_t i=0 ; i<n ; ++i )
            m_g2.m_v[i] = m_dxdtnew.m_v[i] + dt * m_coef.d2 * m_dfdt.m_v[i] + m_coef.c21 * m_g1.m_v[i] / dt;
        boost::numeric::ublas::lu_substitute( m_jac.m_v , m_pm.m_v , m_g2.m_v );


        for( size_t i=0 ; i<n ; ++i )
            m_xtmp.m_v[i] = x[i] + m_coef.a31 * m_g1.m_v[i] + m_coef.a32 * m_g2.m_v[i];
        deriv_func( m_xtmp.m_v , m_dxdtnew.m_v , t + m_coef.c3 * dt );
        for( size_t i=0 ; i<n ; ++i )
            m_g3.m_v[i] = m_dxdtnew.m_v[i] + dt * m_coef.d3 * m_dfdt.m_v[i] + ( m_coef.c31 * m_g1.m_v[i] + m_coef.c32 * m_g2.m_v[i] ) / dt;
        boost::numeric::ublas::lu_substitute( m_jac.m_v , m_pm.m_v , m_g3.m_v );


        for( size_t i=0 ; i<n ; ++i )
            m_xtmp.m_v[i] = x[i] + m_coef.a41 * m_g1.m_v[i] + m_coef.a42 * m_g2.m_v[i] + m_coef.a43 * m_g3.m_v[i];
        deriv_func( m_xtmp.m_v , m_dxdtnew.m_v , t + m_coef.c4 * dt );
        for( size_t i=0 ; i<n ; ++i )
            m_g4.m_v[i] = m_dxdtnew.m_v[i] + dt * m_coef.d4 * m_dfdt.m_v[i] + ( m_coef.c41 * m_g1.m_v[i] + m_coef.c42 * m_g2.m_v[i] + m_coef.c43 * m_g3.m_v[i] ) / dt;
        boost::numeric::ublas::lu_substitute( m_jac.m_v , m_pm.m_v , m_g4.m_v );


        for( size_t i=0 ; i<n ; ++i )
            m_xtmp.m_v[i] = x[i] + m_coef.a51 * m_g1.m_v[i] + m_coef.a52 * m_g2.m_v[i] + m_coef.a53 * m_g3.m_v[i] + m_coef.a54 * m_g4.m_v[i];
        deriv_func( m_xtmp.m_v , m_dxdtnew.m_v , t + dt );
        for( size_t i=0 ; i<n ; ++i )
            m_g5.m_v[i] = m_dxdtnew.m_v[i] + ( m_coef.c51 * m_g1.m_v[i] + m_coef.c52 * m_g2.m_v[i] + m_coef.c53 * m_g3.m_v[i] + m_coef.c54 * m_g4.m_v[i] ) / dt;
        boost::numeric::ublas::lu_substitute( m_jac.m_v , m_pm.m_v , m_g5.m_v );

        for( size_t i=0 ; i<n ; ++i )
            m_xtmp.m_v[i] += m_g5.m_v[i];
        deriv_func( m_xtmp.m_v , m_dxdtnew.m_v , t + dt );
        for( size_t i=0 ; i<n ; ++i )
            xerr[i] = m_dxdtnew.m_v[i] + ( m_coef.c61 * m_g1.m_v[i] + m_coef.c62 * m_g2.m_v[i] + m_coef.c63 * m_g3.m_v[i] + m_coef.c64 * m_g4.m_v[i] + m_coef.c65 * m_g5.m_v[i] ) / dt;
        boost::numeric::ublas::lu_substitute( m_jac.m_v , m_pm.m_v , xerr );

        for( size_t i=0 ; i<n ; ++i )
            xout[i] = m_xtmp.m_v[i] + xerr[i];
    }

    template< class System >
    void do_step( System system , state_type &x , time_type t , time_type dt , state_type &xerr )
    {
        do_step( system , x , t , x , dt , xerr );
    }

    /*
     * do_step without error output - just calls above functions with and neglects the error estimate
     */
    template< class System >
    void do_step( System system , const state_type &x , time_type t , state_type &xout , time_type dt )
    {
        m_x_err_resizer.adjust_size( x , detail::bind( &stepper_type::template resize_x_err<state_type> , detail::ref( *this ) , detail::_1 ) );
        do_step( system , x , t , xout , dt , m_x_err.m_v );
    }

    template< class System >
    void do_step( System system , state_type &x , time_type t , time_type dt )
    {
        m_x_err_resizer.adjust_size( x , detail::bind( &stepper_type::template resize_x_err<state_type> , detail::ref( *this ) , detail::_1 ) );
        do_step( system , x , t , dt , m_x_err.m_v );
    }

    void prepare_dense_output()
    {
        const size_t n = m_g1.m_v.size();
        for( size_t i=0 ; i<n ; ++i )
        {
            m_cont3.m_v[i] = m_coef.d21 * m_g1.m_v[i] + m_coef.d22 * m_g2.m_v[i] + m_coef.d23 * m_g3.m_v[i] + m_coef.d24 * m_g4.m_v[i] + m_coef.d25 * m_g5.m_v[i];
            m_cont4.m_v[i] = m_coef.d31 * m_g1.m_v[i] + m_coef.d32 * m_g2.m_v[i] + m_coef.d33 * m_g3.m_v[i] + m_coef.d34 * m_g4.m_v[i] + m_coef.d35 * m_g5.m_v[i];
        }
    }


    void calc_state( time_type t , state_type &x ,
            const state_type &x_old , time_type t_old ,
            const state_type &x_new , time_type t_new )
    {
        const size_t n = m_g1.m_v.size();
        time_type dt = t_new - t_old;
        time_type s = ( t - t_old ) / dt;
        time_type s1 = 1.0 - s;
        for( size_t i=0 ; i<n ; ++i )
            x[i] = x_old[i] * s1 + s * ( x_new[i] + s1 * ( m_cont3.m_v[i] + s * m_cont4.m_v[i] ) );
    }



    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_impl( x );
        resize_x_err( x );
    }


protected:

    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized = false;
        resized |= adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_dfdt , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_dxdtnew , x , typename is_resizeable<deriv_type>::type() );
        resized |= adjust_size_by_resizeability( m_xtmp , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_g1 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_g2 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_g3 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_g4 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_g5 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_cont3 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_cont4 , x , typename is_resizeable<state_type>::type() );
        resized |= adjust_size_by_resizeability( m_jac , x , typename is_resizeable<matrix_type>::type() );
        resized |= adjust_size_by_resizeability( m_pm , x , typename is_resizeable<pmatrix_type>::type() );
        return resized;
    }

    template< class StateIn >
    bool resize_x_err( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_x_err , x , typename is_resizeable<state_type>::type() );
    }

private:


    resizer_type m_resizer;
    resizer_type m_x_err_resizer;

    wrapped_matrix_type m_jac;
    wrapped_pmatrix_type m_pm;
    wrapped_deriv_type m_dfdt , m_dxdt , m_dxdtnew;
    wrapped_state_type m_g1 , m_g2 , m_g3 , m_g4 , m_g5;
    wrapped_state_type m_cont3 , m_cont4;
    wrapped_state_type m_xtmp;
    wrapped_state_type m_x_err;

    const rosenbrock_coefficients m_coef;
};


} // namespace odeint
} // namespace numeric
} // namespace boost

#endif // BOOST_NUMERIC_ODEINT_STEPPER_ROSENBROCK4_HPP_INCLUDED
