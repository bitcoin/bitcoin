// Boost.uBLAS
//
// Copyright (c) 2018 Fady Essam
// Copyright (c) 2018 Stefan Seefeld
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
// copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_numeric_ublas_opencl_elementwise_hpp_
#define boost_numeric_ublas_opencl_elementwise_hpp_

#include <boost/numeric/ublas/opencl/library.hpp>
#include <boost/numeric/ublas/opencl/vector.hpp>
#include <boost/numeric/ublas/opencl/matrix.hpp>

namespace boost { namespace numeric { namespace ublas { namespace opencl {

namespace compute = boost::compute;
namespace lambda = boost::compute::lambda;

template <typename T, typename L1, typename L2, typename L3, class O>
void element_wise(ublas::matrix<T, L1, opencl::storage> const &a,
		  ublas::matrix<T, L2, opencl::storage> const &b,
		  ublas::matrix<T, L3, opencl::storage> &result,
		  O op, compute::command_queue& queue)
{
  assert(a.device() == b.device() &&
	 a.device() == result.device() &&
	 a.device() == queue.get_device());
  assert(a.size1() == b.size1() && a.size2() == b.size2());

  compute::transform(a.begin(),
		     a.end(),
		     b.begin(),
		     result.begin(),
		     op,
		     queue);
  queue.finish();
}

template <typename T, typename L1, typename L2, typename L3, typename A, class O>
void element_wise(ublas::matrix<T, L1, A> const &a,
		  ublas::matrix<T, L2, A> const &b,
		  ublas::matrix<T, L3, A> &result,
		  O op,
		  compute::command_queue &queue)
{
  ublas::matrix<T, L1, opencl::storage> adev(a, queue);
  ublas::matrix<T, L2, opencl::storage> bdev(b, queue);
  ublas::matrix<T, L3, opencl::storage> rdev(a.size1(), b.size2(), queue.get_context());
  element_wise(adev, bdev, rdev, op, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename L1, typename L2, typename A, typename O>
ublas::matrix<T, L1, A> element_wise(ublas::matrix<T, L1, A> const &a,
				     ublas::matrix<T, L2, A> const &b,
				     O op,
				     compute::command_queue &queue)
{
  ublas::matrix<T, L1, A> result(a.size1(), b.size2());
  element_wise(a, b, result, op, queue);
  return result;
}

template <typename T, typename O>
void element_wise(ublas::vector<T, opencl::storage> const &a,
		  ublas::vector<T, opencl::storage> const &b,
		  ublas::vector<T, opencl::storage> &result,
		  O op,
		  compute::command_queue& queue)
{
  assert(a.device() == b.device() &&
	 a.device() == result.device() &&
	 a.device() == queue.get_device());
  assert(a.size() == b.size());
  compute::transform(a.begin(),
		     a.end(),
		     b.begin(),
		     result.begin(),
		     op,
		     queue);
  queue.finish();
}

template <typename T, typename A, typename O>
void element_wise(ublas::vector<T, A> const &a,
		  ublas::vector<T, A> const &b,
		  ublas::vector<T, A>& result,
		  O op,
		  compute::command_queue &queue)
{
  ublas::vector<T, opencl::storage> adev(a, queue);
  ublas::vector<T, opencl::storage> bdev(b, queue);
  ublas::vector<T, opencl::storage> rdev(a.size(), queue.get_context());
  element_wise(adev, bdev, rdev, op, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename A, typename O>
ublas::vector<T, A> element_wise(ublas::vector<T, A> const &a,
				 ublas::vector<T, A> const &b,
				 O op,
				 compute::command_queue &queue)
{
  ublas::vector<T, A> result(a.size());
  element_wise(a, b, result, op, queue);
  return result;
}

template <typename T, typename L1, typename L2, typename L3>
void element_add(ublas::matrix<T, L1, opencl::storage> const &a,
		 ublas::matrix<T, L2, opencl::storage> const &b,
		 ublas::matrix<T, L3, opencl::storage> &result,
		 compute::command_queue &queue)
{
  element_wise(a, b, result, compute::plus<T>(), queue);
}

template <typename T, typename L1, typename L2, typename L3, typename A>
void element_add(ublas::matrix<T, L1, A> const &a,
		 ublas::matrix<T, L2, A> const &b,
		 ublas::matrix<T, L3, A> &result,
		 compute::command_queue &queue)
{
  element_wise(a, b, result, compute::plus<T>(), queue);
}

template <typename T, typename L1, typename L2, typename A>
ublas::matrix<T, L1, A> element_add(ublas::matrix<T, L1, A> const &a,
				    ublas::matrix<T, L2, A> const &b,
				    compute::command_queue &queue)
{
  return element_wise(a, b, compute::plus<T>(), queue);
}

template <typename T>
void element_add(ublas::vector<T, opencl::storage> const &a,
		 ublas::vector<T, opencl::storage> const &b,
		 ublas::vector<T, opencl::storage> &result,
		 compute::command_queue& queue)
{
  element_wise(a, b, result, compute::plus<T>(), queue);
}

template <typename T, typename A>
void element_add(ublas::vector<T, A> const &a,
		 ublas::vector<T, A> const &b,
		 ublas::vector<T, A> &result,
		 compute::command_queue &queue)
{
  element_wise(a, b, result, compute::plus<T>(), queue);
}

template <typename T, typename A>
ublas::vector<T, A> element_add(ublas::vector<T, A> const &a,
				ublas::vector<T, A> const &b,
				compute::command_queue &queue)
{
  return element_wise(a, b, compute::plus<T>(), queue);
}

template<typename T, typename L>
void element_add(ublas::matrix<T, L, opencl::storage> const &m, T value,
		 ublas::matrix<T, L, opencl::storage> &result,
		 compute::command_queue& queue)
{
  assert(m.device() == result.device() && m.device() == queue.get_device());
  assert(m.size1() == result.size1() && m.size2() == result.size2());
  compute::transform(m.begin(), m.end(), result.begin(), lambda::_1 + value, queue);
  queue.finish();
}

template<typename T, typename L, typename A>
void element_add(ublas::matrix<T, L, A> const &m, T value,
		 ublas::matrix<T, L, A> &result,
		 compute::command_queue& queue)
{
  ublas::matrix<T, L, opencl::storage> mdev(m, queue);
  ublas::matrix<T, L, opencl::storage> rdev(result.size1(), result.size2(), queue.get_context());
  element_add(mdev, value, rdev, queue);
  rdev.to_host(result, queue);
}

template<typename T, typename L, typename A>
ublas::matrix<T, L, A> element_add(ublas::matrix<T, L, A> const &m, T value,
				   compute::command_queue& queue)
{
  ublas::matrix<T, L, A> result(m.size1(), m.size2());
  element_add(m, value, result, queue);
  return result;
}

template<typename T>
void element_add(ublas::vector<T, opencl::storage> const &v, T value,
		 ublas::vector<T, opencl::storage> &result,
		 compute::command_queue& queue)
{
  assert(v.device() == result.device() && v.device() == queue.get_device());
  assert(v.size() == result.size());
  compute::transform(v.begin(), v.end(), result.begin(), lambda::_1 + value, queue);
  queue.finish();
}

template<typename T, typename A>
void element_add(ublas::vector<T, A> const &v, T value,
		 ublas::vector<T, A> &result,
		 compute::command_queue& queue)
{
  ublas::vector<T, opencl::storage> vdev(v, queue);
  ublas::vector<T, opencl::storage> rdev(v.size(), queue.get_context());
  element_add(vdev, value, rdev, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename A>
ublas::vector<T, A> element_add(ublas::vector<T, A> const &v, T value,
				compute::command_queue& queue)
{
  ublas::vector<T, A> result(v.size());
  element_add(v, value, result, queue);
  return result;
}

template <typename T, typename L1, typename L2, typename L3>
void element_sub(ublas::matrix<T, L1, opencl::storage> const &a,
		 ublas::matrix<T, L2, opencl::storage> const &b,
		 ublas::matrix<T, L3, opencl::storage> &result,
		 compute::command_queue& queue)
{
  element_wise(a, b, compute::minus<T>(), result, queue);
}

template <typename T, typename L1, typename L2, typename L3, typename A>
void element_sub(ublas::matrix<T, L1, A> const &a,
		 ublas::matrix<T, L2, A> const &b,
		 ublas::matrix<T, L3, A> &result,
		 compute::command_queue &queue)
{
  element_wise(a, b, result, compute::minus<T>(), queue);
}

template <typename T, typename L1, typename L2, typename A>
ublas::matrix<T, L1, A> element_sub(ublas::matrix<T, L1, A> const &a,
				    ublas::matrix<T, L2, A> const &b,
				    compute::command_queue &queue)
{
  return element_wise(a, b, compute::minus<T>(), queue);
}

template <typename T>
void element_sub(ublas::vector<T, opencl::storage> const &a,
		 ublas::vector<T, opencl::storage> const &b,
		 ublas::vector<T, opencl::storage> &result,
		 compute::command_queue& queue)
{
  element_wise(a, b, result, compute::minus<T>(), queue);
}

template <typename T, typename A>
void element_sub(ublas::vector<T, A> const &a,
		 ublas::vector<T, A> const &b,
		 ublas::vector<T, A> &result,
		 compute::command_queue &queue)
{
  element_wise(a, b, result, compute::minus<T>(), queue);
}

template <typename T, typename A>
ublas::vector<T, A> element_sub(ublas::vector<T, A> const &a,
				ublas::vector<T, A> const &b,
				compute::command_queue &queue)
{
  return element_wise(a, b, compute::minus<T>(), queue);
}

template <typename T, typename L>
void element_sub(ublas::matrix<T, L, opencl::storage> const &m, T value,
		 ublas::matrix<T, L, opencl::storage> &result,
		 compute::command_queue& queue)
{
  assert(m.device() == result.device() && m.device() == queue.get_device());
  assert(m.size1() == result.size1() && m.size2() == result.size2());
  compute::transform(m.begin(), m.end(), result.begin(), lambda::_1 - value, queue);
  queue.finish();
}

template <typename T, typename L, typename A>
void element_sub(ublas::matrix<T, L, A> const &m, T value,
		 ublas::matrix<T, L, A> &result,
		 compute::command_queue& queue)
{
  ublas::matrix<T, L, opencl::storage> mdev(m, queue);
  ublas::matrix<T, L, opencl::storage> rdev(result.size1(), result.size2(), queue.get_context());
  element_sub(mdev, value, rdev, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename L, typename A>
ublas::matrix<T, L, A> element_sub(ublas::matrix<T, L, A> const &m, T value,
				   compute::command_queue& queue)
{
  ublas::matrix<T, L, A> result(m.size1(), m.size2());
  element_sub(m, value, result, queue);
  return result;
}

template <typename T>
void element_sub(ublas::vector<T, opencl::storage> const &v, T value,
		 ublas::vector<T, opencl::storage> &result,
		 compute::command_queue& queue)
{
  assert(v.device() == result.device() && v.device() == queue.get_device());
  assert(v.size() == result.size());
  compute::transform(v.begin(), v.end(), result.begin(), lambda::_1 - value, queue);
  queue.finish();
}

template <typename T, typename A>
void element_sub(ublas::vector<T, A> const &v, T value,
		 ublas::vector<T, A> &result,
		 compute::command_queue& queue)
{
  ublas::vector<T, opencl::storage> vdev(v, queue);
  ublas::vector<T, opencl::storage> rdev(v.size(), queue.get_context());
  element_sub(vdev, value, rdev, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename A>
ublas::vector<T, A> element_sub(ublas::vector<T, A> const &v, T value,
				compute::command_queue& queue)
{
  ublas::vector<T, A> result(v.size());
  element_sub(v, value, result, queue);
  return result;
}

template <typename T, typename L1, typename L2, typename L3>
void element_prod(ublas::matrix<T, L1, opencl::storage> const &a,
		  ublas::matrix<T, L2, opencl::storage> const &b,
		  ublas::matrix<T, L3, opencl::storage> &result,
		  compute::command_queue& queue)
{
  element_wise(a, b, result, compute::multiplies<T>(), queue);
}

template <typename T, typename L1, typename L2, typename L3, typename A>
void element_prod(ublas::matrix<T, L1, A> const &a,
		  ublas::matrix<T, L2, A> const &b,
		  ublas::matrix<T, L3, A> &result,
		  compute::command_queue &queue)
{
  element_wise(a, b, result, compute::multiplies<T>(), queue);
}

template <typename T, typename L1, typename L2, typename A>
ublas::matrix<T, L1, A> element_prod(ublas::matrix<T, L1, A> const &a,
				     ublas::matrix<T, L2, A> const &b,
				     compute::command_queue &queue)
{
  return element_wise(a, b, compute::multiplies<T>(), queue);
}

template <typename T>
void element_prod(ublas::vector<T, opencl::storage> const &a,
		  ublas::vector<T, opencl::storage> const &b,
		  ublas::vector<T, opencl::storage> &result,
		  compute::command_queue& queue)
{
  element_wise(a, b, result, compute::multiplies<T>(), queue);
}

template <typename T, typename A>
void element_prod(ublas::vector<T, A> const &a,
		  ublas::vector<T, A> const &b,
		  ublas::vector<T, A> &result,
		  compute::command_queue &queue)
{
  element_wise(a, b, result, compute::multiplies<T>(), queue);
}

template <typename T, typename A>
ublas::vector<T, A> element_prod(ublas::vector<T, A> const &a,
				 ublas::vector<T, A> const &b,
				 compute::command_queue &queue)
{
  return element_wise(a, b, compute::multiplies<T>(), queue);
}

template <typename T, typename L>
void element_scale(ublas::matrix<T, L, opencl::storage> const &m, T value,
		   ublas::matrix<T, L, opencl::storage> &result,
		   compute::command_queue& queue)
{
  assert(m.device() == result.device() && m.device() == queue.get_device());
  assert(m.size1() == result.size1() && m.size2() == result.size2());
  compute::transform(m.begin(), m.end(), result.begin(), lambda::_1 * value, queue);
  queue.finish();
}

template <typename T, typename L, typename A>
void element_scale(ublas::matrix<T, L, A> const &m, T value,
		   ublas::matrix<T, L, A> &result,
		   compute::command_queue& queue)
{
  ublas::matrix<T, L, opencl::storage> mdev(m, queue);
  ublas::matrix<T, L, opencl::storage> rdev(result.size1(), result.size2(), queue.get_context());
  element_scale(mdev, value, rdev, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename L, typename A>
ublas::matrix<T, L, A>  element_scale(ublas::matrix<T, L, A> const &m, T value,
				      compute::command_queue& queue)
{
  ublas::matrix<T, L, A> result(m.size1(), m.size2());
  element_scale(m, value, result, queue);
  return result;
}

template <typename T>
void element_scale(ublas::vector<T, opencl::storage> const &v, T value,
		   ublas::vector<T, opencl::storage> &result,
		   compute::command_queue& queue)
{
  assert(v.device() == result.device() && v.device() == queue.get_device());
  assert(v.size() == result.size());
  compute::transform(v.begin(), v.end(), result.begin(), lambda::_1 * value, queue);
  queue.finish();
}

template <typename T, typename A>
void element_scale(ublas::vector<T, A> const &v, T value,
		   ublas::vector<T, A> & result,
		   compute::command_queue& queue)
{
  ublas::vector<T, opencl::storage> vdev(v, queue);
  ublas::vector<T, opencl::storage> rdev(v.size(), queue.get_context());
  element_scale(vdev, value, rdev, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename A>
ublas::vector<T,A> element_scale(ublas::vector<T, A> const &v, T value,
				 compute::command_queue& queue)
{
  ublas::vector<T, A> result(v.size());
  element_scale(v, value, result, queue);
  return result;
}

template <typename T, typename L1, typename L2, typename L3>
void element_div(ublas::matrix<T, L1, opencl::storage> const &a,
		 ublas::matrix<T, L2, opencl::storage> const &b,
		 ublas::matrix<T, L3, opencl::storage> &result,
		 compute::command_queue& queue)
{
  element_wise(a, b, result, compute::divides<T>(), queue);
}

template <typename T, typename L1, typename L2, typename L3, typename A>
void element_div(ublas::matrix<T, L1, A> const &a,
		 ublas::matrix<T, L2, A> const &b,
		 ublas::matrix<T, L3, A> &result,
		 compute::command_queue &queue)
{
  element_wise(a, b, result, compute::divides<T>(), queue);
}

template <typename T, typename L1, typename L2, typename A>
ublas::matrix<T, L1, A> element_div(ublas::matrix<T, L1, A> const &a,
				    ublas::matrix<T, L2, A> const &b,
				    compute::command_queue &queue)
{
  return element_wise(a, b, compute::divides<T>(), queue);
}

template <typename T>
void element_div(ublas::vector<T, opencl::storage> const &a,
		 ublas::vector<T, opencl::storage> const &b,
		 ublas::vector<T, opencl::storage> &result,
		 compute::command_queue& queue)
{
  element_wise(a, b, result, compute::divides<T>(), queue);
}

template <typename T, typename A>
void element_div(ublas::vector<T, A> const &a,
		 ublas::vector<T, A> const &b,
		 ublas::vector<T, A> &result,
		 compute::command_queue &queue)
{
  element_wise(a, b, result, compute::divides<T>(), queue);
}

template <typename T, typename A>
ublas::vector<T, A> element_div(ublas::vector<T, A> const &a,
				ublas::vector<T, A> const &b,
				compute::command_queue &queue)
{
  return element_wise(a, b, compute::divides<T>(), queue);
}

}}}}

#endif 
