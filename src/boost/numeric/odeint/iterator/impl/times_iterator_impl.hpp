/*
 [auto_generated]
  boost/numeric/odeint/iterator/detail/times_iterator_impl.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2009-2013 Karsten Ahnert
  Copyright 2009-2013 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_ITERATOR_DETAIL_TIMES_ITERATOR_IMPL_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_ITERATOR_DETAIL_TIMES_ITERATOR_IMPL_HPP_DEFINED

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/throw_exception.hpp>

#include <boost/numeric/odeint/util/unit_helper.hpp>
#include <boost/numeric/odeint/util/copy.hpp>
#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>
#include <boost/numeric/odeint/iterator/detail/ode_iterator_base.hpp>


namespace boost {
namespace numeric {
namespace odeint {


    template< class Iterator , class Stepper , class System , class State , class TimeIterator ,
              typename Tag , typename StepperTag >
    class times_iterator_impl;

    /*
     * Specilization for basic steppers
     */
    /**
     * \brief ODE Iterator with constant step size.
     *
     * Implements an ODE iterator with observer calls at predefined times.
     * Uses controlled steppers. times_iterator is a model of single-pass iterator.
     *
     * The value type of this iterator is the state type of the stepper. Hence one can only access the state and not the current time.
     *
     * \tparam Stepper The stepper type which should be used during the iteration.
     * \tparam System The type of the system function (ODE) which should be solved.
     */
    template< class Iterator , class Stepper , class System , class State , class TimeIterator , typename Tag >
    class times_iterator_impl< Iterator , Stepper , System , State , TimeIterator , Tag , stepper_tag >
        : public detail::ode_iterator_base< Iterator , Stepper , System , State , Tag >
    {
    private:


        typedef Stepper stepper_type;
        typedef System system_type;
        typedef typename boost::numeric::odeint::unwrap_reference< stepper_type >::type unwrapped_stepper_type;
        typedef State state_type;
        typedef TimeIterator time_iterator_type;
        typedef typename traits::time_type< stepper_type >::type time_type;
        typedef typename traits::value_type< stepper_type >::type ode_value_type;
        #ifndef DOXYGEN_SKIP
        typedef detail::ode_iterator_base< Iterator , Stepper , System , State , Tag > base_type;
        #endif

    public:

        /**
         * \brief Constructs a times_iterator. This constructor should be used to construct the begin iterator.
         *
         * \param stepper The stepper to use during the iteration.
         * \param sys The system function (ODE) to solve.
         * \param s The initial state. adaptive_iterator stores a reference of s and changes its value during the iteration.
         * \param t_start Iterator to the begin of a sequence of time values.
         * \param t_end Iterator to the begin of a sequence of time values.
         * \param dt The (initial) time step.
         */
        times_iterator_impl( stepper_type stepper , system_type sys , state_type &s ,
                             time_iterator_type t_start , time_iterator_type t_end , time_type dt )
            : base_type( stepper , sys , *t_start , dt ) ,
              m_t_start( t_start ) , m_t_end( t_end ) , m_state( &s )
        {
            if( t_start == t_end )
                this->m_at_end = true;
        }

        /**
         * \brief Constructs an adaptive_iterator. This constructor should be used to construct the end iterator.
         *
         * \param stepper The stepper to use during the iteration.
         * \param sys The system function (ODE) to solve.
         * \param s The initial state. adaptive_iterator store a reference of s and changes its value during the iteration.
         */
        times_iterator_impl( stepper_type stepper , system_type sys , state_type &s )
            : base_type( stepper , sys ) , m_state( &s ) { }

    protected:

        friend class boost::iterator_core_access;

        void increment()
        {
            unwrapped_stepper_type &stepper = this->m_stepper;
            if( ++m_t_start != m_t_end )
            {
                while( detail::less_with_sign( this->m_t , static_cast<time_type>(*m_t_start) , this->m_dt ) )
                {
                    const time_type current_dt = detail::min_abs( this->m_dt , static_cast<time_type>(*m_t_start) - this->m_t );
                    stepper.do_step( this->m_system , *( this->m_state ) , this->m_t , current_dt );
                    this->m_t += current_dt;
                }

            } else {
                this->m_at_end = true;
            }
         }

    public:
        const state_type& get_state() const
        {
            return *m_state;
        }

    private:
        time_iterator_type m_t_start;
        time_iterator_type m_t_end;
        state_type* m_state;
    };



    /*
     * Specilization for controlled steppers
     */
    /**
     * \brief ODE Iterator with adaptive step size control. The value type of this iterator is the state type of the stepper.
     *
     * Implements an ODE iterator with observer calls at predefined times.
     * Uses controlled steppers. times_iterator is a model of single-pass iterator.
     *
     * The value type of this iterator is the state type of the stepper. Hence one can only access the state and not the current time.
     *
     * \tparam Stepper The stepper type which should be used during the iteration.
     * \tparam System The type of the system function (ODE) which should be solved.
     */
    template< class Iterator , class Stepper , class System , class State , class TimeIterator , typename Tag >
    class times_iterator_impl< Iterator , Stepper , System , State , TimeIterator , Tag , controlled_stepper_tag >
        : public detail::ode_iterator_base< Iterator , Stepper , System , State , Tag >
    {
    private:


        typedef Stepper stepper_type;
        typedef System system_type;
        typedef typename boost::numeric::odeint::unwrap_reference< stepper_type >::type unwrapped_stepper_type;
        typedef State state_type;
        typedef TimeIterator time_iterator_type;
        typedef typename traits::time_type< stepper_type >::type time_type;
        typedef typename traits::value_type< stepper_type >::type ode_value_type;
        #ifndef DOXYGEN_SKIP
        typedef detail::ode_iterator_base< Iterator , Stepper , System , State , Tag > base_type;
        #endif

    public:

        /**
         * \brief Constructs a times_iterator. This constructor should be used to construct the begin iterator.
         *
         * \param stepper The stepper to use during the iteration.
         * \param sys The system function (ODE) to solve.
         * \param s The initial state. adaptive_iterator stores a reference of s and changes its value during the iteration.
         * \param t_start Iterator to the begin of a sequence of time values.
         * \param t_end Iterator to the begin of a sequence of time values.
         * \param dt The (initial) time step.
         */
        times_iterator_impl( stepper_type stepper , system_type sys , state_type &s ,
                             time_iterator_type t_start , time_iterator_type t_end , time_type dt )
            : base_type( stepper , sys , *t_start , dt ) ,
              m_t_start( t_start ) , m_t_end( t_end ) , m_state( &s )
        {
            if( t_start == t_end )
                this->m_at_end = true;
        }

        /**
         * \brief Constructs an adaptive_iterator. This constructor should be used to construct the end iterator.
         *
         * \param stepper The stepper to use during the iteration.
         * \param sys The system function (ODE) to solve.
         * \param s The initial state. adaptive_iterator store a reference of s and changes its value during the iteration.
         */
        times_iterator_impl( stepper_type stepper , system_type sys , state_type &s )
            : base_type( stepper , sys ) , m_state( &s ) { }

    protected:

        friend class boost::iterator_core_access;

        void increment()
        {
            if( ++m_t_start != m_t_end )
            {
                while( detail::less_with_sign( this->m_t , static_cast<time_type>(*m_t_start) , this->m_dt ) )
                {
                    if( detail::less_with_sign( static_cast<time_type>(*m_t_start) - this->m_t , this->m_dt , this->m_dt ) )
                    {
                        // we want to end exactly at the time point
                        time_type current_dt = static_cast<time_type>(*m_t_start) - this->m_t;
                        step_loop( current_dt );
                    } else {
                        step_loop( this->m_dt );
                    }
                }

            } else {
                this->m_at_end = true;
            }
        }

    private:
        void step_loop( time_type &dt )
        {
            unwrapped_stepper_type &stepper = this->m_stepper;
            const size_t max_attempts = 1000;
            size_t trials = 0;
            controlled_step_result res = success;
            do
            {
                res = stepper.try_step( this->m_system , *( this->m_state ) , this->m_t , dt );
                ++trials;
            }
            while( ( res == fail ) && ( trials < max_attempts ) );
            if( trials == max_attempts )
            {
                BOOST_THROW_EXCEPTION( std::overflow_error( "Adaptive iterator : Maximal number of iterations reached. A step size could not be found." ) );
            }
        }

    public:
        const state_type& get_state() const
        {
            return *m_state;
        }


    private:
        time_iterator_type m_t_start;
        time_iterator_type m_t_end;
        state_type* m_state;
    };


    /*
     * Specilization for dense outputer steppers
     */
    /**
     * \brief ODE Iterator with step size control and dense output.
     * Implements an ODE iterator with adaptive step size control. Uses dense-output steppers.
     * times_iterator is a model of single-pass iterator.
     *
     * \tparam Stepper The stepper type which should be used during the iteration.
     * \tparam System The type of the system function (ODE) which should be solved.
     */
    template< class Iterator , class Stepper , class System , class State , class TimeIterator , typename Tag >
    class times_iterator_impl< Iterator , Stepper , System , State , TimeIterator , Tag , dense_output_stepper_tag >
        : public detail::ode_iterator_base< Iterator , Stepper , System , State , Tag >
    {
    private:


        typedef Stepper stepper_type;
        typedef System system_type;
        typedef typename boost::numeric::odeint::unwrap_reference< stepper_type >::type unwrapped_stepper_type;
        typedef State state_type;
        typedef TimeIterator time_iterator_type;
        typedef typename traits::time_type< stepper_type >::type time_type;
        typedef typename traits::value_type< stepper_type >::type ode_value_type;
        #ifndef DOXYGEN_SKIP
        typedef detail::ode_iterator_base< Iterator , Stepper , System , State , Tag > base_type;
        #endif


   public:


        /**
         * \brief Constructs a times_iterator. This constructor should be used to construct the begin iterator.
         *
         * \param stepper The stepper to use during the iteration.
         * \param sys The system function (ODE) to solve.
         * \param s The initial state.
         * \param t_start Iterator to the begin of a sequence of time values.
         * \param t_end Iterator to the begin of a sequence of time values.
         * \param dt The (initial) time step.
         */
        times_iterator_impl( stepper_type stepper , system_type sys , state_type &s ,
                             time_iterator_type t_start , time_iterator_type t_end , time_type dt )
            : base_type( stepper , sys , *t_start , dt ) ,
              m_t_start( t_start ) , m_t_end( t_end ) , m_final_time( *(t_end-1) ) ,
              m_state( &s )
        {
            if( t_start != t_end )
            {
                unwrapped_stepper_type &st = this->m_stepper;
                st.initialize( *( this->m_state ) , this->m_t , this->m_dt );
            } else {
                this->m_at_end = true;
            }
        }

        /**
         * \brief Constructs a times_iterator. This constructor should be used to construct the end iterator.
         *
         * \param stepper The stepper to use during the iteration.
         * \param sys The system function (ODE) to solve.
         * \param s The initial state.
         */
        times_iterator_impl( stepper_type stepper , system_type sys , state_type &s )
            : base_type( stepper , sys ) , m_state( &s ) { }

    protected:

        friend class boost::iterator_core_access;

        void increment()
        {
            unwrapped_stepper_type &st = this->m_stepper;
            if( ++m_t_start != m_t_end )
            {
                this->m_t = static_cast<time_type>(*m_t_start);
                while( detail::less_with_sign( st.current_time() , this->m_t , this->m_dt ) )
                {
                    // make sure we don't go beyond the last point
                    if( detail::less_with_sign( m_final_time-st.current_time() , st.current_time_step() , st.current_time_step() ) )
                    {
                        st.initialize( st.current_state() , st.current_time() , m_final_time-st.current_time() );
                    }
                    st.do_step( this->m_system );
                }
                st.calc_state( this->m_t , *( this->m_state ) );
            } else {
                this->m_at_end = true;
            }
        }

    public:
        const state_type& get_state() const
        {
            return *m_state;
        }


    private:
        time_iterator_type m_t_start;
        time_iterator_type m_t_end;
        time_type m_final_time;
        state_type* m_state;
    };

} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_ITERATOR_DETAIL_TIMES_ITERATOR_IMPL_HPP_DEFINED
