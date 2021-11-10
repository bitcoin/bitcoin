/*
  [auto_generated]
  boost/numeric/odeint/stepper/velocity_verlet.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_VELOCITY_VERLET_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_STEPPER_VELOCITY_VERLET_HPP_DEFINED

#include <boost/numeric/odeint/stepper/base/algebra_stepper_base.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>
#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/copy.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>
// #include <boost/numeric/odeint/util/is_pair.hpp>
// #include <boost/array.hpp>



namespace boost {
namespace numeric {
namespace odeint {



template <
    class Coor ,
    class Velocity = Coor ,
    class Value = double ,
    class Acceleration = Coor ,
    class Time = Value ,
    class TimeSq = Time ,
    class Algebra = typename algebra_dispatcher< Coor >::algebra_type ,
    class Operations = typename operations_dispatcher< Coor >::operations_type ,
    class Resizer = initially_resizer
      >
class velocity_verlet : public algebra_stepper_base< Algebra , Operations >
{
public:

    typedef algebra_stepper_base< Algebra , Operations > algebra_stepper_base_type;
    typedef typename algebra_stepper_base_type::algebra_type algebra_type;
    typedef typename algebra_stepper_base_type::operations_type operations_type;

    typedef Coor coor_type;
    typedef Velocity velocity_type;
    typedef Acceleration acceleration_type;
    typedef std::pair< coor_type , velocity_type > state_type;
    typedef std::pair< velocity_type , acceleration_type > deriv_type;
    typedef state_wrapper< acceleration_type > wrapped_acceleration_type;
    typedef Value value_type;
    typedef Time time_type;
    typedef TimeSq time_square_type;
    typedef Resizer resizer_type;
    typedef stepper_tag stepper_category;

    typedef unsigned short order_type;

    static const order_type order_value = 1;

    /**
     * \return Returns the order of the stepper.
     */
    order_type order( void ) const
    {
        return order_value;
    }


    velocity_verlet( const algebra_type & algebra = algebra_type() )
        : algebra_stepper_base_type( algebra ) , m_first_call( true )
        , m_a1() , m_a2() , m_current_a1( true ) { }


    template< class System , class StateInOut >
    void do_step( System system , StateInOut & x , time_type t , time_type dt )
    {
        do_step_v1( system , x , t , dt );
    }

    
    template< class System , class StateInOut >
    void do_step( System system , const StateInOut & x , time_type t , time_type dt )
    {
        do_step_v1( system , x , t , dt );
    }

    
    template< class System , class CoorIn , class VelocityIn , class AccelerationIn ,
                             class CoorOut , class VelocityOut , class AccelerationOut >
    void do_step( System system , CoorIn const & qin , VelocityIn const & pin , AccelerationIn const & ain ,
                  CoorOut & qout , VelocityOut & pout , AccelerationOut & aout , time_type t , time_type dt )
    {
        const value_type one = static_cast< value_type >( 1.0 );
        const value_type one_half = static_cast< value_type >( 0.5 );

        algebra_stepper_base_type::m_algebra.for_each4(
            qout , qin , pin , ain ,
            typename operations_type::template scale_sum3< value_type , time_type , time_square_type >( one , one * dt , one_half * dt * dt ) );

        typename odeint::unwrap_reference< System >::type & sys = system;

        sys( qout , pin , aout , t + dt );

        algebra_stepper_base_type::m_algebra.for_each4(
            pout , pin , ain , aout ,
            typename operations_type::template scale_sum3< value_type , time_type , time_type >( one , one_half * dt , one_half * dt ) );
    }


    template< class StateIn >
    void adjust_size( const StateIn & x )
    {
        if( resize_impl( x ) )
            m_first_call = true;
    }

    void reset( void )
    {
        m_first_call = true;
    }

    
    /**
     * \fn velocity_verlet::initialize( const AccelerationIn &qin )
     * \brief Initializes the internal state of the stepper.
     * \param deriv The acceleration of x. The next call of `do_step` expects that the acceleration of `x` passed to `do_step`
     *              has the value of `qin`.
     */
    template< class AccelerationIn >
    void initialize( const AccelerationIn & ain )
    {
        // alloc a
        m_resizer.adjust_size( ain ,
                               detail::bind( &velocity_verlet::template resize_impl< AccelerationIn > ,
                                             detail::ref( *this ) , detail::_1 ) );
        boost::numeric::odeint::copy( ain , get_current_acc() );
        m_first_call = false;
    }


    template< class System , class CoorIn , class VelocityIn >
    void initialize( System system , const CoorIn & qin , const VelocityIn & pin , time_type t )
    {
        m_resizer.adjust_size( qin ,
                               detail::bind( &velocity_verlet::template resize_impl< CoorIn > ,
                                             detail::ref( *this ) , detail::_1 ) );
        initialize_acc( system , qin , pin , t );
    }

    bool is_initialized( void ) const
    {
        return ! m_first_call;
    }


private:
    
    template< class System , class CoorIn , class VelocityIn >
    void initialize_acc( System system , const CoorIn & qin , const VelocityIn & pin , time_type t )
    {
        typename odeint::unwrap_reference< System >::type & sys = system;
        sys( qin , pin , get_current_acc() , t );
        m_first_call = false;
    }
    
    template< class System , class StateInOut >
    void do_step_v1( System system , StateInOut & x , time_type t , time_type dt )
    {
        typedef typename odeint::unwrap_reference< StateInOut >::type state_in_type;
        typedef typename odeint::unwrap_reference< typename state_in_type::first_type >::type coor_in_type;
        typedef typename odeint::unwrap_reference< typename state_in_type::second_type >::type momentum_in_type;
        
        typedef typename boost::remove_reference< coor_in_type >::type xyz_type;
        state_in_type & statein = x;
        coor_in_type & qinout = statein.first;
        momentum_in_type & pinout = statein.second;

        // alloc a
        if( m_resizer.adjust_size( qinout ,
                                   detail::bind( &velocity_verlet::template resize_impl< xyz_type > ,
                                                 detail::ref( *this ) , detail::_1 ) )
         || m_first_call )
        {
            initialize_acc( system , qinout , pinout , t );
        }

        // check first
        do_step( system , qinout , pinout , get_current_acc() , qinout , pinout , get_old_acc() , t , dt );
        toggle_current_acc();
    }

    template< class StateIn >
    bool resize_impl( const StateIn & x )
    {
        bool resized = false;
        resized |= adjust_size_by_resizeability( m_a1 , x , typename is_resizeable< acceleration_type >::type() );
        resized |= adjust_size_by_resizeability( m_a2 , x , typename is_resizeable< acceleration_type >::type() );
        return resized;
    }

    acceleration_type & get_current_acc( void )
    {
        return m_current_a1 ? m_a1.m_v : m_a2.m_v ;
    }

    const acceleration_type & get_current_acc( void ) const
    {
        return m_current_a1 ? m_a1.m_v : m_a2.m_v ;
    }

    acceleration_type & get_old_acc( void )
    {
        return m_current_a1 ? m_a2.m_v : m_a1.m_v ;
    }

    const acceleration_type & get_old_acc( void ) const
    {
        return m_current_a1 ? m_a2.m_v : m_a1.m_v ;
    }

    void toggle_current_acc( void )
    {
        m_current_a1 = ! m_current_a1;
    }

    resizer_type m_resizer;
    bool m_first_call;
    wrapped_acceleration_type m_a1 , m_a2;
    bool m_current_a1;
};

/**
 * \class velocity_verlet
 * \brief The Velocity-Verlet algorithm.
 *
 * <a href="http://en.wikipedia.org/wiki/Verlet_integration" >The Velocity-Verlet algorithm</a> is a method for simulation of molecular dynamics systems. It solves the ODE
 * a=f(r,v',t)  where r are the coordinates, v are the velocities and a are the accelerations, hence v = dr/dt, a=dv/dt.
 * 
 * \tparam Coor The type representing the coordinates.
 * \tparam Velocity The type representing the velocities.
 * \tparam Value The type value type.
 * \tparam Acceleration The type representing the acceleration.
 * \tparam Time The time representing the independent variable - the time.
 * \tparam TimeSq The time representing the square of the time.
 * \tparam Algebra The algebra.
 * \tparam Operations The operations type.
 * \tparam Resizer The resizer policy type.
 */


    /**
     * \fn velocity_verlet::velocity_verlet( const algebra_type &algebra )
     * \brief Constructs the velocity_verlet class. This constructor can be used as a default
     * constructor if the algebra has a default constructor. 
     * \param algebra A copy of algebra is made and stored.
     */

    
    /**
     * \fn velocity_verlet::do_step( System system , StateInOut &x , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place.
     * 
     * It can be used like
     * \code
     * pair< coordinates , velocities > state;
     * stepper.do_step( sys , x , t , dt );
     * \endcode
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Second Order System concept.
     * \param x The state of the ODE which should be solved. The state is pair of Coor and Velocity.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    /**
     * \fn velocity_verlet::do_step( System system , const StateInOut &x , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place.
     * 
     * It can be used like
     * \code
     * pair< coordinates , velocities > state;
     * stepper.do_step( sys , x , t , dt );
     * \endcode
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Second Order System concept.
     * \param x The state of the ODE which should be solved. The state is pair of Coor and Velocity.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */    

    

    /**
     * \fn velocity_verlet::do_step( System system , CoorIn const & qin , VelocityIn const & pin , AccelerationIn const & ain , CoorOut & qout , VelocityOut & pout , AccelerationOut & aout , time_type t , time_type dt )
     * \brief This method performs one step. It transforms the result in-place. Additionally to the other methods
     * the coordinates, velocities and accelerations are passed directly to do_step and they are transformed out-of-place.
     * 
     * It can be used like
     * \code
     * coordinates qin , qout;
     * velocities pin , pout;
     * accelerations ain, aout;
     * stepper.do_step( sys , qin , pin , ain , qout , pout , aout , t , dt );
     * \endcode
     *
     * \param system The system function to solve, hence the r.h.s. of the ordinary differential equation. It must fulfill the
     *               Second Order System concept.
     * \param x The state of the ODE which should be solved. The state is pair of Coor and Velocity.
     * \param t The value of the time, at which the step should be performed.
     * \param dt The step size.
     */

    
    /**
     * \fn void velocity_verlet::adjust_size( const StateIn &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */


    /**
     * \fn velocity_verlet::reset( void )
     * \brief Resets the internal state of this stepper. After calling this method it is safe to use all
     * `do_step` method without explicitly initializing the stepper.
     */

    

    /**
     * \fn velocity_verlet::initialize( System system , const CoorIn &qin , const VelocityIn &pin , time_type t )
     * \brief Initializes the internal state of the stepper.
     *
     * This method is equivalent to 
     * \code
     * Acceleration a;
     * system( qin , pin , a , t );
     * stepper.initialize( a );
     * \endcode
     *
     * \param system The system function for the next calls of `do_step`.
     * \param qin The current coordinates of the ODE.
     * \param pin The current velocities of the ODE.
     * \param t The current time of the ODE.
     */
    
    
    /**
     * \fn velocity_verlet::is_initialized()
     * \returns Returns if the stepper is initialized.
    */
    
    
    
    
} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_VELOCITY_VERLET_HPP_DEFINED
