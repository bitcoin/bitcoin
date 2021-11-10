/*
 [auto_generated]
 boost/numeric/odeint/algebra/fusion_algebra.hpp

 [begin_description]
 Algebra for boost::fusion sequences.
 [end_description]

 Copyright 2011-2013 Karsten Ahnert
 Copyright 2011-2013 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_ALGEBRA_FUSION_ALGEBRA_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_ALGEBRA_FUSION_ALGEBRA_HPP_INCLUDED

#include <algorithm>

#include <boost/numeric/odeint/config.hpp>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/view/zip_view.hpp>
#include <boost/fusion/functional/generation/make_fused.hpp>
#include <boost/fusion/algorithm/iteration/accumulate.hpp>

namespace boost {
namespace numeric {
namespace odeint {

namespace detail {

    template< class Value >
    struct fusion_maximum
    {
        template< class Fac1 , class Fac2 >
        Value operator()( Fac1 t1 , const Fac2 t2 ) const
        {
            using std::abs;
            Value a1 = abs( get_unit_value( t1 ) ) , a2 = abs( get_unit_value( t2 ) );
            return ( a1 < a2 ) ? a2 : a1 ;
        }

        typedef Value result_type;
    };
}

/* specialize this if the fundamental numeric type in your fusion sequence is
 * anything else but double (most likely not)
 */
template< typename Sequence >
struct fusion_traits {
    typedef double value_type;
};

struct fusion_algebra
{
    template< class S1 , class Op >
    static void for_each1( S1 &s1 , Op op )
    {
        boost::fusion::for_each( s1 , op );
    };


    template< class S1 , class S2 , class Op >
    static void for_each2( S1 &s1 , S2 &s2 , Op op )
    {
        typedef boost::fusion::vector< S1& , S2& > Sequences;
        Sequences sequences( s1 , s2 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }


    template< class S1 , class S2 , class S3 , class Op >
    static void for_each3( S1 &s1 , S2 &s2 , S3 &s3 , Op op )
    {
        typedef boost::fusion::vector< S1& , S2& , S3& > Sequences;
        Sequences sequences( s1 , s2 , s3 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class Op >
    static void for_each4( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , Op op )
    {
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }


    template< class S1 , class S2 , class S3 , class S4 , class S5 , class Op >
    static void for_each5( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , Op op )
    {
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }


    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class Op >
    static void for_each6( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , Op op )
    {
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }


    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class Op >
    static void for_each7( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , Op op )
    {
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }


    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class Op >
    static void for_each8( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 8 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class Op >
    static void for_each9( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 9 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& , S9& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 , s9 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class Op >
    static void for_each10( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 10 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& , S9& , S10& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 , s9 , s10 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class Op >
    static void for_each11( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 11 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        BOOST_STATIC_ASSERT_MSG( BOOST_RESULT_OF_NUM_ARGS >= 11 , "Macro Parameter BOOST_RESULT_OF_NUM_ARGS to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& , S9& , S10& , S11& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 , s9 , s10 , s11 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class Op >
    static void for_each12( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 12 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        BOOST_STATIC_ASSERT_MSG( BOOST_RESULT_OF_NUM_ARGS >= 12 , "Macro Parameter BOOST_RESULT_OF_NUM_ARGS to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& , S9& , S10& , S11& , S12& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 , s9 , s10 , s11 , s12 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class Op >
    static void for_each13( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 13 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        BOOST_STATIC_ASSERT_MSG( BOOST_RESULT_OF_NUM_ARGS >= 13 , "Macro Parameter BOOST_RESULT_OF_NUM_ARGS to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& , S9& , S10& , S11& , S12& , S13& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 , s9 , s10 , s11 , s12 , s13 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class S14 , class Op >
    static void for_each14( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , S14 &s14 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 14 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        BOOST_STATIC_ASSERT_MSG( BOOST_RESULT_OF_NUM_ARGS >= 14 , "Macro Parameter BOOST_RESULT_OF_NUM_ARGS to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& , S9& , S10& , S11& , S12& , S13& , S14& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 , s9 , s10 , s11 , s12 , s13 , s14 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class S14 , class S15 , class Op >
    static void for_each15( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , S14 &s14 , S15 &s15 , Op op )
    {
        BOOST_STATIC_ASSERT_MSG( BOOST_FUSION_INVOKE_MAX_ARITY >= 15 , "Macro Parameter BOOST_FUSION_INVOKE_MAX_ARITY to small!" );
        BOOST_STATIC_ASSERT_MSG( BOOST_RESULT_OF_NUM_ARGS >= 15 , "Macro Parameter BOOST_RESULT_OF_NUM_ARGS to small!" );
        typedef boost::fusion::vector< S1& , S2& , S3& , S4& , S5& , S6& , S7& , S8& , S9& , S10& , S11& , S12& , S13& , S14& , S15& > Sequences;
        Sequences sequences( s1 , s2 , s3 , s4 , s5 , s6 , s7 , s8 , s9 , s10 , s11 , s12 , s13 , s14 , s15 );
        boost::fusion::for_each( boost::fusion::zip_view< Sequences >( sequences ) , boost::fusion::make_fused( op ) );
    }

    template< class S >
    static typename fusion_traits< S >::value_type norm_inf( const S &s )
    {
        typedef typename fusion_traits< S >::value_type value_type;
        return boost::fusion::accumulate( s , static_cast<value_type>(0) ,
                                          detail::fusion_maximum<value_type>() );
    }

};



} // odeint
} // numeric
} // boost


#endif // BOOST_NUMERIC_ODEINT_ALGEBRA_FUSION_ALGEBRA_HPP_INCLUDED
