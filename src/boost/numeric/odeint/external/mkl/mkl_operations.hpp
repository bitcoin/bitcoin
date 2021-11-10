/*
 [auto_generated]
 boost/numeric/odeint/external/mkl/mkl_operations.hpp

 [begin_description]
 Wrapper classes for intel math kernel library types.
 Get a free, non-commercial download of MKL at
 http://software.intel.com/en-us/articles/non-commercial-software-download/
 [end_description]

 Copyright 2010-2011 Mario Mulansky
 Copyright 2011-2013 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_MKL_MKL_OPERATIONS_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_MKL_MKL_OPERATIONS_HPP_INCLUDED

#include <iostream>

#include <mkl_cblas.h>
#include <boost/numeric/odeint/algebra/default_operations.hpp>

/* exemplary example for writing bindings to the Intel MKL library
 * see test/mkl for how to use mkl with odeint
 * this is a quick and dirty implementation showing the general possibility.
 * It works only with containers based on double and sequential memory allocation.
 */

namespace boost {
namespace numeric {
namespace odeint {

/* only defined for doubles */
struct mkl_operations
{
    //template< class Fac1 , class Fac2 > struct scale_sum2;

    template< class F1 = double , class F2 = F1 >
    struct scale_sum2
    {
        typedef double Fac1;
        typedef double Fac2;
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;

        scale_sum2( const Fac1 alpha1 , const Fac2 alpha2 ) : m_alpha1( alpha1 ) , m_alpha2( alpha2 ) { }

        template< class T1 , class T2 , class T3 >
        void operator()( T1 &t1 , const T2 &t2 , const T3 &t3) const
        {   // t1 = m_alpha1 * t2 + m_alpha2 * t3;
            // we get Containers that have size() and [i]-access

            const int n = t1.size();
            //boost::numeric::odeint::copy( t1 , t3 );
            if( &(t2[0]) != &(t1[0]) )
            {
                cblas_dcopy( n , &(t2[0]) , 1 , &(t1[0]) , 1 );
            }
            cblas_dscal( n , m_alpha1 , &(t1[0]) , 1 );
            cblas_daxpy( n , m_alpha2 , &(t3[0]) , 1 , &(t1[0]) , 1 );
            //daxpby( &n , &m_alpha2 , &(t3[0]) , &one , &m_alpha1 , &(t1[0]) , &one );
        }
    };

    template< class F1 = double , class F2 = F1 , class F3 = F2 >
    struct scale_sum3
    {
        typedef double Fac1;
        typedef double Fac2;
        typedef double Fac3;
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;
        const Fac3 m_alpha3;

        scale_sum3( const Fac1 alpha1 , const Fac2 alpha2 , const Fac3 alpha3 )
            : m_alpha1( alpha1 ) , m_alpha2( alpha2 ) , m_alpha3( alpha3 ) { }

        template< class T1 , class T2 , class T3 , class T4 >
        void operator()( T1 &t1 , const T2 &t2 , const T3 &t3 , const T4 &t4 ) const
        {   // t1 = m_alpha1 * t2 + m_alpha2 * t3 + m_alpha3 * t4;
            // we get Containers that have size() and [i]-access

            const int n = t1.size();
            //boost::numeric::odeint::copy( t1 , t3 );
            if( &(t2[0]) != &(t1[0]) )
            {
                cblas_dcopy( n , &(t2[0]) , 1 , &(t1[0]) , 1 );
            }
            cblas_dscal( n , m_alpha1 , &(t1[0]) , 1 );
            cblas_daxpy( n , m_alpha2 , &(t3[0]) , 1 , &(t1[0]) , 1 );
            //daxpby( &n , &m_alpha2 , &(t3[0]) , &one , &m_alpha1 , &(t1[0]) , &one );
            cblas_daxpy( n , m_alpha3 , &(t4[0]) , 1 , &(t1[0]) , 1 );
        }
    };

    template< class F1 = double , class F2 = F1 , class F3 = F2 , class F4 = F3 >
    struct scale_sum4
    {
        typedef double Fac1;
        typedef double Fac2;
        typedef double Fac3;
        typedef double Fac4;
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;
        const Fac3 m_alpha3;
        const Fac4 m_alpha4;

        scale_sum4( const Fac1 alpha1 , const Fac2 alpha2 , const Fac3 alpha3 , const Fac4 alpha4 )
            : m_alpha1( alpha1 ) , m_alpha2( alpha2 ) , m_alpha3( alpha3 ) , m_alpha4( alpha4 ) { }

        template< class T1 , class T2 , class T3 , class T4 , class T5 >
        void operator()( T1 &t1 , const T2 &t2 , const T3 &t3 , const T4 &t4 , const T5 &t5 ) const
        {   // t1 = m_alpha1 * t2 + m_alpha2 * t3 + m_alpha3 * t4 + m_alpha4 * t5;
            // we get Containers that have size() and [i]-access

            const int n = t1.size();
            //boost::numeric::odeint::copy( t1 , t3 );
            if( &(t2[0]) != &(t1[0]) )
            {
                cblas_dcopy( n , &(t2[0]) , 1 , &(t1[0]) , 1 );
            }

            cblas_dscal( n , m_alpha1 , &(t1[0]) , 1 );
            cblas_daxpy( n , m_alpha2 , &(t3[0]) , 1 , &(t1[0]) , 1 );
            //daxpby( &n , &m_alpha2 , &(t3[0]) , &one , &m_alpha1 , &(t1[0]) , &one );
            cblas_daxpy( n , m_alpha3 , &(t4[0]) , 1 , &(t1[0]) , 1 );
            cblas_daxpy( n , m_alpha4 , &(t5[0]) , 1 , &(t1[0]) , 1 );
        }
    };


    template< class F1 = double , class F2 = F1 , class F3 = F2 , class F4 = F3 , class F5 = F4 >
    struct scale_sum5
    {
        typedef double Fac1;
        typedef double Fac2;
        typedef double Fac3;
        typedef double Fac4;
        typedef double Fac5;
        const Fac1 m_alpha1;
        const Fac2 m_alpha2;
        const Fac3 m_alpha3;
        const Fac4 m_alpha4;
        const Fac5 m_alpha5;

        scale_sum5( const Fac1 alpha1 , const Fac2 alpha2 , const Fac3 alpha3 , const Fac4 alpha4 , const Fac5 alpha5 )
            : m_alpha1( alpha1 ) , m_alpha2( alpha2 ) , m_alpha3( alpha3 ) , m_alpha4( alpha4 ) , m_alpha5( alpha5 ) { }

        template< class T1 , class T2 , class T3 , class T4 , class T5 , class T6   >
        void operator()( T1 &t1 , const T2 &t2 , const T3 &t3 , const T4 &t4 , const T5 &t5 , const T6 &t6 ) const
        {   // t1 = m_alpha1 * t2 + m_alpha2 * t3 + m_alpha3 * t4 + m_alpha4 * t5 + m_alpha5 * t6;
            // we get Containers that have size() and [i]-access

            const int n = t1.size();
            //boost::numeric::odeint::copy( t1 , t3 );
            if( &(t2[0]) != &(t1[0]) )
            {
                cblas_dcopy( n , &(t2[0]) , 1 , &(t1[0]) , 1 );
            }

            cblas_dscal( n , m_alpha1 , &(t1[0]) , 1 );
            cblas_daxpy( n , m_alpha2 , &(t3[0]) , 1 , &(t1[0]) , 1 );
            //daxpby( &n , &m_alpha2 , &(t3[0]) , &one , &m_alpha1 , &(t1[0]) , &one );
            cblas_daxpy( n , m_alpha3 , &(t4[0]) , 1 , &(t1[0]) , 1 );
            cblas_daxpy( n , m_alpha4 , &(t5[0]) , 1 , &(t1[0]) , 1 );
            cblas_daxpy( n , m_alpha5 , &(t6[0]) , 1 , &(t1[0]) , 1 );
        }
    };

};

} // odeint
} // numeric
} // boost

#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_MKL_MKL_OPERATIONS_HPP_INCLUDED
