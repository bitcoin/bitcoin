/*
 [auto_generated]
 boost/numeric/odeint/external/thrust/thrust_resize.hpp

 [begin_description]
 Enable resizing for thrusts device and host_vector.
 [end_description]

 Copyright 2010-2014 Mario Mulansky
 Copyright 2010-2011 Karsten Ahnert

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
 */


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_RESIZE_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_RESIZE_HPP_INCLUDED

#include <boost/range.hpp>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/distance.h>

#include <boost/numeric/odeint/util/resize.hpp>
#include <boost/numeric/odeint/util/same_size.hpp>
#include <boost/numeric/odeint/util/copy.hpp>

namespace boost {
namespace numeric {
namespace odeint {

// some macros that define the necessary utilities

#define ODEINT_THRUST_VECTOR_IS_RESIZEABLE( THRUST_VECTOR ) \
template< class T , class A >                               \
struct is_resizeable< THRUST_VECTOR<T,A> >                  \
{                                                           \
    struct type : public boost::true_type { };              \
    const static bool value = type::value;                  \
};                                                          \

#define ODEINT_TRHUST_VECTOR_RESIZE_IMPL( THRUST_VECTOR )     \
template< class T, class A >                                  \
struct resize_impl< THRUST_VECTOR<T,A> , THRUST_VECTOR<T,A> > \
{                                                             \
    static void resize( THRUST_VECTOR<T,A> &x ,               \
                        const THRUST_VECTOR<T,A> &y )         \
    {                                                         \
        x.resize( y.size() );                                 \
    }                                                         \
};                                                            \
template< class T, class A, typename Range >                  \
struct resize_impl< THRUST_VECTOR<T,A> , Range >              \
{                                                             \
    static void resize( THRUST_VECTOR<T,A> &x ,               \
                        const Range &y )                      \
    {                                                         \
        x.resize( thrust::distance(boost::begin(y),           \
                                   boost::end(y)));           \
    }                                                         \
};                                                            \


#define ODEINT_THRUST_SAME_SIZE_IMPL( THRUST_VECTOR )            \
template< class T , class A >                                    \
struct same_size_impl< THRUST_VECTOR<T,A> , THRUST_VECTOR<T,A> > \
{                                                                \
    static bool same_size( const THRUST_VECTOR<T,A> &x ,         \
                           const THRUST_VECTOR<T,A> &y )         \
    {                                                            \
        return x.size() == y.size();                             \
    }                                                            \
};                                                               \
template< class T , class A, typename Range >                    \
struct same_size_impl< THRUST_VECTOR<T,A> , Range >              \
{                                                                \
    static bool same_size( const THRUST_VECTOR<T,A> &x ,         \
                           const Range &y )                      \
    {                                                            \
        return x.size() == thrust::distance(boost::begin(y),     \
                                            boost::end(y));      \
    }                                                            \
};                                                               \


#define ODEINT_THRUST_COPY_IMPL( THRUST_VECTOR )                        \
template< class Container1 , class T , class A >                        \
struct copy_impl< Container1 , THRUST_VECTOR<T,A> >                     \
{                                                                       \
    static void copy( const Container1 &from , THRUST_VECTOR<T,A> &to ) \
    {                                                                   \
        thrust::copy( boost::begin( from ) , boost::end( from ) ,       \
                      boost::begin( to ) );                             \
    }                                                                   \
};                                                                      \
                                                                        \
template< class T , class A , class Container2 >                        \
struct copy_impl< THRUST_VECTOR<T,A> , Container2 >                     \
{                                                                       \
    static void copy( const THRUST_VECTOR<T,A> &from , Container2 &to ) \
    {                                                                   \
        thrust::copy( boost::begin( from ) , boost::end( from ) ,       \
                      boost::begin( to ) );                             \
    }                                                                   \
};                                                                      \
                                                                        \
template< class T , class A >                                           \
struct copy_impl< THRUST_VECTOR<T,A> , THRUST_VECTOR<T,A> >             \
{                                                                       \
    static void copy( const THRUST_VECTOR<T,A> &from ,                  \
                      THRUST_VECTOR<T,A> &to )                          \
    {                                                                   \
        thrust::copy( boost::begin( from ) , boost::end( from ) ,       \
                      boost::begin( to ) );                             \
    }                                                                   \
};                                                                      \

// add support for the standard thrust containers

ODEINT_THRUST_VECTOR_IS_RESIZEABLE( thrust::device_vector )
ODEINT_TRHUST_VECTOR_RESIZE_IMPL( thrust::device_vector )
ODEINT_THRUST_SAME_SIZE_IMPL( thrust::device_vector )
ODEINT_THRUST_COPY_IMPL( thrust::device_vector )


ODEINT_THRUST_VECTOR_IS_RESIZEABLE( thrust::host_vector )
ODEINT_TRHUST_VECTOR_RESIZE_IMPL( thrust::host_vector )
ODEINT_THRUST_SAME_SIZE_IMPL( thrust::host_vector )
ODEINT_THRUST_COPY_IMPL( thrust::host_vector )


} // odeint
} // numeric
} // boost

// add support for thrust backend vectors, if available

#include <thrust/version.h>

#if THRUST_VERSION >= 100600

#include <thrust/system/cpp/vector.h>
namespace boost { namespace numeric { namespace odeint {
    ODEINT_THRUST_VECTOR_IS_RESIZEABLE( thrust::cpp::vector )
    ODEINT_TRHUST_VECTOR_RESIZE_IMPL( thrust::cpp::vector )
    ODEINT_THRUST_SAME_SIZE_IMPL( thrust::cpp::vector )
    ODEINT_THRUST_COPY_IMPL( thrust::cpp::vector )
} } }

#ifdef _OPENMP
#include <thrust/system/omp/vector.h>
namespace boost { namespace numeric { namespace odeint {
    ODEINT_THRUST_VECTOR_IS_RESIZEABLE( thrust::omp::vector )
    ODEINT_TRHUST_VECTOR_RESIZE_IMPL( thrust::omp::vector )
    ODEINT_THRUST_SAME_SIZE_IMPL( thrust::omp::vector )
    ODEINT_THRUST_COPY_IMPL( thrust::omp::vector )
} } }
#endif // _OPENMP

#ifdef TBB_VERSION_MAJOR
#include <thrust/system/tbb/vector.h>
namespace boost { namespace numeric { namespace odeint {
    ODEINT_THRUST_VECTOR_IS_RESIZEABLE( thrust::tbb::vector )
    ODEINT_TRHUST_VECTOR_RESIZE_IMPL( thrust::tbb::vector )
    ODEINT_THRUST_SAME_SIZE_IMPL( thrust::tbb::vector )
    ODEINT_THRUST_COPY_IMPL( thrust::tbb::vector )
} } }
#endif // TBB_VERSION_MAJOR

#ifdef __CUDACC__
#include <thrust/system/cuda/vector.h>
namespace boost { namespace numeric { namespace odeint {
    ODEINT_THRUST_VECTOR_IS_RESIZEABLE( thrust::cuda::vector )
    ODEINT_TRHUST_VECTOR_RESIZE_IMPL( thrust::cuda::vector )
    ODEINT_THRUST_SAME_SIZE_IMPL( thrust::cuda::vector )
    ODEINT_THRUST_COPY_IMPL( thrust::cuda::vector )
} } }
#endif // __CUDACC__

#endif // THRUST_VERSION >= 100600

#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_RESIZE_HPP_INCLUDED
