/*
 [auto_generated]
 boost/numeric/odeint/stepper/base/explicit_stepper_base.hpp

 [begin_description]
 Base class for all explicit Runge Kutta steppers.
 [end_description]

 Copyright 2010-2013 Karsten Ahnert
 Copyright 2010-2012 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_STEPPER_BASE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_STEPPER_BASE_HPP_INCLUDED


#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/stepper/base/algebra_stepper_base.hpp>

namespace boost {
namespace numeric {
namespace odeint {

/*
 * base class for explicit steppers
 * models the stepper concept
 *
 * this class provides the following overloads
    * do_step( sys , x , t , dt )
    * do_step( sys , in , t , out , dt )
    * do_step( sys , x , dxdt_in , t , dt )
    * do_step( sys , in , dxdt_in , t , out , dt )
 */

template<
class Stepper ,
unsigned short Order ,
class State ,
class Value ,
class Deriv ,
class Time ,
class Algebra ,
class Operations ,
class Resizer
>
class explicit_stepper_base : public algebra_stepper_base< Algebra , Operations >
{
public:

    #ifndef DOXYGEN_SKIP
    typedef explicit_stepper_base< Stepper , Order , State , Value , Deriv , Time , Algebra , Operations , Resizer > internal_stepper_base_type;
    #endif // DOXYGEN_SKIP


    typedef State state_type;
    typedef Value value_type;
    typedef Deriv deriv_type;
    typedef Time time_type;
    typedef Resizer resizer_type;
    typedef Stepper stepper_type;
    typedef stepper_tag stepper_category;
    typedef algebra_stepper_base< Algebra , Operations > algebra_stepper_base_type;
    typedef typename algebra_stepper_base_type::algebra_type algebra_type;
    typedef typename algebra_stepper_base_type::operations_type operations_type;
    typedef unsigned short order_type;

    #ifndef DOXYGEN_SKIP
    typedef state_wrapper< state_type > wrapped_state_type;
    typedef state_wrapper< deriv_type > wrapped_deriv_type;
    #endif // DOXYGEN_SKIP


    static const order_type order_value = Order;


    explicit_stepper_base( const algebra_type &algebra = algebra_type() )
    : algebra_stepper_base_type( algebra )
    { }

    /**
     * \return Returns the order of the stepper.
     */
    order_type order( void ) const
    {
        return order_value;
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
     */
    template< class System , class StateIn , class StateOut >
    void do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_resizer.adjust_size( in , detail::bind( &internal_stepper_base_type::template resize_impl<StateIn> , detail::ref( *this ) , detail::_1 ) );
        sys( in , m_dxdt.m_v ,t );
        this->stepper().do_step_impl( system , in , m_dxdt.m_v , t , out , dt );
    }


    /*
     * Version 4 : do_step( sys , in , dxdt , t , out , dt )
     *
     * this version does not solve the forwarding problem, boost.range can not be used
     */
    template< class System , class StateIn , class DerivIn , class StateOut >
    void do_step( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
    {
        this->stepper().do_step_impl( system , in , dxdt , t , out , dt );
    }


    /*
     * named Version 4: do_step_dxdt_impl( sys , in , dxdt , t , out, dt )
     *
     * this version is needed when this stepper is used for initializing 
     * multistep stepper like adams-bashforth. Hence we provide an explicitely
     * named version. Meant for internal use only.
     */
    template < class System, class StateIn, class DerivIn, class StateOut >
    void do_step_dxdt_impl( System system, const StateIn &in,
                            const DerivIn &dxdt, time_type t, StateOut &out,
                            time_type dt )
    {
        this->stepper().do_step_impl( system , in , dxdt , t , out , dt );
    }

    template< class StateIn >
    void adjust_size( const StateIn &x )
    {
        resize_impl( x );
    }

private:

    stepper_type& stepper( void )
    {
        return *static_cast< stepper_type* >( this );
    }

    const stepper_type& stepper( void ) const
    {
        return *static_cast< const stepper_type* >( this );
    }


    template< class StateIn >
    bool resize_impl( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dxdt , x , typename is_resizeable<deriv_type>::type() );
    }


    template< class System , class StateInOut >
    void do_step_v1( System system , StateInOut &x , time_type t , time_type dt )
    {
        typename odeint::unwrap_reference< System >::type &sys = system;
        m_resizer.adjust_size( x , detail::bind( &internal_stepper_base_type::template resize_impl< StateInOut > , detail::ref( *this ) , detail::_1 ) );
        sys( x , m_dxdt.m_v ,t );
        this->stepper().do_step_impl( system , x , m_dxdt.m_v , t , x , dt );
    }


    resizer_type m_resizer;

protected:

    wrapped_deriv_type m_dxdt;
};


/******* DOXYGEN *********/

/**
 * \class explicit_stepper_base
 * \brief Base class for explicit steppers without step size control and without dense output.
 *
 * This class serves as the base class for all explicit steppers with algebra and operations.
 * Step size control and error estimation as well as dense output are not provided. explicit_stepper_base 
 * is used as the interface in a CRTP (currently recurring template pattern). In order to work 
 * correctly the parent class needs to have a method `do_step_impl( system , in , dxdt_in , t , out , dt )`. 
 * This is method is used by explicit_stepper_base. explicit_stepper_base derives from
 * algebra_stepper_base. An example how this class can be used is
 *
 * \code
 * template< class State , class Value , class Deriv , class Time , class Algebra , class Operations , class Resizer >
 * class custom_euler : public explicit_stepper_base< 1 , State , Value , Deriv , Time , Algebra , Operations , Resizer >
 * {
 *  public:
 *     
 *     typedef explicit_stepper_base< 1 , State , Value , Deriv , Time , Algebra , Operations , Resizer > base_type;
 *
 *     custom_euler( const Algebra &algebra = Algebra() ) { }
 * 
 *     template< class Sys , class StateIn , class DerivIn , class StateOut >
 *     void do_step_impl( Sys sys , const StateIn &in , const DerivIn &dxdt , Time t , StateOut &out , Time dt )
 *     {
 *         m_algebra.for_each3( out , in , dxdt , Operations::scale_sum2< Value , Time >( 1.0 , dt );
 *     }
 *
 *     template< class State >
 *     void adjust_size( const State &x )
 *     {
 *         base_type::adjust_size( x );
 *     }
 * };
 * \endcode
 *
 * For the Stepper concept only the `do_step( sys , x , t , dt )` needs to be implemented. But this class
 * provides additional `do_step` variants since the stepper is explicit. These methods can be used to increase
 * the performance in some situation, for example if one needs to analyze `dxdt` during each step. In this case 
 * one can use 
 *
 * \code
 * sys( x , dxdt , t );
 * stepper.do_step( sys , x , dxdt , t , dt );  // the value of dxdt is used here
 * t += dt;
 * \endcode
 *
 * In detail explicit_stepper_base provides the following `do_step` variants
 *   - `do_step( sys , x , t , dt )` - The classical `do_step` method needed to fulfill the Stepper concept. The state is updated in-place.
 *      A type modelling a Boost.Range can be used for x.
 *   - `do_step( sys , in , t , out , dt )` - This method updates the state out-of-place, hence the result of the step is stored in `out`.
 *   - `do_step( sys , x , dxdt , t , dt )` - This method updates the state in-place, but the derivative at the point `t` must be
 *      explicitly passed in `dxdt`. For an example see the code snippet above.
 *   - `do_step( sys , in , dxdt , t , out , dt )` - This method update the state out-of-place and expects that the derivative at the point 
 *     `t` is explicitly passed in `dxdt`. It is a combination of the two `do_step` methods above.
 *
 * \note The system is always passed as value, which might result in poor performance if it contains data. In this case it can be used with `boost::ref`
 * or `std::ref`, for example `stepper.do_step( boost::ref( sys ) , x , t , dt );`
 *
 * \note The time `t` is not advanced by the stepper. This has to done manually, or by the appropriate `integrate` routines or `iterator`s.
 *
 * \tparam Stepper The stepper on which this class should work. It is used via CRTP, hence explicit_stepper_base
 * provides the interface for the Stepper.
 * \tparam Order The order of the stepper.
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
     * \fn explicit_stepper_base::explicit_stepper_base( const algebra_type &algebra )
     * \brief Constructs a explicit_stepper_base class. This constructor can be used as a default
     * constructor if the algebra has a default constructor.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */

    /**
     * \fn explicit_stepper_base::order_type order( void ) const
     * \return Returns the order of the stepper.
     */

    /**
     * \fn explicit_stepper_base::do_step( System system , StateInOut &x , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place.
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Simple System concept.
     * \param x The state of the ODE which should be solved. After calling do_step the result is updated in x.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */


    /**
     * \fn explicit_stepper_base::do_step( System system , StateInOut &x , const DerivIn &dxdt , time_type t , time_type dt )

     * \brief The method performs one step. Additionally to the other method
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
     * \fn void explicit_stepper_base::do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
     * \brief The method performs one step. The state of the ODE is updated out-of-place.
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
     * \fn void explicit_stepper_base::do_step( System system , const StateIn &in , const DerivIn &dxdt , time_type t , StateOut &out , time_type dt )
     * \brief The method performs one step. The state of the ODE is updated out-of-place.
     * Furthermore, the derivative of x at t is passed to the stepper. 
     * It is supposed to be used in the following way:
     *
     * \code
     * sys( in , dxdt , t );
     * stepper.do_step( sys , in , dxdt , t , out , dt );
     * \endcode
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
     * \fn void explicit_stepper_base::adjust_size( const StateIn &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

} // odeint
} // numeric
} // boost

#endif // BOOST_NUMERIC_ODEINT_STEPPER_BASE_EXPLICIT_STEPPER_BASE_HPP_INCLUDED
