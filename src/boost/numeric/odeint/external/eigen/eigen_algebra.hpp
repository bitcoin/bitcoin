/*
  [auto_generated]
  boost/numeric/odeint/external/eigen/eigen_algebra.hpp

  [begin_description]
  tba.
  [end_description]

  Copyright 2013 Christian Shelton
  Copyright 2013 Karsten Ahnert

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE_1_0.txt or
  copy at http://www.boost.org/LICENSE_1_0.txt)
*/


#ifndef BOOST_NUMERIC_ODEINT_EXTERNAL_EIGEN_EIGEN_ALGEBRA_HPP_INCLUDED
#define BOOST_NUMERIC_ODEINT_EXTERNAL_EIGEN_EIGEN_ALGEBRA_HPP_INCLUDED

#include <Eigen/Dense>
#include <boost/numeric/odeint/algebra/vector_space_algebra.hpp>

// Necessary routines for Eigen matrices to work with vector_space_algebra
// from odeint
// (that is, it lets odeint treat the eigen matrices correctly, knowing
// how to add, multiply, compute the norm, etc)
namespace Eigen {

template<typename D>
inline const
typename Eigen::CwiseBinaryOp<
    internal::scalar_sum_op<typename internal::traits<D>::Scalar>,
    typename DenseBase<D>::ConstantReturnType,
    const D>
operator+(const typename Eigen::MatrixBase<D> &m,
          const typename Eigen::internal::traits<D>::Scalar &s) {
    return CwiseBinaryOp<
        internal::scalar_sum_op<typename internal::traits<D>::Scalar>,
        typename DenseBase<D>::ConstantReturnType,
        const D>(DenseBase<D>::Constant(m.rows(), m.cols(), s), m.derived());
}

template<typename D>
inline const
typename Eigen::CwiseBinaryOp<
    internal::scalar_sum_op<typename internal::traits<D>::Scalar>,
    typename DenseBase<D>::ConstantReturnType,
    const D>
operator+(const typename Eigen::internal::traits<D>::Scalar &s,
          const typename Eigen::MatrixBase<D> &m) {
    return CwiseBinaryOp<
        internal::scalar_sum_op<typename internal::traits<D>::Scalar>,
        typename DenseBase<D>::ConstantReturnType,
        const D>(DenseBase<D>::Constant(m.rows(), m.cols(), s), m.derived());
}

template<typename D1,typename D2>
inline const
typename Eigen::CwiseBinaryOp<
    typename Eigen::internal::scalar_quotient_op<
        typename Eigen::internal::traits<D1>::Scalar>,
    const D1, const D2>
operator/(const Eigen::MatrixBase<D1> &x1, const Eigen::MatrixBase<D2> &x2) {
    return x1.cwiseQuotient(x2);
}


template< typename D >
inline const 
typename Eigen::CwiseUnaryOp<
    typename Eigen::internal::scalar_abs_op<
        typename Eigen::internal::traits< D >::Scalar > ,
        const D >
abs( const Eigen::MatrixBase< D > &m ) {
    return m.cwiseAbs();
}

} // end Eigen namespace


namespace boost {
namespace numeric {
namespace odeint {

template<typename B,int S1,int S2,int O, int M1, int M2>
struct vector_space_norm_inf< Eigen::Matrix<B,S1,S2,O,M1,M2> >
{
    typedef B result_type;
    result_type operator()( const Eigen::Matrix<B,S1,S2,O,M1,M2> &m ) const
    {
        return m.template lpNorm<Eigen::Infinity>();
    }
};

} } } // end boost::numeric::odeint namespace

#endif // BOOST_NUMERIC_ODEINT_EXTERNAL_EIGEN_EIGEN_ALGEBRA_HPP_INCLUDED
