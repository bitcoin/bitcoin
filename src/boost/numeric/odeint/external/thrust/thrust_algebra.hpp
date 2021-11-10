/*
 [auto_generated]
 boost/numeric/odeint/external/thrust/thrust_algebra.hpp

 [begin_description]
 An algebra for thrusts device_vectors.
 [end_description]

 Copyright 2010-2013 Mario Mulansky
 Copyright 2010-2011 Karsten Ahnert
 Copyright 2013 Kyle Lutz

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_ALGEBRA_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_ALGEBRA_HPP_INCLUDED


#include <thrust/device_vector.h>
#include <thrust/for_each.h>
#include <thrust/iterator/zip_iterator.h>

#include <boost/range.hpp>

namespace boost {
namespace numeric {
namespace odeint {

namespace detail {

    // to use in thrust::reduce
    template< class Value >
    struct maximum
    {
        template< class Fac1 , class Fac2 >
        __host__ __device__
        Value operator()( const Fac1 t1 , const Fac2 t2 ) const
        {
            return ( abs( t1 ) < abs( t2 ) ) ? t2 : t1 ;
        }

        typedef Value result_type;
    };

}




/** ToDO extend until for_each14 for rk78 */

/*
 * The const versions are needed for boost.range to work, i.e.
 * it allows you to do
 * for_each1( make_pair( vec1.begin() , vec1.begin() + 10 ) , op );
 */

struct thrust_algebra
{
    template< class StateType , class Operation >
    static void for_each1( StateType &s , Operation op )
    {
        thrust::for_each( boost::begin(s) , boost::end(s) , op );
    }

    template< class StateType1 , class StateType2 , class Operation >
    static void for_each2( StateType1 &s1 , StateType2 &s2 , Operation op )
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( boost::begin(s1) ,
                        boost::begin(s2) ) ) ,
                        thrust::make_zip_iterator( thrust::make_tuple( boost::end(s1) ,
                                boost::end(s2) ) ) ,
                                op);
    }

    template< class StateType1 , class StateType2 , class StateType3 , class Operation >
    static void for_each3( StateType1 &s1 , StateType2 &s2 , StateType3 &s3 , Operation op )
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( boost::begin(s1) ,
                        boost::begin(s2) ,
                        boost::begin(s3) ) ) ,
                        thrust::make_zip_iterator( thrust::make_tuple( boost::end(s1) ,
                                boost::end(s2) ,
                                boost::end(s3) ) ) ,
                                op);
    }

    template< class StateType1 , class StateType2 , class StateType3 , class StateType4 ,
    class Operation >
    static void for_each4( StateType1 &s1 , StateType2 &s2 , StateType3 &s3 , StateType4 &s4 ,
            Operation op )
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( boost::begin(s1) ,
                        boost::begin(s2) ,
                        boost::begin(s3) ,
                        boost::begin(s4) ) ) ,
                        thrust::make_zip_iterator( thrust::make_tuple( boost::end(s1) ,
                                boost::end(s2) ,
                                boost::end(s3) ,
                                boost::end(s4) ) ) ,
                                op);
    }

    template< class StateType1 , class StateType2 , class StateType3 ,
    class StateType4 , class StateType5 ,class Operation >
    static void for_each5( StateType1 &s1 , StateType2 &s2 , StateType3 &s3 , StateType4 &s4 ,
            StateType5 &s5 , Operation op )
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( boost::begin(s1) ,
                        boost::begin(s2) ,
                        boost::begin(s3) ,
                        boost::begin(s4) ,
                        boost::begin(s5) ) ) ,
                        thrust::make_zip_iterator( thrust::make_tuple( boost::end(s1) ,
                                boost::end(s2) ,
                                boost::end(s3) ,
                                boost::end(s4) ,
                                boost::end(s5) ) ) ,
                                op);
    }

    template< class StateType1 , class StateType2 , class StateType3 ,
    class StateType4 , class StateType5 , class StateType6 , class Operation >
    static void for_each6( StateType1 &s1 , StateType2 &s2 , StateType3 &s3 , StateType4 &s4 ,
            StateType5 &s5 , StateType6 &s6 , Operation op )
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( boost::begin(s1) ,
                        boost::begin(s2) ,
                        boost::begin(s3) ,
                        boost::begin(s4) ,
                        boost::begin(s5) ,
                        boost::begin(s6) ) ) ,
                        thrust::make_zip_iterator( thrust::make_tuple( boost::end(s1) ,
                                boost::end(s2) ,
                                boost::end(s3) ,
                                boost::end(s4) ,
                                boost::end(s5) ,
                                boost::end(s6) ) ) ,
                                op);
    }

    template< class StateType1 , class StateType2 , class StateType3 , class StateType4 ,
    class StateType5 , class StateType6 , class StateType7 , class Operation >
    static void for_each7( StateType1 &s1 , StateType2 &s2 , StateType3 &s3 , StateType4 &s4 ,
            StateType5 &s5 , StateType6 &s6 , StateType7 &s7 , Operation op )
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( boost::begin(s1) ,
                        boost::begin(s2) ,
                        boost::begin(s3) ,
                        boost::begin(s4) ,
                        boost::begin(s5) ,
                        boost::begin(s6) ,
                        boost::begin(s7) ) ) ,
                        thrust::make_zip_iterator( thrust::make_tuple( boost::end(s1) ,
                                boost::end(s2) ,
                                boost::end(s3) ,
                                boost::end(s4) ,
                                boost::end(s5) ,
                                boost::end(s6) ,
                                boost::end(s7) ) ) ,
                                op);
    }

    template< class StateType1 , class StateType2 , class StateType3 , class StateType4 ,
    class StateType5 , class StateType6 , class StateType7 , class StateType8 , class Operation >
    static void for_each8( StateType1 &s1 , StateType2 &s2 , StateType3 &s3 , StateType4 &s4 ,
            StateType5 &s5 , StateType6 &s6 , StateType7 &s7 , StateType8 &s8 , Operation op )
    {
        thrust::for_each(
                thrust::make_zip_iterator( thrust::make_tuple( boost::begin(s1) ,
                        boost::begin(s2) ,
                        boost::begin(s3) ,
                        boost::begin(s4) ,
                        boost::begin(s5) ,
                        boost::begin(s6) ,
                        boost::begin(s7) ,
                        boost::begin(s8) ) ) ,
                thrust::make_zip_iterator( thrust::make_tuple( boost::end(s1) ,
                        boost::end(s2) ,
                        boost::end(s3) ,
                        boost::end(s4) ,
                        boost::end(s5) ,
                        boost::end(s6) ,
                        boost::end(s7) ,
                        boost::end(s8) ) ) ,
                op);
    }

    template< class S >
    static typename S::value_type norm_inf( const S &s )
    {
        typedef typename S::value_type value_type;
        return thrust::reduce( boost::begin( s ) , boost::end( s ) ,
                               static_cast<value_type>(0) ,
                               detail::maximum<value_type>() );
    }

};


} // odeint
} // numeric
} // boost



#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_ALGEBRA_HPP_INCLUDED
