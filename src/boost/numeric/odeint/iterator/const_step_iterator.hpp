
/*
 [auto_generated]
 boost/numeric/odeint/iterator/const_step_iterator.hpp

 [begin_description]
 Iterator for iterating through the solution of an ODE with constant step size.
 [end_description]

 Copyright 2012-2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_ITERATOR_CONST_STEP_ODE_ITERATOR_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_ITERATOR_CONST_STEP_ODE_ITERATOR_HPP_INCLUDED


#include <boost/numeric/odeint/util/stepper_traits.hpp>
#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/iterator/detail/ode_iterator_base.hpp>
#include <boost/numeric/odeint/iterator/impl/const_step_iterator_impl.hpp>


namespace boost {
namespace numeric {
namespace odeint {


    /* use the const_step_iterator_impl with the right tags */
    template< class Stepper , class System , class State
#ifndef DOXYGEN_SKIP
    , class StepperTag = typename base_tag< typename traits::stepper_category< Stepper >::type >::type
#endif
    >
    class const_step_iterator : public const_step_iterator_impl<
            const_step_iterator< Stepper , System , State , StepperTag > ,
            Stepper , System , State , detail::ode_state_iterator_tag , StepperTag
        >
    {
        typedef typename traits::time_type< Stepper >::type time_type;
        typedef const_step_iterator< Stepper , System , State , StepperTag > iterator_type;

    public:
        const_step_iterator( Stepper stepper , System sys , State &s , time_type t_start , time_type t_end , time_type dt )
            : const_step_iterator_impl< iterator_type , Stepper , System , State , detail::ode_state_iterator_tag , StepperTag >( stepper , sys , s , t_start , t_end , dt )
        {}

        const_step_iterator( Stepper stepper , System sys , State &s )
            : const_step_iterator_impl< iterator_type , Stepper , System , State , detail::ode_state_iterator_tag , StepperTag >( stepper , sys , s )
        {}
    };

    /* make functions */

    template< class Stepper , class System , class State >
    const_step_iterator< Stepper , System, State > make_const_step_iterator_begin(
        Stepper stepper ,
        System system , 
        State &x ,
        typename traits::time_type< Stepper >::type t_start ,
        typename traits::time_type< Stepper >::type t_end ,
        typename traits::time_type< Stepper >::type dt )
    {
        return const_step_iterator< Stepper , System , State >( stepper , system , x , t_start , t_end , dt );
    }

    template< class Stepper , class System , class State >
    const_step_iterator< Stepper , System , State > make_const_step_iterator_end(
        Stepper stepper ,
        System system , 
        State &x )
    {
        return const_step_iterator< Stepper , System , State >( stepper , system , x );
    }

    template< class Stepper , class System , class State >
    std::pair< const_step_iterator< Stepper , System , State > , const_step_iterator< Stepper , System , State > >
    make_const_step_range(
        Stepper stepper ,
        System system , 
        State &x ,
        typename traits::time_type< Stepper >::type t_start ,
        typename traits::time_type< Stepper >::type t_end ,
        typename traits::time_type< Stepper >::type dt )
    {
        return std::make_pair(
            const_step_iterator< Stepper , System , State >( stepper , system , x , t_start , t_end , dt ) ,
            const_step_iterator< Stepper , System , State >( stepper , system , x )
            );
    }



    /**
     * \class const_step_iterator
     *
     * \brief ODE Iterator with constant step size. The value type of this iterator is the state type of the stepper.
     *
     * Implements an iterator representing the solution of an ODE from t_start
     * to t_end evaluated at steps with constant step size dt.
     * After each iteration the iterator dereferences to the state x at the next
     * time t+dt.
     * This iterator can be used with Steppers and
     * DenseOutputSteppers and it always makes use of the all the given steppers
     * capabilities. A for_each over such an iterator range behaves similar to
     * the integrate_const routine.
     *
     * const_step_iterator is a model of single-pass iterator.
     *
     * The value type of this iterator is the state type of the stepper. Hence one can only access the state and not the current time.
     *
     * \tparam Stepper The stepper type which should be used during the iteration.
     * \tparam System The type of the system function (ODE) which should be solved.
     * \tparam State The state type of the ODE.
     */


    /**
     * \fn make_const_step_iterator_begin(
        Stepper stepper ,
        System system ,
        State &x ,
        typename traits::time_type< Stepper >::type t_start ,
        typename traits::time_type< Stepper >::type t_end ,
        typename traits::time_type< Stepper >::type dt )
     *
     * \brief Factory function for const_step_iterator. Constructs a begin iterator.
     *
     * \param stepper The stepper to use during the iteration.
     * \param system The system function (ODE) to solve.
     * \param x The initial state. const_step_iterator stores a reference of s and changes its value during the iteration.
     * \param t_start The initial time.
     * \param t_end The end time, at which the iteration should stop.
     * \param dt The initial time step.
     * \returns The const step iterator.
     */


    /**
     * \fn make_const_step_iterator_end( Stepper stepper , System system , State &x )
     * \brief Factory function for const_step_iterator. Constructs a end iterator.
     *
     * \param stepper The stepper to use during the iteration.
     * \param system The system function (ODE) to solve.
     * \param x The initial state. const_step_iterator stores a reference of s and changes its value during the iteration.
     * \returns The const_step_iterator.
     */


    /**
     * \fn make_const_step_range( Stepper stepper , System system , State &x ,
        typename traits::time_type< Stepper >::type t_start ,
        typename traits::time_type< Stepper >::type t_end ,
        typename traits::time_type< Stepper >::type dt )
     *
     * \brief Factory function to construct a single pass range of const step iterators. A range is here a pair
     * of const_step_iterator.
     *
     * \param stepper The stepper to use during the iteration.
     * \param system The system function (ODE) to solve.
     * \param x The initial state. const_step_iterator store a reference of s and changes its value during the iteration.
     * \param t_start The initial time.
     * \param t_end The end time, at which the iteration should stop.
     * \param dt The initial time step.
     * \returns The const step range.
     */


} // namespace odeint
} // namespace numeric
} // namespace boost

//#include <boost/numeric/odeint/iterator/impl/const_step_iterator_dense_output_impl.hpp>

#endif // BOOST_NUMERIC_ODEINT_ITERATOR_CONST_STEP_ODE_ITERATOR_HPP_INCLUDED
