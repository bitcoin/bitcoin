// Boost.uBLAS
//
// Copyright (c) 2018 Fady Essam
// Copyright (c) 2018 Stefan Seefeld
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
// copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_numeric_ublas_opencl_prod_hpp_
#define boost_numeric_ublas_opencl_prod_hpp_

#include <boost/numeric/ublas/opencl/library.hpp>
#include <boost/numeric/ublas/opencl/vector.hpp>
#include <boost/numeric/ublas/opencl/matrix.hpp>
#include <boost/numeric/ublas/opencl/transpose.hpp>
#include <boost/compute/buffer.hpp>

namespace boost { namespace numeric { namespace ublas { namespace opencl {

#define ONE_DOUBLE_COMPLEX  {{1.0, 00.0}}
#define ONE_FLOAT_COMPLEX  {{1.0f, 00.0f}}

template <typename T, typename L1, typename L2>
typename std::enable_if<is_numeric<T>::value>::type
prod(ublas::matrix<T, L1, opencl::storage> const &a,
     ublas::matrix<T, L2, opencl::storage> const &b,
     ublas::matrix<T, L1, opencl::storage> &result,
     compute::command_queue &queue)
{
  assert(a.device() == b.device() &&
	 a.device() == result.device() &&
	 a.device() == queue.get_device());
  assert(a.size2() == b.size1());

  result.fill(0, queue);

  //to hold matrix b with layout 1 if the b has different layout
  std::unique_ptr<ublas::matrix<T, L1, opencl::storage>> bl1;

  cl_event event = NULL;

  cl_mem buffer_a = a.begin().get_buffer().get();
  cl_mem buffer_b = b.begin().get_buffer().get();
  cl_mem buffer_result = result.begin().get_buffer().get();

  if (!(std::is_same<L1, L2>::value))
  {
    bl1.reset(new ublas::matrix<T, L1, opencl::storage>(b.size1(), b.size2(), queue.get_context()));
    change_layout(b, *bl1, queue);
    buffer_b = bl1->begin().get_buffer().get();
  }

  clblasOrder Order = std::is_same<L1, ublas::basic_row_major<> >::value ? clblasRowMajor : clblasColumnMajor;
  size_t lda = Order == clblasRowMajor ? a.size2() : a.size1();
  size_t ldb = Order == clblasRowMajor ? b.size2() : a.size2();
  size_t ldc = Order == clblasRowMajor ? b.size2() : a.size1();

  if (std::is_same<T, float>::value)
    clblasSgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), b.size2(), a.size2(),
		1, buffer_a, 0, lda,
		buffer_b, 0, ldb, 1,
		buffer_result, 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, double>::value)
    clblasDgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), b.size2(), a.size2(),
		1, buffer_a, 0, lda,
		buffer_b, 0, ldb, 1,
		buffer_result, 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<float>>::value)
    clblasCgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), b.size2(), a.size2(),
		ONE_FLOAT_COMPLEX, buffer_a, 0, lda,
		buffer_b, 0, ldb, ONE_FLOAT_COMPLEX,
		buffer_result, 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<double>>::value)
    clblasZgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), b.size2(), a.size2(),
		ONE_DOUBLE_COMPLEX, buffer_a, 0, lda,
		buffer_b, 0, ldb, ONE_DOUBLE_COMPLEX,
		buffer_result, 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  clWaitForEvents(1, &event);
}

template <typename T, typename L1, typename L2, typename A>
typename std::enable_if<is_numeric<T>::value>::type
prod(ublas::matrix<T, L1, A> const &a,
     ublas::matrix<T, L2, A> const &b,
     ublas::matrix<T, L1, A> &result,
     compute::command_queue &queue)
{
  ublas::matrix<T, L1, opencl::storage> adev(a, queue);
  ublas::matrix<T, L2, opencl::storage> bdev(b, queue);
  ublas::matrix<T, L1, opencl::storage> rdev(a.size1(), b.size2(), queue.get_context());
  prod(adev, bdev, rdev, queue);
  rdev.to_host(result,queue);
}

template <typename T, typename L1, typename L2, typename A>
typename std::enable_if<is_numeric<T>::value, ublas::matrix<T, L1, A>>::type
prod(ublas::matrix<T, L1, A> const &a,
     ublas::matrix<T, L2, A> const &b,
     compute::command_queue &queue)
{
  ublas::matrix<T, L1, A> result(a.size1(), b.size2());
  prod(a, b, result, queue);
  return result;
}

template <typename T, typename L>
typename std::enable_if<is_numeric<T>::value>::type
prod(ublas::matrix<T, L, opencl::storage> const &a,
     ublas::vector<T, opencl::storage> const &b,
     ublas::vector<T, opencl::storage> &result,
     compute::command_queue &queue)
{
  assert(a.device() == b.device() &&
	 a.device() == result.device() &&
	 a.device() == queue.get_device());
  assert(a.size2() == b.size());
  result.fill(0, queue);

  cl_event event = NULL;
  clblasOrder Order = std::is_same<L, ublas::basic_row_major<> >::value ? clblasRowMajor : clblasColumnMajor;
  int lda = Order == clblasRowMajor ? a.size2() : a.size1();
  int ldb = Order == clblasRowMajor ? 1 : a.size2();
  int ldc = Order == clblasRowMajor ? 1 : a.size1();

  if (std::is_same<T, float>::value)
    clblasSgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), 1, a.size2(),
		1, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, 1,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, double>::value)
    clblasDgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), 1, a.size2(),
		1, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, 1,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<float>>::value)
    clblasCgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), 1, a.size2(),
		ONE_FLOAT_COMPLEX, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, ONE_FLOAT_COMPLEX,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<double>>::value)
    clblasZgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size1(), 1, a.size2(),
		ONE_DOUBLE_COMPLEX, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, ONE_DOUBLE_COMPLEX,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  clWaitForEvents(1, &event);
}

template <typename T, typename L, typename A>
typename std::enable_if<is_numeric<T>::value>::type
prod(ublas::matrix<T, L, A> const &a,
     ublas::vector<T, A> const &b,
     ublas::vector<T, A> &result,
     compute::command_queue &queue)
{
  ublas::matrix<T, L, opencl::storage> adev(a, queue);
  ublas::vector<T, opencl::storage> bdev(b, queue);
  ublas::vector<T, opencl::storage> rdev(a.size1(), queue.get_context());
  prod(adev, bdev, rdev, queue);
  rdev.to_host(result, queue);
}

template <typename T, typename L, typename A>
typename std::enable_if<is_numeric<T>::value, ublas::vector<T, A>>::type
prod(ublas::matrix<T, L, A> const &a,
     ublas::vector<T, A> const &b,
     compute::command_queue &queue)
{
  ublas::vector<T, A> result(a.size1());
  prod(a, b, result, queue);
  return result;
}

template <typename T, typename L>
typename std::enable_if<is_numeric<T>::value>::type
prod(ublas::vector<T, opencl::storage> const &a,
     ublas::matrix<T, L, opencl::storage> const &b,
     ublas::vector<T, opencl::storage> &result,
     compute::command_queue &queue)
{
  assert(a.device() == b.device() &&
	 a.device() == result.device() &&
	 a.device() == queue.get_device());
  assert(a.size() == b.size1());
  result.fill(0, queue);
  cl_event event = NULL;
  clblasOrder Order = std::is_same<L, ublas::basic_row_major<> >::value ? clblasRowMajor : clblasColumnMajor;
  size_t lda = Order == clblasRowMajor ? a.size() : 1;
  size_t ldb = Order == clblasRowMajor ? b.size2() : a.size();
  size_t ldc = Order == clblasRowMajor ? b.size2() : 1;

  if (std::is_same<T, float>::value)
    clblasSgemm(Order, clblasNoTrans, clblasNoTrans,
		1, b.size2(), a.size(),
		1, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, 1,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, double>::value)
    clblasDgemm(Order, clblasNoTrans, clblasNoTrans,
		1, b.size2(), a.size(),
		1, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, 1,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<float>>::value)
    clblasCgemm(Order, clblasNoTrans, clblasNoTrans,
		1, b.size2(), a.size(),
		ONE_FLOAT_COMPLEX, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, ONE_FLOAT_COMPLEX,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<double>>::value)
    clblasZgemm(Order, clblasNoTrans, clblasNoTrans,
		1, b.size2(), a.size(),
		ONE_DOUBLE_COMPLEX, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, ONE_DOUBLE_COMPLEX,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  clWaitForEvents(1, &event);
}

template <class T, class L, class A>
typename std::enable_if<is_numeric<T>::value>::type
prod(ublas::vector<T, A> const &a,
     ublas::matrix<T, L, A> const &b,
     ublas::vector<T, A> &result,
     compute::command_queue &queue)
{
  ublas::vector<T, opencl::storage> adev(a, queue);
  ublas::matrix<T, L, opencl::storage> bdev(b, queue);
  ublas::vector<T, opencl::storage> rdev(b.size2(), queue.get_context());
  prod(adev, bdev, rdev, queue);
  rdev.to_host(result, queue);
}

template <class T, class L, class A>
typename std::enable_if<is_numeric<T>::value, ublas::vector<T, A>>::type
prod(ublas::vector<T, A> const &a,
     ublas::matrix<T, L, A> const &b,
     compute::command_queue &queue)
{
  ublas::vector<T, A> result(b.size2());
  prod(a, b, result, queue);
  return result;
}

template<class T>
typename std::enable_if<std::is_fundamental<T>::value, T>::type
inner_prod(ublas::vector<T, opencl::storage> const &a,
	   ublas::vector<T, opencl::storage> const &b,
	   compute::command_queue &queue)
{
  assert(a.device() == b.device() && a.device() == queue.get_device());
  assert(a.size() == b.size());
  return compute::inner_product(a.begin(), a.end(), b.begin(), T(0), queue);
}

template<class T, class A>
typename std::enable_if<std::is_fundamental<T>::value, T>::type
inner_prod(ublas::vector<T, A> const &a,
	   ublas::vector<T, A> const &b,
	   compute::command_queue& queue)
{
  ublas::vector<T, opencl::storage> adev(a, queue);
  ublas::vector<T, opencl::storage> bdev(b, queue);
  return inner_prod(adev, bdev, queue);
}

template <class T, class L>
typename std::enable_if<is_numeric<T>::value>::type
outer_prod(ublas::vector<T, opencl::storage> const &a,
	   ublas::vector<T, opencl::storage> const &b,
	   ublas::matrix<T, L, opencl::storage> &result,
	   compute::command_queue & queue)
{
  assert(a.device() == b.device() &&
	 a.device() == result.device() &&
	 a.device() == queue.get_device());
  result.fill(0, queue);
  cl_event event = NULL;
  clblasOrder Order = std::is_same<L, ublas::basic_row_major<> >::value ? clblasRowMajor : clblasColumnMajor;
  size_t lda = Order == clblasRowMajor ? 1 : a.size();
  size_t ldb = Order == clblasRowMajor ? b.size() : 1;
  size_t ldc = Order == clblasRowMajor ? b.size() : a.size();

  if (std::is_same<T, float>::value)
    clblasSgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size(), b.size(), 1,
		1, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, 1,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, double>::value)
    clblasDgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size(), b.size(), 1,
		1, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, 1,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<float>>::value)
    clblasCgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size(), b.size(), 1,
		ONE_FLOAT_COMPLEX, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, ONE_FLOAT_COMPLEX,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  else if (std::is_same<T, std::complex<double>>::value)
    clblasZgemm(Order, clblasNoTrans, clblasNoTrans,
		a.size(), b.size(), 1,
		ONE_DOUBLE_COMPLEX, a.begin().get_buffer().get(), 0, lda,
		b.begin().get_buffer().get(), 0, ldb, ONE_DOUBLE_COMPLEX,
		result.begin().get_buffer().get(), 0, ldc,
		1, &(queue.get()), 0, NULL, &event);
  clWaitForEvents(1, &event);
}

template <class T, class L, class A>
typename std::enable_if<is_numeric<T>::value>::type
outer_prod(ublas::vector<T, A> const &a,
	   ublas::vector<T, A> const &b,
	   ublas::matrix<T, L, A> &result,
	   compute::command_queue &queue)
{
  ublas::vector<T, opencl::storage> adev(a, queue);
  ublas::vector<T, opencl::storage> bdev(b, queue);
  ublas::matrix<T, L, opencl::storage> rdev(a.size(), b.size(), queue.get_context());
  outer_prod(adev, bdev, rdev, queue);
  rdev.to_host(result, queue);
}

template <class T,class L = ublas::basic_row_major<>, class A>
typename std::enable_if<is_numeric<T>::value, ublas::matrix<T, L, A>>::type
outer_prod(ublas::vector<T, A> const &a,
	   ublas::vector<T, A> const &b,
	   compute::command_queue &queue)
{
  ublas::matrix<T, L, A> result(a.size(), b.size());
  outer_prod(a, b, result, queue);
  return result;
}

#undef ONE_DOUBLE_COMPLEX
#undef ONE_FLOAT_COMPLEX

}}}}

#endif 
