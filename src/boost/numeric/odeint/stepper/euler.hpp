/*
 [auto_generated]
 boost/numeric/odeint/stepper/euler.hpp

 [begin_description]
 Implementation of the classical explicit Euler stepper. This method is really simple and should only
 be used for demonstration purposes.
 [end_description]

 Copyright 2010-2013 Karsten Ahnert
 Copyright 2010-2013 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_EULER_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_EULER_HPP_INCLUDED


#include <boost/numeric/odeint/stepper/base/explicit_stepper_base.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

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
class euler
: public explicit_stepper_base<
  euler< State , Value , Deriv , Time , Algebra , Operations , Resizer > ,
  1 , State , Value , Deriv , Time , Algebra , Operations , Resizer >
#else
class euler : public explicit_stepper_base
#endif
{
public :

    #ifndef DOXYGEN_SKIP
    typedef explicit_stepper_base< euler< State , Value , Deriv , Time , Algebra , Operations , Resizer > , 1 , State , Value , Deriv , Time , Algebra , Operations , Resizer > stepper_base_type;
    #else
    typedef explicit_stepper_base< euler< ... > , ... > stepper_base_type;
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
    #endif 


    euler( const algebra_type &algebra = algebra_type() ) : stepper_base_type( algebra )
    { }

    template< class System , class StateIn , class DerivIn , class StateOut >
    void do_step_impl( System /* system */ , const StateIn &in , const DerivIn &dxdt , time_type /* t */ , StateOut &out , time_type dt )
    {
        stepper_base_type::m_algebra.for_each3( out , in , dxdt ,
                typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , dt ) );

    }

    template< class StateOut , class StateIn1 , class StateIn2 >
    void calc_state( StateOut &x , time_type t ,  const StateIn1 &old_state , time_type t_old , const StateIn2 & /*current_state*/ , time_type /* t_new */ ) const
    {
        const time_type delta = t - t_old;
        stepper_base_type::m_algebra.for_each3( x , old_state , stepper_base_type::m_dxdt.m_v ,
                typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , delta ) );
    }

    template< class StateType >
    void adjust_size( const StateType &x )
    {
        stepper_base_type::adjust_size( x );
    }
};



/********** DOXYGEN ***********/

/**
 * \class euler
 * \brief An implementation of the Euler method.
 *
 * The Euler method is a very simply solver for ordinary differential equations. This method should not be used
 * for real applications. It is only useful for demonstration purposes. Step size control is not provided but
 * trivial continuous output is available.
 * 
 * This class derives from explicit_stepper_base and inherits its interface via CRTP (current recurring template pattern),
 * see explicit_stepper_base
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
     * \fn euler::euler( const algebra_type &algebra )
     * \brief Constructs the euler class. This constructor can be used as a default
     * constructor of the algebra has a default constructor.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */
    
    /**
     * \fn euler::do_step_impl( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
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
     * \fn euler::calc_state( StateOut &x , time_type t ,  const StateIn1 &old_state , time_type t_old , const StateIn2 &current_state , time_type t_new ) const
     * \brief This method is used for continuous output and it calculates the state `x` at a time `t` from the 
     * knowledge of two states `old_state` and `current_state` at time points `t_old` and `t_new`.
     */

    /**
     * \fn euler::adjust_size( const StateType &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_EULER_HPP_INCLUDED
