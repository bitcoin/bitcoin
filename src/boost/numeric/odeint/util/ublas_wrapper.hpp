/*
 [auto_generated]
 boost/numeric/odeint/util/ublas_wrapper.hpp

 [begin_description]
 Resizing for ublas::vector and ublas::matrix
 [end_description]

 Copyright 2011-2013 Mario Mulansky
 Copyright 2011-2013 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_UTIL_UBLAS_WRAPPER_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_UTIL_UBLAS_WRAPPER_HPP_INCLUDED


#include <boost/type_traits/integral_constant.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>

#include <boost/numeric/odeint/algebra/vector_space_algebra.hpp>
#include <boost/numeric/odeint/algebra/default_operations.hpp>

#include <boost/numeric/odeint/util/is_resizeable.hpp>
#include <boost/numeric/odeint/util/state_wrapper.hpp>


/* extend ublas by a few operations */

/* map norm_inf onto reduce( v , default_operations::maximum ) */
namespace boost { namespace numeric { namespace odeint {

    template< typename T , typename A >
    struct vector_space_norm_inf< boost::numeric::ublas::vector<T,A> >
    {
        typedef T result_type;

        result_type operator()( const boost::numeric::ublas::vector<T,A> &x ) const
        {
            return boost::numeric::ublas::norm_inf( x );
        }
    };


    template< class T , class L , class A >
    struct vector_space_norm_inf< boost::numeric::ublas::matrix<T,L,A> >
    {
        typedef T result_type;

        result_type operator()( const boost::numeric::ublas::matrix<T,L,A> &x ) const
        {
            return boost::numeric::ublas::norm_inf( x );
        }
    };
} } }

/* additional operations:
 * abs( v )
 * v / w
 * a + v
 */
namespace boost { namespace numeric { namespace ublas {


    // elementwise abs - calculates absolute values of the elements
    template<class T>
    struct scalar_abs: public scalar_unary_functor<T> {
        typedef typename scalar_unary_functor<T>::value_type value_type;
        typedef typename scalar_unary_functor<T>::argument_type argument_type;
        typedef typename scalar_unary_functor<T>::result_type result_type;

        static BOOST_UBLAS_INLINE
        result_type apply (argument_type t) {
            using std::abs;
            return abs (t);
        }
    };


    // (abs v) [i] = abs (v [i])
    template<class E>
    BOOST_UBLAS_INLINE
    typename vector_unary_traits<E, scalar_abs<typename E::value_type> >::result_type
    abs (const vector_expression<E> &e) {
        typedef typename vector_unary_traits<E, scalar_abs<typename E::value_type> >::expression_type expression_type;
        return expression_type (e ());
    }

    // (abs m) [i] = abs (m [i])
    template<class E>
    BOOST_UBLAS_INLINE
    typename matrix_unary1_traits<E, scalar_abs<typename E::value_type> >::result_type
    abs (const matrix_expression<E> &e) {
        typedef typename matrix_unary1_traits<E, scalar_abs<typename E::value_type> >::expression_type expression_type;
        return expression_type (e ());
    }


    // elementwise division (v1 / v2) [i] = v1 [i] / v2 [i]
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    typename vector_binary_traits<E1, E2, scalar_divides<typename E1::value_type,
                                                         typename E2::value_type> >::result_type
    operator / (const vector_expression<E1> &e1,
                const vector_expression<E2> &e2) {
        typedef typename vector_binary_traits<E1, E2, scalar_divides<typename E1::value_type,
                                                                     typename E2::value_type> >::expression_type expression_type;
        return expression_type (e1 (), e2 ());
    }


    // elementwise division (m1 / m2) [i] = m1 [i] / m2 [i]
    template<class E1, class E2>
    BOOST_UBLAS_INLINE
    typename matrix_binary_traits<E1, E2, scalar_divides<typename E1::value_type,
    typename E2::value_type> >::result_type
    operator / (const matrix_expression<E1> &e1,
                const matrix_expression<E2> &e2) {
        typedef typename matrix_binary_traits<E1, E2, scalar_divides<typename E1::value_type,
                                                                     typename E2::value_type> >::expression_type expression_type;
        return expression_type (e1 (), e2 ());
    }

    // addition with scalar
    // (t + v) [i] = t + v [i]
    template<class T1, class E2>
    BOOST_UBLAS_INLINE
    typename enable_if< is_convertible<T1, typename E2::value_type >,
    typename vector_binary_scalar1_traits<const T1, E2, scalar_plus<T1, typename E2::value_type> >::result_type
    >::type
    operator + (const T1 &e1,
                const vector_expression<E2> &e2) {
        typedef typename vector_binary_scalar1_traits<const T1, E2, scalar_plus<T1, typename E2::value_type> >::expression_type expression_type;
        return expression_type (e1, e2 ());
    }

    // addition with scalar
    // (t + m) [i] = t + m [i]
    template<class T1, class E2>
    BOOST_UBLAS_INLINE
    typename enable_if< is_convertible<T1, typename E2::value_type >,
    typename matrix_binary_scalar1_traits<const T1, E2, scalar_plus<T1, typename E2::value_type> >::result_type
    >::type
    operator + (const T1 &e1,
                const matrix_expression<E2> &e2) {
        typedef typename matrix_binary_scalar1_traits<const T1, E2, scalar_plus<T1, typename E2::value_type> >::expression_type expression_type;
        return expression_type (e1, e2 ());
    }

} } }




/* add resize functionality */
namespace boost {
namespace numeric {
namespace odeint {

/*
 * resizeable specialization for boost::numeric::ublas::vector
 */
template< class T , class A >
struct is_resizeable< boost::numeric::ublas::vector< T , A > >
{
    typedef boost::true_type type;
    const static bool value = type::value;
};


/*
 * resizeable specialization for boost::numeric::ublas::matrix
 */
template< class T , class L , class A >
struct is_resizeable< boost::numeric::ublas::matrix< T , L , A > >
{
    typedef boost::true_type type;
    const static bool value = type::value;
};


/*
 * resizeable specialization for boost::numeric::ublas::permutation_matrix
 */
template< class T , class A >
struct is_resizeable< boost::numeric::ublas::permutation_matrix< T , A > >
{
    typedef boost::true_type type;
    const static bool value = type::value;
};


// specialization for ublas::matrix
// same size and resize specialization for matrix-matrix resizing
template< class T , class L , class A , class T2 , class L2 , class A2 >
struct same_size_impl< boost::numeric::ublas::matrix< T , L , A > , boost::numeric::ublas::matrix< T2 , L2 , A2 > >
{
    static bool same_size( const boost::numeric::ublas::matrix< T , L , A > &m1 ,
                           const boost::numeric::ublas::matrix< T2 , L2 , A2 > &m2 )
    {
        return ( ( m1.size1() == m2.size1() ) && ( m1.size2() == m2.size2() ) );
    }
};

template< class T , class L , class A , class T2 , class L2 , class A2 >
struct resize_impl< boost::numeric::ublas::matrix< T , L , A > , boost::numeric::ublas::matrix< T2 , L2 , A2 > >
{
    static void resize( boost::numeric::ublas::matrix< T , L , A > &m1 ,
                        const boost::numeric::ublas::matrix< T2 , L2 , A2 > &m2 )
    {
        m1.resize( m2.size1() , m2.size2() );
    }
};



// same size and resize specialization for matrix-vector resizing
template< class T , class L , class A , class T_V , class A_V >
struct same_size_impl< boost::numeric::ublas::matrix< T , L , A > , boost::numeric::ublas::vector< T_V , A_V > >
{
    static bool same_size( const boost::numeric::ublas::matrix< T , L , A > &m ,
                           const boost::numeric::ublas::vector< T_V , A_V > &v )
    {
        return ( ( m.size1() == v.size() ) && ( m.size2() == v.size() ) );
    }
};

template< class T , class L , class A , class T_V , class A_V >
struct resize_impl< boost::numeric::ublas::matrix< T , L , A > , boost::numeric::ublas::vector< T_V , A_V > >
{
    static void resize( boost::numeric::ublas::matrix< T , L , A > &m ,
                        const boost::numeric::ublas::vector< T_V , A_V > &v )
    {
        m.resize( v.size() , v.size() );
    }
};



// specialization for ublas::permutation_matrix
// same size and resize specialization for matrix-vector resizing
template< class T , class A , class T_V , class A_V >
struct same_size_impl< boost::numeric::ublas::permutation_matrix< T , A > ,
                       boost::numeric::ublas::vector< T_V , A_V > >
{
    static bool same_size( const boost::numeric::ublas::permutation_matrix< T , A > &m ,
                           const boost::numeric::ublas::vector< T_V , A_V > &v )
    {
        return ( m.size() == v.size() ); // && ( m.size2() == v.size() ) );
    }
};

template< class T , class A , class T_V , class A_V >
struct resize_impl< boost::numeric::ublas::vector< T_V , A_V > ,
                    boost::numeric::ublas::permutation_matrix< T , A > >
{
    static void resize( const boost::numeric::ublas::vector< T_V , A_V > &v,
                        boost::numeric::ublas::permutation_matrix< T , A > &m )
    {
        m.resize( v.size() , v.size() );
    }
};







template< class T , class A >
struct state_wrapper< boost::numeric::ublas::permutation_matrix< T , A > > // with resizing
{
    typedef boost::numeric::ublas::permutation_matrix< T , A > state_type;
    typedef state_wrapper< state_type > state_wrapper_type;

    state_type m_v;

    state_wrapper() : m_v( 1 ) // permutation matrix constructor requires a size, choose 1 as default
    { }

};




} } }


#endif // BOOST_NUMERIC_ODEINT_UTIL_UBLAS_WRAPPER_HPP_INCLUDED
