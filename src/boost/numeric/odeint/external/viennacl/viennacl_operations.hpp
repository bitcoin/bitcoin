/*
 [auto_generated]
 boost/numeric/odeint/external/viennacl_operations.hpp

 [begin_description]
 ViennaCL operations.
 [end_description]

 Copyright 2012 Denis Demidov
 Copyright 2012 Karsten Ahnert
 Copyright 2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_VIENNACL_VIENNACL_OPERATIONS_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_VIENNACL_VIENNACL_OPERATIONS_HPP_INCLUDED

#include <viennacl/vector.hpp>

#ifdef VIENNACL_WITH_OPENCL
#  include <viennacl/generator/custom_operation.hpp>
#endif

namespace boost {
namespace numeric {
namespace odeint {


#ifdef VIENNACL_WITH_OPENCL
struct viennacl_operations
{

    template< class Fac1 = double , class Fac2 = Fac1 >
    struct scale_sum2
    {
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;

        scale_sum2( Fac1 alpha1 , Fac2 alpha2 )
            : m_alpha1( alpha1 ) , m_alpha2( alpha2 )
        { }

        template< class T1 , class T2 , class T3 >
        void operator()( viennacl::vector<T1> &v1 ,
                const viennacl::vector<T2> &v2 ,
                const viennacl::vector<T3> &v3
                ) const
        {
            using namespace viennacl;

            static generator::symbolic_vector    <0, T1>   sym_v1;
            static generator::symbolic_vector    <1, T2>   sym_v2;
            static generator::symbolic_vector    <2, T3>   sym_v3;
            static generator::cpu_symbolic_scalar<3, Fac1> sym_a1;
            static generator::cpu_symbolic_scalar<4, Fac2> sym_a2;

            static generator::custom_operation op(
                    sym_v1 = sym_a1 * sym_v2
                           + sym_a2 * sym_v3,
                    "scale_sum2"
                    );

            ocl::enqueue( op(v1, v2, v3, m_alpha1, m_alpha2) );
        }

        typedef void result_type;
    };


    template< class Fac1 = double , class Fac2 = Fac1 , class Fac3 = Fac2 >
    struct scale_sum3
    {
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;
        const Fac3 m_alpha3;

        scale_sum3( Fac1 alpha1 , Fac2 alpha2 , Fac3 alpha3 )
            : m_alpha1( alpha1 ) , m_alpha2( alpha2 ) , m_alpha3( alpha3 )
        { }

        template< class T1 , class T2 , class T3 , class T4 >
        void operator()( viennacl::vector<T1> &v1 ,
                const viennacl::vector<T2> &v2 ,
                const viennacl::vector<T3> &v3 ,
                const viennacl::vector<T4> &v4
                ) const
        {
            using namespace viennacl;

            static generator::symbolic_vector    <0, T1>   sym_v1;
            static generator::symbolic_vector    <1, T2>   sym_v2;
            static generator::symbolic_vector    <2, T3>   sym_v3;
            static generator::symbolic_vector    <3, T4>   sym_v4;
            static generator::cpu_symbolic_scalar<4, Fac1> sym_a1;
            static generator::cpu_symbolic_scalar<5, Fac2> sym_a2;
            static generator::cpu_symbolic_scalar<6, Fac3> sym_a3;

            static generator::custom_operation op(
                    sym_v1 = sym_a1 * sym_v2
                           + sym_a2 * sym_v3
                           + sym_a3 * sym_v4,
                    "scale_sum3"
                    );

            ocl::enqueue( op(v1, v2, v3, v4, m_alpha1, m_alpha2, m_alpha3) );
        }

        typedef void result_type;
    };


    template< class Fac1 = double , class Fac2 = Fac1 , class Fac3 = Fac2 , class Fac4 = Fac3 >
    struct scale_sum4
    {
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;
        const Fac3 m_alpha3;
        const Fac4 m_alpha4;

        scale_sum4( Fac1 alpha1 , Fac2 alpha2 , Fac3 alpha3 , Fac4 alpha4 )
        : m_alpha1( alpha1 ) , m_alpha2( alpha2 ) , m_alpha3( alpha3 ) , m_alpha4( alpha4 ) { }

        template< class T1 , class T2 , class T3 , class T4 , class T5 >
        void operator()( viennacl::vector<T1> &v1 ,
                const viennacl::vector<T2> &v2 ,
                const viennacl::vector<T3> &v3 ,
                const viennacl::vector<T4> &v4 ,
                const viennacl::vector<T5> &v5
                ) const
        {
            using namespace viennacl;

            static generator::symbolic_vector    <0, T1>   sym_v1;
            static generator::symbolic_vector    <1, T2>   sym_v2;
            static generator::symbolic_vector    <2, T3>   sym_v3;
            static generator::symbolic_vector    <3, T4>   sym_v4;
            static generator::symbolic_vector    <4, T5>   sym_v5;
            static generator::cpu_symbolic_scalar<5, Fac1> sym_a1;
            static generator::cpu_symbolic_scalar<6, Fac2> sym_a2;
            static generator::cpu_symbolic_scalar<7, Fac3> sym_a3;
            static generator::cpu_symbolic_scalar<8, Fac4> sym_a4;

            static generator::custom_operation op(
                    sym_v1 = sym_a1 * sym_v2
                           + sym_a2 * sym_v3
                           + sym_a3 * sym_v4
                           + sym_a4 * sym_v5,
                    "scale_sum4"
                    );

            ocl::enqueue( op(v1, v2, v3, v4, v5,
                        m_alpha1, m_alpha2, m_alpha3, m_alpha4) );
        }

        typedef void result_type;
    };


    template< class Fac1 = double , class Fac2 = Fac1 , class Fac3 = Fac2 , class Fac4 = Fac3 , class Fac5 = Fac4 >
    struct scale_sum5
    {
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;
        const Fac3 m_alpha3;
        const Fac4 m_alpha4;
        const Fac5 m_alpha5;

        scale_sum5( Fac1 alpha1 , Fac2 alpha2 , Fac3 alpha3 , Fac4 alpha4 , Fac5 alpha5 )
        : m_alpha1( alpha1 ) , m_alpha2( alpha2 ) , m_alpha3( alpha3 ) , m_alpha4( alpha4 ) , m_alpha5( alpha5 ) { }

        template< class T1 , class T2 , class T3 , class T4 , class T5 , class T6 >
        void operator()( viennacl::vector<T1> &v1 ,
                const viennacl::vector<T2> &v2 ,
                const viennacl::vector<T3> &v3 ,
                const viennacl::vector<T4> &v4 ,
                const viennacl::vector<T5> &v5 ,
                const viennacl::vector<T6> &v6
                ) const
        {
            using namespace viennacl;

            static generator::symbolic_vector    < 0, T1>   sym_v1;
            static generator::symbolic_vector    < 1, T2>   sym_v2;
            static generator::symbolic_vector    < 2, T3>   sym_v3;
            static generator::symbolic_vector    < 3, T4>   sym_v4;
            static generator::symbolic_vector    < 4, T5>   sym_v5;
            static generator::symbolic_vector    < 5, T6>   sym_v6;
            static generator::cpu_symbolic_scalar< 6, Fac1> sym_a1;
            static generator::cpu_symbolic_scalar< 7, Fac2> sym_a2;
            static generator::cpu_symbolic_scalar< 8, Fac3> sym_a3;
            static generator::cpu_symbolic_scalar< 9, Fac4> sym_a4;
            static generator::cpu_symbolic_scalar<10, Fac5> sym_a5;

            static generator::custom_operation op(
                    sym_v1 = sym_a1 * sym_v2
                           + sym_a2 * sym_v3
                           + sym_a3 * sym_v4
                           + sym_a4 * sym_v5
                           + sym_a5 * sym_v6,
                    "scale_sum5"
                    );

            ocl::enqueue( op(v1, v2, v3, v4, v5, v6,
                        m_alpha1, m_alpha2, m_alpha3, m_alpha4, m_alpha5) );
        }

        typedef void result_type;
    };


};
#else
struct viennacl_operations : public default_operations {};
#endif


} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_VIENNACL_VIENNACL_OPERATIONS_HPP_INCLUDED
