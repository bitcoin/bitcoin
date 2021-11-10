// Boost.uBLAS
//
// Copyright (c) 2018 Fady Essam
// Copyright (c) 2018 Stefan Seefeld
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or
// copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef boost_numeric_ublas_opencl_vector_hpp_
#define boost_numeric_ublas_opencl_vector_hpp_

#include <boost/numeric/ublas/opencl/library.hpp>
#include <boost/numeric/ublas/functional.hpp>
#include <boost/compute/core.hpp>
#include <boost/compute/algorithm.hpp>
#include <boost/compute/buffer.hpp>
#include <boost/compute/container/vector.hpp>

namespace boost { namespace numeric { namespace ublas { namespace opencl {

class storage;

namespace compute = boost::compute;

} // namespace opencl

template <class T>
class vector<T, opencl::storage> : public boost::compute::vector<T>
{
  typedef std::size_t size_type;
public:
  vector() : compute::vector<T>() {}
  vector(size_type size, compute::context context)
    : compute::vector<T>(size, context)
  { device_ = context.get_device();}
  vector(size_type size, T value, compute::command_queue queue)
    : compute::vector<T>(size, value, queue.get_context())
  {
    queue.finish();
    device_ = queue.get_device();
  }

  template <typename A>
  vector(vector<T, A> const &v, compute::command_queue &queue)
    : vector(v.size(), queue.get_context())
  {
    this->from_host(v, queue);
  }
  

  const compute::device device() const { return device_;}
  compute::device device() { return device_;}

  template<class A>
  void from_host(ublas::vector<T, A> const &v, compute::command_queue & queue)
  {
    assert(this->device() == queue.get_device());
    compute::copy(v.begin(),
		  v.end(),
		  this->begin(),
		  queue);
    queue.finish();
  }

  template<class A>
  void to_host(ublas::vector<T, A>& v, compute::command_queue& queue) const
  {
    assert(this->device() == queue.get_device());
    compute::copy(this->begin(),
		  this->end(),
		  v.begin(),
		  queue);
    queue.finish();
  }

  void fill(T value, compute::command_queue & queue)
  {
    assert(this->device() == queue.get_device());
    compute::fill(this->begin(), this->end(), value, queue);
    queue.finish();
  }

private:
  compute::device device_;
};

}}}

#endif
