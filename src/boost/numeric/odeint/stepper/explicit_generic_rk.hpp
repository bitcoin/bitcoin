/*
 [auto_generated]
 boost/numeric/odeint/stepper/explicit_generic_rk.hpp

 [begin_description]
 Implementation of the generic Runge-Kutta steppers. This is the base class for many Runge-Kutta steppers.
 [end_description]

 Copyright 2011-2013 Mario Mulansky
 Copyright 2011-2013 Karsten Ahnert
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_EXPLICIT_GENERIC_RK_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_EXPLICIT_GENERIC_RK_HPP_INCLUDED


#include <boost/array.hpp>


#include <boost/numeric/odeint/stepper/base/explicit_stepper_base.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>
#include <boost/numeric/odeint/stepper/detail/generic_rk_algorithm.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

namespace boost {
namespace numeric {
namespace odeint {

//forward declarations

#ifndef DOXYGEN_SKIP
template<
size_t StageCount,
size_t Order,
class State ,
class Value = double ,
class Deriv = State ,
class Time = Value ,
class Algebra = typename algebra_dispatcher< State >::algebra_type ,
class Operations = typename operations_dispatcher< State >::operations_type ,
class Resizer = initially_resizer
>
class explicit_generic_rk;


struct stage_vector;

template< class T , class Constant >
struct array_wrapper
{
    typedef const typename boost::array< T , Constant::value > type;
};

template< class T , size_t i >
struct stage
{
    T c;
    boost::array< T , i > a;
};


template< class T , class Constant >
struct stage_wrapper
{
    typedef stage< T , Constant::value > type;
};
#endif


template<
size_t StageCount,
size_t Order,
class State ,
class Value ,
class Deriv ,
class Time ,
class Algebra ,
class Operations ,
class Resizer
>
#ifndef DOXYGEN_SKIP
class explicit_generic_rk : public explicit_stepper_base<
explicit_generic_rk< StageCount , Order , State , Value , Deriv , Time , Algebra , Operations , Resizer > ,
Order , State , Value , Deriv , Time , Algebra , Operations , Resizer >
#else
class explicit_generic_rk : public explicit_stepper_base
#endif
{

public:

    #ifndef DOXYGEN_SKIP
    typedef explicit_stepper_base<
            explicit_generic_rk< StageCount , Order , State , Value , Deriv ,Time , Algebra , Operations , Resizer > ,
            Order , State , Value , Deriv , Time , Algebra ,
            Operations , Resizer > stepper_base_type;
    #else
    typedef explicit_stepper_base< ... > stepper_base_type;
    #endif

    typedef typename stepper_base_type::state_type state_type;
    typedef typename stepper_base_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_base_type::value_type value_type;
    typedef typename stepper_base_type::deriv_type deriv_type;
    typedef typename stepper_base_type::wrapped_deriv_type wrapped_deriv_type;
    typedef typename stepper_base_type::time_type time_type;
    typedef typename stepper_base_type::algebra_type algebra_type;
    typedef typename stepper_base_type::operations_type operations_type;
    typedef typename stepper_base_type::resizer_type resizer_type;

    #ifndef DOXYGEN_SKIP
    typedef explicit_generic_rk< StageCount , Order , State , Value , Deriv ,Time , Algebra , Operations , Resizer > stepper_type;
    #endif 

    typedef detail::generic_rk_algorithm< StageCount , Value , Algebra , Operations > rk_algorithm_type;

    typedef typename rk_algorithm_type::coef_a_type coef_a_type;
    typedef typename rk_algorithm_type::coef_b_type coef_b_type;
    typedef typename rk_algorithm_type::coef_c_type coef_c_type;

    #ifndef DOXYGEN_SKIP
    static const size_t stage_count = StageCount;
    #endif

public:

    explicit_generic_rk( const coef_a_type &a , const coef_b_type &b , const coef_c_type &c ,
            const algebra_type &algebra = algebra_type() )
    : stepper_base_type( algebra ) , m_rk_algorithm( a , b , c )
    { }


    template< class System , class StateIn , class DerivIn , class StateOut >
    void do_step_impl( System system , const StateIn &in , const DerivIn &dxdt ,
            time_type t , StateOut &out , time_type dt )
    {
        m_resizer.adjust_size( in , detail::bind( &stepper_type::template resize_impl< StateIn > , detail::ref( *this ) , detail::_1 ) );

        // actual calculation done in generic_rk.hpp
        m_rk_algorithm.do_step( stepper_base_type::m_algebra , system , in , dxdt , t , out , dt , m_x_tmp.m_v , m_F );
    }

    template< class StateIn >
    void adjust_size( const StateIn &x )
    {
        resize_impl( x );
        stepper_base_type::adjust_size( x );
    }

private:

    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized( false );
        resized |= adjust_size_by_resizeability( m_x_tmp , x , typename is_resizeable<state_type>::type() );
        for( size_t i = 0 ; i < StageCount-1 ; ++i )
        {
            resized |= adjust_size_by_resizeability( m_F[i] , x , typename is_resizeable<deriv_type>::type() );
        }
        return resized;
    }


    rk_algorithm_type m_rk_algorithm;

    resizer_type m_resizer;

    wrapped_state_type m_x_tmp;
    wrapped_deriv_type m_F[StageCount-1];

};



/*********** DOXYGEN *************/

/**
 * \class explicit_generic_rk
 * \brief A generic implementation of explicit Runge-Kutta algorithms. This class is as a base class
 * for all explicit Runge-Kutta steppers.
 *
 * This class implements the explicit Runge-Kutta algorithms without error estimation in a generic way.
 * The Butcher tableau is passed to the stepper which constructs the stepper scheme with the help of a
 * template-metaprogramming algorithm. ToDo : Add example!
 * 
 * This class derives explicit_stepper_base which provides the stepper interface.
 *
 * \tparam StageCount The number of stages of the Runge-Kutta algorithm.
 * \tparam Order The order of the stepper.
 * \tparam State The type representing the state of the ODE.
 * \tparam Value The floating point type which is used in the computations.
 * \tparam Time The type representing the independent variable - the time - of the ODE.
 * \tparam Algebra The algebra type.
 * \tparam Operations The operations type.
 * \tparam Resizer The resizer policy type.
 */

    /**
     * \fn explicit_generic_rk::explicit_generic_rk( const coef_a_type &a , const coef_b_type &b , const coef_c_type &c , const algebra_type &algebra )
     * \brief Constructs the explicit_generic_rk class. See examples section for details on the coefficients.
     * \param a Triangular matrix of parameters b in the Butcher tableau.
     * \param b Last row of the butcher tableau.
     * \param c Parameters to calculate the time points in the Butcher tableau.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */
    
    /**
     * \fn explicit_generic_rk::do_step_impl( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
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
     * \fn explicit_generic_rk::adjust_size( const StateIn &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

}
}
}
#endif // BOOST_NUMERIC_ODEINT_STEPPER_EXPLICIT_GENERIC_RK_HPP_INCLUDED
