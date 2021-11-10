/*
 [auto_generated]
 boost/numeric/odeint/external/openmp/openmp_nested_algebra.hpp

 [begin_description]
 Nested parallelized algebra for OpenMP.
 [end_description]

 Copyright 2013 Karsten Ahnert
 Copyright 2013 Mario Mulansky
 Copyright 2013 Pascal Germroth

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_OPENMP_OPENMP_NESTED_ALGEBRA_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_OPENMP_OPENMP_NESTED_ALGEBRA_HPP_INCLUDED

#include <boost/assert.hpp>
#include <boost/range.hpp>
#include <boost/numeric/odeint/algebra/norm_result_type.hpp>
#include <boost/numeric/odeint/util/n_ary_helper.hpp>

namespace boost {
namespace numeric {
namespace odeint {

/** \brief OpenMP-parallelized algebra, wrapping another, non-parallelized algebra.
 *
 * NestedState must be a model of Random Access Range, where the elements are sub-states
 * which will be processed in parallel.
 */
template< class InnerAlgebra >
struct openmp_nested_algebra
{

#if __cplusplus >= 201103L // C++11 supports _Pragma

#define BOOST_ODEINT_GEN_LOCAL(z, n, unused) \
    BOOST_ASSERT_MSG( len == boost::size(s ## n), "All nested state ranges must have the same size." ); \
    typename boost::range_iterator<S ## n>::type beg ## n = boost::begin(s ## n);
#define BOOST_ODEINT_GEN_BODY(n) \
    const size_t len = boost::size(s0); \
    BOOST_PP_REPEAT(n, BOOST_ODEINT_GEN_LOCAL, ~) \
    _Pragma("omp parallel for schedule(runtime)") \
    for( size_t i = 0 ; i < len ; i++ ) \
        InnerAlgebra::for_each##n( \
            BOOST_PP_ENUM_BINARY_PARAMS(n, beg, [i] BOOST_PP_INTERCEPT) , \
            op \
        );
BOOST_ODEINT_GEN_FOR_EACH(BOOST_ODEINT_GEN_BODY)
#undef BOOST_ODEINT_GEN_BODY
#undef BOOST_ODEINT_GEN_LOCAL

#else

    template< class S0 , class Op > static void for_each1 ( S0 &s0 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each1( beg0 [i] , op );
    }
    template< class S0 , class S1 , class Op > static void for_each2 ( S0 &s0 , S1 &s1 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each2( beg0 [i] , beg1 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class Op > static void for_each3 ( S0 &s0 , S1 &s1 , S2 &s2 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each3( beg0 [i] , beg1 [i] , beg2 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class Op > static void for_each4 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each4( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class Op > static void for_each5 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each5( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class Op > static void for_each6 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each6( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class Op > static void for_each7 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each7( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class Op > static void for_each8 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each8( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class Op > static void for_each9 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        typename boost::range_iterator<S8>::type beg8 = boost::begin(s8);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each9( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , beg8 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class Op > static void for_each10 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        typename boost::range_iterator<S8>::type beg8 = boost::begin(s8);
        typename boost::range_iterator<S9>::type beg9 = boost::begin(s9);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each10( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , beg8 [i] , beg9 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class Op > static void for_each11 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        typename boost::range_iterator<S8>::type beg8 = boost::begin(s8);
        typename boost::range_iterator<S9>::type beg9 = boost::begin(s9);
        typename boost::range_iterator<S10>::type beg10 = boost::begin(s10);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each11( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , beg8 [i] , beg9 [i] , beg10 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class Op > static void for_each12 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        typename boost::range_iterator<S8>::type beg8 = boost::begin(s8);
        typename boost::range_iterator<S9>::type beg9 = boost::begin(s9);
        typename boost::range_iterator<S10>::type beg10 = boost::begin(s10);
        typename boost::range_iterator<S11>::type beg11 = boost::begin(s11);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each12( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , beg8 [i] , beg9 [i] , beg10 [i] , beg11 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class Op > static void for_each13 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        typename boost::range_iterator<S8>::type beg8 = boost::begin(s8);
        typename boost::range_iterator<S9>::type beg9 = boost::begin(s9);
        typename boost::range_iterator<S10>::type beg10 = boost::begin(s10);
        typename boost::range_iterator<S11>::type beg11 = boost::begin(s11);
        typename boost::range_iterator<S12>::type beg12 = boost::begin(s12);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each13( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , beg8 [i] , beg9 [i] , beg10 [i] , beg11 [i] , beg12 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class Op > static void for_each14 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        typename boost::range_iterator<S8>::type beg8 = boost::begin(s8);
        typename boost::range_iterator<S9>::type beg9 = boost::begin(s9);
        typename boost::range_iterator<S10>::type beg10 = boost::begin(s10);
        typename boost::range_iterator<S11>::type beg11 = boost::begin(s11);
        typename boost::range_iterator<S12>::type beg12 = boost::begin(s12);
        typename boost::range_iterator<S13>::type beg13 = boost::begin(s13);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each14( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , beg8 [i] , beg9 [i] , beg10 [i] , beg11 [i] , beg12 [i] , beg13 [i] , op );
    }
    template< class S0 , class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class S14 , class Op > static void for_each15 ( S0 &s0 , S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , S14 &s14 , Op op ) {
        const size_t len = boost::size(s0);
        typename boost::range_iterator<S0>::type beg0 = boost::begin(s0);
        typename boost::range_iterator<S1>::type beg1 = boost::begin(s1);
        typename boost::range_iterator<S2>::type beg2 = boost::begin(s2);
        typename boost::range_iterator<S3>::type beg3 = boost::begin(s3);
        typename boost::range_iterator<S4>::type beg4 = boost::begin(s4);
        typename boost::range_iterator<S5>::type beg5 = boost::begin(s5);
        typename boost::range_iterator<S6>::type beg6 = boost::begin(s6);
        typename boost::range_iterator<S7>::type beg7 = boost::begin(s7);
        typename boost::range_iterator<S8>::type beg8 = boost::begin(s8);
        typename boost::range_iterator<S9>::type beg9 = boost::begin(s9);
        typename boost::range_iterator<S10>::type beg10 = boost::begin(s10);
        typename boost::range_iterator<S11>::type beg11 = boost::begin(s11);
        typename boost::range_iterator<S12>::type beg12 = boost::begin(s12);
        typename boost::range_iterator<S13>::type beg13 = boost::begin(s13);
        typename boost::range_iterator<S14>::type beg14 = boost::begin(s14);
        #pragma omp parallel for schedule(runtime)
        for( size_t i = 0 ; i < len ; i++ ) InnerAlgebra::for_each15( beg0 [i] , beg1 [i] , beg2 [i] , beg3 [i] , beg4 [i] , beg5 [i] , beg6 [i] , beg7 [i] , beg8 [i] , beg9 [i] , beg10 [i] , beg11 [i] , beg12 [i] , beg13 [i] , beg14 [i] , op );
    }

#endif


    template< class NestedState >
    static typename norm_result_type< typename NestedState::value_type >::type norm_inf( const NestedState &s )
    {
        typedef typename boost::range_iterator<const NestedState>::type iterator;
        typedef typename std::iterator_traits<iterator>::value_type value_type;
        typedef typename norm_result_type<value_type>::type result_type;
        result_type init = static_cast< result_type >( 0 );
        const size_t len = boost::size(s);
        iterator beg = boost::begin(s);
#       pragma omp parallel for reduction(max: init) schedule(dynamic)
        for( size_t i = 0 ; i < len ; i++ )
            init = (std::max)( init , InnerAlgebra::norm_inf( beg[i] ) );
        return init;
    }

};


}
}
}

#endif
