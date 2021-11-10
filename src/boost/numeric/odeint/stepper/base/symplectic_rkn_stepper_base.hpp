/*
 [auto_generated]
 boost/numeric/odeint/stepper/base/symplectic_rkn_stepper_base.hpp

 [begin_description]
 Base class for symplectic Runge-Kutta-Nystrom steppers.
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011-2013 Mario Mulansky
 Copyright 2012 Christoph Koke

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_STEPPER_BASE_SYMPLECTIC_RKN_STEPPER_BASE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_BASE_SYMPLECTIC_RKN_STEPPER_BASE_HPP_INCLUDED

#include <boost/array.hpp>

#include <boost/numeric/odeint/util/bind.hpp>
#include <boost/numeric/odeint/util/unwrap_reference.hpp>

#include <boost/numeric/odeint/util/copy.hpp>
#include <boost/numeric/odeint/util/is_pair.hpp>

#include <boost/numeric/odeint/util/state_wrapper.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>

#include <boost/numeric/odeint/stepper/base/algebra_stepper_base.hpp>




namespace boost {
namespace numeric {
namespace odeint {


template<
size_t NumOfStages ,
unsigned short Order ,
class Coor ,
class Momentum ,
class Value ,
class CoorDeriv ,
class MomentumDeriv ,
class Time ,
class Algebra ,
class Operations ,
class Resizer
>
class symplectic_nystroem_stepper_base : public algebra_stepper_base< Algebra , Operations >
{

public:

    typedef algebra_stepper_base< Algebra , Operations > algebra_stepper_base_type;
    typedef typename algebra_stepper_base_type::algebra_type algebra_type;
    typedef typename algebra_stepper_base_type::operations_type operations_type;

    const static size_t num_of_stages = NumOfStages;
    typedef Coor coor_type;
    typedef Momentum momentum_type;
    typedef std::pair< coor_type , momentum_type > state_type;
    typedef CoorDeriv coor_deriv_type;
    typedef state_wrapper< coor_deriv_type> wrapped_coor_deriv_type;
    typedef MomentumDeriv momentum_deriv_type;
    typedef state_wrapper< momentum_deriv_type > wrapped_momentum_deriv_type;
    typedef std::pair< coor_deriv_type , momentum_deriv_type > deriv_type;
    typedef Value value_type;
    typedef Time time_type;
    typedef Resizer resizer_type;
    typedef stepper_tag stepper_category;
    
    #ifndef DOXYGEN_SKIP
    typedef symplectic_nystroem_stepper_base< NumOfStages , Order , Coor , Momentum , Value ,
            CoorDeriv , MomentumDeriv , Time , Algebra , Operations , Resizer > internal_stepper_base_type;
    #endif 
    typedef unsigned short order_type;

    static const order_type order_value = Order;

    typedef boost::array< value_type , num_of_stages > coef_type;

    symplectic_nystroem_stepper_base( const coef_type &coef_a , const coef_type &coef_b , const algebra_type &algebra = algebra_type() )
        : algebra_stepper_base_type( algebra ) , m_coef_a( coef_a ) , m_coef_b( coef_b ) ,
          m_dqdt_resizer() , m_dpdt_resizer() , m_dqdt() , m_dpdt() 
    { }


    order_type order( void ) const
    {
        return order_value;
    }

    /*
     * Version 1 : do_step( system , x , t , dt )
     *
     * This version does not solve the forwarding problem, boost.range can not be used.
     */
    template< class System , class StateInOut >
    void do_step( System system , const StateInOut &state , time_type t , time_type dt )
    {
        typedef typename odeint::unwrap_reference< System >::type system_type;
        do_step_impl( system , state , t , state , dt , typename is_pair< system_type >::type() );
    }

    /**
     * \brief Same function as above. It differs only in a different const specifier in order
     * to solve the forwarding problem, can be used with Boost.Range.
     */
    template< class System , class StateInOut >
    void do_step( System system , StateInOut &state , time_type t , time_type dt )
    {
        typedef typename odeint::unwrap_reference< System >::type system_type;
        do_step_impl( system , state , t , state , dt , typename is_pair< system_type >::type() );
    }




    /*
     * Version 2 : do_step( system , q , p , t , dt );
     *
     * For Convenience
     *
     * The two overloads are needed in order to solve the forwarding problem.
     */
    template< class System , class CoorInOut , class MomentumInOut >
    void do_step( System system , CoorInOut &q , MomentumInOut &p , time_type t , time_type dt )
    {
        do_step( system , std::make_pair( detail::ref( q ) , detail::ref( p ) ) , t , dt );
    }

    /**
     * \brief Same function as do_step( system , q , p , t , dt ). It differs only in a different const specifier in order
     * to solve the forwarding problem, can be called with Boost.Range.
     */
    template< class System , class CoorInOut , class MomentumInOut >
    void do_step( System system , const CoorInOut &q , const MomentumInOut &p , time_type t , time_type dt )
    {
        do_step( system , std::make_pair( detail::ref( q ) , detail::ref( p ) ) , t , dt );
    }





    /*
     * Version 3 : do_step( system , in , t , out , dt )
     *
     * The forwarding problem is not solved in this version
     */
    template< class System , class StateIn , class StateOut >
    void do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
    {
        typedef typename odeint::unwrap_reference< System >::type system_type;
        do_step_impl( system , in , t , out , dt , typename is_pair< system_type >::type() );
    }


    template< class StateType >
    void adjust_size( const StateType &x )
    {
        resize_dqdt( x );
        resize_dpdt( x );
    }

    /** \brief Returns the coefficients a. */
    const coef_type& coef_a( void ) const { return m_coef_a; }

    /** \brief Returns the coefficients b. */
    const coef_type& coef_b( void ) const { return m_coef_b; }

private:

    // stepper for systems with function for dq/dt = f(p) and dp/dt = -f(q)
    template< class System , class StateIn , class StateOut >
    void do_step_impl( System system , const StateIn &in , time_type /* t */ , StateOut &out , time_type dt , boost::mpl::true_ )
    {
        typedef typename odeint::unwrap_reference< System >::type system_type;
        typedef typename odeint::unwrap_reference< typename system_type::first_type >::type coor_deriv_func_type;
        typedef typename odeint::unwrap_reference< typename system_type::second_type >::type momentum_deriv_func_type;
        system_type &sys = system;
        coor_deriv_func_type &coor_func = sys.first;
        momentum_deriv_func_type &momentum_func = sys.second;

        typedef typename odeint::unwrap_reference< StateIn >::type state_in_type;
        typedef typename odeint::unwrap_reference< typename state_in_type::first_type >::type coor_in_type;
        typedef typename odeint::unwrap_reference< typename state_in_type::second_type >::type momentum_in_type;
        const state_in_type &state_in = in;
        const coor_in_type &coor_in = state_in.first;
        const momentum_in_type &momentum_in = state_in.second;

        typedef typename odeint::unwrap_reference< StateOut >::type state_out_type;
        typedef typename odeint::unwrap_reference< typename state_out_type::first_type >::type coor_out_type;
        typedef typename odeint::unwrap_reference< typename state_out_type::second_type >::type momentum_out_type;
        state_out_type &state_out = out;
        coor_out_type &coor_out = state_out.first;
        momentum_out_type &momentum_out = state_out.second;

        m_dqdt_resizer.adjust_size( coor_in , detail::bind( &internal_stepper_base_type::template resize_dqdt< coor_in_type > , detail::ref( *this ) , detail::_1 ) );
        m_dpdt_resizer.adjust_size( momentum_in , detail::bind( &internal_stepper_base_type::template resize_dpdt< momentum_in_type > , detail::ref( *this ) , detail::_1 ) );

        // ToDo: check sizes?

        for( size_t l=0 ; l<num_of_stages ; ++l )
        {
            if( l == 0 )
            {
                coor_func( momentum_in , m_dqdt.m_v );
                this->m_algebra.for_each3( coor_out , coor_in , m_dqdt.m_v ,
                        typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_a[l] * dt ) );
                momentum_func( coor_out , m_dpdt.m_v );
                this->m_algebra.for_each3( momentum_out , momentum_in , m_dpdt.m_v ,
                        typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_b[l] * dt ) );
            }
            else
            {
                coor_func( momentum_out , m_dqdt.m_v );
                this->m_algebra.for_each3( coor_out , coor_out , m_dqdt.m_v ,
                        typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_a[l] * dt ) );
                momentum_func( coor_out , m_dpdt.m_v );
                this->m_algebra.for_each3( momentum_out , momentum_out , m_dpdt.m_v ,
                        typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_b[l] * dt ) );
            }
        }
    }


    // stepper for systems with only function dp /dt = -f(q), dq/dt = p, time not required but still expected for compatibility reasons
    template< class System , class StateIn , class StateOut >
    void do_step_impl( System system , const StateIn &in , time_type  /* t */ , StateOut &out , time_type dt , boost::mpl::false_ )
    {
        typedef typename odeint::unwrap_reference< System >::type momentum_deriv_func_type;
        momentum_deriv_func_type &momentum_func = system;

        typedef typename odeint::unwrap_reference< StateIn >::type state_in_type;
        typedef typename odeint::unwrap_reference< typename state_in_type::first_type >::type coor_in_type;
        typedef typename odeint::unwrap_reference< typename state_in_type::second_type >::type momentum_in_type;
        const state_in_type &state_in = in;
        const coor_in_type &coor_in = state_in.first;
        const momentum_in_type &momentum_in = state_in.second;

        typedef typename odeint::unwrap_reference< StateOut >::type state_out_type;
        typedef typename odeint::unwrap_reference< typename state_out_type::first_type >::type coor_out_type;
        typedef typename odeint::unwrap_reference< typename state_out_type::second_type >::type momentum_out_type;
        state_out_type &state_out = out;
        coor_out_type &coor_out = state_out.first;
        momentum_out_type &momentum_out = state_out.second;


        // m_dqdt not required when called with momentum_func only - don't resize
        // m_dqdt_resizer.adjust_size( coor_in , detail::bind( &internal_stepper_base_type::template resize_dqdt< coor_in_type > , detail::ref( *this ) , detail::_1 ) );
        m_dpdt_resizer.adjust_size( momentum_in , detail::bind( &internal_stepper_base_type::template resize_dpdt< momentum_in_type > , detail::ref( *this ) , detail::_1 ) );


        // ToDo: check sizes?

        // step 0
        this->m_algebra.for_each3( coor_out  , coor_in , momentum_in ,
                        typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_a[0] * dt ) );
        momentum_func( coor_out , m_dpdt.m_v );
        this->m_algebra.for_each3( momentum_out , momentum_in , m_dpdt.m_v ,
                                           typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_b[0] * dt ) );

        for( size_t l=1 ; l<num_of_stages ; ++l )
        {
            this->m_algebra.for_each3( coor_out , coor_out , momentum_out ,
                        typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_a[l] * dt ) );
            momentum_func( coor_out , m_dpdt.m_v );
            this->m_algebra.for_each3( momentum_out , momentum_out , m_dpdt.m_v ,
                                       typename operations_type::template scale_sum2< value_type , time_type >( 1.0 , m_coef_b[l] * dt ) );
        }
    }

    template< class StateIn >
    bool resize_dqdt( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dqdt , x , typename is_resizeable<coor_deriv_type>::type() );
    }

    template< class StateIn >
    bool resize_dpdt( const StateIn &x )
    {
        return adjust_size_by_resizeability( m_dpdt , x , typename is_resizeable<momentum_deriv_type>::type() );
    }


    const coef_type m_coef_a;
    const coef_type m_coef_b;

    resizer_type m_dqdt_resizer;
    resizer_type m_dpdt_resizer;
    wrapped_coor_deriv_type m_dqdt;
    wrapped_momentum_deriv_type m_dpdt;

};

/********* DOXYGEN *********/

/**
 * \class symplectic_nystroem_stepper_base
 * \brief Base class for all symplectic steppers of Nystroem type.
 *
 * This class is the base class for the symplectic Runge-Kutta-Nystroem steppers. Symplectic steppers are usually
 * used to solve Hamiltonian systems and they conserve the phase space volume, see
 * <a href="http://en.wikipedia.org/wiki/Symplectic_integrator">en.wikipedia.org/wiki/Symplectic_integrator</a>. 
 * Furthermore, the energy is conserved
 * in average. In detail this class of steppers can be used to solve separable Hamiltonian systems which can be written
 * in the form H(q,p) = H1(p) + H2(q). q is usually called the coordinate, while p is the momentum. The equations of motion
 * are dq/dt = dH1/dp, dp/dt = -dH2/dq.
 *
 * ToDo : add formula for solver and explanation of the coefficients
 * 
 * symplectic_nystroem_stepper_base uses odeints algebra and operation system. Step size and error estimation are not
 * provided for this class of solvers. It derives from algebra_stepper_base. Several `do_step` variants are provided:
 *
 * - `do_step( sys , x , t , dt )` - The classical `do_step` method. The sys can be either a pair of function objects
 *    for the coordinate or the momentum part or one function object for the momentum part. `x` is a pair of coordinate
 *    and momentum. The state is updated in-place.
 * - `do_step( sys , q , p , t , dt )` - This method is similar to the method above with the difference that the coordinate
 *    and the momentum are passed explicitly and not packed into a pair.
 * - `do_step( sys , x_in , t , x_out , dt )` - This method transforms the state out-of-place. `x_in` and `x_out` are here pairs
 *    of coordinate and momentum.
 *
 * \tparam NumOfStages Number of stages.
 * \tparam Order The order of the stepper.
 * \tparam Coor The type representing the coordinates q.
 * \tparam Momentum The type representing the coordinates p.
 * \tparam Value The basic value type. Should be something like float, double or a high-precision type.
 * \tparam CoorDeriv The type representing the time derivative of the coordinate dq/dt.
 * \tparam MomemtnumDeriv The type representing the time derivative of the momentum dp/dt.
 * \tparam Time The type representing the time t.
 * \tparam Algebra The algebra.
 * \tparam Operations The operations.
 * \tparam Resizer The resizer policy.
 */

    /**
     * \fn symplectic_nystroem_stepper_base::symplectic_nystroem_stepper_base( const coef_type &coef_a , const coef_type &coef_b , const algebra_type &algebra )
     * \brief Constructs a symplectic_nystroem_stepper_base class. The parameters of the specific Nystroem method and the
     * algebra have to be passed.
     * \param coef_a The coefficients a.
     * \param coef_b The coefficients b.
     * \param algebra A copy of algebra is made and stored inside explicit_stepper_base.
     */

    /**
     * \fn symplectic_nystroem_stepper_base::order( void ) const
     * \return Returns the order of the stepper.
     */

    /**
     * \fn symplectic_nystroem_stepper_base::do_step( System system , const StateInOut &state , time_type t , time_type dt )
     * \brief This method performs one step. The system can be either a pair of two function object
     * describing the momentum part and the coordinate part or one function object describing only
     * the momentum part. In this case the coordinate is assumed to be trivial dq/dt = p. The state
     * is updated in-place.
     *
     * \note boost::ref or std::ref can be used for the system as well as for the state. So, it is correct
     * to write `stepper.do_step( make_pair( std::ref( fq ) , std::ref( fp ) ) , make_pair( std::ref( q ) , std::ref( p ) ) , t , dt )`.
     *
     * \note This method solves the forwarding problem.
     *
     * \param system The system, can be represented as a pair of two function object or one function object. See above.
     * \param state The state of the ODE. It is a pair of Coor and Momentum. The state is updated in-place, therefore, the
     * new value of the state will be written into this variable.
     * \param t The time of the ODE. It is not advanced by this method.
     * \param dt The time step.
     */

    /**
     * \fn symplectic_nystroem_stepper_base::do_step( System system , CoorInOut &q , MomentumInOut &p , time_type t , time_type dt )
     * \brief This method performs one step. The system can be either a pair of two function object
     * describing the momentum part and the coordinate part or one function object describing only
     * the momentum part. In this case the coordinate is assumed to be trivial dq/dt = p. The state
     * is updated in-place.
     *
     * \note boost::ref or std::ref can be used for the system. So, it is correct
     * to write `stepper.do_step( make_pair( std::ref( fq ) , std::ref( fp ) ) , q , p , t , dt )`.
     *
     * \note This method solves the forwarding problem.
     *
     * \param system The system, can be represented as a pair of two function object or one function object. See above.
     * \param q The coordinate of the ODE. It is updated in-place. Therefore, the new value of the coordinate will be written
     * into this variable.
     * \param p The momentum of the ODE. It is updated in-place. Therefore, the new value of the momentum will be written info
     * this variable.
     * \param t The time of the ODE. It is not advanced by this method.
     * \param dt The time step.
     */

    /**
     * \fn symplectic_nystroem_stepper_base::do_step( System system , const StateIn &in , time_type t , StateOut &out , time_type dt )
     * \brief This method performs one step. The system can be either a pair of two function object
     * describing the momentum part and the coordinate part or one function object describing only
     * the momentum part. In this case the coordinate is assumed to be trivial dq/dt = p. The state
     * is updated out-of-place.
     *
     * \note boost::ref or std::ref can be used for the system. So, it is correct
     * to write `stepper.do_step( make_pair( std::ref( fq ) , std::ref( fp ) ) , x_in , t , x_out , dt )`.
     *
     * \note This method NOT solve the forwarding problem.
     *
     * \param system The system, can be represented as a pair of two function object or one function object. See above.
     * \param in The state of the ODE, which is a pair of coordinate and momentum. The state is updated out-of-place, therefore the 
     * new value is written into out
     * \param t The time of the ODE. It is not advanced by this method.
     * \param out The new state of the ODE.
     * \param dt The time step.
     */

    /**
     * \fn symplectic_nystroem_stepper_base::adjust_size( const StateType &x )
     * \brief Adjust the size of all temporaries in the stepper manually.
     * \param x A state from which the size of the temporaries to be resized is deduced.
     */

} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_STEPPER_BASE_SYMPLECTIC_RKN_STEPPER_BASE_HPP_INCLUDED
