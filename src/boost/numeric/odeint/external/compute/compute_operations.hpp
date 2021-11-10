/*
 [auto_generated]
 boost/numeric/odeint/external/compute/compute_operations.hpp

 [begin_description]
 Operations of Boost.Compute zipped iterators. Is the counterpart of the compute_algebra.
 [end_description]

 Copyright 2009-2011 Karsten Ahnert
 Copyright 2009-2011 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_COMPUTE_COMPUTE_OPERATIONS_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_COMPUTE_COMPUTE_OPERATIONS_HPP_DEFINED

#include <boost/preprocessor/repetition.hpp>
#include <boost/compute.hpp>

namespace boost {
namespace numeric {
namespace odeint {

struct compute_operations {

#define BOOST_ODEINT_COMPUTE_TEMPL_FAC(z, n, unused)                           \
    , class Fac ## n = BOOST_PP_CAT(Fac, BOOST_PP_DEC(n))

#define BOOST_ODEINT_COMPUTE_MEMB_FAC(z, n, unused)                            \
    const Fac ## n m_alpha ## n;

#define BOOST_ODEINT_COMPUTE_PRM_FAC(z, n, unused)                             \
    BOOST_PP_COMMA_IF(n) const Fac ## n alpha ## n

#define BOOST_ODEINT_COMPUTE_INIT_FAC(z, n, unused)                            \
    BOOST_PP_COMMA_IF(n) m_alpha ## n (alpha ## n)

#define BOOST_ODEINT_COMPUTE_PRM_STATE(z, n, unused)                           \
    BOOST_PP_COMMA_IF(n) StateType ## n &s ## n

#define BOOST_ODEINT_COMPUTE_BEGIN_STATE(z, n, unused)                         \
    BOOST_PP_COMMA_IF( BOOST_PP_DEC(n) ) s ## n.begin()

#define BOOST_ODEINT_COMPUTE_END_STATE(z, n, unused)                           \
    BOOST_PP_COMMA_IF( BOOST_PP_DEC(n) ) s ## n.end()

#define BOOST_ODEINT_COMPUTE_LAMBDA(z, n, unused)                              \
    BOOST_PP_EXPR_IF(n, +) m_alpha ## n * bc::lambda::get< n >(bc::_1)

#define BOOST_ODEINT_COMPUTE_OPERATIONS(z, n, unused)                          \
    template<                                                                  \
        class Fac0 = double                                                    \
        BOOST_PP_REPEAT_FROM_TO(1, n, BOOST_ODEINT_COMPUTE_TEMPL_FAC, ~)       \
        >                                                                      \
    struct scale_sum ## n {                                                    \
        BOOST_PP_REPEAT(n, BOOST_ODEINT_COMPUTE_MEMB_FAC, ~)                   \
        scale_sum ## n(                                                        \
                BOOST_PP_REPEAT(n, BOOST_ODEINT_COMPUTE_PRM_FAC, ~)            \
                )                                                              \
            : BOOST_PP_REPEAT(n, BOOST_ODEINT_COMPUTE_INIT_FAC, ~)             \
        { }                                                                    \
        template< BOOST_PP_ENUM_PARAMS(BOOST_PP_INC(n), class StateType) >     \
        void operator()(                                                       \
                BOOST_PP_REPEAT(                                               \
                    BOOST_PP_INC(n),                                           \
                    BOOST_ODEINT_COMPUTE_PRM_STATE, ~)                         \
                ) const                                                        \
        {                                                                      \
            namespace bc = boost::compute;                                     \
            bc::transform(                                                     \
                    bc::make_zip_iterator(                                     \
                        boost::make_tuple(                                     \
                            BOOST_PP_REPEAT_FROM_TO(                           \
                                1, BOOST_PP_INC(n),                            \
                                BOOST_ODEINT_COMPUTE_BEGIN_STATE, ~)           \
                            )                                                  \
                        ),                                                     \
                    bc::make_zip_iterator(                                     \
                        boost::make_tuple(                                     \
                            BOOST_PP_REPEAT_FROM_TO(                           \
                                1, BOOST_PP_INC(n),                            \
                                BOOST_ODEINT_COMPUTE_END_STATE, ~)             \
                            )                                                  \
                        ),                                                     \
                    s0.begin(),                                                \
                    BOOST_PP_REPEAT(n, BOOST_ODEINT_COMPUTE_LAMBDA, ~)         \
                    );                                                         \
        }                                                                      \
    };

BOOST_PP_REPEAT_FROM_TO(2, 8, BOOST_ODEINT_COMPUTE_OPERATIONS, ~)

#undef BOOST_ODEINT_COMPUTE_TEMPL_FAC
#undef BOOST_ODEINT_COMPUTE_MEMB_FAC
#undef BOOST_ODEINT_COMPUTE_PRM_FAC
#undef BOOST_ODEINT_COMPUTE_INIT_FAC
#undef BOOST_ODEINT_COMPUTE_PRM_STATE
#undef BOOST_ODEINT_COMPUTE_BEGIN_STATE
#undef BOOST_ODEINT_COMPUTE_END_STATE
#undef BOOST_ODEINT_COMPUTE_LAMBDA
#undef BOOST_ODEINT_COMPUTE_OPERATIONS

    template<class Fac1 = double, class Fac2 = Fac1>
    struct scale_sum_swap2 {
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;

        scale_sum_swap2(const Fac1 alpha1, const Fac2 alpha2)
            : m_alpha1(alpha1), m_alpha2(alpha2) { }

        template<class State0, class State1, class State2>
        void operator()(State0 &s0, State1 &s1, State2 &s2) const {
            namespace bc = boost::compute;

            bc::command_queue &queue   = bc::system::default_queue();
            const bc::context &context = queue.get_context();

            const char source[] = BOOST_COMPUTE_STRINGIZE_SOURCE(
                    kernel void scale_sum_swap2(
                        F1 a1, F2 a2,
                        global T0 *x0, global T1 *x1, global T2 *x2,
                        )
                    {
                        uint i = get_global_id(0);
                        T0 tmp = x0[i];
                        x0[i]  = a1 * x1[i] + a2 * x2[i];
                        x1[i]  = tmp;
                    }
                    );

            std::stringstream options;
            options
                << " -DT0=" << bc::type_name<typename State0::value_type>()
                << " -DT1=" << bc::type_name<typename State1::value_type>()
                << " -DT2=" << bc::type_name<typename State2::value_type>()
                << " -DF1=" << bc::type_name<Fac1>()
                << " -DF2=" << bc::type_name<Fac2>();

            bc::program program =
                bc::program::build_with_source(source, context, options.str());

            bc::kernel kernel(program, "scale_sum_swap2");
            kernel.set_arg(0, m_alpha1);
            kernel.set_arg(1, m_alpha2);
            kernel.set_arg(2, s0.get_buffer());
            kernel.set_arg(3, s1.get_buffer());
            kernel.set_arg(4, s2.get_buffer());

            queue.enqueue_1d_range_kernel(kernel, 0, s0.size());

        }
    };

    template<class Fac1 = double>
    struct rel_error {
        const Fac1 m_eps_abs, m_eps_rel, m_a_x, m_a_dxdt;

        rel_error(const Fac1 eps_abs, const Fac1 eps_rel, const Fac1 a_x, const Fac1 a_dxdt)
            : m_eps_abs(eps_abs), m_eps_rel(eps_rel), m_a_x(a_x), m_a_dxdt(a_dxdt) { }


        template <class State0, class State1, class State2>
        void operator()(State0 &s0, State1 &s1, State2 &s2) const {
            namespace bc = boost::compute;
            using bc::_1;
            using bc::lambda::get;

            bc::for_each(
                    bc::make_zip_iterator(
                        boost::make_tuple(
                            s0.begin(),
                            s1.begin(),
                            s2.begin()
                            )
                        ),
                    bc::make_zip_iterator(
                        boost::make_tuple(
                            s0.end(),
                            s1.end(),
                            s2.end()
                            )
                        ),
                    get<0>(_1) = abs( get<0>(_1) ) /
                        (m_eps_abs + m_eps_rel * (m_a_x * abs(get<1>(_1) + m_a_dxdt * abs(get<2>(_1)))))
                    );
        }
    };
};

} // odeint
} // numeric
} // boost

#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_COMPUTE_COMPUTE_OPERATIONS_HPP_DEFINED
