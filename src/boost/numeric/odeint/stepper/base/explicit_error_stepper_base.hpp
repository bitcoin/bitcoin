/*
 [auto_generated]
 boost/numeric/odeint/stepper/base/explicit_error_stepper_base.hpp

 [begin_description]
 Base class for all explicit Runge Kutta stepper which are also error steppers.
 [end_description]

 Copyright 2010-2013 Karsten Ahnert
 Copyright 2010-2012 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_ERROR_STEPPER_BASE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_ERROR_STEPPER_BASE_HPP_INCLUDED

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>


#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/stepper/base/algebra_stepper_base.hpp>

namespace boost {
namespace numeric {
namespace odeint {


/*
 * base class for explicit stepper and error steppers
 * models the stepper AND the error stepper concept
 *
 * this class provides the following do_step variants:
    * do_step( sys , x , t , dt )
    * do_step( sys , x , dxdt , t , dt )
    * do_step( sys , in , t , out , dt )
    * do_step( sys , in , dxdt , t , out , dt )
    * do_step( sys , x , t , dt , xerr )
    * do_step( sys , x , dxdt , t , dt , xerr )
    * do_step( sys , in , t , out , dt , xerr )
    * do_step( sys , in , dxdt , t , out , dt , xerr )
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
class explicit_error_stepper_base : public algebra_stepper_base< Algebra , Operations >
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
    typedef explicit_error_stepper_tag stepper_category;
    #ifndef DOXYGEN_SKIP
    typedef state_wrapper< state_type > wrapped_state_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    typedef explicit_error_stepper_base< Stepper , Order , StepperOrder , ErrorOrder ,
            State , Value , Deriv , Time , Algebra , Operations , Resizer > internal_stepper_base_type;
    #endif

    typedef unsigned short order_type;
    static const order_type order_value = Order;
    static const order_type stepper_order_value = StepperOrder;
    static const order_type error_order_value = ErrorOrder;


    explicit_error_stepper_base( const algebra_type &algebra = algebra_type() )
    : algebra_stepper_base_type( algebra )
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
     * Version 1 : do_step( sys , x , t , dt )
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
     * Version 2 : do_step( sys , x , dxdt , t , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     *
     * the disable is needed to avoid ambiguous overloads if state_type = time_type
     */
    template< class System , class StateInOut , class DerivIn >
    typename boost::disable_if< boost::is_same< DerivIn , time_type > , void >::type
    do_step( System system , StateInOut &x , const DerivIn &dxdt , time_type t , time_type dt )
    {
        this->stepper().do_step_impl( system , x , dxdt , t , x , dt );
    }


    /*
     * named Version 2: do_step_dxdt_impl( sys , in , dxdt , t , dt )
     *
     * this version is needed when this stepper is used for initializing 
     * multistep stepper like adams-bashforth. Hence we provide an explicitely
     * named version that is not disabled. Meant for internal use only.
     */
    template < class System, class StateInOut, class DerivIn >
    void do_step_dxdt_impl( System system, StateInOut &x, const DerivIn &dxdt,
                            time_type t, time_type dt )
    {
        this->stepper().do_step_impl( system , x , dxdt , t , x , dt );
    }



    /*
     * Version 3 : do_step( sys , in , t , out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     *
     * the disable is needed to avoid ambiguous overloads if state_type = time_type
     */
    template< class System , class StateIn , class StateOut >
    typename boost::disable_if< boost::is_same< StateIn , time_type > , void >::type
    do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_resizer.adjust_size( in , detail::bind( &internal_stepper_base_type::template resize_impl<StateIn> , detail::ref( *this ) , detail::_1 ) );
        sys( in , m_dxdt.m_v ,t );
        this->stepper().do_step_impl( system , in , m_dxdt.m_v , t , out , dt );
    }

    /*
     * Version 4 :do_step( sys , in , dxdt , t , out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     *
     * the disable is needed to avoid ambiguous overloads if state_type = time_type
     */
    template< class System , class StateIn , class DerivIn , class StateOut >
    typename boost::disable_if< boost::is_same< DerivIn , time_type > , void >::type
    do_step( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
    {
        this->stepper().do_step_impl( system , in , dxdt , t , out , dt );
    }
    
    /*
     * named Version 4: do_step_dxdt_impl( sys , in , dxdt , t , out, dt )
     *
     * this version is needed when this stepper is used for initializing 
     * multistep stepper like adams-bashforth. Hence we provide an explicitely
     * named version that is not disabled. Meant for internal use only.
     */
    template < class System, class StateIn, class DerivIn, class StateOut >
    void do_step_dxdt_impl( System system, const StateIn &in,
                            const DerivIn &dxdt, time_type t, StateOut &out,
                            time_type dt )
    {
        this->stepper().do_step_impl( system , in , dxdt , t , out , dt );
    }

    /*
     * Version  5 :do_step( sys , x , t , dt , xerr )
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
     * Version 6 :do_step( sys , x , dxdt , t , dt , xerr )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     *
     * the disable is needed to avoid ambiguous overloads if state_type = time_type
     */
    template< class System , class StateInOut , class DerivIn , class Err >
    typename boost::disable_if< boost::is_same< DerivIn , time_type > , void >::type
    do_step( System system , StateInOut &x , const DerivIn &dxdt , time_type t , time_type dt , Err &xerr )
    {
        this->stepper().do_step_impl( system , x , dxdt , t , x , dt , xerr );
    }


    /*
     * Version 7 : do_step( sys , in , t , out , dt , xerr )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System , class StateIn , class StateOut , class Err >
    void do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt , Err &xerr )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_resizer.adjust_size( in , detail::bind( &internal_stepper_base_type::template resize_impl<StateIn> , detail::ref( *this ) , detail::_1 ) );
        sys( in , m_dxdt.m_v ,t );
        this->stepper().do_step_impl( system , in , m_dxdt.m_v , t , out , dt , xerr );
    }


    /*
     * Version 8 : do_step( sys , in , dxdt , t , out , dt , xerr )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System , class StateIn , class DerivIn , class StateOut , class Err >
    void do_step( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt , Err &xerr )
    {
        this->stepper().do_step_impl( system , in , dxdt , t , out , dt , xerr );
    }

    template< class StateIn >
    void adjust_size( const StateIn &x )
    {
        resize_impl( x );
    }



private:

    template< class System , class StateInOut >
    void do_step_v1( System system , StateInOut &x , time_type t , time_type dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_resizer.adjust_size( x , detail::bind( &internal_stepper_base_type::template resize_impl<StateInOut> , detail::ref( *this ) , detail::_1 ) );
        sys( x , m_dxdt.m_v , t );
        this->stepper().do_step_impl( system , x , m_dxdt.m_v , t , x , dt );
    }

    template< class System , class StateInOut , class Err >
    void do_step_v5( System system , StateInOut &x , time_type t , time_type dt , Err &xerr )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_resizer.adjust_size( x , detail::bind( &internal_stepper_base_type::template resize_impl<StateInOut> , detail::ref( *this ) , detail::_1 ) );
        sys( x , m_dxdt.m_v ,t );
        this->stepper().do_step_impl( system , x , m_dxdt.m_v , t , x , dt , xerr );
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

protected:

    wrapped_deriv_type m_dxdt;
};




/******** DOXYGEN *******/

/**
 * \class explicit_error_stepper_base
 * \brief Base class for explicit steppers with error estimation. This class can used with 
 * controlled steppers for step size control.
 *
 * This class serves as the base class for all explicit steppers with algebra and operations. In contrast to
 * explicit_stepper_base it also estimates the error and can be used in a controlled stepper to provide
 * step size control.
 *
 * \note This stepper provides `do_step` methods with and without error estimation. It has therefore three orders,
 * one for the order of a step if the error is not estimated. The other two orders are the orders of the step and 
 * the error step if the error estimation is performed.
 *
 * explicit_error_stepper_base  is used as the interface in a CRTP (currently recurring template
 * pattern). In order to work correctly the parent class needs to have a method
 * `do_step_impl( system , in , dxdt_in , t , out , dt , xerr )`. 
 * explicit_error_stepper_base derives from algebra_stepper_base.
 *
 * explicit_error_stepper_base provides several overloaded `do_step` methods, see the list below. Only two of them
 * are needed to fulfill the Error Stepper concept. The other ones are for convenience and for performance. Some
 * of them simply update the state out-of-place, while other expect that the first derivative at `t` is passed to the
 * stepper.
 *
 * - `do_step( sys , x , t , dt )` - The classical `do_step` method needed to fulfill the Error Stepper concept. The
 *      state is updated in-place. A type modelling a Boost.Range can be used for x.
 * - `do_step( sys , x , dxdt , t , dt )` - This method updates the state in-place, but the derivative at the point `t`
 *      must be explicitly passed in `dxdt`.
 * - `do_step( sys , in , t , out , dt )` - This method updates the state out-of-place, hence the result of the step
 *      is stored in `out`.
 * - `do_step( sys , in , dxdt , t , out , dt )` - This method update the state out-of-place and expects that the
 *     derivative at the point `t` is explicitly passed in `dxdt`. It is a combination of the two `do_step` methods
 *     above.
 * - `do_step( sys , x , t , dt , xerr )` - This `do_step` method is needed to fulfill the Error Stepper concept. The
 *     state is updated in-place and an error estimate is calculated. A type modelling a Boost.Range can be used for x.
 * - `do_step( sys , x , dxdt , t , dt , xerr )` - This method updates the state in-place, but the derivative at the
 *      point `t` must be passed in `dxdt`. An error estimate is calculated.
 * - `do_step( sys , in , t , out , dt , xerr )` - This method updates the state out-of-place and estimates the error
 *      during the step.
 * - `do_step( sys , in , dxdt , t , out , dt , xerr )` - This methods updates the state out-of-place and estimates
 *      the error during the step. Furthermore, the derivative at `t` must be passed in `dxdt`.
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
     * \fn explicit_error_stepper_base::explicit_error_stepper_base( const algebra_type &algebra = algebra_type() )
     *
     * \brief Constructs a explicit_error_stepper_base class. This constructor can be used as a default
     * constructor if the algebra has a default constructor.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */

    /**
     * \fn explicit_error_stepper_base::order( void ) const
     * \return Returns the order of the stepper if it used without error estimation.
     */

    /**
     * \fn explicit_error_stepper_base::stepper_order( void ) const
     * \return Returns the order of a step if the stepper is used without error estimation.
     */

    /**
     * \fn explicit_error_stepper_base::error_order( void ) const
     * \return Returns the order of an error step if the stepper is used without error estimation.
     */

    /**
     * \fn explicit_error_stepper_base::do_step( System system , StateInOut &x , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place.
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    /**
     * \fn explicit_error_stepper_base::do_step( System system , StateInOut &x , const DerivIn &dxdt , time_type t , time_type dt )
     * \brief The method performs one step with the stepper passed by Stepper. Additionally to the other method
     * the derivative of x is also passed to this method. It is supposed to be used in the following way:
     *
     * \code
     * sys( x , dxdt , t );
     * stepper.do_step( sys , x , dxdt , t , dt );
     * \endcode
     *
     * The result is updated in place in x. This method is disabled if Time and Deriv are of the same type. In this
     * case the method could not be distinguished from other `do_step` versions.
     * 
     * \note This method does not solve the forwarding problem.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param dxdt The derivative of x at t.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    /**
     * \fn explicit_error_stepper_base::do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * This method is disabled if StateIn and Time are the same type. In this case the method can not be distinguished from
     * other `do_step` variants.
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
     * \fn explicit_error_stepper_base::do_step( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * Furthermore, the derivative of x at t is passed to the stepper. It is supposed to be used in the following way:
     *
     * \code
     * sys( in , dxdt , t );
     * stepper.do_step( sys , in , dxdt , t , out , dt );
     * \endcode
     *
     * This method is disabled if DerivIn and Time are of same type.
     *
     * \note This method does not solve the forwarding problem.
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
     * \fn explicit_error_stepper_base::do_step( System system , StateInOut &x , time_type t , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper and estimates the error. The state of the ODE
     * is updated in-place.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. x is updated by this method.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     * \param xerr The estimation of the error is stored in xerr.
     */

    /**
     * \fn explicit_error_stepper_base::do_step( System system , StateInOut &x , const DerivIn &dxdt , time_type t , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper. Additionally to the other method
     * the derivative of x is also passed to this method. It is supposed to be used in the following way:
     *
     * \code
     * sys( x , dxdt , t );
     * stepper.do_step( sys , x , dxdt , t , dt , xerr );
     * \endcode
     *
     * The result is updated in place in x. This method is disabled if Time and DerivIn are of the same type. In this
     * case the method could not be distinguished from other `do_step` versions.
     * 
     * \note This method does not solve the forwarding problem.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param dxdt The derivative of x at t.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     * \param xerr The error estimate is stored in xerr.
     */

    /**
     * \fn explicit_error_stepper_base::do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * Furthermore, the error is estimated.
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
     * \fn explicit_error_stepper_base::do_step( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt , Err &xerr )
     * \brief The method performs one step with the stepper passed by Stepper. The state of the ODE is updated out-of-place.
     * Furthermore, the derivative of x at t is passed to the stepper and the error is estimated. It is supposed to be used in the following way:
     *
     * \code
     * sys( in , dxdt , t );
     * stepper.do_step( sys , in , dxdt , t , out , dt );
     * \endcode
     *
     * This method is disabled if DerivIn and Time are of same type.
     *
     * \note This method does not solve the forwarding problem.
     *
     * \param system The system function to solve, hence the r.h.s. of the ODE. It must fulfill the
     *               Simple System concept.
     * \param in The state of the ODE which should be solved. in is not modified in this method
     * \param dxdt The derivative of x at t.
     * \param t The value of the time, at which the step should be performed.
     * \param out The result of the step is written in out.
     * \param dt The step size.
     * \param xerr The error estimate.
     */


    /**
     * \fn explicit_error_stepper_base::adjust_size( const StateIn &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

} // odeint
} // numeric
} // boost

#endif // BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_ERROR_STEPPER_BASE_HPP_INCLUDED
