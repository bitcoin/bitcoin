#ifndef BOOST_THREAD_EXPERIMENTAL_PARALLEL_V1_EXCEPTION_LIST_HPP
#define BOOST_THREAD_EXPERIMENTAL_PARALLEL_V1_EXCEPTION_LIST_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/thread/detail/config.hpp>
#include <boost/thread/experimental/parallel/v1/inline_namespace.hpp>

#include <boost/exception_ptr.hpp>
#include <exception>
#include <list>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace experimental
{
namespace parallel
{
BOOST_THREAD_INLINE_NAMESPACE(v1)
{

  class BOOST_SYMBOL_VISIBLE exception_list: public std::exception
  {
    typedef std::list<exception_ptr> exception_ptr_list;
    exception_ptr_list list_;
  public:
    typedef exception_ptr_list::const_iterator const_iterator;

    ~exception_list() BOOST_NOEXCEPT_OR_NOTHROW {}

    void add(exception_ptr const& e)
    {
      list_.push_back(e);
    }
    size_t size() const BOOST_NOEXCEPT
    {
      return list_.size();
    }
    const_iterator begin() const BOOST_NOEXCEPT
    {
      return list_.begin();
    }
    const_iterator end() const BOOST_NOEXCEPT
    {
      return list_.end();
    }
    const char* what() const BOOST_NOEXCEPT_OR_NOTHROW
    {
      return "exception_list";
    }

  };
}

} // parallel
} // experimental
} // boost
#include <boost/config/abi_suffix.hpp>

#endif
