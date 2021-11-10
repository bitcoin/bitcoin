/*
  [auto_generated]
  boost/numeric/odeint/external/thrust/thrust_operations_dispatcher.hpp

  [begin_description]
  operations_dispatcher specialization for thrust
  [end_description]

  Copyright 2013-2014 Karsten Ahnert
  Copyright 2013-2014 Mario Mulansky

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_OPERATIONS_DISPATCHER_HPP_DEFINED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_OPERATIONS_DISPATCHER_HPP_DEFINED

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

#include <boost/numeric/odeint/external/thrust/thrust_operations.hpp>
#include <boost/numeric/odeint/algebra/operations_dispatcher.hpp>

// support for the standard thrust containers

namespace boost {
namespace numeric {
namespace odeint {

// specialization for thrust host_vector
template< class T , class A >
struct operations_dispatcher< thrust::host_vector< T , A > >
{
    typedef thrust_operations operations_type;
};

// specialization for thrust device_vector
template< class T , class A >
struct operations_dispatcher< thrust::device_vector< T , A > >
{
    typedef thrust_operations operations_type;
};

} // namespace odeint
} // namespace numeric
} // namespace boost

// add support for thrust backend vectors, if available

#include <thrust/version.h>

#if THRUST_VERSION >= 100600

// specialization for thrust cpp vector
#include <thrust/system/cpp/vector.h>
namespace boost { namespace numeric { namespace odeint {
    template< class T , class A >
    struct operations_dispatcher< thrust::cpp::vector< T , A > >
    {
        typedef thrust_operations operations_type;
    };
} } }

// specialization for thrust omp vector
#ifdef _OPENMP
#include <thrust/system/omp/vector.h>
namespace boost { namespace numeric { namespace odeint {
    template< class T , class A >
    struct operations_dispatcher< thrust::omp::vector< T , A > >
    {
        typedef thrust_operations operations_type;
    };
} } }
#endif // _OPENMP

// specialization for thrust tbb vector
#ifdef TBB_VERSION_MAJOR
#include <thrust/system/tbb/vector.h>
namespace boost { namespace numeric { namespace odeint {
    template< class T , class A >
    struct operations_dispatcher< thrust::tbb::vector< T , A > >
    {
        typedef thrust_operations operations_type;
    };
} } }
#endif // TBB_VERSION_MAJOR

// specialization for thrust cuda vector
#ifdef __CUDACC__
#include <thrust/system/cuda/vector.h>
namespace boost { namespace numeric { namespace odeint {
    template< class T , class A >
    struct operations_dispatcher< thrust::cuda::vector< T , A > >
    {
        typedef thrust_operations operations_type;
    };
} } }
#endif // __CUDACC__

#endif // THRUST_VERSION >= 100600


#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_THRUST_THRUST_OPERATIONS_DISPATCHER_HPP_DEFINED

