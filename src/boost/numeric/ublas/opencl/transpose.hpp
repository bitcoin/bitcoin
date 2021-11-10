// Boost.uBLAS
//
// Copyright (c) 2018 Fady Essam
// Copyright (c) 2018 Stefan Seefeld
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
// copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_numeric_ublas_opencl_transpose_hpp_
#define boost_numeric_ublas_opencl_transpose_hpp_

#include <boost/numeric/ublas/opencl/library.hpp>
#include <boost/numeric/ublas/opencl/vector.hpp>
#include <boost/numeric/ublas/opencl/matrix.hpp>

// Kernel for transposition of various data types
#define OPENCL_TRANSPOSITION_KERNEL(DATA_TYPE)	\
"__kernel void transpose(__global "  #DATA_TYPE "* in, __global " #DATA_TYPE "* result, unsigned int width, unsigned int height) \n"                       \
"{ \n"								        \
"  unsigned int column_index = get_global_id(0); \n"			\
"  unsigned int row_index = get_global_id(1); \n"			\
"  if (column_index < width && row_index < height) \n"			\
"  { \n"							      	\
"    unsigned int index_in = column_index + width * row_index; \n"	\
"    unsigned int index_result = row_index + height * column_index; \n"	\
"    result[index_result] = in[index_in]; \n"				\
"  } \n"								\
"} \n"


namespace boost { namespace numeric { namespace ublas { namespace opencl {

template<class T, class L1, class L2>
typename std::enable_if<is_numeric<T>::value>::type
change_layout(ublas::matrix<T, L1, opencl::storage> const &m,
	      ublas::matrix<T, L2, opencl::storage> &result,
	      compute::command_queue& queue)
{
  assert(m.size1() == result.size1() && m.size2() == result.size2());
  assert(m.device() == result.device() && m.device() == queue.get_device());
  assert(!(std::is_same<L1, L2>::value));
  char const *kernel;
  if (std::is_same<T, float>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(float);
  else if (std::is_same<T, double>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(double);
  else if (std::is_same<T, std::complex<float>>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(float2);
  else if (std::is_same<T, std::complex<double>>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(double2);
  size_t len = strlen(kernel);
  cl_int err;
  cl_context c_context = queue.get_context().get();
  cl_program program = clCreateProgramWithSource(c_context, 1, &kernel, &len, &err);
  clBuildProgram(program, 1, &queue.get_device().get(), NULL, NULL, NULL);
  cl_kernel c_kernel = clCreateKernel(program, "transpose", &err);
  size_t width = std::is_same < L1, ublas::basic_row_major<>>::value ? m.size2() : m.size1();
  size_t height = std::is_same < L1, ublas::basic_row_major<>>::value ? m.size1() : m.size2();
  size_t global_size[2] = { width , height };
  clSetKernelArg(c_kernel, 0, sizeof(T*), &m.begin().get_buffer().get());
  clSetKernelArg(c_kernel, 1, sizeof(T*), &result.begin().get_buffer().get());
  clSetKernelArg(c_kernel, 2, sizeof(unsigned int), &width);
  clSetKernelArg(c_kernel, 3, sizeof(unsigned int), &height);
  cl_command_queue c_queue = queue.get();
  cl_event event = NULL;
  clEnqueueNDRangeKernel(c_queue, c_kernel, 2, NULL, global_size, NULL, 0, NULL, &event);
  clWaitForEvents(1, &event);
}

template<class T, class L1, class L2, class A>
typename std::enable_if<is_numeric<T>::value>::type
change_layout(ublas::matrix<T, L1, A> const &m,
	      ublas::matrix<T, L2, A> &result,
	      compute::command_queue& queue)
{
  ublas::matrix<T, L1, opencl::storage> mdev(m, queue);
  ublas::matrix<T, L2, opencl::storage> rdev(result.size1(), result.size2(), queue.get_context());
  change_layout(mdev, rdev, queue);
  rdev.to_host(result, queue);
}

template<class T, class L>
typename std::enable_if<is_numeric<T>::value>::type
trans(ublas::matrix<T, L, opencl::storage> const &m,
      ublas::matrix<T, L, opencl::storage> &result,
      compute::command_queue& queue)
{
  assert(m.size1() == result.size2() && m.size2() == result.size1());
  assert(m.device() == result.device() && m.device() == queue.get_device());
  char const *kernel;
  if (std::is_same<T, float>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(float);
  else if (std::is_same<T, double>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(double);
  else if (std::is_same<T, std::complex<float>>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(float2);
  else if (std::is_same<T, std::complex<double>>::value)
    kernel = OPENCL_TRANSPOSITION_KERNEL(double2);
  size_t len = strlen(kernel);
  cl_int err;
  cl_context c_context = queue.get_context().get();
  cl_program program = clCreateProgramWithSource(c_context, 1, &kernel, &len, &err);
  clBuildProgram(program, 1, &queue.get_device().get(), NULL, NULL, NULL);
  cl_kernel c_kernel = clCreateKernel(program, "transpose", &err);
  size_t width = std::is_same <L, ublas::basic_row_major<>>::value ? m.size2() : m.size1();
  size_t height = std::is_same <L, ublas::basic_row_major<>>::value ? m.size1() : m.size2();
  size_t global_size[2] = { width , height };
  clSetKernelArg(c_kernel, 0, sizeof(T*), &m.begin().get_buffer().get());
  clSetKernelArg(c_kernel, 1, sizeof(T*), &result.begin().get_buffer().get());
  clSetKernelArg(c_kernel, 2, sizeof(unsigned int), &width);
  clSetKernelArg(c_kernel, 3, sizeof(unsigned int), &height);
  cl_command_queue c_queue = queue.get();
  cl_event event = NULL;
  clEnqueueNDRangeKernel(c_queue, c_kernel, 2, NULL, global_size, NULL, 0, NULL, &event);
  clWaitForEvents(1, &event);
}

template<class T, class L, class A>
typename std::enable_if<is_numeric<T>::value>::type
trans(ublas::matrix<T, L, A> const &m,
      ublas::matrix<T, L, A> &result,
      compute::command_queue& queue)
{
  ublas::matrix<T, L, opencl::storage> mdev(m, queue);
  ublas::matrix<T, L, opencl::storage> rdev(result.size1(), result.size2(), queue.get_context());
  trans(mdev, rdev, queue);
  rdev.to_host(result, queue);
}

template<class T, class L, class A>
typename std::enable_if<is_numeric<T>::value, ublas::matrix<T, L, A>>::type
trans(ublas::matrix<T, L, A>& m, compute::command_queue& queue)
{
  ublas::matrix<T, L, A> result(m.size2(), m.size1());
  trans(m, result, queue);
  return result;
}

}}}}

#endif
