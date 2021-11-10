/*
 boost/numeric/odeint/stepper/detail/controlled_adams_bashforth_moulton.hpp

 [begin_description]
 Implemetation of an controlled adams bashforth moulton stepper.
 [end_description]

 Copyright 2017 Valentin Noah Hartmann

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef BOOST_NUMERIC_ODEINT_STEPPER_CONTROLLED_ADAMS_BASHFORTH_MOULTON_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_STEPPER_CONTROLLED_ADAMS_BASHFORTH_MOULTON_HPP_INCLUDED

#include <boost/numeric/odeint/stepper/stepper_categories.hpp>
#include <boost/numeric/odeint/stepper/controlled_step_result.hpp>

#include <boost/numeric/odeint/stepper/adaptive_adams_bashforth_moulton.hpp>
#include <boost/numeric/odeint/stepper/detail/pid_step_adjuster.hpp>

#include <boost/numeric/odeint/util/unwrap_reference.hpp>
#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/resizer.hpp>

#include <boost/numeric/odeint/util/copy.hpp>
#include <boost/numeric/odeint/util/bind.hpp>

#include <iostream>

namespace boost {
namespace numeric {
namespace odeint {

template<
size_t MaxOrder,
class State,
class Value = double,
class Algebra = typename algebra_dispatcher< State >::algebra_type
>
class default_order_adjuster
{
public:
    typedef State state_type;
    typedef Value value_type;
    typedef state_wrapper< state_type > wrapped_state_type;

    typedef Algebra algebra_type;

    default_order_adjuster( const algebra_type &algebra = algebra_type() )
    : m_algebra( algebra )
    {};

    size_t adjust_order(size_t order, size_t init, boost::array<wrapped_state_type, 4> &xerr)
    {
        using std::abs;

        value_type errc = abs(m_algebra.norm_inf(xerr[2].m_v));

        value_type errm1 = 3*errc;
        value_type errm2 = 3*errc;

        if(order > 2)
        {
            errm2 = abs(m_algebra.norm_inf(xerr[0].m_v));
        }
        if(order >= 2)
        {
            errm1 = abs(m_algebra.norm_inf(xerr[1].m_v));
        }

        size_t o_new = order;

        if(order == 2 && errm1 <= 0.5*errc)
        {
            o_new = order - 1;
        }
        else if(order > 2 && errm2 < errc && errm1 < errc)
        {
            o_new = order - 1;
        }

        if(init < order)
        {
            return order+1;
        }
        else if(o_new == order - 1)
        {
            return order-1;
        }
        else if(order <= MaxOrder)
        {
            value_type errp = abs(m_algebra.norm_inf(xerr[3].m_v));

            if(order > 1 && errm1 < errc && errp)
            {
                return order-1;
            }
            else if(order < MaxOrder && errp < (0.5-0.25*order/MaxOrder) * errc)
            {
                return order+1;
            }
        }

        return order;
    };
private:
    algebra_type m_algebra;
};

template<
class ErrorStepper,
class StepAdjuster = detail::pid_step_adjuster< typename ErrorStepper::state_type, 
    typename ErrorStepper::value_type,
    typename ErrorStepper::deriv_type,
    typename ErrorStepper::time_type,
    typename ErrorStepper::algebra_type,
    typename ErrorStepper::operations_type,
    detail::H211PI
    >,
class OrderAdjuster = default_order_adjuster< ErrorStepper::order_value,
    typename ErrorStepper::state_type,
    typename ErrorStepper::value_type,
    typename ErrorStepper::algebra_type
    >,
class Resizer = initially_resizer
>
class controlled_adams_bashforth_moulton
{
public:
    typedef ErrorStepper stepper_type;

    static const typename stepper_type::order_type order_value = stepper_type::order_value;
    
    typedef typename stepper_type::state_type state_type;
    typedef typename stepper_type::value_type value_type;
    typedef typename stepper_type::deriv_type deriv_type;
    typedef typename stepper_type::time_type time_type;

    typedef typename stepper_type::algebra_type algebra_type;
    typedef typename stepper_type::operations_type operations_type;
    typedef Resizer resizer_type;

    typedef StepAdjuster step_adjuster_type;
    typedef OrderAdjuster order_adjuster_type;
    typedef controlled_stepper_tag stepper_category;

    typedef typename stepper_type::wrapped_state_type wrapped_state_type;
    typedef typename stepper_type::wrapped_deriv_type wrapped_deriv_type;
    typedef boost::array< wrapped_state_type , 4 > error_storage_type;

    typedef typename stepper_type::coeff_type coeff_type;
    typedef controlled_adams_bashforth_moulton< ErrorStepper , StepAdjuster , OrderAdjuster , Resizer > controlled_stepper_type;

    controlled_adams_bashforth_moulton(step_adjuster_type step_adjuster = step_adjuster_type())
    :m_stepper(),
    m_dxdt_resizer(), m_xerr_resizer(), m_xnew_resizer(),
    m_step_adjuster( step_adjuster ), m_order_adjuster()
    {};

    template< class ExplicitStepper, class System >
    void initialize(ExplicitStepper stepper, System system, state_type &inOut, time_type &t, time_type dt)
    {
        m_stepper.initialize(stepper, system, inOut, t, dt);
    };

    template< class System >
    void initialize(System system, state_type &inOut, time_type &t, time_type dt)
    {
        m_stepper.initialize(system, inOut, t, dt);
    };

    template< class ExplicitStepper, class System >
    void initialize_controlled(ExplicitStepper stepper, System system, state_type &inOut, time_type &t, time_type &dt)
    {
        reset();
        coeff_type &coeff = m_stepper.coeff();

        m_dxdt_resizer.adjust_size( inOut , detail::bind( &controlled_stepper_type::template resize_dxdt_impl< state_type > , detail::ref( *this ) , detail::_1 ) );

        controlled_step_result res = fail;

        for( size_t i=0 ; i<order_value; ++i )
        {
            do
            {
                res = stepper.try_step( system, inOut, t, dt );
            }
            while(res != success);

            system( inOut , m_dxdt.m_v , t );
            
            coeff.predict(t-dt, dt);
            coeff.do_step(m_dxdt.m_v);
            coeff.confirm();

            if(coeff.m_eo < order_value)
            {
                ++coeff.m_eo;
            }
        }
    }

    template< class System >
    controlled_step_result try_step(System system, state_type & inOut, time_type &t, time_type &dt)
    {
        m_xnew_resizer.adjust_size( inOut , detail::bind( &controlled_stepper_type::template resize_xnew_impl< state_type > , detail::ref( *this ) , detail::_1 ) );

        controlled_step_result res = try_step(system, inOut, t, m_xnew.m_v, dt);

        if(res == success)
        {
            boost::numeric::odeint::copy( m_xnew.m_v , inOut);
        }

        return res;
    };

    template< class System >
    controlled_step_result try_step(System system, const state_type & in, time_type &t, state_type & out, time_type &dt)
    {
        m_xerr_resizer.adjust_size( in , detail::bind( &controlled_stepper_type::template resize_xerr_impl< state_type > , detail::ref( *this ) , detail::_1 ) );
        m_dxdt_resizer.adjust_size( in , detail::bind( &controlled_stepper_type::template resize_dxdt_impl< state_type > , detail::ref( *this ) , detail::_1 ) );

        m_stepper.do_step_impl(system, in, t, out, dt, m_xerr[2].m_v);

        coeff_type &coeff = m_stepper.coeff();

        time_type dtPrev = dt;
        dt = m_step_adjuster.adjust_stepsize(coeff.m_eo, dt, m_xerr[2].m_v, out, m_stepper.dxdt() );

        if(dt / dtPrev >= step_adjuster_type::threshold())
        {
            system(out, m_dxdt.m_v, t+dtPrev);

            coeff.do_step(m_dxdt.m_v);
            coeff.confirm();

            t += dtPrev;

            size_t eo = coeff.m_eo;

            // estimate errors for next step
            double factor = 1;
            algebra_type m_algebra;

            m_algebra.for_each2(m_xerr[2].m_v, coeff.phi[1][eo].m_v, 
                typename operations_type::template scale_sum1<double>(factor*dt*(coeff.gs[eo])));

            if(eo > 1)
            {
                m_algebra.for_each2(m_xerr[1].m_v, coeff.phi[1][eo-1].m_v, 
                    typename operations_type::template scale_sum1<double>(factor*dt*(coeff.gs[eo-1])));
            }
            if(eo > 2)
            {
                m_algebra.for_each2(m_xerr[0].m_v, coeff.phi[1][eo-2].m_v, 
                    typename operations_type::template scale_sum1<double>(factor*dt*(coeff.gs[eo-2])));
            }
            if(eo < order_value && coeff.m_eo < coeff.m_steps_init-1)
            {
                m_algebra.for_each2(m_xerr[3].m_v, coeff.phi[1][eo+1].m_v, 
                    typename operations_type::template scale_sum1<double>(factor*dt*(coeff.gs[eo+1])));
            }

            // adjust order
            coeff.m_eo = m_order_adjuster.adjust_order(coeff.m_eo, coeff.m_steps_init-1, m_xerr);

            return success;
        }
        else
        {
            return fail;
        }
    };

    void reset() { m_stepper.reset(); };

private:
    template< class StateType >
    bool resize_dxdt_impl( const StateType &x )
    {
        return adjust_size_by_resizeability( m_dxdt, x, typename is_resizeable<deriv_type>::type() );
    };
    template< class StateType >
    bool resize_xerr_impl( const StateType &x )
    {
        bool resized( false );

        for(size_t i=0; i<m_xerr.size(); ++i)
        {
            resized |= adjust_size_by_resizeability( m_xerr[i], x, typename is_resizeable<state_type>::type() );
        }
        return resized;
    };
    template< class StateType >
    bool resize_xnew_impl( const StateType &x )
    {
        return adjust_size_by_resizeability( m_xnew, x, typename is_resizeable<state_type>::type() );
    };

    stepper_type m_stepper;

    wrapped_deriv_type m_dxdt;
    error_storage_type m_xerr;
    wrapped_state_type m_xnew;

    resizer_type m_dxdt_resizer;
    resizer_type m_xerr_resizer;
    resizer_type m_xnew_resizer;

    step_adjuster_type m_step_adjuster;
    order_adjuster_type m_order_adjuster;
};

} // odeint
} // numeric
} // boost

#endif