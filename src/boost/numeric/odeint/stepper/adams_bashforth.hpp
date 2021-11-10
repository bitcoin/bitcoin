/*
 [auto_generated]
 boost/numeric/odeint/stepper/adams_bashforth.hpp

 [begin_description]
 Implementaton of the Adam-Bashforth method a multistep method used for the predictor step in the
 Adams-Bashforth-Moulton method.
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011-2013 Mario Mulansky
 Copyright 2012 Christoph Koke
 Copyright 2013 Pascal Germroth

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_BASHFORTH_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_BASHFORTH_HPP_INCLUDED

#include <boost/static_assert.hpp>

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>

#include <boost/numeric/odeint/algebra/range_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/stepper/runge_kutta4.hpp>
#include <boost/numeric/odeint/stepper/extrapolation_stepper.hpp>

#include <boost/numeric/odeint/stepper/base/algebra_stepper_base.hpp>

#include <boost/numeric/odeint/stepper/detail/adams_bashforth_coefficients.hpp>
#include <boost/numeric/odeint/stepper/detail/adams_bashforth_call_algebra.hpp>
#include <boost/numeric/odeint/stepper/detail/rotating_buffer.hpp>

#include <boost/mpl/arithmetic.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/mpl/equal_to.hpp>

namespace mpl = boost::mpl;


namespace boost {
namespace numeric {
namespace odeint {

    using mpl::int_;

    /* if N >= 4, returns the smallest even number > N, otherwise returns 4 */
    template < int N >
    struct order_helper
        : mpl::max< typename mpl::eval_if<
                        mpl::equal_to< mpl::modulus< int_< N >, int_< 2 > >,
                                       int_< 0 > >,
                        int_< N >, int_< N + 1 > >::type,
                    int_< 4 > >::type
    { };

template<
size_t Steps ,
class State ,
class Value = double ,
class Deriv = State ,
class Time = Value ,
class Algebra = typename algebra_dispatcher< State >::algebra_type ,
class Operations = typename operations_dispatcher< State >::operations_type ,
class Resizer = initially_resizer ,
class InitializingStepper = extrapolation_stepper< order_helper<Steps>::value, 
                                                   State, Value, Deriv, Time,
                                                   Algebra, Operations, Resizer >
>
class adams_bashforth : public algebra_stepper_base< Algebra , Operations >
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
    typedef Resizer resizer_type;
    typedef stepper_tag stepper_category;

    typedef InitializingStepper initializing_stepper_type;

    typedef algebra_stepper_base< Algebra , Operations > algebra_stepper_base_type;
    typedef typename algebra_stepper_base_type::algebra_type algebra_type;
    typedef typename algebra_stepper_base_type::operations_type operations_type;
#ifndef DOXYGEN_SKIP
    typedef adams_bashforth< Steps , State , Value , Deriv , Time , Algebra , Operations , Resizer , InitializingStepper > stepper_type;
#endif
    static const size_t steps = Steps;



    typedef unsigned short order_type;
    static const order_type order_value = steps;

    typedef detail::rotating_buffer< wrapped_deriv_type , steps > step_storage_type;


    
    order_type order( void ) const { return order_value; }

    adams_bashforth( const algebra_type &algebra = algebra_type() )
    : algebra_stepper_base_type( algebra ) ,
      m_step_storage() , m_resizer() , m_coefficients() ,
      m_steps_initialized( 0 ) , m_initializing_stepper()
    { }



    /*
     * Version 1 : do_step( system , x , t , dt );
     *
     * solves the forwarding problem
     */
    template< class System , class StateInOut >
    void do_step( System system , StateInOut &x , time_type t , time_type dt )
    {
        do_step( system , x , t , x , dt );
    }

    /**
     * \brief Second version to solve the forwarding problem, can be called with Boost.Range as StateInOut.
     */
    template< class System , class StateInOut >
    void do_step( System system , const StateInOut &x , time_type t , time_type dt )
    {
        do_step( system , x , t , x , dt );
    }



    /*
     * Version 2 : do_step( system , in , t , out , dt );
     *
     * solves the forwarding problem
     */

    template< class System , class StateIn , class StateOut >
    void do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
    {
        do_step_impl( system , in , t , out , dt );
    }

    /**
     * \brief Second version to solve the forwarding problem, can be called with Boost.Range as StateOut.
     */
    template< class System , class StateIn , class StateOut >
    void do_step( System system , const StateIn &in , time_type t , const StateOut &out , time_type dt )
    {
        do_step_impl( system , in , t , out , dt );
    }


    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_impl( x );
    }

    const step_storage_type& step_storage( void ) const
    {
        return m_step_storage;
    }

    step_storage_type& step_storage( void )
    {
        return m_step_storage;
    }

    template< class ExplicitStepper , class System , class StateIn >
    void initialize( ExplicitStepper explicit_stepper , System system , StateIn &x , time_type &t , time_type dt )
    {
        typename odeint::unwrap_reference< ExplicitStepper >::type &stepper = explicit_stepper;
        typename odeint::unwrap_reference< System >::type &sys = system;

        m_resizer.adjust_size( x , detail::bind( &stepper_type::template resize_impl<StateIn> , detail::ref( *this ) , detail::_1 ) );

        for( size_t i=0 ; i+1<steps ; ++i )
        {
            if( i != 0 ) m_step_storage.rotate();
            sys( x , m_step_storage[0].m_v , t );
            stepper.do_step_dxdt_impl( system, x, m_step_storage[0].m_v, t,
                                       dt );
            t += dt;
        }
        m_steps_initialized = steps;
    }

    template< class System , class StateIn >
    void initialize( System system , StateIn &x , time_type &t , time_type dt )
    {
        initialize( detail::ref( m_initializing_stepper ) , system , x , t , dt );
    }

    void reset( void )
    {
        m_steps_initialized = 0;
    }

    bool is_initialized( void ) const
    {
        return m_steps_initialized >= ( steps - 1 );
    }

    const initializing_stepper_type& initializing_stepper( void ) const { return m_initializing_stepper; }

    initializing_stepper_type& initializing_stepper( void ) { return m_initializing_stepper; }

private:

    template< class System , class StateIn , class StateOut >
    void do_step_impl( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        if( m_resizer.adjust_size( in , detail::bind( &stepper_type::template resize_impl<StateIn> , detail::ref( *this ) , detail::_1 ) ) )
        {
            m_steps_initialized = 0;
        }

        if( m_steps_initialized + 1 < steps )
        {
            if( m_steps_initialized != 0 ) m_step_storage.rotate();
            sys( in , m_step_storage[0].m_v , t );
            m_initializing_stepper.do_step_dxdt_impl(
                system, in, m_step_storage[0].m_v, t, out, dt );
            ++m_steps_initialized;
        }
        else
        {
            m_step_storage.rotate();
            sys( in , m_step_storage[0].m_v , t );
            detail::adams_bashforth_call_algebra< steps , algebra_type , operations_type >()( this->m_algebra , in , out , m_step_storage , m_coefficients , dt );
        }
    }


    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        bool resized( false );
        for( size_t i=0 ; i<steps ; ++i )
        {
            resized |= adjust_size_by_resizeability( m_step_storage[i] , x , typename is_resizeable<deriv_type>::type() );
        }
        return resized;
    }

    step_storage_type m_step_storage;
    resizer_type m_resizer;
    detail::adams_bashforth_coefficients< value_type , steps > m_coefficients;
    size_t m_steps_initialized;
    initializing_stepper_type m_initializing_stepper;

};


/***** DOXYGEN *****/

/**
 * \class adams_bashforth
 * \brief The Adams-Bashforth multistep algorithm.
 *
 * The Adams-Bashforth method is a multi-step algorithm with configurable step
 * number. The step number is specified as template parameter Steps and it 
 * then uses the result from the previous Steps steps. See also
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
     * \fn adams_bashforth::adams_bashforth( const algebra_type &algebra )
     * \brief Constructs the adams_bashforth class. This constructor can be used as a default
     * constructor if the algebra has a default constructor. 
     * \param algebra A copy of algebra is made and stored.
     */

    /**
     * \fn order_type adams_bashforth::order( void ) const
     * \brief Returns the order of the algorithm, which is equal to the number of steps.
     * \return order of the method.
     */

    /**
     * \fn void adams_bashforth::do_step( System system , StateInOut &x , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place.
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    /**
     * \fn void adams_bashforth::do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
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
     * \fn void adams_bashforth::adjust_size( const StateType &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */


    /**
     * \fn const step_storage_type& adams_bashforth::step_storage( void ) const
     * \brief Returns the storage of intermediate results.
     * \return The storage of intermediate results.
     */

    /**
     * \fn step_storage_type& adams_bashforth::step_storage( void )
     * \brief Returns the storage of intermediate results.
     * \return The storage of intermediate results.
     */

    /**
     * \fn void adams_bashforth::initialize( ExplicitStepper explicit_stepper , System system , StateIn &x , time_type &t , time_type dt )
     * \brief Initialized the stepper. Does Steps-1 steps with the explicit_stepper to fill the buffer.
     * \param explicit_stepper the stepper used to fill the buffer of previous step results
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    /**
     * \fn void adams_bashforth::initialize( System system , StateIn &x , time_type &t , time_type dt )
     * \brief Initialized the stepper. Does Steps-1 steps with an internal instance of InitializingStepper to fill the buffer.
     * \note The state x and time t are updated to the values after Steps-1 initial steps.
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The initial state of the ODE which should be solved, updated in this method.
     * \param t The initial value of the time, updated in this method.
     * \param dt The step size.
     */

    /**
     * \fn void adams_bashforth::reset( void )
     * \brief Resets the internal buffer of the stepper.
     */

    /**
     * \fn bool adams_bashforth::is_initialized( void ) const
     * \brief Returns true if the stepper has been initialized.
     * \return bool true if stepper is initialized, false otherwise
     */

    /**
     * \fn const initializing_stepper_type& adams_bashforth::initializing_stepper( void ) const
     * \brief Returns the internal initializing stepper instance.
     * \return initializing_stepper
     */

    /**
     * \fn const initializing_stepper_type& adams_bashforth::initializing_stepper( void ) const
     * \brief Returns the internal initializing stepper instance.
     * \return initializing_stepper
     */

    /**
     * \fn initializing_stepper_type& adams_bashforth::initializing_stepper( void )
     * \brief Returns the internal initializing stepper instance.
     * \return initializing_stepper
     */

} // odeint
} // numeric
} // boost



#endif // BOOST_NUMERIC_ODEINT_STEPPER_ADAMS_BASHFORTH_HPP_INCLUDED
