/*
 [auto_generated]
 boost/numeric/odeint/stepper/runge_kutta_fehlberg87.hpp

 [begin_description]
 Implementation of the Runge-Kutta-Fehlberg stepper with the generic stepper.
 [end_description]

 Copyright 2011-2013 Mario Mulansky
 Copyright 2012-2013 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA_FEHLBERG87_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA_FEHLBERG87_HPP_INCLUDED


#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>

#include <boost/numeric/odeint/stepper/explicit_error_generic_rk.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

#include <boost/array.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>




namespace boost {
namespace numeric {
namespace odeint {


#ifndef DOXYGEN_SKIP
template< class Value = double >
struct rk78_coefficients_a1 : boost::array< Value , 1 >
{
    rk78_coefficients_a1( void )
            {
        (*this)[0] = static_cast< Value >( 2 )/static_cast< Value >( 27 );
            }
};

template< class Value = double >
struct rk78_coefficients_a2 : boost::array< Value , 2 >
{
    rk78_coefficients_a2( void )
            {
        (*this)[0] = static_cast< Value >( 1 )/static_cast< Value >( 36 );
        (*this)[1] = static_cast< Value >( 1 )/static_cast< Value >( 12 );
            }
};


template< class Value = double >
struct rk78_coefficients_a3 : boost::array< Value , 3 >
{
    rk78_coefficients_a3( void )
            {
        (*this)[0] = static_cast< Value >( 1 )/static_cast< Value >( 24 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 1 )/static_cast< Value >( 8 );
            }
};

template< class Value = double >
struct rk78_coefficients_a4 : boost::array< Value , 4 >
{
    rk78_coefficients_a4( void )
            {
        (*this)[0] = static_cast< Value >( 5 )/static_cast< Value >( 12 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( -25 )/static_cast< Value >( 16 );
        (*this)[3] = static_cast< Value >( 25 )/static_cast< Value >( 16 );
            }
};

template< class Value = double >
struct rk78_coefficients_a5 : boost::array< Value , 5 >
{
    rk78_coefficients_a5( void )
            {
        (*this)[0] = static_cast< Value >( 1 )/static_cast< Value >( 20 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( 1 )/static_cast< Value >( 4 );
        (*this)[4] = static_cast< Value >( 1 )/static_cast< Value >( 5 );
            }
};


template< class Value = double >
struct rk78_coefficients_a6 : boost::array< Value , 6 >
{
    rk78_coefficients_a6( void )
            {
        (*this)[0] = static_cast< Value >( -25 )/static_cast< Value >( 108 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( 125 )/static_cast< Value >( 108 );
        (*this)[4] = static_cast< Value >( -65 )/static_cast< Value >( 27 );
        (*this)[5] = static_cast< Value >( 125 )/static_cast< Value >( 54 );
            }
};

template< class Value = double >
struct rk78_coefficients_a7 : boost::array< Value , 7 >
{
    rk78_coefficients_a7( void )
            {
        (*this)[0] = static_cast< Value >( 31 )/static_cast< Value >( 300 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( 0 );
        (*this)[4] = static_cast< Value >( 61 )/static_cast< Value >( 225 );
        (*this)[5] = static_cast< Value >( -2 )/static_cast< Value >( 9 );
        (*this)[6] = static_cast< Value >( 13 )/static_cast< Value >( 900 );
            }
};

template< class Value = double >
struct rk78_coefficients_a8 : boost::array< Value , 8 >
{
    rk78_coefficients_a8( void )
            {
        (*this)[0] = static_cast< Value >( 2 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( -53 )/static_cast< Value >( 6 );
        (*this)[4] = static_cast< Value >( 704 )/static_cast< Value >( 45 );
        (*this)[5] = static_cast< Value >( -107 )/static_cast< Value >( 9 );
        (*this)[6] = static_cast< Value >( 67 )/static_cast< Value >( 90 );
        (*this)[7] = static_cast< Value >( 3 );
            }
};

template< class Value = double >
struct rk78_coefficients_a9 : boost::array< Value , 9 >
{
    rk78_coefficients_a9( void )
            {
        (*this)[0] = static_cast< Value >( -91 )/static_cast< Value >( 108 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( 23 )/static_cast< Value >( 108 );
        (*this)[4] = static_cast< Value >( -976 )/static_cast< Value >( 135 );
        (*this)[5] = static_cast< Value >( 311 )/static_cast< Value >( 54 );
        (*this)[6] = static_cast< Value >( -19 )/static_cast< Value >( 60 );
        (*this)[7] = static_cast< Value >( 17 )/static_cast< Value >( 6 );
        (*this)[8] = static_cast< Value >( -1 )/static_cast< Value >( 12 );
            }
};

template< class Value = double >
struct rk78_coefficients_a10 : boost::array< Value , 10 >
{
    rk78_coefficients_a10( void )
            {
        (*this)[0] = static_cast< Value >( 2383 )/static_cast< Value >( 4100 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( -341 )/static_cast< Value >( 164 );
        (*this)[4] = static_cast< Value >( 4496 )/static_cast< Value >( 1025 );
        (*this)[5] = static_cast< Value >( -301 )/static_cast< Value >( 82 );
        (*this)[6] = static_cast< Value >( 2133 )/static_cast< Value >( 4100 );
        (*this)[7] = static_cast< Value >( 45 )/static_cast< Value >( 82 );
        (*this)[8] = static_cast< Value >( 45 )/static_cast< Value >( 164 );
        (*this)[9] = static_cast< Value >( 18 )/static_cast< Value >( 41 );
            }
};

template< class Value = double >
struct rk78_coefficients_a11 : boost::array< Value , 11 >
{
    rk78_coefficients_a11( void )
            {
        (*this)[0] = static_cast< Value >( 3 )/static_cast< Value >( 205 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( 0 );
        (*this)[4] = static_cast< Value >( 0 );
        (*this)[5] = static_cast< Value >( -6 )/static_cast< Value >( 41 );
        (*this)[6] = static_cast< Value >( -3 )/static_cast< Value >( 205 );
        (*this)[7] = static_cast< Value >( -3 )/static_cast< Value >( 41 );
        (*this)[8] = static_cast< Value >( 3 )/static_cast< Value >( 41 );
        (*this)[9] = static_cast< Value >( 6 )/static_cast< Value >( 41 );
        (*this)[10] = static_cast< Value >( 0 );
            }
};

template< class Value = double >
struct rk78_coefficients_a12 : boost::array< Value , 12 >
{
    rk78_coefficients_a12( void )
            {
        (*this)[0] = static_cast< Value >( -1777 )/static_cast< Value >( 4100 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( -341 )/static_cast< Value >( 164 );
        (*this)[4] = static_cast< Value >( 4496 )/static_cast< Value >( 1025 );
        (*this)[5] = static_cast< Value >( -289 )/static_cast< Value >( 82 );
        (*this)[6] = static_cast< Value >( 2193 )/static_cast< Value >( 4100 );
        (*this)[7] = static_cast< Value >( 51 )/static_cast< Value >( 82 );
        (*this)[8] = static_cast< Value >( 33 )/static_cast< Value >( 164 );
        (*this)[9] = static_cast< Value >( 12 )/static_cast< Value >( 41 );
        (*this)[10] = static_cast< Value >( 0 );
        (*this)[11] = static_cast< Value >( 1 );
            }
};

template< class Value = double >
struct rk78_coefficients_b : boost::array< Value , 13 >
{
    rk78_coefficients_b( void )
            {
        (*this)[0] = static_cast< Value >( 0 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( 0 );
        (*this)[4] = static_cast< Value >( 0 );
        (*this)[5] = static_cast< Value >( 34 )/static_cast<Value>( 105 );
        (*this)[6] = static_cast< Value >( 9 )/static_cast<Value>( 35 );
        (*this)[7] = static_cast< Value >( 9 )/static_cast<Value>( 35 );
        (*this)[8] = static_cast< Value >( 9 )/static_cast<Value>( 280 );
        (*this)[9] = static_cast< Value >( 9 )/static_cast<Value>( 280 );
        (*this)[10] = static_cast< Value >( 0 );
        (*this)[11] = static_cast< Value >( 41 )/static_cast<Value>( 840 );
        (*this)[12] = static_cast< Value >( 41 )/static_cast<Value>( 840 );
            }
};

template< class Value = double >
struct rk78_coefficients_db : boost::array< Value , 13 >
{
    rk78_coefficients_db( void )
            {
        (*this)[0] = static_cast< Value >( 0 ) - static_cast< Value >( 41 )/static_cast<Value>( 840 );
        (*this)[1] = static_cast< Value >( 0 );
        (*this)[2] = static_cast< Value >( 0 );
        (*this)[3] = static_cast< Value >( 0 );
        (*this)[4] = static_cast< Value >( 0 );
        (*this)[5] = static_cast< Value >( 0 );
        (*this)[6] = static_cast< Value >( 0 );
        (*this)[7] = static_cast< Value >( 0 );
        (*this)[8] = static_cast< Value >( 0 );
        (*this)[9] = static_cast< Value >( 0 );
        (*this)[10] = static_cast< Value >( 0 ) - static_cast< Value >( 41 )/static_cast<Value>( 840 );
        (*this)[11] = static_cast< Value >( 41 )/static_cast<Value>( 840 );
        (*this)[12] = static_cast< Value >( 41 )/static_cast<Value>( 840 );
            }
};


template< class Value = double >
struct rk78_coefficients_c : boost::array< Value , 13 >
{
    rk78_coefficients_c( void )
            {
        (*this)[0] = static_cast< Value >( 0 );
        (*this)[1] = static_cast< Value >( 2 )/static_cast< Value >( 27 );
        (*this)[2] = static_cast< Value >( 1 )/static_cast< Value >( 9 );
        (*this)[3] = static_cast< Value >( 1 )/static_cast<Value>( 6 );
        (*this)[4] = static_cast< Value >( 5 )/static_cast<Value>( 12 );
        (*this)[5] = static_cast< Value >( 1 )/static_cast<Value>( 2 );
        (*this)[6] = static_cast< Value >( 5 )/static_cast<Value>( 6 );
        (*this)[7] = static_cast< Value >( 1 )/static_cast<Value>( 6 );
        (*this)[8] = static_cast< Value >( 2 )/static_cast<Value>( 3 );
        (*this)[9] = static_cast< Value >( 1 )/static_cast<Value>( 3 );
        (*this)[10] = static_cast< Value >( 1 );
        (*this)[11] = static_cast< Value >( 0 );
        (*this)[12] = static_cast< Value >( 1 );
            }
};
#endif // DOXYGEN_SKIP





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
class runge_kutta_fehlberg78 : public explicit_error_generic_rk< 13 , 8 , 8 , 7 , State , Value , Deriv , Time ,
Algebra , Operations , Resizer >
#else
class runge_kutta_fehlberg78 : public explicit_error_generic_rk
#endif
{

public:
#ifndef DOXYGEN_SKIP
    typedef explicit_error_generic_rk< 13 , 8 , 8 , 7 , State , Value , Deriv , Time ,
            Algebra , Operations , Resizer > stepper_base_type;
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


    runge_kutta_fehlberg78( const algebra_type &algebra = algebra_type() ) : stepper_base_type(
            boost::fusion::make_vector( rk78_coefficients_a1<Value>() , rk78_coefficients_a2<Value>() , rk78_coefficients_a3<Value>() ,
                    rk78_coefficients_a4<Value>() , rk78_coefficients_a5<Value>() , rk78_coefficients_a6<Value>() ,
                    rk78_coefficients_a7<Value>() , rk78_coefficients_a8<Value>() , rk78_coefficients_a9<Value>() ,
                    rk78_coefficients_a10<Value>() , rk78_coefficients_a11<Value>() , rk78_coefficients_a12<Value>() ) ,
            rk78_coefficients_b<Value>() , rk78_coefficients_db<Value>() , rk78_coefficients_c<Value>() , algebra )
    { }
};



/************* DOXYGEN *************/

/**
 * \class runge_kutta_fehlberg78
 * \brief The Runge-Kutta Fehlberg 78 method.
 *
 * The Runge-Kutta Fehlberg 78 method is a standard method for high-precision applications.
 * The method is explicit and fulfills the Error Stepper concept. Step size control
 * is provided but continuous output is not available for this method.
 * 
 * This class derives from explicit_error_stepper_base and inherits its interface via CRTP (current recurring template pattern).
 * Furthermore, it derivs from explicit_error_generic_rk which is a generic Runge-Kutta algorithm with error estimation.
 * For more details see explicit_error_stepper_base and explicit_error_generic_rk.
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
     * \fn runge_kutta_fehlberg78::runge_kutta_fehlberg78( const algebra_type &algebra )
     * \brief Constructs the runge_kutta_cash_fehlberg78 class. This constructor can be used as a default
     * constructor if the algebra has a default constructor.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */

}
}
}

#endif //BOOST_NUMERIC_ODEINT_STEPPER_RUNGE_KUTTA_FEHLBERG87_HPP_INCLUDED
