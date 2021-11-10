/*
 [auto_generated]
 boost/numeric/odeint/stepper/adams_bashforth_moulton.hpp

 [begin_description]
 Implementation of the Adams-Bashforth-Moulton method, a predictor-corrector multistep method.
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011-2013 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_BASHFORTH_MOULTON_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_BASHFORTH_MOULTON_HPP_INCLUDED


#include <boost/numeric/odeint/util/bind.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/odeint/stepper/adams_bashforth.hpp>
#include <boost/numeric/odeint/stepper/adams_moulton.hpp>



namespace boost {
namespace numeric {
namespace odeint {


template<
size_t Steps ,
class State ,
class Value = double ,
class Deriv = State ,
class Time = Value ,
class Algebra = typename algebra_dispatcher< State >::algebra_type ,
class Operations = typename operations_dispatcher< State >::operations_type ,
class Resizer = initially_resizer,
class InitializingStepper = runge_kutta4< State , Value , Deriv , Time , Algebra , Operations, Resizer >
>
class adams_bashforth_moulton
{

#ifndef DOXYGEN_SKIP
    BOOST_STATIC_ASSERT(( Steps > 0 ));
    BOOST_STATIC_ASSERT(( Steps < 9 ));
#endif

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
    typedef InitializingStepper initializing_stepper_type;

    static const size_t steps = Steps;
#ifndef DOXYGEN_SKIP
    typedef adams_bashforth< steps , state_type , value_type , deriv_type , time_type , algebra_type , operations_type , resizer_type, initializing_stepper_type > adams_bashforth_type;
    typedef adams_moulton< steps , state_type , value_type , deriv_type , time_type , algebra_type , operations_type , resizer_type > adams_moulton_type;
    typedef adams_bashforth_moulton< steps , state_type , value_type , deriv_type , time_type , algebra_type , operations_type , resizer_type , initializing_stepper_type> stepper_type;
#endif //DOXYGEN_SKIP
    typedef unsigned short order_type;
    static const order_type order_value = steps;

    /** \brief Constructs the adams_bashforth class. */
    adams_bashforth_moulton( void )
    : m_adams_bashforth() , m_adams_moulton( m_adams_bashforth.algebra() )
    , m_x() , m_resizer()
    { }

    adams_bashforth_moulton( const algebra_type &algebra )
    : m_adams_bashforth( algebra ) , m_adams_moulton( m_adams_bashforth.algebra() )
    , m_x() , m_resizer()    
    { }

    order_type order( void ) const { return order_value; }

    template< class System , class StateInOut >
    void do_step( System system , StateInOut &x , time_type t , time_type dt )
    {
        do_step_impl1( system , x , t , dt );
    }

    /**
     * \brief Second version to solve the forwarding problem, can be called with Boost.Range as StateInOut.
     */
    template< class System , class StateInOut >
    void do_step( System system , const StateInOut &x , time_type t , time_type dt )
    {
        do_step_impl1( system , x , t , dt );
    }

    template< class System , class StateIn , class StateOut >
    void do_step( System system , const StateIn &in , time_type t , const StateOut &out , time_type dt )
    {
        do_step_impl2( system , in , t , out , dt );
    }

    /**
     * \brief Second version to solve the forwarding problem, can be called with Boost.Range as StateOut.
     */
    template< class System , class StateIn , class StateOut >
    void do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
    {
        do_step_impl2( system , in ,t , out , dt );
    }


    template< class StateType >
    void adjust_size( const StateType &x )
    {
        m_adams_bashforth.adjust_size( x );
        m_adams_moulton.adjust_size( x );
        resize_impl( x );
    }


    template< class ExplicitStepper , class System , class StateIn >
    void initialize( ExplicitStepper explicit_stepper , System system , StateIn &x , time_type &t , time_type dt )
    {
        m_adams_bashforth.initialize( explicit_stepper , system , x , t , dt );
    }


    template< class System , class StateIn >
    void initialize( System system , StateIn &x , time_type &t , time_type dt )
    {
        m_adams_bashforth.initialize( system , x , t , dt );
    }


    void reset(void)
    {
        m_adams_bashforth.reset();
    }



private:
    
    template< typename System , typename StateInOut >
    void do_step_impl1( System system , StateInOut &x , time_type t , time_type dt )
    {
        if( m_adams_bashforth.is_initialized() )
        {
            m_resizer.adjust_size( x , detail::bind( &stepper_type::template resize_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) );
            m_adams_bashforth.do_step( system , x , t , m_x.m_v , dt );
            m_adams_moulton.do_step( system , x , m_x.m_v , t+dt , x , dt , m_adams_bashforth.step_storage() );
        }
        else
        {
            m_adams_bashforth.do_step( system , x , t , dt );
        }
    }
    
    template< typename System , typename StateIn , typename StateInOut >
    void do_step_impl2( System system , StateIn const &in , time_type t , StateInOut & out , time_type dt )
    {
        if( m_adams_bashforth.is_initialized() )
        {
            m_resizer.adjust_size( in , detail::bind( &stepper_type::template resize_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) );        
            m_adams_bashforth.do_step( system , in , t , m_x.m_v , dt );
            m_adams_moulton.do_step( system , in , m_x.m_v , t+dt , out , dt , m_adams_bashforth.step_storage() );
        }
        else
        {
            m_adams_bashforth.do_step( system , in , t , out , dt );
        }
    }

    
    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_x , x , typename is_resizeable< state_type >::type() );
    }

    adams_bashforth_type m_adams_bashforth;
    adams_moulton_type m_adams_moulton;
    wrapped_state_type m_x;
    resizer_type m_resizer;
};


/********* DOXYGEN ********/

/**
 * \class adams_bashforth_moulton
 * \brief The Adams-Bashforth-Moulton multistep algorithm.
 *
 * The Adams-Bashforth method is a multi-step predictor-corrector algorithm 
 * with configurable step number. The step number is specified as template 
 * parameter Steps and it then uses the result from the previous Steps steps. 
 * See also
 * <a href="http://en.wikipedia.org/wiki/Linear_multistep_method">en.wikipedia.org/wiki/Linear_multistep_method</a>.
 * Currently, a maximum of Steps=8 is supported.
 * The method is explicit and fulfills the Stepper concept. Step size control
 * or continuous output are not provided.
 * 
 * This class derives from algebra_base and inherits its interface via
 * CRTP (current recurring template pattern). For more details see
 * algebra_stepper_base.
 *
 * \tparam Steps The number of steps (maximal 8).
 * \tparam State The state type.
 * \tparam Value The value type.
 * \tparam Deriv The type representing the time derivative of the state.
 * \tparam Time The time representing the independent variable - the time.
 * \tparam Algebra The algebra type.
 * \tparam Operations The operations type.
 * \tparam Resizer The resizer policy type.
 * \tparam InitializingStepper The stepper for the first two steps.
 */

    /**
     * \fn adams_bashforth_moulton::adams_bashforth_moulton( const algebra_type &algebra )
     * \brief Constructs the adams_bashforth class. This constructor can be used as a default
     * constructor if the algebra has a default constructor. 
     * \param algebra A copy of algebra is made and stored.
     */

    /**
     * \fn adams_bashforth_moulton::order( void ) const
     * \brief Returns the order of the algorithm, which is equal to the number of steps+1.
     * \return order of the method.
     */

    /**
     * \fn adams_bashforth_moulton::do_step( System system , StateInOut &x , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place.
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */


    /**
     * \fn adams_bashforth_moulton::do_step( System system , const StateIn &in , time_type t , const StateOut &out , time_type dt )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dt The step size.
     */

    /**
     * \fn adams_bashforth_moulton::adjust_size( const StateType &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

    /**
     * \fn adams_bashforth_moulton::initialize( ExplicitStepper explicit_stepper , System system , StateIn &x , time_type &t , time_type dt )
     * \brief Initialized the stepper. Does Steps-1 steps with the explicit_stepper to fill the buffer.
     * \note The state x and time t are updated to the values after Steps-1 initial steps.
     * \param explicit_stepper the stepper used to fill the buffer of previous step results
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The initial state of the ODE which should be solved, updated after in this method.
     * \param t The initial time, updated in this method.
     * \param dt The step size.
     */

    /**
     * \fn adams_bashforth_moulton::initialize( System system , StateIn &x , time_type &t , time_type dt )
     * \brief Initialized the stepper. Does Steps-1 steps using the standard initializing stepper 
     * of the underlying adams_bashforth stepper.
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    /**
     * \fn adams_bashforth_moulton::reset( void )
     * \brief Resets the internal buffers of the stepper.
     */


} // odeint
} // numeric
} // boost



#endif // BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_BASHFORTH_MOULTON_HPP_INCLUDED
