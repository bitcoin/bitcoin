// Boost.uBLAS
//
// Copyright (c) 2018 Fady Essam
// Copyright (c) 2018 Stefan Seefeld
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
// copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_numeric_ublas_opencl_misc_hpp_
#define boost_numeric_ublas_opencl_misc_hpp_

#include <boost/numeric/ublas/opencl/library.hpp>
#include <boost/numeric/ublas/opencl/vector.hpp>
#include <boost/numeric/ublas/opencl/matrix.hpp>

namespace boost { namespace numeric { namespace ublas { namespace opencl {

template <typename T>
typename std::enable_if<is_numeric<T>::value, T>::type
a_sum(ublas::vector<T, opencl::storage> const &v, compute::command_queue& queue)
{
  compute::vector<T> scratch_buffer(v.size(), queue.get_context());
  compute::vector<T> result_buffer(1, queue.get_context());
  cl_event event;
  if (std::is_same<T, float>::value)
    clblasSasum(v.size(),
		result_buffer.begin().get_buffer().get(), //result buffer
		0, //offset in result buffer
		v.begin().get_buffer().get(), //input buffer
		0, //offset in input buffer
		1, //increment in input buffer
		scratch_buffer.begin().get_buffer().get(),
		1, //number of command queues
		&(queue.get()), //queue
		0, // number of events waiting list
		NULL, //event waiting list
		&event); //event
  else if (std::is_same<T, double>::value)
    clblasDasum(v.size(),
		result_buffer.begin().get_buffer().get(), //result buffer
		0, //offset in result buffer
		v.begin().get_buffer().get(), //input buffer
		0, //offset in input buffer
		1, //increment in input buffer
		scratch_buffer.begin().get_buffer().get(),
		1, //number of command queues
		&(queue.get()), //queue
		0, // number of events waiting list
		NULL, //event waiting list
		&event); //event
  else if (std::is_same<T, std::complex<float>>::value)
    clblasScasum(v.size(),
		 result_buffer.begin().get_buffer().get(), //result buffer
		 0, //offset in result buffer
		 v.begin().get_buffer().get(), //input buffer
		 0, //offset in input buffer
		 1, //increment in input buffer
		 scratch_buffer.begin().get_buffer().get(),
		 1, //number of command queues
		 &(queue.get()), //queue
		 0, // number of events waiting list
		 NULL, //event waiting list
		 &event); //event
  else if (std::is_same<T, std::complex<double>>::value)
    clblasDzasum(v.size(),
		 result_buffer.begin().get_buffer().get(), //result buffer
		 0, //offset in result buffer
		 v.begin().get_buffer().get(), //input buffer
		 0, //offset in input buffer
		 1, //increment in input buffer
		 scratch_buffer.begin().get_buffer().get(),
		 1, //number of command queues
		 &(queue.get()), //queue
		 0, // number of events waiting list
		 NULL, //event waiting list
		 &event); //event
  clWaitForEvents(1, &event);
  return result_buffer[0];
}

template <typename T, typename A>
typename std::enable_if<is_numeric<T>::value, T>::type
a_sum(ublas::vector<T, A> const &v, compute::command_queue& queue)
{
  ublas::vector<T, opencl::storage> vdev(v, queue);
  return a_sum(vdev, queue);
}

template <typename T>
typename std::enable_if<std::is_same<T, float>::value |
			std::is_same<T, double>::value,
			T>::type
norm_1(ublas::vector<T, opencl::storage> const &v, compute::command_queue& queue)
{
  return a_sum(v, queue);
}

template <typename T, typename A>
typename std::enable_if<std::is_same<T, float>::value |
			std::is_same<T, double>::value,
			T>::type
norm_1(ublas::vector<T, A> const &v, compute::command_queue& queue)
{
  ublas::vector<T, opencl::storage> vdev(v, queue);
  return norm_1(vdev, queue);
}

template <typename T>
typename std::enable_if<is_numeric<T>::value, T>::type
norm_2(ublas::vector<T, opencl::storage> const &v, compute::command_queue& queue)
{
  compute::vector<T> scratch_buffer(2*v.size(), queue.get_context());
  compute::vector<T> result_buffer(1, queue.get_context());
  cl_event event;
  if (std::is_same<T, float>::value)
    clblasSnrm2(v.size(),
		result_buffer.begin().get_buffer().get(), //result buffer
		0, //offset in result buffer
		v.begin().get_buffer().get(), //input buffer
		0, //offset in input buffer
		1, //increment in input buffer
		scratch_buffer.begin().get_buffer().get(),
		1, //number of command queues
		&(queue.get()), //queue
		0, // number of events waiting list
		NULL, //event waiting list
		&event); //event
  else if (std::is_same<T, double>::value)
    clblasDnrm2(v.size(),
		result_buffer.begin().get_buffer().get(), //result buffer
		0, //offset in result buffer
		v.begin().get_buffer().get(), //input buffer
		0, //offset in input buffer
		1, //increment in input buffer
		scratch_buffer.begin().get_buffer().get(),
		1, //number of command queues
		&(queue.get()), //queue
		0, // number of events waiting list
		NULL, //event waiting list
		&event); //event
  else if (std::is_same<T, std::complex<float>>::value)
    clblasScnrm2(v.size(),
		 result_buffer.begin().get_buffer().get(), //result buffer
		 0, //offset in result buffer
		 v.begin().get_buffer().get(), //input buffer
		 0, //offset in input buffer
		 1, //increment in input buffer
		 scratch_buffer.begin().get_buffer().get(),
		 1, //number of command queues
		 &(queue.get()), //queue
		 0, // number of events waiting list
		 NULL, //event waiting list
		 &event); //event
  else if (std::is_same<T, std::complex<double>>::value)
    clblasDznrm2(v.size(),
		 result_buffer.begin().get_buffer().get(), //result buffer
		 0, //offset in result buffer
		 v.begin().get_buffer().get(), //input buffer
		 0, //offset in input buffer
		 1, //increment in input buffer
		 scratch_buffer.begin().get_buffer().get(),
		 1, //number of command queues
		 &(queue.get()), //queue
		 0, // number of events waiting list
		 NULL, //event waiting list
		 &event); //event
  clWaitForEvents(1, &event);
  return result_buffer[0];
}

template <typename T, typename A>
typename std::enable_if<is_numeric<T>::value, T>::type
norm_2(ublas::vector<T, A> const &v, compute::command_queue& queue)
{
  ublas::vector<T, opencl::storage> vdev(v, queue);
  return norm_2(vdev, queue);
}

}}}}

#endif 
