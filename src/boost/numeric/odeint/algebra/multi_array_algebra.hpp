/*
  [auto_generated]
  boost/numeric/odeint/algebra/multi_array_algebra.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2009-2012 Karsten Ahnert
  Copyright 2009-2012 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_ALGEBRA_MULTI_ARRAY_ALGEBRA_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_ALGEBRA_MULTI_ARRAY_ALGEBRA_HPP_DEFINED


#include <boost/multi_array.hpp>

#include <boost/numeric/odeint/algebra/detail/for_each.hpp>
#include <boost/numeric/odeint/algebra/detail/norm_inf.hpp>
#include <boost/numeric/odeint/algebra/norm_result_type.hpp>
#include <boost/numeric/odeint/algebra/algebra_dispatcher.hpp>


namespace boost {
namespace numeric {
namespace odeint {

// not ready
struct multi_array_algebra
{
    template< class S1 , class Op >
    static void for_each1( S1 &s1 , Op op )
    {
        detail::for_each1( s1.data() , s1.data() + s1.num_elements() , op );
    }

    template< class S1 , class S2 , class Op >
    static void for_each2( S1 &s1 , S2 &s2 , Op op )
    {
        detail::for_each2( s1.data() , s1.data() + s1.num_elements() , s2.data() , op );
    }

    template< class S1 , class S2 , class S3 , class Op >
    static void for_each3( S1 &s1 , S2 &s2 , S3 &s3 , Op op )
    {
        detail::for_each3( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class Op >
    static void for_each4( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , Op op )
    {
        detail::for_each4( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class Op >
    static void for_each5( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , Op op )
    {
        detail::for_each5( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 , class Op >
    static void for_each6( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , Op op )
    {
        detail::for_each6( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class Op >
    static void for_each7( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , Op op )
    {
        detail::for_each7( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class Op >
    static void for_each8( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , Op op )
    {
        detail::for_each8( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class S9 , class Op >
    static void for_each9( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , Op op )
    {
        detail::for_each9( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , s9.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class S9 , class S10 , class Op >
    static void for_each10( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , Op op )
    {
        detail::for_each10( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , s9.data() , s10.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class S9 , class S10 , class S11 , class Op >
    static void for_each11( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , Op op )
    {
        detail::for_each11( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , s9.data() , s10.data() , s11.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class Op >
    static void for_each12( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , Op op )
    {
        detail::for_each12( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , s9.data() , s10.data() , s11.data() , s12.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class Op >
    static void for_each13( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , Op op )
    {
        detail::for_each13( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , s9.data() , s10.data() , s11.data() , s12.data() , s13.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class S14 , class Op >
    static void for_each14( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , S14 &s14 , Op op )
    {
        detail::for_each14( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , s9.data() , s10.data() , s11.data() , s12.data() , s13.data() , s14.data() , op );
    }

    template< class S1 , class S2 , class S3 , class S4 , class S5 , class S6 ,class S7 , class S8 , class S9 , class S10 , class S11 , class S12 , class S13 , class S14 , class S15 , class Op >
    static void for_each15( S1 &s1 , S2 &s2 , S3 &s3 , S4 &s4 , S5 &s5 , S6 &s6 , S7 &s7 , S8 &s8 , S9 &s9 , S10 &s10 , S11 &s11 , S12 &s12 , S13 &s13 , S14 &s14 , S15 &s15 , Op op )
    {
        detail::for_each15( s1.data() , s1.data() + s1.num_elements() , s2.data() , s3.data() , s4.data() , s5.data() , s6.data() , s7.data() , s8.data() , s9.data() , s10.data() , s11.data() , s12.data() , s13.data() , s14.data() , s15.data() , op );
    }

    template< typename S >
    static typename norm_result_type<S>::type norm_inf( const S &s )
    {
        return detail::norm_inf( s.data() , s.data() + s.num_elements()  , static_cast< typename norm_result_type<S>::type >( 0 ) );
    }
};

template< class T , size_t N >
struct algebra_dispatcher< boost::multi_array< T , N > >
{
    typedef multi_array_algebra algebra_type;
};


} // namespace odeint
} // namespace numeric
} // namespace boost


#endif // BOOST_NUMERIC_ODEINT_ALGEBRA_MULTI_ARRAY_ALGEBRA_HPP_DEFINED
