/*
 [auto_generated]
 boost/numeric/odeint/stepper/base/explicit_error_stepper_fsal_base.hpp

 [begin_description]
 Base class for all explicit first-same-as-last Runge Kutta steppers.
 [end_description]

 Copyright 2010-2013 Karsten Ahnert
 Copyright 2010-2012 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_ERROR_STEPPER_FSAL_BASE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_ERROR_STEPPER_FSAL_BASE_HPP_INCLUDED

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>


#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>
#include <boost/numeric/odeint/util/copy.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/stepper/base/algebra_stepper_base.hpp>

namespace boost {
namespace numeric {
namespace odeint {

/*
 * base class for explicit stepper and error steppers with the fsal property
 * models the stepper AND the error stepper fsal concept
 * 
 * this class provides the following do_step overloads
    * do_step( sys , x , t , dt )
    * do_step( sys , x , dxdt , t , dt )
    * do_step( sys , in , t , out , dt )
    * do_step( sys , in , dxdt_in , t , out , dxdt_out , dt )
    * do_step( sys , x , t , dt , xerr )
    * do_step( sys , x , dxdt , t , dt , xerr )
    * do_step( sys , in , t , out , dt , xerr )
    * do_step( sys , in , dxdt_in , t , out , dxdt_out , dt , xerr )
 */
template<
class Stepper ,
unsigned short Order ,
unsigned short StepperOrder ,
unsigned short ErrorOrder ,
class State ,
class Value ,
class Deriv ,
class Time ,
class Algebra ,
class Operations ,
class Resizer
>
class explicit_error_stepper_fsal_base : public algebra_stepper_base< Algebra , Operations >
{
public:

    typedef algebra_stepper_base< Algebra , Operations > algebra_stepper_base_type;
    typedef typename algebra_stepper_base_type::algebra_type algebra_type;

    typedef State state_type;
    typedef Value value_type;
    typedef Deriv deriv_type;
    typedef Time time_type;
    typedef Resizer resizer_type;
    typedef Stepper stepper_type;
    typedef explicit_error_stepper_fsal_tag stepper_category;

    #ifndef DOXYGEN_SKIP
    typedef state_wrapper< state_type > wrapped_state_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    typedef explicit_error_stepper_fsal_base< Stepper , Order , StepperOrder , ErrorOrder ,
            State , Value , Deriv , Time , Algebra , Operations , Resizer > internal_stepper_base_type;
    #endif 


    typedef unsigned short order_type;
    static const order_type order_value = Order;
    static const order_type stepper_order_value = StepperOrder;
    static const order_type error_order_value = ErrorOrder;

    explicit_error_stepper_fsal_base( const algebra_type &algebra = algebra_type() )
    : algebra_stepper_base_type( algebra ) , m_first_call( true )
    { }

    order_type order( void ) const
    {
        return order_value;
    }

    order_type stepper_order( void ) const
    {
        return stepper_order_value;
    }

    order_type error_order( void ) const
    {
        return error_order_value;
    }


    /*
     * version 1 : do_step( sys , x , t , dt )
     *
     * the two overloads are needed in order to solve the forwarding problem
     */
    template< class System , class StateInOut >
    void do_step( System system , StateInOut &x , time_type t , time_type dt )
    {
        do_step_v1( system , x , t , dt );
    }

    /**
     * \brief Second version to solve the forwarding problem, can be called with Boost.Range as StateInOut.
     */
    template< class System , class StateInOut >
    void do_step( System system , const StateInOut &x , time_type t , time_type dt )
    {
        do_step_v1( system , x , t , dt );
    }


    /*
     * version 2 : do_step( sys , x , dxdt , t , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     *
     * the disable is needed to avoid ambiguous overloads if state_type = time_type
     */
    template< class System , class StateInOut , class DerivInOut >
    typename boost::disable_if< boost::is_same< StateInOut , time_type > , void >::type
    do_step( System system , StateInOut &x , DerivInOut &dxdt , time_type t , time_type dt )
    {
        m_first_call = true;
        this->stepper().do_step_impl( system , x , dxdt , t , x , dxdt , dt );
    }


    /*
     * named Version 2: do_step_dxdt_impl( sys , in , dxdt , t , dt )
     *
     * this version is needed when this stepper is used for initializing 
     * multistep stepper like adams-bashforth. Hence we provide an explicitely
     * named version that is not disabled. Meant for internal use only.
     */
    template< class System , class StateInOut , class DerivInOut >
    void do_step_dxdt_impl( System system , StateInOut &x , DerivInOut &dxdt , time_type t , time_type dt )
    {
        m_first_call = true;
        this->stepper().do_step_impl( system , x , dxdt , t , x , dxdt , dt );
    }

    /*
     * version 3 : do_step( sys , in , t , out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not
     * be used.
     *
     * the disable is needed to avoid ambiguous overloads if 
     * state_type = time_type
     */
    template< class System , class StateIn , class StateOut >
    typename boost::disable_if< boost::is_same< StateIn , time_type > , void >::type
    do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
    {
        if( m_resizer.adjust_size( in , detail::bind( &internal_stepper_base_type::template resize_impl< StateIn > , detail::ref( *this ) , detail::_1 ) ) || m_first_call )
        {
            initialize( system , in , t );
        }
        this->stepper().do_step_impl( system , in , m_dxdt.m_v , t , out , m_dxdt.m_v , dt );
    }


    /*
     * version 4 : do_step( sys , in , dxdt_in , t , out , dxdt_out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System, class StateIn, class DerivIn, class StateOut,
               class DerivOut >
    void do_step( System system, const StateIn &in, const DerivIn &dxdt_in,
                  time_type t, StateOut &out, DerivOut &dxdt_out, time_type dt )
    {
        m_first_call = true;
        this->stepper().do_step_impl( system, in, dxdt_in, t, out, dxdt_out,
                                      dt );
    }





    /*
     * version 5 : do_step( sys , x , t , dt , xerr )
     *
     * the two overloads are needed in order to solve the forwarding problem
     */
    template< class System , class StateInOut , class Err >
    void do_step( System system , StateInOut &x , time_type t , time_type dt , Err &xerr )
    {
        do_step_v5( system , x , t , dt , xerr );
    }

    /**
     * \brief Second version to solve the forwarding problem, can be called with Boost.Range as StateInOut.
     */
    template< class System , class StateInOut , class Err >
    void do_step( System system , const StateInOut &x , time_type t , time_type dt , Err &xerr )
    {
        do_step_v5( system , x , t , dt , xerr );
    }


    /*
     * version 6 : do_step( sys , x , dxdt , t , dt , xerr )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     *
     * the disable is needed to avoid ambiguous overloads if state_type = time_type
     */
    template< class System , class StateInOut , class DerivInOut , class Err >
    typename boost::disable_if< boost::is_same< StateInOut , time_type > , void >::type
    do_step( System system , StateInOut &x , DerivInOut &dxdt , time_type t , time_type dt , Err &xerr )
    {
        m_first_call = true;
        this->stepper().do_step_impl( system , x , dxdt , t , x , dxdt , dt , xerr );
    }




    /*
     * version 7 : do_step( sys , in , t , out , dt , xerr )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System , class StateIn , class StateOut , class Err >
    void do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt , Err &xerr )
    {
        if( m_resizer.adjust_size( in , detail::bind( &internal_stepper_base_type::template resize_impl< StateIn > , detail::ref( *this ) , detail::_1 ) ) || m_first_call )
        {
            initialize( system , in , t );
        }
        this->stepper().do_step_impl( system , in , m_dxdt.m_v , t , out , m_dxdt.m_v , dt , xerr );
    }


    /*
     * version 8 : do_step( sys , in , dxdt_in , t , out , dxdt_out , dt , xerr )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System , class StateIn , class DerivIn , class StateOut , class DerivOut , class Err >
    void do_step( System system , const StateIn &in , const DerivIn &dxdt_in , time_type t ,
            StateOut &out , DerivOut &dxdt_out , time_type dt , Err &xerr )
    {
        m_first_call = true;
        this->stepper().do_step_impl( system , in , dxdt_in , t , out , dxdt_out , dt , xerr );
    }

    template< class StateIn >
    void adjust_size( const StateIn &x )
    {
        resize_impl( x );
    }

    void reset( void )
    {
        m_first_call = true;
    }

    template< class DerivIn >
    void initialize( const DerivIn &deriv )
    {
        boost::numeric::odeint::copy( deriv , m_dxdt.m_v );
        m_first_call = false;
    }

    template< class System , class StateIn >
    void initialize( System system , const StateIn &x , time_type t )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        sys( x , m_dxdt.m_v , t );
        m_first_call = false;
    }

    bool is_initialized( void ) const
    {
        return ! m_first_call;
    }



private:

    template< class System , class StateInOut >
    void do_step_v1( System system , StateInOut &x , time_type t , time_type dt )
    {
        if( m_resizer.adjust_size( x , detail::bind( &internal_stepper_base_type::template resize_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) ) || m_first_call )
        {
            initialize( system , x , t );
        }
        this->stepper().do_step_impl( system , x , m_dxdt.m_v , t , x , m_dxdt.m_v , dt );
    }

    template< class System , class StateInOut , class Err >
    void do_step_v5( System system , StateInOut &x , time_type t , time_type dt , Err &xerr )
    {
        if( m_resizer.adjust_size( x , detail::bind( &internal_stepper_base_type::template resize_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) ) || m_first_call )
        {
            initialize( system , x , t );
        }
        this->stepper().do_step_impl( system , x , m_dxdt.m_v , t , x , m_dxdt.m_v , dt , xerr );
    }

    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
    }


    stepper_type& stepper( void )
    {
        return *static_cast< stepper_type* >( this );
    }

    const stepper_type& stepper( void ) const
    {
        return *static_cast< const stepper_type* >( this );
    }


    resizer_type m_resizer;
    bool m_first_call;

protected:


    wrapped_deriv_type m_dxdt;
};


/******* DOXYGEN *******/

/**
 * \class explicit_error_stepper_fsal_base
 * \brief Base class for explicit steppers with error estimation and stepper fulfilling the FSAL (first-same-as-last)
 * property. This class can be used with controlled steppers for step size control.
 *
 * This class serves as the base class for all explicit steppers with algebra and operations and which fulfill the FSAL
 * property. In contrast to explicit_stepper_base it also estimates the error and can be used in a controlled stepper
 * to provide step size control.
 *
 * The FSAL property means that the derivative of the system at t+dt is already used in the current step going from
 * t to t +dt. Therefore, some more do_steps method can be introduced and the controlled steppers can explicitly make use
 * of this property.
 *
 * \note This stepper provides `do_step` methods with and without error estimation. It has therefore three orders,
 * one for the order of a step if the error is not estimated. The other two orders are the orders of the step and 
 * the error step if the error estimation is performed.
 *
 * explicit_error_stepper_fsal_base  is used as the interface in a CRTP (currently recurring template
 * pattern). In order to work correctly the parent class needs to have a method
 * `do_step_impl( system , in , dxdt_in , t , out , dxdt_out , dt , xerr )`. 
 * explicit_error_stepper_fsal_base derives from algebra_stepper_base.
 *
 * This class can have an intrinsic state depending on the explicit usage of the `do_step` method. This means that some
 * `do_step` methods are expected to be called in order. For example the `do_step( sys , x , t , dt , xerr )` will keep track 
 * of the derivative of `x` which is the internal state. The first call of this method is recognized such that one
 * does not explicitly initialize the internal state, so it is safe to use this method like
 *
 * \code
 * stepper_type stepper;
 * stepper.do_step( sys , x , t , dt , xerr );
 * stepper.do_step( sys , x , t , dt , xerr );
 * stepper.do_step( sys , x , t , dt , xerr );
 * \endcode
 *
 * But it is unsafe to call this method with different system functions after each other. Do do so, one must initialize the
 * internal state with the `initialize` method or reset the internal state with the `reset` method.
 *
 * explicit_error_stepper_fsal_base provides several overloaded `do_step` methods, see the list below. Only two of them are needed
 * to fulfill the Error Stepper concept. The other ones are for convenience and for better performance. Some of them
 * simply update the state out-of-place, while other expect that the first derivative at `t` is passed to the stepper.
 *
 * - `do_step( sys , x , t , dt )` - The classical `do_step` method needed to fulfill the Error Stepper concept. The
 *      state is updated in-place. A type modelling a Boost.Range can be used for x.
 * - `do_step( sys , x , dxdt , t , dt )` - This method updates the state x and the derivative dxdt in-place. It is expected
 *     that dxdt has the value of the derivative of x at time t.
 * - `do_step( sys , in , t , out , dt )` - This method updates the state out-of-place, hence the result of the step
 *      is stored in `out`.
 * - `do_step( sys , in , dxdt_in , t , out , dxdt_out , dt )` - This method updates the state and the derivative
 *     out-of-place. It expects that the derivative at the point `t` is explicitly passed in `dxdt_in`.
 * - `do_step( sys , x , t , dt , xerr )` - This `do_step` method is needed to fulfill the Error Stepper concept. The
 *     state is updated in-place and an error estimate is calculated. A type modelling a Boost.Range can be used for x.
 * - `do_step( sys , x , dxdt , t , dt , xerr )` - This method updates the state and the derivative in-place. It is assumed
 *      that the dxdt has the value of the derivative of x at time t. An error estimate is calculated.
 * - `do_step( sys , in , t , out , dt , xerr )` - This method updates the state out-of-place and estimates the error
 *      during the step.
 * - `do_step( sys , in , dxdt_in , t , out , dxdt_out , dt , xerr )` - This methods updates the state and the derivative
 *      out-of-place and estimates the error during the step. It is assumed the dxdt_in is derivative of in at time t.
 *
 * \note The system is always passed as value, which might result in poor performance if it contains data. In this
 *      case it can be used with `boost::ref` or `std::ref`, for example `stepper.do_step( boost::ref( sys ) , x , t , dt );`
 *
 * \note The time `t` is not advanced by the stepper. This has to done manually, or by the appropriate `integrate`
 *      routines or `iterator`s.
 *
 * \tparam Stepper The stepper on which this class should work. It is used via CRTP, hence explicit_stepper_base
 * provides the interface for the Stepper.
 * \tparam Order The order of a stepper if the stepper is used without error estimation.
 * \tparam StepperOrder The order of a step if the stepper is used with error estimation. Usually Order and StepperOrder have 
 * the same value.
 * \tparam ErrorOrder The order of the error step if the stepper is used with error estimation.
 * \tparam State The state type for the stepper.
 * \tparam Value The value type for the stepper. This should be a floating point type, like float,
 * double, or a multiprecision type. It must not necessary be the value_type of the State. For example
 * the State can be a `vector< complex< double > >` in this case the Value must be double.
 * The default value is double.
 * \tparam Deriv The type representing time derivatives of the state type. It is usually the same type as the
 * state type, only if used with Boost.Units both types differ.
 * \tparam Time The type representing the time. Usually the same type as the value type. When Boost.Units is
 * used, this type has usually a unit.
 * \tparam Algebra The algebra type which must fulfill the Algebra Concept.
 * \tparam Operations The type for the operations which must fulfill the Operations Concept.
 * \tparam Resizer The resizer policy class.
 */



    /**
     * \fn explicit_error_stepper_fsal_base::explicit_error_stepper_fsal_base( const algebra_type &algebra )
     * \brief Constructs a explicit_stepper_fsal_base class. This constructor can be used as a default
     * constructor if the algebra has a default constructor.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */


    /**
     * \fn explicit_error_stepper_fsal_base::order( void ) const
     * \return Returns the order of the stepper if it used without error estimation.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::stepper_order( void ) const
     * \return Returns the order of a step if the stepper is used without error estimation.
     */


    /**
     * \fn explicit_error_stepper_fsal_base::error_order( void ) const
     * \return Returns the order of an error step if the stepper is used without error estimation.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , StateInOut &x , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place.
     *
     * \note This method uses the internal state of the stepper.
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */


    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , StateInOut &x , DerivInOut &dxdt , time_type t , time_type dt )
     * \brief The method performs one step with the stepper passed by Stepper. Additionally to the other methods
     * the derivative of x is also passed to this method. Therefore, dxdt must be evaluated initially:
     *
     * \code
     * ode( x , dxdt , t );
     * for( ... )
     * {
     *     stepper.do_step( ode , x , dxdt , t , dt );
     *     t += dt;
     * }
     * \endcode
     *
     * \note This method does NOT use the initial state, since the first derivative is explicitly passed to this method.
     *
     * The result is updated in place in x as well as the derivative dxdt. This method is disabled if
     * Time and StateInOut are of the same type. In this case the method could not be distinguished from other `do_step`
     * versions.
     * 
     * \note This method does not solve the forwarding problem.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param dxdt The derivative of x at t. After calling `do_step` dxdt is updated to the new value.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * This method is disabled if StateIn and Time are the same type. In this case the method can not be distinguished from
     * other `do_step` variants.
     *
     * \note This method uses the internal state of the stepper.
     *
     * \note This method does not solve the forwarding problem. 
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dt The step size.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , const StateIn &in , const DerivIn &dxdt_in , time_type t , StateOut &out , DerivOut &dxdt_out , time_type dt )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * Furthermore, the derivative of x at t is passed to the stepper and updated by the stepper to its new value at
     * t+dt.
     *
     * \note This method does not solve the forwarding problem.
     *
     * \note This method does NOT use the internal state of the stepper.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param dxdt_in The derivative of x at t.
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dxdt_out The updated derivative of `out` at `t+dt`.
     * \param dt The step size.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , StateInOut &x , time_type t , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper and estimates the error. The state of the ODE
     * is updated in-place.
     *
     *
     * \note This method uses the internal state of the stepper.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. x is updated by this method.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     * \param xerr The estimation of the error is stored in xerr.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , StateInOut &x , DerivInOut &dxdt , time_type t , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper. Additionally to the other method
     * the derivative of x is also passed to this method and updated by this method.
     *
     * \note This method does NOT use the internal state of the stepper.
     *
     * The result is updated in place in x. This method is disabled if Time and Deriv are of the same type. In this
     * case the method could not be distinguished from other `do_step` versions. This method is disabled if StateInOut and
     * Time are of the same type.
     *
     * \note This method does NOT use the internal state of the stepper.
     * 
     * \note This method does not solve the forwarding problem.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param dxdt The derivative of x at t. After calling `do_step` this value is updated to the new value at `t+dt`.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     * \param xerr The error estimate is stored in xerr.
     */


    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * Furthermore, the error is estimated.
     *
     * \note This method uses the internal state of the stepper.
     *
     * \note This method does not solve the forwarding problem. 
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dt The step size.
     * \param xerr The error estimate.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::do_step( System system , const StateIn &in , const DerivIn &dxdt_in , time_type t , StateOut &out , DerivOut &dxdt_out , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * Furthermore, the derivative of x at t is passed to the stepper and the error is estimated.
     *
     * \note This method does NOT use the internal state of the stepper.
     *
     * \note This method does not solve the forwarding problem.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param dxdt_in The derivative of x at t.
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dxdt_out The new derivative at `t+dt` is written into this variable.
     * \param dt The step size.
     * \param xerr The error estimate.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::adjust_size( const StateIn &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::reset( void )
     * \brief Resets the internal state of this stepper. After calling this method it is safe to use all
     * `do_step` method without explicitly initializing the stepper.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::initialize( const DerivIn &deriv )
     * \brief Initializes the internal state of the stepper.
     * \param deriv The derivative of x. The next call of `do_step` expects that the derivative of `x` passed to `do_step`
     *              has the value of `deriv`.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::initialize( System system , const StateIn &x , time_type t )
     * \brief Initializes the internal state of the stepper.
     *
     * This method is equivalent to 
     * \code
     * Deriv dxdt;
     * system( x , dxdt , t );
     * stepper.initialize( dxdt );
     * \endcode
     *
     * \param system The system function for the next calls of `do_step`.
     * \param x The current state of the ODE.
     * \param t The current time of the ODE.
     */

    /**
     * \fn explicit_error_stepper_fsal_base::is_initialized( void ) const
     * \brief Returns if the stepper is already initialized. If the stepper is not initialized, the first 
     * call of `do_step` will initialize the state of the stepper. If the stepper is already initialized
     * the system function can not be safely exchanged between consecutive `do_step` calls.
     */

} // odeint
} // numeric
} // boost

#endif // BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_ERROR_STEPPER_FSAL_BASE_HPP_INCLUDED
